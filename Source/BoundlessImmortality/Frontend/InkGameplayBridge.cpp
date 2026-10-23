#include "InkMenuDirector.h"
#include <limits>
#include "InkPawnSafety.h"
#include "InkFrontendWidget.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Cultivation/CultivationHealthComponent.h"
#include "Cultivation/CultivationPrototypeClient.h"
#include "Wuling/WulingPrototypeSubsystem.h"
#include "Wuling/WulingInventoryProjection.h"
#include "Frontend/InkInventoryModel.h"
#include "LiveWeapons/LiveWeaponSwarm.h"
#include "LiveWeapons/LiveWeaponGameMode.h"
#include "LiveWeapons/LiveGMOverlay.h"
#include "LiveWeapons/LiveAudienceAdapterComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CultivationGameplayComponent.h"

bool AInkMenuDirector::EnsureGameplaySystems()
{
    if(bMenuActive||!GameplayPawn.IsValid()||!GetGameInstance())return false;
    auto* C=GetGameInstance()->GetSubsystem<UCultivationSubsystem>();if(!C)return false;
    if(GameplayCultivation.Get()!=C)
    {
        if(auto* Previous=GameplayCultivation.Get())Previous->OnAnnouncement.RemoveDynamic(this,&ThisClass::HandleGameplayAnnouncement);
        GameplayCultivation=C;C->OnAnnouncement.AddDynamic(this,&ThisClass::HandleGameplayAnnouncement);
    }
    if(!IsValid(GameplaySwarm))
    {
        // A dedicated live GameMode creates its own pair. Wait for that instead of racing it.
        if(auto* Mode=GetWorld()->GetAuthGameMode<ALiveWeaponGameMode>())
        {
            if(!IsValid(Mode->Swarm))return false;GameplaySwarm=Mode->Swarm;GameplayAudience=Mode->Audience;bOwnsGameplaySwarm=false;
        }
        else
        {
            for(TActorIterator<ALiveGMOverlay> It(GetWorld());It;++It)
                if(IsValid(It->Swarm)&&It->Swarm!=PresentationSwarm){GameplaySwarm=It->Swarm;GameplayAudience=It->Audience;bOwnsGameplaySwarm=false;break;}
            if(!GameplaySwarm)
            {
                for(TActorIterator<ALiveWeaponSwarm> It(GetWorld());It;++It)
                    if(*It!=PresentationSwarm&&It->GetPresentationMode()==ELiveWeaponPresentationMode::Gameplay&&It->GetAnchor()==GameplayPawn.Get()){GameplaySwarm=*It;break;}
            }
            if(!GameplaySwarm)
            {
                const FTransform Transform(GameplayPawn->GetActorRotation(),GameplayPawn->GetActorLocation());
                GameplaySwarm=GetWorld()->SpawnActor<ALiveWeaponSwarm>(SwarmClass,Transform);bOwnsGameplaySwarm=IsValid(GameplaySwarm);
            }
        }
    }
    if(!IsValid(GameplaySwarm))return false;
    if(!GameplayAudience)GameplayAudience=DirectorAudience;
    if(auto* Host=GetGameInstance()->GetSubsystem<ULiveInteractionSubsystem>())
    {
        const auto& Session=Host->GetCurrentSession();
        if(Session.Nonce.IsValid())
        {
            const FString SessionKey=Session.PlatformId+TEXT("|")+Session.AppId+TEXT("|")+Session.RoomId+FString::FromInt(Session.bLocalTest?1:0);
            if(!GameplaySessionKey.IsEmpty()&&SessionKey!=GameplaySessionKey)GameplaySwarm->ResetParticipants();
            GameplaySessionKey=SessionKey;
        }
    }
    C->SetAnchor(GameplayPawn.Get(),C->GetAnchorId(),C->GetParticipantSnapshot(C->GetAnchorId()).DisplayName.IsEmpty()?TEXT("主播"):C->GetParticipantSnapshot(C->GetAnchorId()).DisplayName);
    if(auto* Health=C->GetAnchorHealth();Health&&ObservedHealth.Get()!=Health)
    {
        if(auto* Previous=ObservedHealth.Get())Previous->OnDied.RemoveDynamic(this,&ThisClass::HandleAnchorDied);
        ObservedHealth=Health;Health->OnDied.AddDynamic(this,&ThisClass::HandleAnchorDied);
    }
    GameplaySwarm->SetPresentationMode(ELiveWeaponPresentationMode::Gameplay);GameplaySwarm->SetAnchor(GameplayPawn.Get());
    GameplaySwarm->SetActorHiddenInGame(false);GameplaySwarm->SetActorTickEnabled(true);GameplaySwarm->bAutomaticAttacks=bGameplayAutomaticAttacks;
    if(GameplayAudience==DirectorAudience&&!bGameplayAdapterBound){GameplayAudience->Bind(GameplaySwarm);bGameplayAdapterBound=true;}
    // Retained roster members resume their own durable progress when returning from the menu.
    auto* Shared=GetGameInstance()->GetSubsystem<UCultivationPrototypeClient>();
    if(Shared)Shared->TrackParticipant(C->GetAnchorId(),false);
    for(const auto& S:GameplaySwarm->GetRoster().GetStates())if(S.bParticipating&&!S.bPersonalSword)
    {
        if(Shared&&Shared->IsParticipantRevoked(S.Profile.ParticipantId)){GameplaySwarm->RemoveParticipant(S.Profile.ParticipantId);continue;}
        C->RegisterParticipant(S.Profile.ParticipantId,S.Profile.DisplayName,S.Profile.AvatarUrl);
        if(Shared)Shared->TrackParticipant(S.Profile.ParticipantId,false);
    }
    bGameplaySystemsReady=true;
    if(auto* Wuling=GetGameInstance()->GetSubsystem<UWulingPrototypeSubsystem>())Wuling->SetGameplayActive(true);
    bool HasPresenter=false;for(TActorIterator<ALiveGMOverlay> It(GetWorld());It;++It){HasPresenter=true;break;}
    if(!HasPresenter){FActorSpawnParameters Params;Params.Owner=this;GetWorld()->SpawnActor<ALiveGMOverlay>(ALiveGMOverlay::StaticClass(),FTransform::Identity,Params);}
    return true;
}

