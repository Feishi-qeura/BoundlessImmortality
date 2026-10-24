#include "LiveWeaponHUD.h"
#include "LiveGMTestSession.h"
#include "LiveGMOverlay.h"
#include "LiveTitleEffectsComponent.h"
#include "LiveWeaponGameMode.h"
#include "LiveWeaponSwarm.h"
#include "LiveAudienceAdapterComponent.h"
#include "LiveAnchorAbilityComponent.h"
#include "LiveAnchorController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "LiveMonster.h"
#include "LiveMonsterRegistrySubsystem.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Cultivation/CultivationHealthComponent.h"
#include "Cultivation/CultivationNameplate.h"
#include "LiveInteractionSubsystem.h"
#include "Frontend/InkMenuDirector.h"
#include "Frontend/InkFrontendWidget.h"
#include "Frontend/InkAnchorStatusWidget.h"
#include "Frontend/UI/InkUiResponsiveLayout.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/GameInstance.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"

namespace
{
    bool HasVisibleAnchorStatusForWeaponHUD(UWorld* World,APlayerController* Player)
    {
        for(TActorIterator<AInkMenuDirector> It(World);It;++It)
        {
            const auto* Frontend=It->FrontendWidget.Get();
            if(Frontend&&Frontend->UsesRebuiltPresentation()&&Frontend->GetOwningPlayer()==Player
                &&Frontend->IsInViewport()&&Frontend->IsVisible()&&Frontend->GetRenderOpacity()>0
                &&!Frontend->GetRuntimePageLayoutVersion(TEXT("HUD")).IsNone())return true;
            const auto* StatusWidget=Frontend?Frontend->AnchorStatusWidget.Get():nullptr;
            if(Frontend&&Frontend->GetOwningPlayer()==Player&&Frontend->IsInViewport()&&Frontend->IsVisible()&&Frontend->GetRenderOpacity()>0&&Frontend->CurrentPage==TEXT("HUD")
                &&IsValid(StatusWidget)&&StatusWidget->IsVisible()&&StatusWidget->GetRenderOpacity()>0&&StatusWidget->bHasAnchor)return true;
        }
        return false;
    }
}
BOUNDLESSIMMORTALITY_API bool InkNewUiSuppressesLegacyWeaponHud(UWorld* World,APlayerController* Player){return HasVisibleAnchorStatusForWeaponHUD(World,Player);}

