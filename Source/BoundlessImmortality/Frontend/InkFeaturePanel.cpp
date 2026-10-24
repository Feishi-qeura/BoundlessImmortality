#include "InkFeaturePanel.h"
#include "Frontend/UiRollout20261024/LogicJourney/InkJourneyAdapter.h"
#include "Frontend/UiRollout20261024/Journey/JourneyRolloutWidgets.h"
#include "Frontend/UiRollout20261024/LogicJourney/InkIntegratedActivityPanels.h"
#include "Frontend/UI/InkUiAssetPaths.h"
#include "WulingFrontendData.h"
#include "InkFrontendWidget.h"
#include "UI/InkUiTheme.h"
#include "UI/InkWulingToolsPanel.h"
#include "UI/InkWulingRecordsPanel.h"
#include "UI/InkWulingSectPanel.h"
#include "UI/InkGiftRulesPanel.h"
#include "UI/InkLaunchEventPanel.h"
#include "InkPermanentActivitiesPanel.h"
#include "UI/InkJourneyMapPanel.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/ScaleBox.h"
#include "Components/SafeZone.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ButtonSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WidgetSwitcherSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
    UCanvasPanelSlot* Place(UCanvasPanel* Parent,UWidget* Widget,float X,float Y,float W,float H)
    {auto* S=Parent->AddChildToCanvas(Widget);S->SetPosition(FVector2D(X,Y));S->SetSize(FVector2D(W,H));return S;}
    void ClearCanvas(UCanvasPanel* Canvas)
    {
        TArray<UWidget*> Old;
        TFunction<void(UWidget*)> Collect=[&](UWidget* W){if(auto* P=Cast<UPanelWidget>(W))for(auto* C:P->GetAllChildren()){Collect(C);Old.Add(C);}};
        Collect(Canvas);Canvas->ClearChildren();
        for(auto* W:Old)W->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);
    }
}

