#include "LiveGMOverlay.h"
#include "LiveGMTestSession.h"
#include "LiveGMControlPanel.h"
#include "LiveTitleEffectsComponent.h"
#include "Misc/CommandLine.h"
#include "LiveAudienceAdapterComponent.h"
#include "LiveWeaponSwarm.h"
#include "LiveMonster.h"
#include "LiveMonsterRegistrySubsystem.h"
#include "LiveWeaponHUD.h"
#include "LiveWeaponGameMode.h"
#include "Frontend/InkMenuDirector.h"
#include "Frontend/InkFrontendWidget.h"
#include "Frontend/InkAnchorStatusWidget.h"
#include "Frontend/UI/InkUiResponsiveLayout.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Cultivation/CultivationHealthComponent.h"
#include "Cultivation/CultivationNameplate.h"
#include "LiveInteractionSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Styling/CoreStyle.h"
#include "InputCoreTypes.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"

namespace
{
    bool HasVisibleAnchorStatus(UWorld* World,APlayerController* Player)
    {
        for(TActorIterator<AInkMenuDirector> It(World);It;++It)
        {
            const auto* Frontend=It->FrontendWidget.Get();
            const auto* StatusWidget=Frontend?Frontend->AnchorStatusWidget.Get():nullptr;
            if(Frontend&&Frontend->GetOwningPlayer()==Player&&Frontend->IsInViewport()&&Frontend->IsVisible()&&Frontend->GetRenderOpacity()>0&&Frontend->CurrentPage==TEXT("HUD")
                &&IsValid(StatusWidget)&&StatusWidget->IsVisible()&&StatusWidget->GetRenderOpacity()>0&&StatusWidget->bHasAnchor)return true;
        }
        return false;
    }
}

ALiveGMOverlay::ALiveGMOverlay()
{
    PrimaryActorTick.bCanEverTick=true;
    TestSession=CreateDefaultSubobject<ULiveGMTestSession>(TEXT("GMTestSession"));
    OwnedAudience=CreateDefaultSubobject<ULiveAudienceAdapterComponent>(TEXT("GMAudience"));Audience=OwnedAudience;
    SwarmClass=ALiveWeaponSwarm::StaticClass();MonsterClass=ALiveMonster::StaticClass();
}
void ALiveGMOverlay::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    auto* PC=GetWorld()->GetFirstPlayerController();if(!PC||!PC->IsLocalController()||!PC->GetPawn())return;
    ALiveWeaponSwarm* RuntimeSwarm=nullptr;ULiveAudienceAdapterComponent* RuntimeAudience=nullptr;
    for(TActorIterator<AInkMenuDirector> It(GetWorld());It;++It)
        if(It->bGameplaySystemsReady&&!It->bMenuActive){RuntimeSwarm=It->GameplaySwarm;RuntimeAudience=It->GameplayAudience;break;}
    if(!RuntimeSwarm)if(auto* Mode=GetWorld()->GetAuthGameMode<ALiveWeaponGameMode>()){RuntimeSwarm=Mode->Swarm;RuntimeAudience=Mode->Audience;}
    if(RuntimeSwarm&&RuntimeAudience&&(Swarm!=RuntimeSwarm||Audience!=RuntimeAudience))
    {
        if(!bBorrowingGameplaySystems&&Swarm&&Swarm!=RuntimeSwarm){if(OwnedAudience)OwnedAudience->Unbind();Swarm->Destroy();}
        Swarm=RuntimeSwarm;Audience=RuntimeAudience;bBorrowingGameplaySystems=Audience!=OwnedAudience;Anchor=RuntimeSwarm->GetAnchor();
    }
    if(!Panel)
    {
        Panel=CreateWidget<ULiveGMOverlayWidget>(PC);if(!Panel)return;
        Panel->Configure(this);Panel->AddToPlayerScreen(100);
    }
    if(Swarm&&Anchor.Get()!=PC->GetPawn()){Anchor=PC->GetPawn();Swarm->SetAnchor(Anchor.Get());}
    Panel->bDrawCultivationPresentation=Cast<ALiveWeaponHUD>(PC->GetHUD())==nullptr;
    for(TActorIterator<AInkMenuDirector> It(GetWorld());It;++It)if(It->bMenuActive||(It->FrontendWidget&&It->FrontendWidget->CurrentPage!=TEXT("HUD")))Panel->bDrawCultivationPresentation=false;
    if(Panel->bPanelOpen)Panel->bDrawCultivationPresentation=false;