void AInkMenuDirector::SuspendGameplaySystems()
{
    if(!bGameplaySystemsReady)return;
    if(GetGameInstance())if(auto* Wuling=GetGameInstance()->GetSubsystem<UWulingPrototypeSubsystem>())Wuling->SetGameplayActive(false);
    ClearGameplayEncounters();
    if(IsValid(GameplaySwarm))
    {
        bGameplayAutomaticAttacks=GameplaySwarm->bAutomaticAttacks;GameplaySwarm->bAutomaticAttacks=false;
        GameplaySwarm->SetActorTickEnabled(false);GameplaySwarm->SetActorHiddenInGame(true);
    }
    if(bGameplayAdapterBound&&DirectorAudience){DirectorAudience->Unbind();bGameplayAdapterBound=false;}
    if(auto* C=GameplayCultivation.Get())
    {
        // Keep HTTP active long enough to flush gifts and explicit presence leaves.
        // Potion room ownership survives leaving and remains governed by UTC.
        if(auto* Shared=GetGameInstance()->GetSubsystem<UCultivationPrototypeClient>())for(const auto& P:C->GetParticipants())Shared->LeaveParticipant(P.ParticipantId);
        C->LeaveRoomParticipants();C->SetAnchor(nullptr);C->SaveRecords();
    }
    bGameplaySystemsReady=false;
    if(auto* Move=WulingMovement.Get())Move->MaxWalkSpeed=WulingBaseWalkSpeed;
    WulingMovement.Reset();WulingAppliedMoveFactor=1;
}