TSubclassOf<UInkFeaturePanel> UInkFeaturePanel::ClassForRoute(FName Route)
{
    auto NewWidget=[](const TCHAR* Path,UClass* Fallback)->TSubclassOf<UInkFeaturePanel>
    {if(auto* Class=LoadClass<UInkFeaturePanel>(nullptr,Path,nullptr,LOAD_NoWarn|LOAD_Quiet))return Class;return Fallback;};
    if(Route==TEXT("CharacterProfile"))
        if(auto* Class=LoadClass<UInkFeaturePanel>(nullptr,TEXT("/Game/newUI/Ink/Widgets/Character/WBP_CharacterProfile.WBP_CharacterProfile_C"),nullptr,LOAD_NoWarn|LOAD_Quiet))return Class;
    if(Route==TEXT("Wuling"))return NewWidget(TEXT("/Game/newUI/Ink/Widgets/Wuling/WBP_WulingSect.WBP_WulingSect_C"),UInkWulingSectPanel::StaticClass());
    if(Route==TEXT("GiftRules"))return UInkGiftRulesPanel::StaticClass();
    if(Route==TEXT("WulingWorld"))return UInkWulingPanel::StaticClass();
    if(Route==TEXT("LaunchEvent"))return NewWidget(TEXT("/Game/newUI/Ink/Widgets/Activities/WBP_LaunchEvent.WBP_LaunchEvent_C"),UInkLaunchEventPanel::StaticClass());
    if(Route==TEXT("LaunchRank"))return UInkLaunchEventPanel::StaticClass();
    if(Route==TEXT("Extensions")||Route==TEXT("WorldCultivation")||Route==TEXT("ResourceMap"))return NewWidget(TEXT("/Game/newUI/Ink/Widgets/Journey/WBP_JourneyMap.WBP_JourneyMap_C"),UInkJourneyMapPanel::StaticClass());
    if(Route==TEXT("JourneyWorldDetail")||Route==TEXT("JourneyResourceDetail")||Route==TEXT("RegionDetail")||Route==TEXT("ResourceDetail"))return NewWidget(TEXT("/Game/newUI/Ink/Widgets/Journey/WBP_JourneyDestination.WBP_JourneyDestination_C"),UInkJourneyDestinationPanel::StaticClass());
    if(Route==TEXT("WulingGuild")||Route==TEXT("WulingOrders")||Route==TEXT("WulingOffline")||Route==TEXT("WulingRights"))return UInkWulingRecordsPanel::StaticClass();
    if(Route==TEXT("WulingCraft")||Route==TEXT("WulingSources")||Route==TEXT("WulingBreakthrough")||Route==TEXT("WulingEquipment")||Route==TEXT("WulingConstruction")||Route==TEXT("WulingActivity")||Route==TEXT("WulingRace")||Route==TEXT("WulingShop")||Route==TEXT("WulingMarket"))return UInkWulingToolsPanel::StaticClass();
    static const TMap<FName,TSubclassOf<UInkFeaturePanel>> Routes={
        {TEXT("Extensions"),UInkFeatureHubPanel::StaticClass()},{TEXT("SignIn"),UInkSignInGroupPanel::StaticClass()},
        {TEXT("MonthlySignIn"),UInkMonthlySignInPanel::StaticClass()},{TEXT("DailyFortune"),UInkDailyFortunePanel::StaticClass()},
        {TEXT("Activities"),UInkIntegratedActivitiesPanel::StaticClass()},{TEXT("DailyActivities"),UInkIntegratedDailyPanel::StaticClass()},
        {TEXT("TimedActivities"),UInkIntegratedLimitedPanel::StaticClass()},{TEXT("PermanentActivities"),UInkIntegratedPermanentPanel::StaticClass()},{TEXT("Draw"),UInkDrawPanel::StaticClass()},
        {TEXT("Expedition"),UInkExpeditionPanel::StaticClass()},{TEXT("WorldCultivation"),UInkWorldCultivationPanel::StaticClass()},
        {TEXT("ResourceContest"),UInkResourceContestPanel::StaticClass()},{TEXT("CharacterProfile"),UInkCharacterProfilePanel::StaticClass()},
        {TEXT("CharacterEquipment"),UInkCharacterEquipmentPanel::StaticClass()},{TEXT("CharacterSkills"),UInkCharacterSkillsPanel::StaticClass()},
        {TEXT("CharacterArtifacts"),UInkCharacterArtifactsPanel::StaticClass()},{TEXT("CharacterFate"),UInkCharacterFatePanel::StaticClass()},
        {TEXT("CharacterAppearance"),UInkCharacterAppearancePanel::StaticClass()},
        {TEXT("DailyActivityDetail"),UInkDailyActivityDetailPanel::StaticClass()},{TEXT("TimedActivityDetail"),UInkTimedActivityDetailPanel::StaticClass()},
        {TEXT("ExpeditionDetail"),UInkExpeditionDetailPanel::StaticClass()},{TEXT("RegionDetail"),UInkRegionDetailPanel::StaticClass()},
        {TEXT("ResourceDetail"),UInkResourceDetailPanel::StaticClass()},{TEXT("ResourceRanking"),UInkResourceRankingPanel::StaticClass()},
        {TEXT("ResourceRewards"),UInkResourceRewardsPanel::StaticClass()}};
    if(const auto* Found=Routes.Find(Route))return *Found;
    return nullptr;
}

void UInkFeaturePanel::Configure(UInkFrontendWidget* InFrontend,FName InPage)
{
    FrontendOwner=InFrontend;PageId=InPage;PageOpenElapsed=0;SetIsFocusable(true);
    if(!bSurfaceInitialized)RebuildPanel();
    RefreshLiveData();
}

void UInkFeaturePanel::SetContext(const FString& Entry)
{if(SelectedEntry!=Entry){SelectedEntry=Entry;RebuildPanel();}}

void UInkFeaturePanel::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
    Super::NativeTick(Geometry,DeltaSeconds);
    if(!bEmbedded&&FrontendOwner&&FrontendOwner->CurrentPage==PageId&&
        (PageId==TEXT("SignIn")||(PageId==TEXT("DailyActivityDetail")&&SelectedEntry==TEXT("ACT_SIGN"))))RefreshLiveData();
    if(bEmbedded)return;
    PageOpenElapsed+=FMath::Max(0.f,DeltaSeconds);
    const bool Reduced=FrontendOwner&&FrontendOwner->bReducedMotion;
    const float Alpha=FMath::Clamp(PageOpenElapsed/(Reduced?.12f:.28f),0.f,1.f);
    if(ShellCanvas){ShellCanvas->SetRenderOpacity(Alpha);ShellCanvas->SetRenderTranslation(FVector2D(0,Reduced?0:12*(1-Alpha)));}
    const auto Size=Geometry.GetLocalSize();if(Size.X<=0||Size.Y<=0)return;
    if(UsesResponsiveShell())
    {
        if(!bHasLayout||!Size.Equals(ResponsiveSafeSize,.5)){bHasLayout=true;ResponsiveSafeSize=Size;bPortraitLayout=Size.X/Size.Y<1.25;RebuildPanel();}
        return;
    }
    const bool Portrait=Size.X<Size.Y;
    if(!bHasLayout||Portrait!=bPortraitLayout){bHasLayout=true;bPortraitLayout=Portrait;RebuildPanel();}
}