#if !UE_BUILD_SHIPPING
    if(PC->WasInputKeyJustPressed(EKeys::F10))Panel->TogglePanel();
#endif
}
bool ALiveGMOverlay::IsLocalSimulation()const{return Swarm&&Audience&&Audience->GetStatus()==TEXT("本地模拟");}
bool ALiveGMOverlay::ExecuteGMAction(FName Action)
{
#if !UE_BUILD_SHIPPING
    if(Action==TEXT("GMToggle")){if(Panel)Panel->TogglePanel();return true;}
    auto* PC=GetWorld()->GetFirstPlayerController();if(!PC||!PC->GetPawn())return false;
    if(Action==TEXT("EnableTest"))
    {
        if(!Swarm)
        {
            Swarm=GetWorld()->SpawnActor<ALiveWeaponSwarm>(SwarmClass);if(!Swarm)return false;
            Anchor=PC->GetPawn();Swarm->SetAnchor(Anchor.Get());Audience->Bind(Swarm);
        }
        const bool Enabled=Audience->EnableDevelopmentSimulation();
        if(!Enabled&&!bBorrowingGameplaySystems){Audience->Unbind();Swarm->Destroy();Swarm=nullptr;Anchor.Reset();}
        return Enabled;
    }
    if(!IsLocalSimulation())return false;
    const FString ActionText=Action.ToString();
    if(ActionText.StartsWith(TEXT("Gift")))
    {
        auto* Host=GetGameInstance()?GetGameInstance()->GetSubsystem<ULiveInteractionSubsystem>():nullptr;
        if(!Host||!Host->IsLocalTestMode())return false;
        static const TCHAR* Gifts[]={TEXT("仙女棒"),TEXT("爱的爆炸"),TEXT("稀有宝箱"),TEXT("超能喷射"),TEXT("神秘空投"),TEXT("草莓甜点"),TEXT("超级空投"),TEXT("生命药水")};
        const int32 Index=FCString::Atoi(*ActionText.Mid(4));if(Index<0||Index>=8)return false;
        if(Swarm->GetParticipantCount()==0)Audience->AppendSimulatedAudience(1);
        for(const auto& S:Swarm->GetRoster().GetStates())if(S.bParticipating&&!S.bPersonalSword)
        {
            FString RawId=S.Profile.ParticipantId;RawId.RemoveFromStart(TEXT("local:"));
            const bool Applied=Host->SimulateGift(RawId,S.Profile.DisplayName,Gifts[Index],1);
            Feedback=Applied?FString(TEXT("测试礼物已提交："))+Gifts[Index]+TEXT(" ×1"):TEXT("测试礼物未提交：需显式开发会话");return Applied;
        }
        return false;
    }
    if(Action==TEXT("Add1"))return Audience->AppendSimulatedAudience(1)>0;
    if(Action==TEXT("Add10"))return Audience->AppendSimulatedAudience(10)>0;
    if(Action==TEXT("Add100"))return Audience->AppendSimulatedAudience(100)>0;
    if(Action==TEXT("Fill1000"))return Audience->AppendSimulatedAudience(1000)>0;
    if(Action==TEXT("ClearAudience"))return Audience->ClearSimulatedAudience();
    if(Action==TEXT("Hitboxes")){Swarm->bDrawAttackHitboxes=!Swarm->bDrawAttackHitboxes;return true;}
    const FName Types[]={TEXT("Sword"),TEXT("Axe"),TEXT("Spear"),TEXT("Bow")};
    for(int32 I=0;I<4;++I)if(Action==Types[I])
    {
        for(const auto& State:Swarm->GetRoster().GetStates())Swarm->SwitchParticipantWeapon(State.Profile.ParticipantId,ELiveWeaponType(I));
        return true;
    }
    if(Action==TEXT("ClearMonsters"))
    {
        // Never remove pre-existing level monsters: only objects created by this panel.
        for(auto Weak:TestMonsters)if(auto* Monster=Weak.Get())Monster->Destroy();TestMonsters.Reset();return true;
    }
    if(Action==TEXT("SpawnMonsters"))
    {
        TestMonsters.RemoveAll([](const auto& Weak){return !Weak.IsValid();});
        for(int32 I=0;I<12;++I)
        {
            const float Angle=I*2.399963f,Radius=900+I%5*110;
            FVector Position=PC->GetPawn()->GetActorLocation()+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,0);
            FHitResult Floor;FCollisionQueryParams Params;Params.AddIgnoredActor(PC->GetPawn());
            if(GetWorld()->LineTraceSingleByObjectType(Floor,Position+FVector(0,0,200),Position-FVector(0,0,2000),FCollisionObjectQueryParams(ECC_WorldStatic),Params))Position.Z=Floor.ImpactPoint.Z+45;
            const FTransform Transform(FRotator::ZeroRotator,Position);
            auto* Monster=GetWorld()->SpawnActorDeferred<ALiveMonster>(MonsterClass,Transform,this,nullptr,ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
            if(Monster){Monster->MaxHealth=2000;Monster->FollowTarget=PC->GetPawn();Monster->FinishSpawning(Transform);TestMonsters.Add(Monster);}
        }
        return true;
    }
#endif
    return false;
}
void ALiveGMOverlay::EndPlay(const EEndPlayReason::Type Reason)
{
    if(Panel){Panel->RemoveFromParent();Panel=nullptr;}if(OwnedAudience)OwnedAudience->Unbind();
    if(Swarm&&!bBorrowingGameplaySystems)Swarm->Destroy();for(auto Weak:TestMonsters)if(auto* Monster=Weak.Get())Monster->Destroy();
    Super::EndPlay(Reason);
}
void ULiveGMActionButton::BindAction(ALiveGMOverlay* InHost,FName InAction)
{
    Host=InHost;Action=InAction;InitIsFocusable(false);OnClicked.AddDynamic(this,&ULiveGMActionButton::ApplyAction);
}
void ULiveGMActionButton::ApplyAction()
{
    if(auto* Owner=Host.Get()){const bool Applied=Owner->ExecuteGMAction(Action);if(!Action.ToString().StartsWith(TEXT("Gift")))Owner->Feedback=Applied?TEXT("操作完成"):TEXT("未执行：人数已满或会话不支持本地模拟");}
}
UTextBlock* ULiveGMOverlayWidget::MakeText(const FString& Text,int32 Size)
{
    auto* Label=WidgetTree->ConstructWidget<UTextBlock>();Label->SetText(FText::FromString(Text));
    FSlateFontInfo Font=FCoreStyle::GetDefaultFontStyle("Regular",Size);if(Host.IsValid()&&Host->NameFont)Font.FontObject=Host->NameFont;
    Label->SetFont(Font);Label->SetColorAndOpacity(FLinearColor(.85f,.95f,1));return Label;
}
void ULiveGMOverlayWidget::Configure(ALiveGMOverlay* InHost)
{
    Host=InHost;SetVisibility(ESlateVisibility::SelfHitTestInvisible);SetIsFocusable(false);
    if(InHost&&(!InHost->NameFont||InHost->NameFont->GetPathName()==TEXT("/Game/LiveWeapons/Fonts/F_LiveNames.F_LiveNames")))InHost->NameFont=LoadObject<UFont>(nullptr,TEXT("/Game/UI/Ink/Fonts/Shared/F_InkBody.F_InkBody"),nullptr,LOAD_NoWarn|LOAD_Quiet);
    if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this,TEXT("GMWidgetTree"));
    auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>();Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);WidgetTree->RootWidget=Root;
    auto* Border=WidgetTree->ConstructWidget<UBorder>();Border->SetBrushColor(FLinearColor(.015f,.025f,.055f,.96f));Border->SetPadding(FMargin(12));
    DebugPanel=Border;Border->SetVisibility(bPanelOpen?ESlateVisibility::SelfHitTestInvisible:ESlateVisibility::Collapsed);