void ALiveWeaponHUD::DrawHUD()
{
    Super::DrawHUD();auto* Mode=GetWorld()->GetAuthGameMode<ALiveWeaponGameMode>();if(!Canvas||!Mode||!Mode->Swarm||!PlayerOwner||!PlayerOwner->GetPawn())return;
    if(!NameFont||NameFont->GetPathName()==TEXT("/Game/LiveWeapons/Fonts/F_LiveNames.F_LiveNames"))NameFont=LoadObject<UFont>(nullptr,TEXT("/Game/newUI/Ink/Fonts/Shared/F_InkBody.F_InkBody"),nullptr,LOAD_NoWarn|LOAD_Quiet);
    if(Cultivation.Get()!=(GetGameInstance()?GetGameInstance()->GetSubsystem<UCultivationSubsystem>():nullptr))
    {
        if(auto* Old=Cultivation.Get())Old->OnAnnouncement.RemoveDynamic(this,&ThisClass::HandleAnnouncement);
        Cultivation=GetGameInstance()?GetGameInstance()->GetSubsystem<UCultivationSubsystem>():nullptr;
        if(auto* State=Cultivation.Get())State->OnAnnouncement.AddDynamic(this,&ThisClass::HandleAnnouncement);
    }
    if(!RealmBanners)RealmBanners=LoadObject<UTexture2D>(nullptr,TEXT("/Game/newUI/Ink/Textures/Shared/T_RealmBanners.T_RealmBanners"),nullptr,LOAD_NoWarn|LOAD_Quiet);
    if(!RealmChart)RealmChart=LoadObject<UTexture2D>(nullptr,TEXT("/Game/newUI/Ink/Textures/Shared/T_CohesionIcons.T_CohesionIcons"),nullptr,LOAD_NoWarn|LOAD_Quiet);
    if(!WeaponTitles)WeaponTitles=LoadObject<UTexture2D>(nullptr,TEXT("/Game/newUI/Ink/Textures/Leaderboard/T_LeaderboardTitles.T_LeaderboardTitles"),nullptr,LOAD_NoWarn|LOAD_Quiet);
    auto* Swarm=Mode->Swarm.Get();auto* Adapter=Mode->Audience.Get();const APawn* Pawn=PlayerOwner->GetPawn();const double Now=GetWorld()->GetTimeSeconds();
    const bool NewAnchorHUD=HasVisibleAnchorStatusForWeaponHUD(GetWorld(),PlayerOwner);
    const float UIScale=FMath::Max(.01f,UWidgetLayoutLibrary::GetViewportScale(this));
    if(!NewAnchorHUD)DrawRect(FLinearColor(.025,.04,.035,.88),18,18,300,112);
    if(auto* State=NewAnchorHUD?nullptr:Cultivation.Get())
    {
        const auto Snapshot=State->GetParticipantSnapshot(State->GetAnchorId());auto* Health=State->GetAnchorHealth();
        UFont* Font=NameFont?NameFont.Get():GEngine->GetSmallFont();
        DrawText(Snapshot.DisplayName+TEXT(" · ")+Snapshot.RealmName.ToString(),FLinearColor(.9f,.85f,.62f,1),28,25,Font,.85f);
        const bool ResetProgress=AnchorProgressContext.Update(Snapshot.ParticipantId,Snapshot.RealmIndex,Health?Health->MaxHealth:0,Snapshot.RequiredCultivation);
        if(ResetProgress){AnchorHealthProgress.Reset(Health?Health->GetHealthFraction():0);AnchorCultivationProgress.Reset(Snapshot.RequiredCultivation>0?float(Snapshot.Cultivation/Snapshot.RequiredCultivation):1);}
        if(Health){AnchorHealthProgress.SetTarget(Health->GetHealthFraction());AnchorHealthProgress.Tick(GetWorld()->GetDeltaSeconds());DrawRect(FLinearColor(.12f,.14f,.13f,1),28,47,278,10);DrawRect(FLinearColor(.25f,.09f,.07f,1),28,47,278*AnchorHealthProgress.Trail,10);DrawRect(FLinearColor(.72f,.26f,.20f,1),28,47,278*AnchorHealthProgress.Value,10);DrawText(FString::Printf(TEXT("血量 %.0f / %.0f"),Health->Health,Health->MaxHealth),FLinearColor::White,28,60,Font,.75f);}
        const float XP=Snapshot.RequiredCultivation>0?FMath::Clamp(float(Snapshot.Cultivation/Snapshot.RequiredCultivation),0.f,1.f):1.f;
        AnchorCultivationProgress.SetTarget(XP);AnchorCultivationProgress.Tick(GetWorld()->GetDeltaSeconds());
        DrawRect(FLinearColor(.12f,.14f,.13f,1),28,82,278,8);DrawRect(FLinearColor(.10f,.22f,.19f,1),28,82,278*FMath::Max(XP,AnchorCultivationProgress.Trail),8);DrawRect(FLinearColor(.32f,.53f,.48f,1),28,82,278*FMath::Min(XP,AnchorCultivationProgress.Trail),8);
        DrawText(FString::Printf(TEXT("修为 %.0f / %.0f · %.1f/秒"),Snapshot.Cultivation,Snapshot.RequiredCultivation,Snapshot.EffectiveCultivationPerSecond),FLinearColor::White,28,94,Font,.7f);
    }
    auto* Ability=Pawn->FindComponentByClass<ULiveAnchorAbilityComponent>();
    DrawText(FString::Printf(TEXT("WASD 移动   空格推怪  %s"),Ability&&Ability->GetRemainingCooldown()>0?*FString::Printf(TEXT("%.1fs"),Ability->GetRemainingCooldown()):TEXT("就绪")),FLinearColor::White,28,NewAnchorHUD?210*UIScale:137,GEngine->GetSmallFont(),.75f);
    if(Now<AnnouncementUntil)
    {
        const auto Rect=InkUiLayout::ResolveHud(FVector2D(Canvas->SizeX,Canvas->SizeY)/UIScale).Announcement;
        const FVector2D Position=Rect.GetTopLeft()*UIScale,Extent=Rect.GetSize()*UIScale;
        UFont* AnnouncementFont=NameFont?NameFont.Get():GEngine->GetSmallFont();
        const float TextScale=.8f*UIScale,Available=FMath::Max(0.f,Extent.X-20.f*UIScale);
        FString Caption=Announcement.Replace(TEXT("\r"),TEXT(" ")).Replace(TEXT("\n"),TEXT(" "));
        float CaptionWidth=0,CaptionHeight=0;GetTextSize(Caption,CaptionWidth,CaptionHeight,AnnouncementFont,TextScale);
        if(CaptionWidth>Available)
        {
            do
            {
                if(Caption.IsEmpty())break;
                const int32 Last=Caption.Len()-1;
                const bool Pair=Last>0&&Caption[Last]>=0xDC00&&Caption[Last]<=0xDFFF&&Caption[Last-1]>=0xD800&&Caption[Last-1]<=0xDBFF;
                Caption.LeftChopInline(Pair?2:1);
                GetTextSize(Caption+TEXT("…"),CaptionWidth,CaptionHeight,AnnouncementFont,TextScale);
            }while(CaptionWidth>Available);
            Caption+=TEXT("…");
        }
        DrawRect(FLinearColor(.04f,.08f,.07f,.9f),Position.X,Position.Y,Extent.X,Extent.Y);
        DrawText(Caption,FLinearColor(.95f,.88f,.65f,1),Position.X+10*UIScale,Position.Y+10*UIScale,AnnouncementFont,TextScale);
    }
#if !UE_BUILD_SHIPPING
    if(!NewAnchorHUD&&Adapter->GetStatus()==TEXT("本地模拟")){DrawRect(FLinearColor(0,0,0,.7),18,Canvas->SizeY-62,630,44);DrawText(TEXT("本地测试：F1 / F2 / F3 加入 100 / 500 / 1000 人 · F4 补怪 · 1–4 切换首位观众武器"),FLinearColor(.7,.85,1),30,Canvas->SizeY-49,GEngine->GetSmallFont());}
#endif
    Samples.Reset();if(!ULiveTitleEffectsComponent::IsPresentationBlocked(GetWorld())&&!Swarm->IsHidden()&&Swarm->GetPresentationMode()==ELiveWeaponPresentationMode::Gameplay)Swarm->GetPresentationSamples(Samples);
    DrawnNameplates=0;
    if(bCollectNameplateDiagnostics){PaintedNameplateSizes.Reset();PaintedNameplateDistances.Reset();}
    for(const auto& Sample:Samples)
    {
        FVector2D Projected;if(!PlayerOwner->ProjectWorldLocationToScreen(Sample.Position,Projected,true))continue;
        if(!ULiveTitleEffectsComponent::IsAnchorVisible(PlayerOwner,Sample.Position,Swarm->TitleEffects&&Swarm->TitleEffects->bOccludeWithScene))continue;
        if(Projected.X<0||Projected.Y<0||Projected.X>Canvas->SizeX||Projected.Y>Canvas->SizeY)continue;
        const float Distance=FVector::Distance(Swarm->GetWeaponPosition(Sample.ParticipantId),Pawn->GetActorLocation());
        const auto Plate=CultivationNameplate::Layout(FMath::Clamp(NameplateScale,.05f,1.f)*CultivationNameplate::DistanceScale(Distance,Swarm->NameplateDistanceSettings));
        FVector2D Origin=Projected-FVector2D(Plate.Size.X*.5,Plate.Size.Y);
        auto Data=CultivationNameplate::Resolve(Cultivation.Get(),Swarm->GetProgressionOwnerId(Sample.ParticipantId),Sample.DisplayName);
        Data.AvatarTexture=Adapter->GetAvatar(Sample.ParticipantId);
        Data.bDrawPlaques=!(Swarm->TitleEffects&&Swarm->TitleEffects->ActiveParticipantIds.Contains(Sample.ParticipantId));
        CultivationNameplate::DrawCanvas(Data,Plate,Origin,Canvas,NameFont?NameFont.Get():GEngine->GetSmallFont(),RealmBanners,RealmChart,WeaponTitles);
        ++DrawnNameplates;Adapter->RequestVisibleAvatar(Sample.ParticipantId);
        if(bCollectNameplateDiagnostics){PaintedNameplateSizes.Add(Sample.ParticipantId,Plate.Size);PaintedNameplateDistances.Add(Sample.ParticipantId,Distance);}
    }
    TArray<TWeakObjectPtr<ALiveMonster>> Monsters;GetWorld()->GetSubsystem<ULiveMonsterRegistrySubsystem>()->CollectInRange(Pawn->GetActorLocation(),5000,Monsters);
    for(auto Weak:Monsters)if(auto* M=Weak.Get())
    {
        FVector2D P;if(!PlayerOwner->ProjectWorldLocationToScreen(M->GetActorLocation()+FVector(0,0,65),P,true)||P.X<0||P.X>Canvas->SizeX||P.Y<0||P.Y>Canvas->SizeY)continue;
        auto& Bar=MonsterProgress.FindOrAdd(M);Bar.SetTarget(M->GetHealthFraction());Bar.Tick(GetWorld()->GetDeltaSeconds());
        DrawRect(FLinearColor(.04f,.015f,.015f,.9f),P.X-28,P.Y,56,7);DrawRect(FLinearColor(.28f,.08f,.05f,1),P.X-27,P.Y+1,54*Bar.Trail,5);DrawRect(FLinearColor(.95f,.22f,.12f,1),P.X-27,P.Y+1,54*Bar.Value,5);
    }
    for(auto It=MonsterProgress.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
    Adapter->NotifyPresented();
    DrawGMPanel();
}
void ALiveWeaponHUD::DrawGMPanel()
{
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("LiveStress")))return;
    for(TActorIterator<ALiveGMOverlay> It(GetWorld());It;++It)if(It->Panel)return;
    auto* Mode=GetWorld()->GetAuthGameMode<ALiveWeaponGameMode>();if(!Mode||!Mode->Audience)return;
    const float Width=336,X=FMath::Max(18.f,float(Canvas->SizeX)-Width-18),Y=190*FMath::Max(.01f,UWidgetLayoutLibrary::GetViewportScale(this));
    UFont* Font=NameFont?NameFont.Get():GEngine->GetSmallFont();
    auto Text=[&](const FString& Label,float PX,float PY,FLinearColor Color=FLinearColor::White){DrawText(Label,Color,PX,PY,Font,.9f);};
    auto Button=[&](FName Id,const FString& Label,float PX,float PY,float W,bool Enabled=true)
    {
        DrawRect(Enabled?FLinearColor(.08f,.22f,.32f,1):FLinearColor(.08f,.10f,.14f,1),PX,PY,W,32);
        Text(Label,PX+10,PY+7,Enabled?FLinearColor(.85f,.96f,1):FLinearColor(.40f,.46f,.52f));
        if(Enabled)AddHitBox(FVector2D(PX,PY),FVector2D(W,32),Id,true,100);
    };
    DrawRect(FLinearColor(.015f,.025f,.055f,.96f),X,Y,Width,bGMPanelOpen?566:52);
    Text(TEXT("GM · 剑灵测试"),X+16,Y+18,FLinearColor(.5f,.9f,1));
    Button(TEXT("GMToggle"),bGMPanelOpen?TEXT("收起 F10"):TEXT("展开 F10"),X+Width-100,Y+10,88);
    if(!bGMPanelOpen)return;
    const bool Local=Mode->Audience->GetStatus()==TEXT("本地模拟");
    Text(Local?TEXT("本地模拟已开启 · 上限 1000 名"):TEXT("第 1 步：开启本地模拟，再添加剑灵"),X+16,Y+58);
    Button(TEXT("EnableTest"),Local?TEXT("本地模拟已开启"):TEXT("开启本地模拟"),X+16,Y+84,304,!Local);
    Button(TEXT("Add1"),TEXT("+1 剑灵"),X+16,Y+126,148,Local);Button(TEXT("Add10"),TEXT("+10 剑灵"),X+172,Y+126,148,Local);
    Button(TEXT("Add100"),TEXT("+100 剑灵"),X+16,Y+168,148,Local);Button(TEXT("Fill1000"),TEXT("补足 1000 剑灵"),X+172,Y+168,148,Local);
    const FName Actions[]={TEXT("Sword"),TEXT("Axe"),TEXT("Spear"),TEXT("Bow")};const TCHAR* Labels[]={TEXT("全体剑"),TEXT("全体斧"),TEXT("全体枪"),TEXT("全体弓")};
    for(int I=0;I<4;++I)Button(Actions[I],Labels[I],X+16+I*78,Y+210,70,Local);
    Button(TEXT("SpawnMonsters"),TEXT("生成 12 只怪物"),X+16,Y+252,148,Local);Button(TEXT("ClearMonsters"),TEXT("清空怪物"),X+172,Y+252,148,Local);
    Button(TEXT("ClearAudience"),TEXT("清空测试剑灵"),X+16,Y+294,304,Local);
    Button(TEXT("Hitboxes"),Mode->Swarm->bDrawAttackHitboxes?TEXT("隐藏攻击检测框"):TEXT("显示攻击检测框"),X+16,Y+336,304,Local);
    const TCHAR* GiftLabels[]={TEXT("仙女棒"),TEXT("爱的爆炸"),TEXT("稀有宝箱"),TEXT("超能喷射"),TEXT("神秘空投"),TEXT("草莓甜点"),TEXT("超级空投"),TEXT("生命药水")};
    for(int32 I=0;I<8;++I)Button(FName(*FString::Printf(TEXT("Gift%d"),I)),GiftLabels[I],X+16+(I%2)*156,Y+378+(I/2)*36,148,Local);
    Text(GMFeedback.IsEmpty()?TEXT("开发礼物测试 · 每次一份"):GMFeedback,X+16,Y+532,FLinearColor(.6f,.8f,.9f));