void UInkFeaturePanel::RebuildPanel()
{
    if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this);
    if(!bEmbedded&&UsesResponsiveShell()){RebuildResponsivePanel();return;}
    if(bEmbedded)
    {
        if(BodyCanvas)ClearCanvas(BodyCanvas);
        else
        {
            BodyCanvas=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("EmbeddedFeatureBody"));
            if(UsesResponsiveShell())
            {
                auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("SignInContentScroll"));
                Scroll->SetAllowOverscroll(false);Scroll->SetScrollbarThickness(FVector2D(6,6));
                EmbeddedContentSize=WidgetTree->ConstructWidget<USizeBox>();Scroll->AddChild(EmbeddedContentSize);EmbeddedContentSize->SetContent(BodyCanvas);
                WidgetTree->RootWidget=Scroll;
            }
            else WidgetTree->RootWidget=BodyCanvas;
        }
        BodyCanvas->SetClipping(EWidgetClipping::ClipToBoundsAlways);BuildPanelContent();bSurfaceInitialized=true;
        if(EmbeddedContentSize)
        {
            float Extent=BodyHeight;
            for(auto* Child:BodyCanvas->GetAllChildren())if(auto* S=Cast<UCanvasPanelSlot>(Child->Slot))Extent=FMath::Max(Extent,float(S->GetPosition().Y+S->GetSize().Y+16));
            EmbeddedContentSize->SetHeightOverride(Extent);EmbeddedContentSize->SetWidthOverride(BodyWidth);
        }
        if(FrontendOwner)FrontendOwner->RefreshExtensionActions();return;
    }
    if(!ShellCanvas)
    {
        auto* Root=WidgetTree->ConstructWidget<UOverlay>();WidgetTree->RootWidget=Root;
        auto* Dim=WidgetTree->ConstructWidget<UBorder>();Dim->SetBrushColor(FLinearColor(.025f,.035f,.04f,.68f));
        auto* DimLayout=Root->AddChildToOverlay(Dim);DimLayout->SetHorizontalAlignment(HAlign_Fill);DimLayout->SetVerticalAlignment(VAlign_Fill);
        auto* Safe=WidgetTree->ConstructWidget<USafeZone>();Safe->SetSidesToPad(true,true,true,true);
        auto* SafeSlot=Root->AddChildToOverlay(Safe);SafeSlot->SetPadding(FMargin(12));SafeSlot->SetHorizontalAlignment(HAlign_Fill);SafeSlot->SetVerticalAlignment(VAlign_Fill);
        auto* Scale=WidgetTree->ConstructWidget<UScaleBox>();Scale->SetStretch(EStretch::ScaleToFit);Safe->SetContent(Scale);
        PanelSize=WidgetTree->ConstructWidget<USizeBox>();Scale->SetContent(PanelSize);
        ShellCanvas=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("FeatureShell"));PanelSize->SetContent(ShellCanvas);
    }
    else ClearCanvas(ShellCanvas);
    const float Width=bPortraitLayout?720:1180,Height=bPortraitLayout?1220:760;
    PanelSize->SetWidthOverride(Width);PanelSize->SetHeightOverride(Height);
    BodyWidth=Width-88;BodyHeight=Height-230;
    auto* Background=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("FeaturePaper"));
    FSlateBrush Brush=InkPresentationStyle::Surface(InkPresentationStyle::ESurface::FeaturePaper,FVector2D(Width,Height));
    Background->SetBrush(Brush);Background->SetVisibility(ESlateVisibility::HitTestInvisible);Place(ShellCanvas,Background,0,0,Width,Height);
    Surface(ShellCanvas,TEXT("FeatureHeadingArt"),InkPresentationStyle::ESurface::FeatureHeading,34,30,Width-68,74);
    Label(ShellCanvas,TEXT("FeatureHeading"),PanelTitle().ToString(),60,42,Width-180,54,36);
    Action(ShellCanvas,TEXT("Close"),TEXT("×"),Width-92,42,44,44);
    BodyCanvas=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("FeatureBody"));Place(ShellCanvas,BodyCanvas,44,124,BodyWidth,BodyHeight);BodyCanvas->SetClipping(EWidgetClipping::ClipToBoundsAlways);
    BuildPanelContent();
    StatusText=Label(ShellCanvas,TEXT("FeatureFeedback"),PreviewFeedback,48,Height-100,Width-300,62,18);
    StatusText->SetColorAndOpacity(FLinearColor(.53f,.22f,.14f,1));
    Action(ShellCanvas,TEXT("Back"),TEXT("返回"),Width-238,Height-90,184,54);
    bSurfaceInitialized=true;
    if(FrontendOwner)FrontendOwner->RefreshExtensionActions();
}