void AInkMenuDirector::HandleGameplayAnnouncement(const FString& Text)
{
    if(bMenuActive)return;OnGameplayAnnouncement.Broadcast(Text);
}
void AInkMenuDirector::HandleAnchorDied()
{
    if(bMenuActive)return;bRespawnOnNextStart=true;
    if(auto* Pawn=GameplayPawn.Get())if(auto* Action=Pawn->FindComponentByClass<UCultivationGameplayComponent>())
    {
        Action->InterruptAction();
    }
    ReturnToMainMenu();
    const FText Message=FText::FromString(TEXT("气息已竭，已退出战斗。点击“开始游戏”回到出生点，恢复血量后重整仙途。"));
    if(FrontendWidget)FrontendWidget->SetFeedback(Message);
    OnGameplayAnnouncement.Broadcast(Message.ToString());
}
void AInkMenuDirector::RememberSafeGameplayTransform()
{
    if(auto* Character=Cast<ACharacter>(GameplayPawn.Get()))if(Character->GetCharacterMovement()->IsMovingOnGround()&&Character->GetCharacterMovement()->CurrentFloor.IsWalkableFloor())
    {LastSafeGameplayTransform=Character->GetActorTransform();bHasSafeGameplayTransform=true;}
}
bool AInkMenuDirector::GetSafeReentryTransform(FTransform& Out)const
{
    if(bHasSafeGameplayTransform){Out=LastSafeGameplayTransform;return true;}
    if(GetWorld())if(auto* Mode=GetWorld()->GetAuthGameMode())if(auto* Start=Mode->FindPlayerStart(LocalController.Get()))
    {Out=Start->GetActorTransform();return true;}
    // Menu staging coordinates and camera framing do not define a playable spawn.
    return false;
}
bool AInkMenuDirector::RespawnGameplayPawn(APawn* Pawn)
{
    if(!IsGameplayPawnCandidate(Pawn))return false;auto* Health=Pawn->FindComponentByClass<UCultivationHealthComponent>();
    const bool Revive=bRespawnOnNextStart||(Health&&Health->bDead);
    FTransform Fallback=Pawn->GetActorTransform();bool bHasFallback=GetSafeReentryTransform(Fallback);
    if(Revive)if(auto* Mode=GetWorld()->GetAuthGameMode())if(auto* Start=Mode->FindPlayerStart(LocalController.Get()))
    {Fallback=Start->GetActorTransform();bHasFallback=true;}
    FTransform Resolved;
    // Older maps place PlayerStart outside the playable island. Retain their validated
    // stage-coordinate fallback, tested with the GAMEPLAY capsule; the menu actor is
    // never adopted, moved or kept alive by this location-only recovery path.
    if(((Revive&&!bHasFallback)||!InkPawnSafety::ResolveTransform(Pawn,Revive?Fallback:Pawn->GetActorTransform(),Fallback,Resolved,GetPresentationPawn()))&&
       !InkPawnSafety::FindSupportedTransform(Pawn,PresentationSpawnTransform,Resolved,GetPresentationPawn()))
    {PresentationStatus=TEXT("出生点地面尚未就绪，请稍后重试。");return false;}
    Pawn->SetActorTransform(Resolved,false,nullptr,ETeleportType::TeleportPhysics);InkPawnSafety::ClearFallingMotion(Pawn);
    LastSafeGameplayTransform=Resolved;bHasSafeGameplayTransform=true;
    if(Revive&&Health)Health->Revive();bRespawnOnNextStart=false;return true;
}

