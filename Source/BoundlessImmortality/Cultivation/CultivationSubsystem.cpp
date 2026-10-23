#include "CultivationSubsystem.h"
#include "CultivationRules.h"
#include "CultivationHealthComponent.h"
#include "Frontend/InkDamageFeedbackComponent.h"
#include "CultivationGameplayComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/FileManager.h"

namespace
{
using FObject=TSharedPtr<FJsonObject>;
FObject WriteRecord(const FCultivationParticipantRecord& R)
{
    auto J=MakeShared<FJsonObject>();
    J->SetStringField(TEXT("participant_id"),R.ParticipantId);J->SetStringField(TEXT("display_name"),R.DisplayName);J->SetStringField(TEXT("avatar_url"),R.AvatarUrl);
    J->SetBoolField(TEXT("is_anchor"),R.bIsAnchor);J->SetNumberField(TEXT("realm_index"),R.RealmIndex);J->SetNumberField(TEXT("cultivation"),R.Cultivation);J->SetStringField(TEXT("title_id"),R.TitleId.ToString());
    J->SetNumberField(TEXT("wand_count"),double(R.WandCount));J->SetNumberField(TEXT("wand_expires_utc"),R.WandExpiresUtc);J->SetNumberField(TEXT("potion_count"),double(R.PotionCount));J->SetNumberField(TEXT("potion_expires_utc"),R.PotionExpiresUtc);J->SetStringField(TEXT("potion_room_id"),R.PotionRoomId);
    J->SetNumberField(TEXT("soul_active_until"),R.SoulActiveUntil);J->SetNumberField(TEXT("soul_ready_at"),R.SoulReadyAt);J->SetNumberField(TEXT("jet_active_until"),R.JetActiveUntil);J->SetNumberField(TEXT("jet_ready_at"),R.JetReadyAt);J->SetNumberField(TEXT("version"),double(R.Version));J->SetNumberField(TEXT("last_seen_utc"),R.LastSeenUtc);
    TArray<TSharedPtr<FJsonValue>> Unlocks,Stars;for(auto G:R.UnlockedGifts)Unlocks.Add(MakeShared<FJsonValueNumber>(uint8(G)));for(double Ready:R.StarReadyAt)Stars.Add(MakeShared<FJsonValueNumber>(Ready));
    J->SetArrayField(TEXT("unlocked"),Unlocks);J->SetArrayField(TEXT("star_ready_at"),Stars);
    auto Materials=MakeShared<FJsonObject>();for(const auto& P:R.Materials)Materials->SetNumberField(P.Key.ToString(),P.Value);J->SetObjectField(TEXT("materials"),Materials);return J;
}
double Number(const FObject& J,const TCHAR* Key,double Default=0){double V=Default;J->TryGetNumberField(Key,V);return FMath::IsFinite(V)?V:Default;}
bool ReadRecord(const FObject& J,FCultivationParticipantRecord& R)
{
    if(!J.IsValid()||!J->TryGetStringField(TEXT("participant_id"),R.ParticipantId)||R.ParticipantId.IsEmpty()||R.ParticipantId.Len()>1024)return false;
    J->TryGetStringField(TEXT("display_name"),R.DisplayName);J->TryGetStringField(TEXT("avatar_url"),R.AvatarUrl);J->TryGetBoolField(TEXT("is_anchor"),R.bIsAnchor);
    R.RealmIndex=FMath::Clamp(int32(Number(J,TEXT("realm_index"))),0,1024);R.Cultivation=FMath::Clamp(Number(J,TEXT("cultivation")),0.0,1.e30);
    FString Title;J->TryGetStringField(TEXT("title_id"),Title);R.TitleId=FName(Title);
    R.WandCount=int64(FMath::Clamp(Number(J,TEXT("wand_count")),0.0,9007199254740991.0));R.WandExpiresUtc=Number(J,TEXT("wand_expires_utc"));R.PotionCount=int64(FMath::Clamp(Number(J,TEXT("potion_count")),0.0,9007199254740991.0));R.PotionExpiresUtc=Number(J,TEXT("potion_expires_utc"));J->TryGetStringField(TEXT("potion_room_id"),R.PotionRoomId);
    R.SoulActiveUntil=Number(J,TEXT("soul_active_until"));R.SoulReadyAt=Number(J,TEXT("soul_ready_at"));R.JetActiveUntil=Number(J,TEXT("jet_active_until"));R.JetReadyAt=Number(J,TEXT("jet_ready_at"));R.Version=int64(FMath::Clamp(Number(J,TEXT("version")),0.0,9.e15));R.LastSeenUtc=Number(J,TEXT("last_seen_utc"));
    const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;
    if(J->TryGetArrayField(TEXT("unlocked"),Values))for(const auto& V:*Values){double N=0;if(V->TryGetNumber(N)&&N>=0&&N<=7)R.UnlockedGifts.AddUnique(ECultivationGift(int32(N)));}
    if(J->TryGetArrayField(TEXT("star_ready_at"),Values))for(const auto& V:*Values){double N=0;if(R.StarReadyAt.Num()<64&&V->TryGetNumber(N)&&FMath::IsFinite(N))R.StarReadyAt.Add(N);}
    const FObject* Materials=nullptr;if(J->TryGetObjectField(TEXT("materials"),Materials))for(const auto& P:(*Materials)->Values){double N=0;if(P.Value->TryGetNumber(N)&&FMath::IsFinite(N))R.Materials.Add(FName(P.Key),int32(FMath::Clamp(N,0.0,double(MAX_int32))));}
    return true;
}
}
double UCultivationSubsystem::UtcNow(){return double(FDateTime::UtcNow().GetTicks()-FDateTime(1970,1,1).GetTicks())/ETimespan::TicksPerSecond;}
void UCultivationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("WulingPrototype"))&&FParse::Param(FCommandLine::Get(),TEXT("GWLocalTest")))SetWulingAuthorityEnabled(true);
#endif
    bInitialized=true;LoadRecords();
}
void UCultivationSubsystem::Deinitialize(){SaveRecords();AnchorPawn.Reset();bInitialized=false;Super::Deinitialize();}
TStatId UCultivationSubsystem::GetStatId()const{RETURN_QUICK_DECLARE_CYCLE_STAT(UCultivationSubsystem,STATGROUP_Tickables);}
bool UCultivationSubsystem::IsTickable()const{return bInitialized&&!HasAnyFlags(RF_ClassDefaultObject)&&GetWorld()&&GetWorld()->IsGameWorld();}
UWorld* UCultivationSubsystem::GetTickableGameObjectWorld()const{return GetWorld();}
FString UCultivationSubsystem::StoragePath()const{return FPaths::ProjectSavedDir()/TEXT("CultivationPrototype")/(bDevelopmentContext?TEXT("development.json"):TEXT("platform.json"));}
void UCultivationSubsystem::LoadRecords()
{
    if(!bPersistLocalRecords)return;FString Json;if(!FFileHelper::LoadFileToString(Json,*StoragePath()))return;
    // Import does not persist, avoiding a write during read/recovery.
    const bool Before=bPersistLocalRecords;bPersistLocalRecords=false;ImportRecords(Json,true);bPersistLocalRecords=Before;
    for(auto& Pair:Records)Pair.Value.bPresent=false;bDirty=false;
}
void UCultivationSubsystem::SetRoomContext(const FString& RoomId,bool Development)
{
    if(RoomId==CurrentRoomId&&Development==bDevelopmentContext)return;
    SaveRecords();LeaveRoomParticipants();
    const bool SwitchStorage=Development!=bDevelopmentContext;bDevelopmentContext=Development;CurrentRoomId=RoomId;RoomPotionSources.Reset();bAuthoritativePotionSources=false;
    if(SwitchStorage){Records.Reset();GiftReceipts.Reset();DamageWindow.Reset();RealmAdvanceTimes.Reset();LoadRecords();}
    if(AnchorPawn.IsValid())RegisterParticipant(AnchorId,TEXT("主播"),TEXT(""),true);
    OnStateChanged.Broadcast();
}
void UCultivationSubsystem::SetAnchor(APawn* Pawn,const FString& Id,const FString& Name)
{
    AnchorPawn=Pawn;if(!Pawn)return;
    if(!Id.IsEmpty()&&Id!=AnchorId){if(auto* Old=Records.Find(AnchorId)){Old->bPresent=false;Old->bIsAnchor=false;}AnchorId=Id;bAuraCacheDirty=true;}
    // Pawn reattachment must not discard the profile supplied by the live provider.
    const FString Avatar=Records.Contains(AnchorId)?Records.FindChecked(AnchorId).AvatarUrl:FString();
    RegisterParticipant(AnchorId,Name,Avatar,true);
    if(!Pawn->FindComponentByClass<UCultivationHealthComponent>())
    {
        auto* Health=NewObject<UCultivationHealthComponent>(Pawn,TEXT("CultivationHealth"));Pawn->AddInstanceComponent(Health);Health->RegisterComponent();
    }
    auto* Feedback=Pawn->FindComponentByClass<UInkDamageFeedbackComponent>();
    if(!Feedback){Feedback=NewObject<UInkDamageFeedbackComponent>(Pawn,TEXT("InkDamageFeedback"));Pawn->AddInstanceComponent(Feedback);Feedback->RegisterComponent();}
    Feedback->Observe(Pawn->FindComponentByClass<UCultivationHealthComponent>());
}
UCultivationHealthComponent* UCultivationSubsystem::GetAnchorHealth()const{return AnchorPawn.IsValid()?AnchorPawn->FindComponentByClass<UCultivationHealthComponent>():nullptr;}
bool UCultivationSubsystem::RegisterParticipant(const FString& Id,const FString& Name,const FString& Avatar,bool Anchor)
{
    if(Id.TrimStartAndEnd().IsEmpty())return false;
    auto& R=Records.FindOrAdd(Id);const bool Joining=!R.bPresent;R.ParticipantId=Id;R.DisplayName=Name;R.AvatarUrl=Avatar;R.bIsAnchor=Anchor;R.bPresent=true;R.LastSeenUtc=CurrentUtc();
    // Room ownership moves only on entry, never merely on disconnect.
    if(Joining&&R.PotionExpiresUtc>CurrentUtc())R.PotionRoomId=CurrentRoomId;
    if(Anchor)AnchorId=Id;
    if(Joining)
    {
        OnAnnouncement.Broadcast(Name+(Anchor?TEXT(" · 主播已就绪"):TEXT(" 加入仙途")));
        if(R.PotionExpiresUtc>CurrentUtc())OnAnnouncement.Broadcast(Name+TEXT(" 的生命药水效果已恢复"));
    }
    bDirty=true;bAuraCacheDirty=true;OnStateChanged.Broadcast();return true;
}
void UCultivationSubsystem::LeaveParticipant(const FString& Id)
{
    if(auto* R=Records.Find(Id)){R->bPresent=false;R->LastSeenUtc=CurrentUtc();bDirty=true;bAuraCacheDirty=true;OnStateChanged.Broadcast();}
}
void UCultivationSubsystem::LeaveRoomParticipants(){for(auto& P:Records){P.Value.bPresent=false;P.Value.LastSeenUtc=CurrentUtc();}bDirty=true;bAuraCacheDirty=true;}
bool UCultivationSubsystem::ProcessGift(const FCultivationGiftEvent& E)
{
    if(E.bDevelopment!=bDevelopmentContext||E.MessageId.IsEmpty()||E.ParticipantId.IsEmpty()||E.Count<=0)return false;
    const FString Receipt=E.ParticipantId+TEXT("|")+E.MessageId;if(GiftReceipts.Contains(Receipt))return true;
    auto* R=Records.Find(E.ParticipantId);if(!R)return false;
    const auto Previous=*R;
    // Adapter/outbox capture receipt UTC once. Recovery must not grant a fresh six hours.
    const double Now=CurrentUtc();const double Received=FMath::IsFinite(E.EventUtcSeconds)&&E.EventUtcSeconds>0?FMath::Min(Now,E.EventUtcSeconds):Now;
    if(!CultivationRules::ApplyGift(*R,E,Received,Rules))return false;
    if(bWulingAuthority){R->RealmIndex=Previous.RealmIndex;R->Cultivation=Previous.Cultivation;}
    if(E.GiftId==ECultivationGift::LifePotion)R->PotionRoomId=CurrentRoomId;
    GiftReceipts.Add(Receipt);bDirty=true;bAuraCacheDirty=true;
    // The receipt and changed attributes commit in one atomic file replacement.
    if(!SaveRecords()){Records.Add(E.ParticipantId,Previous);GiftReceipts.Remove(Receipt);return false;}
    const int32 RealmAfter=Records.FindChecked(E.ParticipantId).RealmIndex;
    NotifyRealmAdvanced(E.ParticipantId,Previous.RealmIndex,RealmAfter);
    OnStateChanged.Broadcast();return true;
}
bool UCultivationSubsystem::AddCultivation(const FString& Id,double Amount)
{
    if(bWulingAuthority)return false;
    auto* R=Records.Find(Id);if(!R)return false;const int32 Before=R->RealmIndex;const bool Applied=CultivationRules::AddExperience(*R,Amount,Rules);const int32 After=R->RealmIndex;
    bDirty|=Applied;bAuraCacheDirty|=Applied;if(Applied&&After>Before){NotifyRealmAdvanced(Id,Before,After);OnStateChanged.Broadcast();}return Applied;
}
void UCultivationSubsystem::NotifyRealmAdvanced(const FString& Id,int32 Previous,int32 Current)
{
    if(Current<=Previous)return;const auto* R=Records.Find(Id);if(!R)return;
    RealmAdvanceTimes.Add(Id,CurrentUtc());const FString Name=R->DisplayName;const FText Realm=GetRealmName(Current);const int32 Count=Current-Previous;
    const FString Text=Count>1?FString::Printf(TEXT("%s 连破 %d 重境界 · %s"),*Name,Count,*Realm.ToString()):Name+TEXT(" 突破 · ")+Realm.ToString();
    OnRealmAdvanced.Broadcast(Id,Previous,Current);OnAnnouncement.Broadcast(Text);
}
bool UCultivationSubsystem::TryAnchorBreakthrough()
{
    if(bWulingAuthority)return false;
    auto* R=Records.Find(AnchorId);if(!R)return false;const auto Before=*R;if(!CultivationRules::Breakthrough(*R,Rules))return false;
    bDirty=true;bAuraCacheDirty=true;if(!SaveRecords()){Records.Add(AnchorId,Before);return false;}NotifyRealmAdvanced(AnchorId,Before.RealmIndex,R->RealmIndex);OnStateChanged.Broadcast();return true;
}
void UCultivationSubsystem::GrantMaterial(const FString& Id,FName Material,int32 Count)
{
    if(bWulingAuthority)return;
    if(Count<=0||Material.IsNone())return;if(auto* R=Records.Find(Id)){auto& Total=R->Materials.FindOrAdd(Material);Total=int32(FMath::Min(int64(MAX_int32),int64(Total)+Count));++R->Version;bDirty=true;}
}
void UCultivationSubsystem::RecordActualDamage(const FString& Id,double Amount,bool Killed)
{
    if(!Records.Contains(Id)||!FMath::IsFinite(Amount)||Amount<=0)return;
    const double Now=CurrentUtc();DamageWindow.FindOrAdd(Id).Emplace(Now,Amount);OnActualDamage.Broadcast(Id,Amount,Now);AddCultivation(Id,Rules.ExperiencePerHit+(Killed?Rules.ExperiencePerKill:0));
    if(Killed)GrantMaterial(AnchorId,Rules.BreakthroughMaterialId,Rules.MaterialPerKill);
}
void UCultivationSubsystem::GrantOfflineCultivation(const FString& Id,double Seconds)
{
    if(!FMath::IsFinite(Seconds)||Seconds<=0)return;AddCultivation(Id,FMath::Min(Seconds,604800.0)*FMath::Max(0.0,Rules.OfflinePerSecond));
}
bool UCultivationSubsystem::HasRoomPotion(double Now)const
{
    RefreshAuraCache(Now);return bCachedPotion;
}
void UCultivationSubsystem::RefreshAuraCache(double Now)const
{
    if(!bAuraCacheDirty&&Now-AuraCacheUtc<.05)return;
    bAuraCacheDirty=false;AuraCacheUtc=Now;bCachedJet=bCachedPotion=bCachedEnemyDebuff=bCachedImmunity=false;
    CachedSoulTotal=FCultivationAttributes();CultivationRules::Scale(CachedSoulTotal,0);CachedSoulHolders.Reset();
    const auto* Anchor=Records.Find(AnchorId);const int32 Realm=Anchor?Anchor->RealmIndex:0;
    for(const auto& Source:RoomPotionSources)if(Source.RoomId==CurrentRoomId&&Source.ExpiresUtcSeconds>Now)bCachedPotion=true;
    for(const auto& P:Records)
    {
        const auto& R=P.Value;
        if(!bAuthoritativePotionSources&&R.PotionExpiresUtc>Now&&R.PotionRoomId==CurrentRoomId)bCachedPotion=true;
        if(!R.bPresent)continue;
        if(R.SoulActiveUntil>Now&&CultivationRules::Has(R,ECultivationGift::RareChest)){CultivationRules::AddScaled(CachedSoulTotal,CultivationRules::PersonalAttributes(R,Rules,Realm),1);CachedSoulHolders.Add(R.ParticipantId);}
        bCachedJet|=R.JetActiveUntil>Now&&CultivationRules::Has(R,ECultivationGift::SuperJet);
        bCachedEnemyDebuff|=CultivationRules::Has(R,ECultivationGift::MysteryAirdrop);bCachedImmunity|=CultivationRules::Has(R,ECultivationGift::SuperAirdrop);
    }
}
FCultivationAttributes UCultivationSubsystem::ResolveAttributes(const FCultivationParticipantRecord& R,double Now)const
{
    if(bWulingAuthority)if(const auto* Attributes=WulingAttributes.Find(R.ParticipantId))return *Attributes;
    const auto* A=Records.Find(AnchorId);const int32 Realm=A?A->RealmIndex:0;
    auto Result=CultivationRules::PersonalAttributes(R,Rules,Realm);RefreshAuraCache(Now);
    const double Share=R.bIsAnchor?Rules.SoulAnchorShare:Rules.SoulAudienceShare;
    CultivationRules::AddScaled(Result,CachedSoulTotal,Share);
    if(CachedSoulHolders.Contains(R.ParticipantId))CultivationRules::AddScaled(Result,CultivationRules::PersonalAttributes(R,Rules,Realm),-Share);
    if(bCachedJet)CultivationRules::AddFlat(Result,Rules.JetFlatBonus);
    if(!R.bIsAnchor){Result.MaxHealth=0;Result.Defense=0;Result.HealthRegeneration=0;}
    Result.CriticalChance=FMath::Clamp(Result.CriticalChance,0.0,1.0);Result.EvasionChance=FMath::Clamp(Result.EvasionChance,0.0,1.0);return Result;
}
double UCultivationSubsystem::CultivationRate(const FCultivationParticipantRecord& R,double Now)const
{
    double Multiplier=R.WandExpiresUtc>Now?CultivationRules::StackedMultiplier(Rules.WandBase,Rules.WandStackCoefficient,R.WandCount):1;
    if(R.PotionExpiresUtc>Now)Multiplier*=CultivationRules::StackedMultiplier(Rules.PotionBase,Rules.PotionStackCoefficient,R.PotionCount);
    else if(HasRoomPotion(Now))Multiplier*=R.bIsAnchor?Rules.PotionAnchorMultiplier:Rules.PotionAudienceMultiplier;
    return FMath::Min(1.e30,FMath::Max(0.0,Rules.MeditationPerSecond*ResolveAttributes(R,Now).CultivationSpeed*Multiplier));
}
FText UCultivationSubsystem::GetRealmName(int32 Index)const{return Rules.RealmNames.IsValidIndex(Index)?Rules.RealmNames[Index]:FText::GetEmpty();}
FCultivationParticipantSnapshot UCultivationSubsystem::GetParticipantSnapshot(const FString& Id)const
{
    FCultivationParticipantSnapshot S;const auto* R=Records.Find(Id);if(!R)return S;const double Now=CurrentUtc();const auto* A=Records.Find(AnchorId);
    S.ParticipantId=Id;S.DisplayName=R->DisplayName;S.AvatarUrl=R->AvatarUrl;S.bIsAnchor=R->bIsAnchor;S.bPresent=R->bPresent;S.RealmIndex=R->RealmIndex;S.RealmName=GetRealmName(R->RealmIndex);S.TitleId=R->TitleId;S.TitleName=CultivationRules::TitleName(R->TitleId);
    if(bWulingAuthority)if(const auto* Honor=WulingHonorTitles.Find(Id);Honor&&!Honor->IsEmpty())S.TitleName=FText::FromString(*Honor);
    S.Cultivation=R->Cultivation;S.RequiredCultivation=CultivationRules::Threshold(Rules,R->RealmIndex);S.EffectiveCultivationPerSecond=CultivationRate(*R,Now);S.PersonalAttributes=CultivationRules::PersonalAttributes(*R,Rules,A?A->RealmIndex:0);S.Attributes=ResolveAttributes(*R,Now);
    S.PotionExpiresUtc=R->PotionExpiresUtc;S.WandCount=R->WandExpiresUtc>Now?R->WandCount:0;S.PotionCount=R->PotionExpiresUtc>Now?R->PotionCount:0;
    S.BreakthroughMaterialCount=R->Materials.FindRef(Rules.BreakthroughMaterialId);S.BreakthroughMaterialCost=CultivationRules::MaterialCost(Rules,R->RealmIndex);
    if(bWulingAuthority)if(const auto* Attributes=WulingAttributes.Find(Id))S.PersonalAttributes=*Attributes;
    if(bWulingAuthority){S.EffectiveCultivationPerSecond=0;if(const auto* M=WulingGrowthMetrics.Find(Id)){S.RequiredCultivation=M->X;S.BreakthroughMaterialCount=int32(M->Y);S.BreakthroughMaterialCost=int32(M->Z);S.EffectiveCultivationPerSecond=M->W;}}
    if(const double* Advanced=RealmAdvanceTimes.Find(Id))S.RealmAdvancePulse=FMath::Clamp(1-(Now-*Advanced)/2,0.0,1.0);
    for(double Ready:R->StarReadyAt)if(Ready<=Now)++S.ReadyStarCharges;
    if(const auto* Window=DamageWindow.Find(Id))for(const auto& Damage:*Window)if(Damage.Key>Now-60)S.ActualDPS+=Damage.Value/60;return S;
}
TArray<FCultivationParticipantSnapshot> UCultivationSubsystem::GetParticipants()const
{
    TArray<FCultivationParticipantSnapshot> Out;for(const auto& P:Records)if(P.Value.bPresent)Out.Add(GetParticipantSnapshot(P.Key));return Out;
}
double UCultivationSubsystem::GetEnemyAttributeMultiplier()const
{
    RefreshAuraCache(CurrentUtc());return bCachedEnemyDebuff?FMath::Clamp(1-Rules.EnemyReduction,0.0,1.0):1;
}
double UCultivationSubsystem::GetAnchorImmunityChance()const
{
    RefreshAuraCache(CurrentUtc());return bCachedImmunity?FMath::Clamp(Rules.ImmunityChance,0.0,1.0):0;
}
double UCultivationSubsystem::ConsumeStarHealing(double Fraction)
{
    if(!FMath::IsFinite(Fraction)||Fraction<=0||Fraction>Rules.StarHealthThreshold||!FMath::IsFinite(Rules.StarCooldown)||!FMath::IsFinite(Rules.StarHealFraction)||Rules.StarHealFraction<=0)return 0;const double Now=CurrentUtc();
    // One charge total per incoming damage event, including with multiple sword holders.
    TArray<FString> Ids;Records.GetKeys(Ids);Ids.Sort();
    for(const FString& Id:Ids)
    {
        auto& R=Records[Id];if(!R.bPresent||!CultivationRules::Has(R,ECultivationGift::Strawberry))continue;
        for(double& Ready:R.StarReadyAt)if(Ready<=Now)
        {
            const double Previous=Ready,Next=Now+FMath::Max(.01,Rules.StarCooldown);if(!FMath::IsFinite(Next))return 0;
            const int64 OldVersion=R.Version;Ready=Next;++R.Version;bDirty=true;
            if(!SaveRecords()){Ready=Previous;R.Version=OldVersion;return 0;}return FMath::Clamp(Rules.StarHealFraction,0.0,1.0);
        }
    }
    return 0;
}
bool UCultivationSubsystem::IsMeditating()const
{
    const auto* P=AnchorPawn.Get();if(!P||!P->GetController()||!P->IsLocallyControlled())return false;
    const auto* Health=GetAnchorHealth();if(Health&&Health->bDead)return false;
    const auto* C=P->FindComponentByClass<UCultivationGameplayComponent>();return C&&C->State==ECultivationActionState::MeditationLoop&&!C->IsMenuPresentation();
}
void UCultivationSubsystem::SetRoomPotionSources(const TArray<FCultivationPotionSource>& Sources){RoomPotionSources=Sources;bAuthoritativePotionSources=true;bAuraCacheDirty=true;OnStateChanged.Broadcast();}
bool UCultivationSubsystem::EquipTitle(const FString& Id,FName Title)
{
    auto* R=Records.Find(Id);if(!R)return false;for(auto Gift:R->UnlockedGifts)if(CultivationRules::GiftTitle(Gift)==Title){R->TitleId=Title;bDirty=true;OnStateChanged.Broadcast();return true;}return false;
}
void UCultivationSubsystem::Tick(float Delta)
{
    if(!FMath::IsFinite(Delta)||Delta<=0)return;const double Now=CurrentUtc();const bool Meditation=IsMeditating();
    for(auto& P:Records)
    {
        auto& R=P.Value;if(!R.bPresent)continue;
        if(CultivationRules::Has(R,ECultivationGift::RareChest)&&R.SoulReadyAt<=Now){R.SoulActiveUntil=Now+Rules.SoulDuration;R.SoulReadyAt=Now+FMath::Max(.01,Rules.SoulCooldown);bDirty=true;bAuraCacheDirty=true;}
        if(CultivationRules::Has(R,ECultivationGift::SuperJet)&&R.JetReadyAt<=Now){R.JetActiveUntil=Now+Rules.JetDuration;R.JetReadyAt=Now+FMath::Max(.01,Rules.JetCooldown);bDirty=true;bAuraCacheDirty=true;}
    }
    if(Meditation&&!bWulingAuthority)
    {
        // Resolve every rate before granting XP so a tier gained by one viewer cannot make
        // later viewers in the map iteration receive an asymmetric rate during this step.
        TArray<TPair<FString,double>> Grants;for(const auto& P:Records)if(P.Value.bPresent)Grants.Emplace(P.Key,CultivationRate(P.Value,Now)*Delta);
        for(const auto& Grant:Grants)AddCultivation(Grant.Key,Grant.Value);
    }
    for(auto& P:DamageWindow)P.Value.RemoveAll([Now](const auto& Damage){return Damage.Key<=Now-60;});
    SaveElapsed+=Delta;NotifyElapsed+=Delta;if(SaveElapsed>=5){SaveElapsed=0;if(bDirty)SaveRecords();}
    if(NotifyElapsed>=.25){NotifyElapsed=0;OnStateChanged.Broadcast();}
}
bool UCultivationSubsystem::ApplyWulingGrowth(const FString& Id,int32 Realm,double Qi,double Threshold,int32 MaterialCount,int32 MaterialCost,double Rate)
{
    if(!bWulingAuthority||Realm<0||Realm>=Rules.RealmNames.Num()||!FMath::IsFinite(Qi)||Qi<0||!FMath::IsFinite(Threshold)||Threshold<0)return false;
    auto* R=Records.Find(Id);if(!R)return false;const int32 Before=R->RealmIndex;
    R->RealmIndex=Realm;R->Cultivation=Qi;WulingGrowthMetrics.Add(Id,FVector4d(Threshold,MaterialCount,MaterialCost,Rate));bAuraCacheDirty=true;
    NotifyRealmAdvanced(Id,Before,Realm);OnStateChanged.Broadcast();return true;
}
void UCultivationSubsystem::ApplyWulingAttributes(const FString& Id,const FCultivationAttributes& Attributes)
{if(bWulingAuthority&&Records.Contains(Id)){WulingAttributes.Add(Id,Attributes);bAuraCacheDirty=true;OnStateChanged.Broadcast();}}
FString UCultivationSubsystem::ExportRecords()const
{
    auto Root=MakeShared<FJsonObject>();Root->SetNumberField(TEXT("schema"),1);Root->SetBoolField(TEXT("development"),bDevelopmentContext);Root->SetStringField(TEXT("room_id"),CurrentRoomId);
    TArray<FString> Keys;Records.GetKeys(Keys);Keys.Sort();TArray<TSharedPtr<FJsonValue>> Values;
    for(const FString& Id:Keys)Values.Add(MakeShared<FJsonValueObject>(WriteRecord(Records[Id])));Root->SetArrayField(TEXT("records"),Values);
    Keys=GiftReceipts.Array();Keys.Sort();Values.Reset();for(const FString& Receipt:Keys)Values.Add(MakeShared<FJsonValueString>(Receipt));Root->SetArrayField(TEXT("receipts"),Values);
    FString Out;FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Out));return Out;
}
bool UCultivationSubsystem::ImportRecords(const FString& Json,bool Replace)
{
    return ImportInternal(Json,Replace,false);
}
bool UCultivationSubsystem::ImportAuthoritativeRecords(const FString& Json)
{
    return ImportInternal(Json,false,true);
}
bool UCultivationSubsystem::ImportInternal(const FString& Json,bool Replace,bool Authoritative)
{
    FObject Root;auto Reader=TJsonReaderFactory<>::Create(Json);if(!FJsonSerializer::Deserialize(Reader,Root)||!Root)return false;
    bool Dev=bDevelopmentContext;if(Root->TryGetBoolField(TEXT("development"),Dev)&&Dev!=bDevelopmentContext)return false;
    const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;if(!Root->TryGetArrayField(TEXT("records"),Values)||Values->Num()>100000)return false;
    TMap<FString,FCultivationParticipantRecord> Incoming;
    for(const auto& V:*Values)
    {
        if(V->Type!=EJson::Object)return false;FCultivationParticipantRecord R;if(!ReadRecord(V->AsObject(),R))return false;R.RealmIndex=FMath::Clamp(R.RealmIndex,0,FMath::Max(0,Rules.RealmNames.Num()-1));Incoming.Add(R.ParticipantId,R);
    }
    const auto Before=Records;const auto OldReceipts=GiftReceipts;
    if(Replace){Records.Reset();GiftReceipts.Reset();}
    if(Authoritative)
    {
        TArray<FString> Remove;for(const FString& Receipt:GiftReceipts)for(const auto& P:Incoming)if(Receipt.StartsWith(P.Key+TEXT("|"))){Remove.Add(Receipt);break;}
        for(const FString& Receipt:Remove)GiftReceipts.Remove(Receipt);
    }
    for(auto& P:Incoming)
    {
        auto* Existing=Records.Find(P.Key);if(!Authoritative&&Existing&&Existing->Version>P.Value.Version)continue;
        if(Existing)P.Value.bPresent=Existing->bPresent;Records.Add(P.Key,P.Value);
    }
    if(Root->TryGetArrayField(TEXT("receipts"),Values))for(const auto& V:*Values){FString Receipt;if(V->TryGetString(Receipt)&&Receipt.Len()<=4096)GiftReceipts.Add(Receipt);}
    bDirty=true;bAuraCacheDirty=true;if(!SaveRecords()){Records=Before;GiftReceipts=OldReceipts;return false;}OnStateChanged.Broadcast();return true;
}
bool UCultivationSubsystem::SaveRecords()
{
    if(!bPersistLocalRecords){bDirty=false;return true;}
    const FString Path=StoragePath(),Temp=Path+TEXT(".pending");IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
    if(!FFileHelper::SaveStringToFile(ExportRecords(),*Temp,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))return false;
    if(!IFileManager::Get().Move(*Path,*Temp,true,true,false,true))return false;bDirty=false;return true;
}