UTextBlock* UInkFeaturePanel::Label(UCanvasPanel* Parent,FName Name,const FString& Value,float X,float Y,float W,float H,int32 FontSize,bool bCenter)
{
    FontSize=FMath::Max(24,FontSize);H=FMath::Max(32.f,H);
    auto* T=WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);
    auto Font=InkUiTypography::MakeFont(FontSize>=40?EInkUiTextRole::Display:EInkUiTextRole::Body,FontSize);T->SetFont(Font);
    T->SetText(FText::FromString(Value));T->SetAutoWrapText(true);T->SetColorAndOpacity(InkPresentationStyle::Ink());
    T->SetJustification(bCenter?ETextJustify::Center:ETextJustify::Left);T->SetVisibility(ESlateVisibility::HitTestInvisible);
    Place(Parent,T,X,Y,W,H)->SetZOrder(PageId==TEXT("CharacterProfile")?2:4);return T;
}

UInkFrontendActionButton* UInkFeaturePanel::Action(UCanvasPanel* Parent,const FString& Id,const FString& Caption,float X,float Y,float W,float H,bool bPrimary,bool bBlue,bool bSelected)
{
    const FName Name(*(TEXT("FeatureAction_")+Id.Replace(TEXT("|"),TEXT("_"))));
    auto* B=WidgetTree->ConstructWidget<UInkFrontendActionButton>(UInkFrontendActionButton::StaticClass(),MakeUniqueObjectName(WidgetTree,UInkFrontendActionButton::StaticClass(),Name));
    B->Action=FName(*(TEXT("Ext_")+Id));B->BindFrontend(FrontendOwner);
    const bool Close=Id==TEXT("Close"),Light=!Close&&(bPrimary||bSelected||bBlue);
    auto Style=Close?InkPresentationStyle::CloseButton():InkPresentationStyle::FeatureButton(bSelected,bPrimary,bBlue);Style.SetNormalPadding(FMargin(0));Style.SetPressedPadding(FMargin(0,1,0,0));B->SetStyle(Style);
    auto* T=WidgetTree->ConstructWidget<UTextBlock>();auto Font=InkUiTypography::MakeFont(EInkUiTextRole::Button,UsesResponsiveShell()?28:H<50?21:25);
    // The theme converts text pixel sizes to Slate points; the cross is an icon.
    if(Close)Font.Size=H>=64?40:32;
    T->SetFont(InkPresentationStyle::ButtonFont(Font,Light));
    T->SetText(FText::FromString(Caption));T->SetAutoWrapText(false);T->SetJustification(ETextJustify::Center);
    T->SetColorAndOpacity(Light?FLinearColor(.98f,.95f,.86f,1):InkPresentationStyle::Ink());T->SetVisibility(ESlateVisibility::HitTestInvisible);B->SetContent(T);
    auto* ButtonLayout=CastChecked<UButtonSlot>(T->Slot);ButtonLayout->SetHorizontalAlignment(HAlign_Fill);ButtonLayout->SetVerticalAlignment(VAlign_Center);ButtonLayout->SetPadding(FMargin(UsesResponsiveShell()?4:10,0));
    Place(Parent,B,X,Y,W,H)->SetZOrder(PageId==TEXT("CharacterProfile")?0:2);return B;
}