void AInkMenuDirector::PublishGameplayData()
{
    if(!bPublishGameplayData||bMenuActive||!FrontendWidget||!GetGameInstance())return;
    auto* C=GetGameInstance()->GetSubsystem<UCultivationSubsystem>();if(!C)return;
    if(auto* W=GetGameInstance()->GetSubsystem<UWulingPrototypeSubsystem>();W&&W->IsEnabled()&&W->IsConnected())
        WulingInventoryProjection::Apply(*FrontendWidget->GetInventoryModel(),W->Snapshot());
    const auto Anchor=C->GetParticipantSnapshot(C->GetAnchorId());if(Anchor.ParticipantId.IsEmpty())return;
    TArray<FInkCharacterAttribute> Attributes;
    const auto Add=[&](FName Id,const TCHAR* Label,const FString& Value,double Number=std::numeric_limits<double>::quiet_NaN(),double Base=0){FInkCharacterAttribute Row;Row.AttributeId=Id;Row.Label=FText::FromString(Label);Row.Value=FText::FromString(Value);Row.bHasNumericValue=FMath::IsFinite(Number);Row.NumericValue=Row.bHasNumericValue?Number:0;Row.RatingBaseValue=Base;Attributes.Add(Row);};
    const auto& A=Anchor.Attributes;auto* Health=C->GetAnchorHealth();
    if(C->IsWulingAuthorityEnabled())if(auto* Character=Cast<ACharacter>(GameplayPawn.Get()))
    {
        auto* Move=Character->GetCharacterMovement();
        if(WulingMovement.Get()!=Move){WulingMovement=Move;WulingBaseWalkSpeed=Move->MaxWalkSpeed;WulingAppliedMoveFactor=1;}
        const float Previous=WulingBaseWalkSpeed*WulingAppliedMoveFactor;
        if(!FMath::IsNearlyEqual(Move->MaxWalkSpeed,Previous,.1f))WulingBaseWalkSpeed=Move->MaxWalkSpeed/WulingAppliedMoveFactor;
        WulingAppliedMoveFactor=float(FMath::Clamp(A.MoveSpeed,1.,1.10));Move->MaxWalkSpeed=WulingBaseWalkSpeed*WulingAppliedMoveFactor;
    }
    Add(TEXT("Health"),TEXT("血量"),FString::Printf(TEXT("%.0f / %.0f"),Health?Health->Health:A.MaxHealth,A.MaxHealth),A.MaxHealth);
    Add(TEXT("Attack"),TEXT("剑灵攻击"),FString::Printf(TEXT("%.0f"),A.Attack),A.Attack);Add(TEXT("Defense"),TEXT("防御"),FString::Printf(TEXT("%.0f"),A.Defense),A.Defense);
    Add(TEXT("SpiritDamage"),TEXT("剑灵伤害"),FString::Printf(TEXT("%.0f"),A.SpiritDamage),A.SpiritDamage);Add(TEXT("Critical"),TEXT("会心一击"),FString::Printf(TEXT("%.1f%%"),A.CriticalChance*100),A.CriticalChance);
    Add(TEXT("Evasion"),TEXT("闪避"),FString::Printf(TEXT("%.1f%%"),A.EvasionChance*100),A.EvasionChance);Add(TEXT("Luck"),TEXT("幸运（预留）"),FString::Printf(TEXT("%.1f"),A.Luck));
    const auto* Character=Cast<ACharacter>(GameplayPawn.Get());const auto* Movement=Character?Character->GetCharacterMovement():nullptr;
    const auto* Defaults=Character?Character->GetClass()->GetDefaultObject<ACharacter>():nullptr;
    const double BaseSpeed=C->IsWulingAuthorityEnabled()?WulingBaseWalkSpeed:Defaults&&Defaults->GetCharacterMovement()?Defaults->GetCharacterMovement()->MaxWalkSpeed:400;
    Add(TEXT("MoveSpeed"),TEXT("实际移速上限"),Movement?FString::Printf(TEXT("%.0f 厘米/秒"),Movement->MaxWalkSpeed):TEXT("—"),Movement?Movement->MaxWalkSpeed:std::numeric_limits<double>::quiet_NaN(),BaseSpeed);Add(TEXT("Swordsmanship"),TEXT("剑法（预留）"),FString::Printf(TEXT("%.1f"),A.Swordsmanship));
    Add(TEXT("CultivationSpeed"),TEXT("修炼速度"),FString::Printf(TEXT("%.2f /秒"),Anchor.EffectiveCultivationPerSecond),Anchor.EffectiveCultivationPerSecond);Add(TEXT("Realm"),TEXT("境界"),Anchor.RealmName.ToString());
    Add(TEXT("Cultivation"),TEXT("修为"),FString::Printf(TEXT("%.0f / %.0f"),Anchor.Cultivation,Anchor.RequiredCultivation));
    TArray<FInkDisciplineEntry> Disciplines;
    FrontendWidget->SetCharacterData(FText::FromString(Anchor.DisplayName),Attributes,Disciplines);
    TArray<FInkLeaderboardEntry> Boards;const auto Participants=C->GetParticipants();
    for(const FName Category:{FName(TEXT("Anchor")),FName(TEXT("CultivationSpeed")),FName(TEXT("Damage"))})
    {
        TArray<FCultivationParticipantSnapshot> Rows;
        for(const auto& P:Participants)if(Category!=TEXT("Anchor")||P.bIsAnchor)Rows.Add(P);
        const auto Metric=[&](const FCultivationParticipantSnapshot& P){return Category==TEXT("Anchor")?double(IsValid(GameplaySwarm)?GameplaySwarm->GetParticipantCount()+1:1):(Category==TEXT("Damage")?P.ActualDPS:P.EffectiveCultivationPerSecond);};
        Rows.Sort([&](const auto& L,const auto& R){const double LM=Metric(L),RM=Metric(R);return LM==RM?L.ParticipantId<R.ParticipantId:LM>RM;});
        for(int32 I=0;I<Rows.Num();++I)
        {
            const auto& P=Rows[I];FInkLeaderboardEntry Entry;Entry.ParticipantId=P.ParticipantId;Entry.DisplayName=FText::FromString(P.DisplayName);Entry.Category=Category;Entry.Rank=I+1;Entry.NumericMetric=Metric(P);
            Entry.Metric=FText::FromString(Category==TEXT("Anchor")?FString::Printf(TEXT("%.0f 把"),Entry.NumericMetric):FString::Printf(TEXT("%.2f /秒"),Entry.NumericMetric));Entry.TitleId=P.TitleId;Entry.Realm=P.RealmName;Entry.RealmIndex=P.RealmIndex;Entry.AvatarUrl=P.AvatarUrl;Boards.Add(Entry);
        }
    }
    FrontendWidget->SetLeaderboardEntriesForScope(Boards,TEXT("Local"));
    auto* Shared=GetGameInstance()->GetSubsystem<UCultivationPrototypeClient>();
    FrontendWidget->SetGlobalLeaderboardConnection(Shared&&Shared->IsConnected(),FText::FromString(Shared?Shared->GetStatus():TEXT("共享原型未连接")));
    FrontendWidget->SetLeaderboardEntriesForScope(Shared?Shared->GetLeaderboardEntries():TArray<FInkLeaderboardEntry>(),TEXT("Global"));
}