#if UE_BUILD_SHIPPING
    Border->SetVisibility(ESlateVisibility::Collapsed);
#endif
    RealmBanners=LoadObject<UTexture2D>(nullptr,TEXT("/Game/UI/Ink/Textures/Shared/T_RealmBanners.T_RealmBanners"),nullptr,LOAD_NoWarn|LOAD_Quiet);
    RealmChart=LoadObject<UTexture2D>(nullptr,TEXT("/Game/UI/Ink/Textures/Shared/T_CohesionIcons.T_CohesionIcons"),nullptr,LOAD_NoWarn|LOAD_Quiet);
    WeaponTitles=LoadObject<UTexture2D>(nullptr,TEXT("/Game/UI/Ink/Textures/Leaderboard/T_LeaderboardTitles.T_LeaderboardTitles"),nullptr,LOAD_NoWarn|LOAD_Quiet);
    auto* PanelSlot=Root->AddChildToCanvas(Border);PanelSlot->SetAnchors(FAnchors(1,0));PanelSlot->SetAlignment(FVector2D(1,0));PanelSlot->SetPosition(FVector2D(-18,190));PanelSlot->SetAutoSize(true);
    Border->SetBrushColor(FLinearColor::Transparent);Border->SetPadding(FMargin(0));
    PanelSlot->SetAutoSize(false);PanelSlot->SetAnchors(FAnchors(1,0,1,1));PanelSlot->SetAlignment(FVector2D(1,0));PanelSlot->SetOffsets(FMargin(-18,140,600,18));
    Controls=CreateWidget<ULiveGMControlPanel>(GetOwningPlayer());Controls->Configure(InHost);Border->SetContent(Controls);

}
void ULiveGMOverlayWidget::TogglePanel()
{
#if !UE_BUILD_SHIPPING
    bPanelOpen=!bPanelOpen;if(DebugPanel)DebugPanel->SetVisibility(bPanelOpen?ESlateVisibility::SelfHitTestInvisible:ESlateVisibility::Collapsed);
    // The frontend HUD uses viewport layer 200. An expanded GM must receive
    // input above its rank cards; restore the player presentation layer on close.
    if(IsInViewport())
    {
        RemoveFromParent();
        if(bPanelOpen)AddToViewport(300);
        else AddToPlayerScreen(100);
    }
#endif
}
void ULiveGMOverlayWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
    Super::NativeTick(Geometry,DeltaSeconds);auto* Owner=Host.Get();if(!Owner)return;
    auto* State=GetGameInstance()?GetGameInstance()->GetSubsystem<UCultivationSubsystem>():nullptr;
    if(Cultivation.Get()!=State)
    {
        if(auto* Old=Cultivation.Get())Old->OnAnnouncement.RemoveDynamic(this,&ThisClass::HandleAnnouncement);
        Cultivation=State;if(State)State->OnAnnouncement.AddDynamic(this,&ThisClass::HandleAnnouncement);
    }
    AnnouncementRemaining=FMath::Max(0.f,AnnouncementRemaining-DeltaSeconds);
    const bool Local=Owner->IsLocalSimulation();
    if(Status)Status->SetText(FText::FromString(Local?FString::Printf(TEXT("本地模拟 · 剑灵 %d / 1000"),Owner->Swarm->GetParticipantCount()):TEXT("开启本地模拟，再添加剑灵")));
    if(Feedback&&!Owner->Feedback.IsEmpty())Feedback->SetText(FText::FromString(Owner->Feedback));
    for(auto Button:ActionButtons)Button->SetIsEnabled(Button->Action==TEXT("GMToggle")||(Button->Action==TEXT("EnableTest")?!Local:Local));
}
void ULiveGMOverlayWidget::HandleAnnouncement(const FString& Text){Announcement=Text;AnnouncementRemaining=5.f;}
void ULiveGMOverlayWidget::NativeDestruct()
{
    if(auto* State=Cultivation.Get())State->OnAnnouncement.RemoveDynamic(this,&ThisClass::HandleAnnouncement);Cultivation.Reset();Super::NativeDestruct();
}
int32 ULiveGMOverlayWidget::NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& CullingRect,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool bParentEnabled)const
{
    auto* Owner=Host.Get();auto* PC=GetOwningPlayer();int32 Highest=Layer;
    auto* Mutable=const_cast<ULiveGMOverlayWidget*>(this);Mutable->DrawnNameplates=0;
    if(bCollectNameplateDiagnostics)
    {
        Mutable->PaintedNameplateAnchors.Reset();Mutable->PaintedRealmBadgeCenters.Reset();Mutable->PaintedWeaponRowCenters.Reset();Mutable->PaintedRealmRowCenters.Reset();Mutable->PaintedNameplateSizes.Reset();Mutable->PaintedNameplateDistances.Reset();
    }
    if(bDrawCultivationPresentation&&!ULiveTitleEffectsComponent::IsPresentationBlocked(GetWorld())&&Owner&&Owner->Swarm&&Owner->Audience&&PC&&!Owner->Swarm->IsHidden()&&Owner->Swarm->GetPresentationMode()==ELiveWeaponPresentationMode::Gameplay)
    {
        auto* State=Cultivation.Get();TArray<FLiveWeaponLabelSample> Samples;Owner->Swarm->GetPresentationSamples(Samples);
        const float Scale=FMath::Max(.01f,UWidgetLayoutLibrary::GetViewportScale(this));
        const auto ViewSize=Geometry.GetLocalSize();const APawn* Pawn=PC->GetPawn();
        for(const auto& Sample:Samples)
        {
            FVector2D Projected;if(!PC->ProjectWorldLocationToScreen(Sample.Position,Projected,true))continue;
            if(!ULiveTitleEffectsComponent::IsAnchorVisible(PC,Sample.Position,Owner->Swarm->TitleEffects&&Owner->Swarm->TitleEffects->bOccludeWithScene))continue;
            if(Projected.X<0||Projected.Y<0||Projected.X>ViewSize.X*Scale||Projected.Y>ViewSize.Y*Scale)continue;
            const float Distance=Pawn?FVector::Distance(Owner->Swarm->GetWeaponPosition(Sample.ParticipantId),Pawn->GetActorLocation()):0.f;
            const auto Plate=CultivationNameplate::Layout(CultivationNameplate::DistanceScale(Distance,Owner->Swarm->NameplateDistanceSettings));
            FVector2D Origin=Projected/Scale-FVector2D(Plate.Size.X*.5f,Plate.Size.Y);
            auto Data=CultivationNameplate::Resolve(State,Owner->Swarm->GetProgressionOwnerId(Sample.ParticipantId),Sample.DisplayName);
            Data.AvatarTexture=Owner->Audience->GetAvatar(Sample.ParticipantId);
            Data.bDrawPlaques=!(Owner->Swarm->TitleEffects&&Owner->Swarm->TitleEffects->ActiveParticipantIds.Contains(Sample.ParticipantId));
            Highest=FMath::Max(Highest,CultivationNameplate::DrawSlate(Data,Plate,Origin,Geometry,Elements,Layer,Owner->NameFont,RealmBanners,RealmChart,WeaponTitles));
            ++Mutable->DrawnNameplates;Owner->Audience->RequestVisibleAvatar(Sample.ParticipantId);
            if(bCollectNameplateDiagnostics)
            {
                Mutable->PaintedNameplateAnchors.Add(Sample.ParticipantId,Projected);
                Mutable->PaintedRealmBadgeCenters.Add(Sample.ParticipantId,(Origin+Plate.Badge.GetCenter())*Scale);
                Mutable->PaintedWeaponRowCenters.Add(Sample.ParticipantId,(Origin+Plate.Weapon.GetCenter())*Scale);
                Mutable->PaintedRealmRowCenters.Add(Sample.ParticipantId,(Origin+Plate.Realm.GetCenter())*Scale);
                Mutable->PaintedNameplateSizes.Add(Sample.ParticipantId,Plate.Size*Scale);
                Mutable->PaintedNameplateDistances.Add(Sample.ParticipantId,Distance);
            }
        }
        const FSlateBrush* White=FCoreStyle::Get().GetBrush("WhiteBrush");
        auto Box=[&](int32 L,FVector2D P,FVector2D S,FLinearColor Color)
        {FSlateDrawElement::MakeBox(Elements,L,Geometry.ToPaintGeometry(FVector2f(S),FSlateLayoutTransform(FVector2f(P))),White,ESlateDrawEffect::None,Color);};
        FSlateFontInfo Font=FCoreStyle::GetDefaultFontStyle("Regular",16);if(Owner->NameFont)Font.FontObject=Owner->NameFont;
        auto Text=[&](const FString& Value,FVector2D Position,FLinearColor Color=FLinearColor::White)
        {FSlateDrawElement::MakeText(Elements,Layer+5,Geometry.ToPaintGeometry(FVector2f(420,30),FSlateLayoutTransform(FVector2f(Position))),Value,Font,ESlateDrawEffect::None,Color);};
        // Keep the standalone fallback when the frontend is absent, hidden, or has no live anchor yet.
        const bool NewAnchorHUD=HasVisibleAnchorStatus(GetWorld(),PC);
        if(State&&!NewAnchorHUD)
        {
                const auto Anchor=State->GetParticipantSnapshot(State->GetAnchorId());const auto* Health=State->GetAnchorHealth();
            if(!Anchor.ParticipantId.IsEmpty()&&Health)
            {
                Box(Layer+3,FVector2D(18,18),FVector2D(262,94),FLinearColor(.025f,.04f,.035f,.82f));
                Text(Anchor.DisplayName+TEXT(" · ")+Anchor.RealmName.ToString(),FVector2D(28,24),FLinearColor(.89f,.87f,.70f,1));
                if(AnchorProgressContext.Update(Anchor.ParticipantId,Anchor.RealmIndex,Health->MaxHealth,Anchor.RequiredCultivation)){AnchorHealthProgress.Reset(Health->GetHealthFraction());AnchorCultivationProgress.Reset(Anchor.RequiredCultivation>0?float(Anchor.Cultivation/Anchor.RequiredCultivation):1);}
                AnchorHealthProgress.SetTarget(Health->GetHealthFraction());AnchorHealthProgress.Tick(GetWorld()->GetDeltaSeconds());
                Box(Layer+3,FVector2D(28,46),FVector2D(240,10),FLinearColor(.12f,.14f,.13f,1));Box(Layer+4,FVector2D(28,46),FVector2D(240*AnchorHealthProgress.Trail,10),FLinearColor(.26f,.10f,.08f,1));Box(Layer+5,FVector2D(28,46),FVector2D(240*AnchorHealthProgress.Value,10),FLinearColor(.72f,.26f,.20f,1));
                Text(FString::Printf(TEXT("血量 %.0f / %.0f"),Health->Health,Health->MaxHealth),FVector2D(28,58));
                const float XP=Anchor.RequiredCultivation>0?FMath::Clamp(float(Anchor.Cultivation/Anchor.RequiredCultivation),0.f,1.f):1.f;
                AnchorCultivationProgress.SetTarget(XP);AnchorCultivationProgress.Tick(GetWorld()->GetDeltaSeconds());
                Box(Layer+3,FVector2D(28,80),FVector2D(240,8),FLinearColor(.12f,.14f,.13f,1));Box(Layer+4,FVector2D(28,80),FVector2D(240*FMath::Max(XP,AnchorCultivationProgress.Trail),8),FLinearColor(.10f,.22f,.19f,1));Box(Layer+5,FVector2D(28,80),FVector2D(240*FMath::Min(XP,AnchorCultivationProgress.Trail),8),FLinearColor(.32f,.53f,.48f,1));
                Text(FString::Printf(TEXT("修为 %.0f / %.0f · %.1f/秒"),Anchor.Cultivation,Anchor.RequiredCultivation,Anchor.EffectiveCultivationPerSecond),FVector2D(28,91));
            }
        }
        if(AnnouncementRemaining>0&&!Announcement.IsEmpty())
        {
            const auto Rect=InkUiLayout::ResolveHud(Geometry.GetLocalSize()).Announcement;
            const float Available=FMath::Max(0.f,Rect.GetSize().X-20.f);
            FString Caption=Announcement.Replace(TEXT("\r"),TEXT(" ")).Replace(TEXT("\n"),TEXT(" "));
            const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
            if(Measure->Measure(Caption,Font).X>Available)
            {
                while(!Caption.IsEmpty()&&Measure->Measure(Caption+TEXT("…"),Font).X>Available)
                {
                    const int32 Last=Caption.Len()-1;
                    const bool Pair=Last>0&&Caption[Last]>=0xDC00&&Caption[Last]<=0xDFFF&&Caption[Last-1]>=0xD800&&Caption[Last-1]<=0xDBFF;
                    Caption.LeftChopInline(Pair?2:1);
                }
                Caption+=TEXT("…");
            }
            Box(Layer+4,Rect.GetTopLeft(),Rect.GetSize(),FLinearColor(.04f,.08f,.07f,.88f));
            Elements.PushClip(FSlateClippingZone(Geometry.MakeChild(FVector2f(Available,30),FSlateLayoutTransform(FVector2f(Rect.Left+10,Rect.Top+5)))));
            Text(Caption,FVector2D(Rect.Left+10,Rect.Top+11),FLinearColor(.95f,.88f,.65f,1));
            Elements.PopClip();
        }
        TArray<TWeakObjectPtr<ALiveMonster>> Monsters;
        if(PC->GetPawn())GetWorld()->GetSubsystem<ULiveMonsterRegistrySubsystem>()->CollectInRange(PC->GetPawn()->GetActorLocation(),5000,Monsters);
        for(auto Weak:Monsters)if(auto* Monster=Weak.Get())
        {
            FVector2D P;if(!PC->ProjectWorldLocationToScreen(Monster->GetActorLocation()+FVector(0,0,65),P,true))continue;P=P/Scale;
            auto& Bar=MonsterProgress.FindOrAdd(Monster);Bar.SetTarget(Monster->GetHealthFraction());Bar.Tick(GetWorld()->GetDeltaSeconds());
            Box(Layer+3,P-FVector2D(28,0),FVector2D(56,7),FLinearColor(.04f,.015f,.015f,.9f));Box(Layer+4,P-FVector2D(27,-1),FVector2D(54*Bar.Trail,5),FLinearColor(.28f,.08f,.05f,1));Box(Layer+5,P-FVector2D(27,-1),FVector2D(54*Bar.Value,5),FLinearColor(.95f,.22f,.12f,1));
        }
        for(auto It=MonsterProgress.CreateIterator();It;++It)if(!It.Key().IsValid())It.RemoveCurrent();
        Owner->Audience->NotifyPresented();Highest=FMath::Max(Highest,Layer+6);
    }
    return Super::NativePaint(Args,Geometry,CullingRect,Elements,Highest,Style,bParentEnabled);
}