UImage* UInkFeaturePanel::Surface(UCanvasPanel* Parent,FName Name,InkPresentationStyle::ESurface Part,float X,float Y,float W,float H)
{if(Part==InkPresentationStyle::ESurface::Card)Part=InkPresentationStyle::ESurface::FeatureCard;auto* I=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),Name);I->SetBrush(InkPresentationStyle::Surface(Part,FVector2D(W,H)));I->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Parent,I,X,Y,W,H);return I;}

UImage* UInkFeaturePanel::Icon(UCanvasPanel* Parent,FName Name,InkPresentationStyle::EIcon Part,float X,float Y,float Size)
{auto* I=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),Name);I->SetBrush(InkPresentationStyle::Icon(Part,FVector2D(Size)));I->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Parent,I,X,Y,Size,Size)->SetZOrder(1);return I;}

void UInkFeaturePanel::Scene(UCanvasPanel* Parent,int32 Tile,float X,float Y,float W,float H)
{
    if(UsesResponsiveShell())
    {
        const FString Name=FString::Printf(TEXT("MI_InkSceneSoft_%d"),Tile);
        const FString Path=InkUiAssets::ResolveNamed(Name);
        if(auto* Material=InkUiAssets::Load<UMaterialInterface>(nullptr,*Path,nullptr,LOAD_NoWarn|LOAD_Quiet))
        {
            auto* Image=WidgetTree->ConstructWidget<UImage>();FSlateBrush B;
            const float FittedHeight=FMath::Min(H,W/3.f);
            auto* Dynamic=UMaterialInstanceDynamic::Create(Material,Image);Dynamic->SetScalarParameterValue(TEXT("FeatherFraction"),InkUiTypography::Theme()->FeatherWidth/FMath::Max(1.f,FittedHeight));
            B.SetResourceObject(Dynamic);B.ImageSize=FVector2D(768,256);B.DrawAs=ESlateBrushDrawType::Image;
            Image->SetBrush(B);Image->SetVisibility(ESlateVisibility::HitTestInvisible);
            auto* Fit=WidgetTree->ConstructWidget<UScaleBox>();Fit->SetStretch(EStretch::ScaleToFit);Fit->SetContent(Image);
            Fit->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Parent,Fit,X,Y,W,H)->SetZOrder(PageId==TEXT("CharacterProfile")?0:1);return;
        }
    }
    auto* I=WidgetTree->ConstructWidget<UImage>();FSlateBrush B;B.DrawAs=ESlateBrushDrawType::Image;B.ImageSize=FVector2D(768,256);
    B.SetResourceObject(InkUiAssets::Load<UTexture2D>(nullptr,TEXT("/Game/newUI/Ink/Textures/Shared/T_CohesionScenes.T_CohesionScenes")));
    B.SetUVRegion(FBox2f(FVector2f((Tile%2)*.5f,(Tile/2)*.25f),FVector2f((Tile%2+1)*.5f,(Tile/2+1)*.25f)));I->SetBrush(B);
    if(Tile==6&&!bPortraitLayout){B.ImageSize=FVector2D(384,256);B.SetUVRegion(FBox2f(FVector2f(0,.75f),FVector2f(.25f,1)));I->SetBrush(B);}
    auto* Fit=WidgetTree->ConstructWidget<UScaleBox>();Fit->SetStretch(Tile==6?EStretch::ScaleToFit:EStretch::ScaleToFill);Fit->SetClipping(EWidgetClipping::ClipToBoundsAlways);Fit->SetContent(I);Fit->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Parent,Fit,X,Y,W,H)->SetZOrder(PageId==TEXT("CharacterProfile")?0:1);
}

void UInkFeaturePanel::Link(UCanvasPanel* Parent,const FString& Route,const FString& Title,const FString& Caption,int32 Tile,float X,float Y,float W,float H)
{
    auto* B=Action(Parent,TEXT("Open_")+Route,TEXT(""),X,Y,W,H);
    auto Style=B->GetStyle();Style.SetNormal(InkPresentationStyle::Surface(InkPresentationStyle::ESurface::FeatureCard,FVector2D(W,H)));auto Hover=Style.Normal;Hover.TintColor=FLinearColor(1.04f,1.05f,1.02f,1);Style.SetHovered(Hover);B->SetStyle(Style);
    auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();B->SetContent(Canvas);auto* ButtonLayout=CastChecked<UButtonSlot>(Canvas->Slot);ButtonLayout->SetPadding(FMargin(0));ButtonLayout->SetHorizontalAlignment(HAlign_Fill);ButtonLayout->SetVerticalAlignment(VAlign_Fill);
    Scene(Canvas,Tile,14,18,128,H-36);
    const bool Compact=H<120;
    Label(Canvas,NAME_None,Title,152,Compact?6:22,W-190,Compact?38:48,Compact?22:28);
    Label(Canvas,NAME_None,Caption,152,Compact?46:76,W-186,Compact?32:H-80,Compact?18:20)->SetColorAndOpacity(FLinearColor(.12f,.14f,.13f,1));
    Label(Canvas,NAME_None,TEXT(">"),W-40,28,26,42,32,true);
}