#endif
}
void ALiveWeaponHUD::NotifyHitBoxClick(FName BoxName)
{
    Super::NotifyHitBoxClick(BoxName);
#if !UE_BUILD_SHIPPING
    if(BoxName==TEXT("GMToggle")){bGMPanelOpen=!bGMPanelOpen;return;}
    if(BoxName.ToString().StartsWith(TEXT("Gift")))
    {
        auto* Mode=GetWorld()->GetAuthGameMode<ALiveWeaponGameMode>();auto* Host=GetGameInstance()?GetGameInstance()->GetSubsystem<ULiveInteractionSubsystem>():nullptr;
        if(!Mode||!Mode->Swarm||!Mode->Audience||!Host||!Host->IsLocalTestMode()){GMFeedback=TEXT("需显式开发会话");return;}
        const int32 Index=FCString::Atoi(*BoxName.ToString().Mid(4));if(Index<0||Index>=8)return;
        static const TCHAR* Gifts[]={TEXT("仙女棒"),TEXT("爱的爆炸"),TEXT("稀有宝箱"),TEXT("超能喷射"),TEXT("神秘空投"),TEXT("草莓甜点"),TEXT("超级空投"),TEXT("生命药水")};
        if(Mode->Swarm->GetParticipantCount()==0)Mode->Audience->AppendSimulatedAudience(1);
        for(const auto& S:Mode->Swarm->GetRoster().GetStates())if(S.bParticipating&&!S.bPersonalSword)
        {
            FString Id=S.Profile.ParticipantId;Id.RemoveFromStart(TEXT("local:"));GMFeedback=Host->SimulateGift(Id,S.Profile.DisplayName,Gifts[Index],1)?TEXT("测试礼物已提交：")+FString(Gifts[Index]):TEXT("测试礼物未提交");return;
        }
        return;
    }
    if(auto* Controller=Cast<ALiveAnchorController>(PlayerOwner))
    {
        const bool Applied=Controller->ExecuteGMAction(BoxName);
        GMFeedback=Applied?TEXT("操作完成"):TEXT("未执行：人数已满或会话不支持本地模拟");
    }
#endif
}
void ALiveWeaponHUD::HandleAnnouncement(const FString& Text){Announcement=Text;AnnouncementUntil=GetWorld()?GetWorld()->GetTimeSeconds()+5.:0;}
void ALiveWeaponHUD::EndPlay(const EEndPlayReason::Type Reason)
{
    if(auto* State=Cultivation.Get())State->OnAnnouncement.RemoveDynamic(this,&ThisClass::HandleAnnouncement);Cultivation.Reset();Super::EndPlay(Reason);
}

