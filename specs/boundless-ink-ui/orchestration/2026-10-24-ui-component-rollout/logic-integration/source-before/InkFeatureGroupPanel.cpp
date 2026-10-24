#include "Frontend/UI/InkUiAssetPaths.h"
#include "InkFeaturePanel.h"
#include "InkFrontendWidget.h"
#include "WulingFrontendData.h"
#include "Serialization/JsonSerializer.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WidgetSwitcherSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UInkFeaturePanel::ConfigureEmbedded(UInkFrontendWidget* Owner,FName Route,bool Portrait,FVector2D Size)
{
    FrontendOwner=Owner;PageId=Route;bEmbedded=true;bPortraitLayout=Portrait;BodyWidth=Size.X;BodyHeight=Size.Y;RebuildPanel();
}
UImage* UInkFeaturePanel::AtlasIcon(UCanvasPanel* Parent,FName Name,int32 Tile,float X,float Y,float Size)
{
    Tile=FMath::Clamp(Tile,0,15);auto* Image=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),Name);
    FSlateBrush Brush;Brush.DrawAs=ESlateBrushDrawType::Image;Brush.ImageSize=FVector2D(Size);
    Brush.SetResourceObject(InkUiAssets::Load<UTexture2D>(nullptr,TEXT("/Game/UI/Ink/Textures/Shared/T_CohesionIcons.T_CohesionIcons")));
    Brush.SetUVRegion(FBox2f(FVector2f((Tile%4)*.25f,(Tile/4)*.25f),FVector2f((Tile%4+1)*.25f,(Tile/4+1)*.25f)));
    Image->SetBrush(Brush);Image->SetVisibility(ESlateVisibility::HitTestInvisible);auto* LayoutSlot=Parent->AddChildToCanvas(Image);LayoutSlot->SetOffsets(FMargin(X,Y,Size,Size));LayoutSlot->SetZOrder(PageId==TEXT("CharacterProfile")?1:3);return Image;
}
void UInkFeaturePanel::Reward(UCanvasPanel* Parent,FName Name,int32 Item,const FString& Caption,float X,float Y,float Size,bool Selected)
{
    Surface(Parent,Name,Selected?InkPresentationStyle::ESurface::FeatureHeading:InkPresentationStyle::ESurface::FeatureCard,X,Y,Size,Size);
    AtlasIcon(Parent,NAME_None,8+Item%4,X+Size*.13f,Y+Size*.03f,Size*.74f);
    auto* T=Label(Parent,NAME_None,Caption,X+3,Y+Size*.74f,Size-6,Size*.24f,FMath::Clamp(int32(Size*.19f),13,21),true);T->SetAutoWrapText(false);
}
void UInkFeatureGroupPanel::SelectRoute(FName Route)
{
    if(Route==TEXT("Activities"))Route=TEXT("DailyActivities");
    if(!IsActivities()&&FrontendOwner)FrontendOwner->SignInSelectedRoute=Route;
    if(SelectedRoute==Route)return;SelectedRoute=Route;RebuildPanel();
}
void UInkFeatureGroupPanel::RefreshLiveData()
{
    if(!FrontendOwner||FrontendOwner->CurrentPage!=PageId)return;
    auto* W=WulingFrontendData::Service(this);if(!W||!W->IsEnabled())return;
    const auto A=WulingFrontendData::Activity(this);FString Key;
    if(A)FJsonSerializer::Serialize(A.ToSharedRef(),TJsonWriterFactory<>::Create(&Key));
    Key+=(W->IsBusy()?TEXT("busy"):TEXT("idle"))+W->GetStatus();
    if(Key!=AuthorityStateKey){AuthorityStateKey=Key;if(!W->IsBusy()&&!PreviewFeedback.IsEmpty())PreviewFeedback=W->GetStatus();RebuildPanel();}
}
void UInkFeatureGroupPanel::BuildPanelContent()
{
    const bool Activities=IsActivities();
    const TArray<FName> Routes=Activities?TArray<FName>{TEXT("DailyActivities"),TEXT("TimedActivities"),TEXT("PermanentActivities")}:TArray<FName>{TEXT("SignIn"),TEXT("DailyFortune"),TEXT("MonthlySignIn")};
    const TArray<FString> Labels=Activities?TArray<FString>{TEXT("日常活动"),TEXT("限时活动"),TEXT("常驻活动")}:TArray<FString>{TEXT("每日签到"),TEXT("每日剑诀"),TEXT("每月签到")};
    if(!Routes.Contains(SelectedRoute))SelectedRoute=Routes[0];
    const auto* Service=WulingFrontendData::Service(this);
    const bool ShowLaunch=Activities;
    const int32 TabCount=Routes.Num()+(ShowLaunch?1:0);
    const float TabH=88,ContentY=TabH+24,TabW=(BodyWidth-(TabCount-1)*8)/TabCount;
    for(int32 I=0;I<Routes.Num();++I)Action(BodyCanvas,TEXT("GroupTab_")+Routes[I].ToString(),Labels[I],I*(TabW+8),0,TabW,TabH,false,false,SelectedRoute==Routes[I]);
    // The launch page already has its own live state and action dispatcher. Keep
    // this entry explicit so it cannot be confused with Wuling organization activity.
    if(ShowLaunch)Action(BodyCanvas,TEXT("Open_LaunchEvent"),TEXT("开服活动"),Routes.Num()*(TabW+8),0,TabW,TabH);
    auto* Switcher=WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(),TEXT("GroupContent"));
    auto* SwitcherSlot=BodyCanvas->AddChildToCanvas(Switcher);SwitcherSlot->SetOffsets(FMargin(0,ContentY,BodyWidth,FMath::Max(88.f,BodyHeight-ContentY)));
    for(FName Route:Routes)
    {
        auto* Child=ContentPanels.FindRef(Route).Get();
        if(!Child)
        {
            TSubclassOf<UInkFeaturePanel> Class=ClassForRoute(Route);if(Route==TEXT("SignIn"))Class=UInkSignInPanel::StaticClass();
            Child=CreateWidget<UInkFeaturePanel>(GetOwningPlayer(),Class);ContentPanels.Add(Route,Child);
        }
        if(Child->GetParent())Child->RemoveFromParent();Child->ConfigureEmbedded(FrontendOwner,Route,bPortraitLayout,FVector2D(BodyWidth-16.f,FMath::Max(88.f,BodyHeight-ContentY)));
        auto* ContentSlot=CastChecked<UWidgetSwitcherSlot>(Switcher->AddChild(Child));ContentSlot->SetHorizontalAlignment(HAlign_Fill);ContentSlot->SetVerticalAlignment(VAlign_Fill);
        if(Route==SelectedRoute)Switcher->SetActiveWidget(Child);
    }
}
void UInkFeatureGroupPanel::HandlePanelAction(const FString& Id)
{
    if(Id.StartsWith(TEXT("GroupTab_"))){SelectRoute(FName(*Id.Mid(9)));return;}
    if(auto* Child=ContentPanels.FindRef(SelectedRoute).Get()){Child->HandleAction(FName(*Id));FeedbackMessage(Child->PreviewFeedback);if(FrontendOwner)FrontendOwner->RefreshExtensionActions();}
}