void UInkFeaturePanel::FeedbackMessage(const FString& Text)
{PreviewFeedback=Text;if(StatusText){StatusText->SetText(FText::FromString(Text));StatusText->SetVisibility(Text.IsEmpty()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);}}

void UInkFeaturePanel::Open(const FString& Route,const FString& Context)
{if(FrontendOwner)FrontendOwner->OpenFeaturePanel(FName(*Route),Context);}

float UInkFeaturePanel::GetBodyContentHeight() const
{
    float Height=BodyHeight;
    if(BodyCanvas)for(auto* Child:BodyCanvas->GetAllChildren())
        if(const auto* Layout=Cast<UCanvasPanelSlot>(Child->Slot))Height=FMath::Max(Height,float(Layout->GetPosition().Y+Layout->GetSize().Y+16));
    return Height;
}

bool UInkFeaturePanel::IsRolloutPageActive() const
{
    if(!FrontendOwner)return false;
    if(!bEmbedded)return FrontendOwner->CurrentPage==PageId;
    const FName GroupRoute=PageId==TEXT("SignIn")||PageId==TEXT("MonthlySignIn")||PageId==TEXT("DailyFortune")?FName(TEXT("SignIn")):FName(TEXT("Activities"));
    const auto* Group=Cast<UInkFeatureGroupPanel>(FrontendOwner->ExtensionPages.FindRef(GroupRoute));
    return FrontendOwner->CurrentPage==GroupRoute&&Group&&Group->SelectedRoute==PageId&&Group->ContentPanels.FindRef(PageId)==this;
}

void UInkFeaturePanel::HandleRolloutSignInRequested()
{
    HandleAction(TEXT("SignInClaim"));
    if(auto* Claim=Cast<UJourneyClaimStateWidget>(GetWidgetFromName(TEXT("RolloutDailyClaim"))))
    {
        auto* Service=WulingFrontendData::Service(this);
        Claim->SetClaimState(Service&&Service->IsBusy()?EDailySignInClaimState::Pending:EDailySignInClaimState::Error,FText::FromString(PreviewFeedback));
    }
}

void UInkFeaturePanel::HandleAction(FName InAction)
{
    const FString Id=InAction.ToString();
    if(Id==TEXT("SignInClaim"))
    {
        InkJourneyAdapter::FRequest Request;Request.ExpectedDay=RolloutSignInDay;
        FString ActionFeedback;InkJourneyAdapter::Submit(WulingFrontendData::Service(this),IsRolloutPageActive(),Request,ActionFeedback);
        FeedbackMessage(ActionFeedback);return;
    }
    if(Id==TEXT("Back")||Id==TEXT("List")){if(FrontendOwner)FrontendOwner->CloseFeaturePanel(false);return;}
    if(Id==TEXT("Close")){if(FrontendOwner)FrontendOwner->CloseFeaturePanel(true);return;}
    if(Id.StartsWith(TEXT("Open_")))
    {
        FString Route=Id.Mid(5),Context;int32 Separator;
        if(Route.FindChar(TEXT('|'),Separator)){Context=Route.Mid(Separator+1);Route=Route.Left(Separator);}
        Open(Route,Context);return;
    }
    HandlePanelAction(Id);
}

void UInkFeaturePanel::HandlePanelAction(const FString& Id)
{
    auto* W=WulingFrontendData::Service(this);
    if(W&&W->IsEnabled())
    {
        if(Id==TEXT("BeginShared")||Id==TEXT("WeekGuard")||Id==TEXT("WeekIron")||Id==TEXT("EndShared"))
        {
            const FString Json=Id==TEXT("BeginShared")?TEXT("{\"activity_id\":\"ACT_SHARED\"}"):(Id==TEXT("WeekGuard")?TEXT("{\"activity_id\":\"WEEK_GUARD\"}"):TEXT("{\"activity_id\":\"WEEK_IRON\"}"));
            FeedbackMessage(W->SubmitPrototypeCommand(Id==TEXT("EndShared")?TEXT("EndActivity"):TEXT("StartActivity"),Json)?TEXT("活动请求已记录，等待确认"):W->GetStatus());return;
        }
    }
    if(FrontendOwner&&Id==TEXT("OriginalCharacter")){FrontendOwner->ShowPage(TEXT("Character"));return;}
    if(FrontendOwner&&Id==TEXT("OriginalInventory")){FrontendOwner->ShowPage(TEXT("Inventory"));return;}
    FeedbackMessage(Id.StartsWith(TEXT("Day_"))?TEXT("该日签到奖励尚未开放"):TEXT("此功能即将开放，敬请期待"));
}

void UInkFeaturePanel::RefreshLiveData()
{
    if(FrontendOwner&&FrontendOwner->CurrentPage==PageId&&PageId==TEXT("DailyActivityDetail")&&SelectedEntry==TEXT("ACT_SIGN"))
    {
        const auto* Service=WulingFrontendData::Service(this);
        if(!Service)return;
        const auto Snapshot=Service->Snapshot();
        double Version=0;int32 SignedState=-1;
        if(Snapshot)Snapshot->TryGetNumberField(TEXT("version"),Version);
        const auto Activities=WulingFrontendData::Activity(this);
        bool Signed=false;
        if(Activities&&Activities->TryGetBoolField(TEXT("signed"),Signed))SignedState=Signed?1:0;
        const FString Day=WulingFrontendData::String(Activities,TEXT("day"));
        const FString ActivityState=FString::Printf(TEXT("%d:%s"),SignedState,*Day);
        const FString Key=FString::Printf(TEXT("activity|%.0f|%d|%d|%d|%s|%s"),Version,Service->IsEnabled(),Service->IsConnected(),Service->IsBusy(),*Service->GetStatus(),*ActivityState);
        if(Key!=AuthorityContentKey){AuthorityContentKey=Key;RebuildPanel();}
        return;
    }
    if(!FrontendOwner||FrontendOwner->CurrentPage!=PageId||!PageId.ToString().StartsWith(TEXT("Character")))return;
    auto* W=WulingFrontendData::Service(this);if(!W||!W->IsEnabled()||!W->Snapshot())return;
    double Version=0;W->Snapshot()->TryGetNumberField(TEXT("version"),Version);const FString Key=FString::Printf(TEXT("%.0f|%d"),Version,W->IsConnected());
    if(Key!=AuthorityContentKey){AuthorityContentKey=Key;RebuildPanel();}
}

void UInkFeaturePanel::ExpandCompactActionRows()
{
    // Preserve each authored row's order and columns while making its touch targets usable.
    struct FRow{float Top,Bottom,Extra;};TArray<FRow> Rows;
    for(auto* C:BodyCanvas->GetAllChildren())if(Cast<UInkFrontendActionButton>(C))if(auto* S=Cast<UCanvasPanelSlot>(C->Slot))
    {
        const float Top=S->GetPosition().Y,H=S->GetSize().Y;if(H>=88)continue;
        auto* Row=Rows.FindByPredicate([Top](const FRow& R){return FMath::IsNearlyEqual(R.Top,Top,.5f);});
        if(Row){Row->Bottom=FMath::Max(Row->Bottom,Top+H);Row->Extra=FMath::Max(Row->Extra,88-H);}else Rows.Add({Top,Top+H,88-H});
    }
    Rows.Sort([](const FRow& A,const FRow& B){return A.Top<B.Top;});
    for(auto* C:BodyCanvas->GetAllChildren())if(auto* S=Cast<UCanvasPanelSlot>(C->Slot))
    {
        auto P=S->GetPosition();float Shift=0;for(const auto& R:Rows)if(P.Y>=R.Bottom-.1f)Shift+=R.Extra;
        P.Y+=Shift;S->SetPosition(P);
        if(Cast<UInkFrontendActionButton>(C)){auto Size=S->GetSize();Size.Y=FMath::Max(88.,Size.Y);S->SetSize(Size);}
    }
}
