#include "Frontend/UI/InkUiAssetPaths.h"
#include "InkFeaturePanel.h"
#include "InkFrontendWidget.h"
#include "WulingFrontendData.h"
#include "UI/InkUiResponsiveLayout.h"
#include "UI/InkUiTheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Materials/MaterialInterface.h"

bool UInkFeaturePanel::UsesResponsiveShell()const
{
    return true;
}

void UInkFeaturePanel::RebuildResponsivePanel()
{
    // Read before clearing the old Slate content, which can reset its scroll offset.
    const float OldScroll=ResponsiveScroll?ResponsiveScroll->GetScrollOffset():0;
    TMap<FName,float> InnerOffsets;
    TFunction<void(UWidget*,bool)> ScrollState=[&](UWidget* Widget,bool Restore)
    {
        if(auto* Scroll=Cast<UScrollBox>(Widget))
        {
            if(Restore){if(const float* Offset=InnerOffsets.Find(Scroll->GetFName()))Scroll->SetScrollOffset(*Offset);}
            else InnerOffsets.Add(Scroll->GetFName(),Scroll->GetScrollOffset());
        }
        if(auto* Panel=Cast<UPanelWidget>(Widget))for(auto* Child:Panel->GetAllChildren())ScrollState(Child,Restore);
    };
    ScrollState(ShellCanvas,false);
    if(!ResponsiveRoot)
    {
        auto* Root=WidgetTree->ConstructWidget<UOverlay>();WidgetTree->RootWidget=Root;
        auto* Dim=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("FeatureBackdrop"));Dim->SetBrushColor(FLinearColor(.005f,.012f,.008f,FMath::Clamp(InkUiTypography::Theme()->BackdropOpacity,0.f,.8f)));
        auto* D=Root->AddChildToOverlay(Dim);D->SetHorizontalAlignment(HAlign_Fill);D->SetVerticalAlignment(VAlign_Fill);
        // Frontend SafeCanvas already owns the single device SafeZone.
        ResponsiveRoot=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("FeatureLayoutRoot"));
        auto* R=Root->AddChildToOverlay(ResponsiveRoot);R->SetHorizontalAlignment(HAlign_Fill);R->SetVerticalAlignment(VAlign_Fill);
    }
    if(ShellCanvas)
    {
        TArray<UWidget*> Old;
        TFunction<void(UWidget*)> Collect=[&](UWidget* W){if(auto* P=Cast<UPanelWidget>(W))for(auto* C:P->GetAllChildren()){Collect(C);Old.Add(C);}};
        Collect(ShellCanvas);ShellCanvas->ClearChildren();for(auto* W:Old)W->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);
    }
    else {ShellCanvas=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("FeatureShell"));ResponsiveRoot->AddChildToCanvas(ShellCanvas);}
    const auto* Group=Cast<UInkFeatureGroupPanel>(this);
    const bool Calendar=Group&&Group->SelectedRoute==TEXT("MonthlySignIn");
    const bool SignFamily=PageId==TEXT("SignIn")||PageId==TEXT("MonthlySignIn")||PageId==TEXT("DailyFortune");
    const bool Dense=Calendar||PageId.ToString().StartsWith(TEXT("Wuling"))||PageId.ToString().StartsWith(TEXT("Character"))||PageId==TEXT("Draw")||PageId==TEXT("WorldCultivation")||PageId==TEXT("Expedition")||PageId.ToString().StartsWith(TEXT("Resource"));
    ResponsiveScroll=nullptr;
    const auto Frame=InkUiLayout::ResolveFeatureFrame(ResponsiveSafeSize,Dense);
    auto* FS=CastChecked<UCanvasPanelSlot>(ShellCanvas->Slot);FS->SetPosition(Frame.GetTopLeft());FS->SetSize(Frame.GetSize());
    const float W=Frame.GetSize().X,H=Frame.GetSize().Y;
    const bool Preserved=PageId==TEXT("CharacterProfile");
    const auto* Theme=InkUiTypography::Theme();
    const float Header=Preserved?104.f:FMath::Clamp(Theme->HeaderHeight,88.f,128.f)+16.f;
    const float Footer=Preserved?144.f:FMath::Clamp(Theme->FooterHeight,104.f,160.f)+40.f;
    BodyWidth=W-48-(Preserved?0.f:16.f);const float ViewHeight=FMath::Max(160.f,H-Header-Footer);
    BodyHeight=ViewHeight;
    if(!SignFamily)bPortraitLayout=BodyWidth<800;
    auto Place=[&](UWidget* Widget,float X,float Y,float Width,float Height){auto* S=ShellCanvas->AddChildToCanvas(Widget);S->SetPosition({X,Y});S->SetSize({Width,Height});return S;};
    auto* Paper=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("FeaturePaper"));
    FSlateBrush B;B.DrawAs=ESlateBrushDrawType::RoundedBox;B.TintColor=InkUiTypography::Theme()->PaperTint;B.OutlineSettings.CornerRadii=FVector4(10,10,10,10);B.OutlineSettings.Color=InkUiTypography::Theme()->GoldTint;B.OutlineSettings.Width=1;
    B.OutlineSettings.RoundingType=ESlateBrushRoundingType::FixedRadius;
    if(auto* M=InkUiAssets::Load<UMaterialInterface>(nullptr,TEXT("/Game/UI/Ink/Materials/Shared/M_InkPaperCloud.M_InkPaperCloud"),nullptr,LOAD_NoWarn|LOAD_Quiet))
    {B.DrawAs=ESlateBrushDrawType::Image;B.SetResourceObject(M);B.TintColor=FLinearColor::White;}
    Paper->SetBrush(B);Paper->SetRenderOpacity(FMath::Clamp(Theme->PaperOpacity,.7f,1.f));Paper->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Paper,0,0,W,H);
    // A single shell paper owns the background. The old full-width scene atlas
    // carried a pale sky backing and washed the paper again. Keep decoration
    // local to a fine title rule, never behind the full page content.
    auto* Rule=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("FeatureHeadingRule"));
    auto RuleTint=Theme->GoldTint;RuleTint.A=FMath::Clamp(Theme->DecorationOpacity,0.f,.3f)+.25f;
    Rule->SetBrushColor(RuleTint);Rule->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Rule,24,94,W-48,1);
    auto* Title=Label(ShellCanvas,TEXT("FeatureHeading"),SignFamily?TEXT("签到有礼"):PanelTitle().ToString(),24,18,W-136,64,44);
    Title->SetAutoWrapText(false);CastChecked<UCanvasPanelSlot>(Title->Slot)->SetZOrder(10);
    auto* Close=Action(ShellCanvas,TEXT("Close"),TEXT("×"),W-104,0,88,88);CastChecked<UCanvasPanelSlot>(Close->Slot)->SetZOrder(10);
    BodyCanvas=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("FeatureBody"));
    {
        ResponsiveScroll=WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("FeatureContentScroll"));
        ResponsiveScroll->SetAllowOverscroll(false);ResponsiveScroll->SetScrollbarThickness(FVector2D(6));
        ResponsiveContentSize=WidgetTree->ConstructWidget<USizeBox>();ResponsiveScroll->AddChild(ResponsiveContentSize);ResponsiveContentSize->SetContent(BodyCanvas);
        Place(ResponsiveScroll,24,Header,W-48,ViewHeight)->SetZOrder(3);
    }
    BodyCanvas->SetClipping(EWidgetClipping::ClipToBoundsAlways);BuildPanelContent();
    ScrollState(BodyCanvas,true);
    if(PageId==TEXT("Draw"))
    {
        const float ActionWidth=(W-240.f)/3.f;
        for(auto* Child:BodyCanvas->GetAllChildren())if(auto* Button=Cast<UInkFrontendActionButton>(Child))
        {
            const int32 Index=Button->Action==TEXT("Ext_Demo_DrawOnce")?0:Button->Action==TEXT("Ext_Demo_DrawTen")?1:Button->Action==TEXT("Ext_Demo_DrawSkip")?2:INDEX_NONE;
            if(Index!=INDEX_NONE)
            {Button->RemoveFromParent();Place(Button,24+Index*(ActionWidth+12),H-96,ActionWidth,88)->SetZOrder(10);}
        }
    }
    // Primary actions remain visible while long descriptions scroll. Protected
    // pages and the Wuling pages with their own fixed footer are not reparented.
    if(!Preserved&&!PageId.ToString().StartsWith(TEXT("Wuling")))
        for(auto* Child:BodyCanvas->GetAllChildren())
            if(auto* Button=Cast<UInkFrontendActionButton>(Child))
            {
                const FString Id=Button->Action.ToString();
                if(Id.StartsWith(TEXT("Ext_EnterRegion_"))||Id.StartsWith(TEXT("Ext_Open_RegionDetail|"))||Id==TEXT("Ext_Preview_Participate")||(PageId==TEXT("Extensions")&&Id==TEXT("Ext_Open_WorldCultivation")))
                {Button->RemoveFromParent();Place(Button,24,H-96,W-216,88)->SetZOrder(10);break;}
            }
    {
        float Extent=ViewHeight;
        for(auto* C:BodyCanvas->GetAllChildren())if(auto* S=Cast<UCanvasPanelSlot>(C->Slot))Extent=FMath::Max(Extent,float(S->GetPosition().Y+S->GetSize().Y+16));
        ResponsiveContentSize->SetHeightOverride(Extent);ResponsiveContentSize->SetWidthOverride(BodyWidth);ResponsiveScroll->SetScrollOffset(OldScroll);
    }
    const auto Authority=WulingFrontendData::Activity(this);auto* Service=WulingFrontendData::Service(this);
    const bool Verse=Group&&Group->SelectedRoute==TEXT("DailyFortune");
    if(SignFamily)
    {
    auto* Claim=Action(ShellCanvas,TEXT("SignInClaim"),Authority?(Verse?TEXT("每日剑诀 · 无数值奖励"):(WulingFrontendData::Signed(Authority)?TEXT("今日已签到"):TEXT("签到 · 记录占位"))):TEXT("领取待同步"),24,H-96,W-216,88,true);
    CastChecked<UCanvasPanelSlot>(Claim->Slot)->SetZOrder(10);
    Claim->SetIsEnabled(Authority&&!Verse&&!WulingFrontendData::Signed(Authority)&&Service&&!Service->IsBusy());
    }
    auto* Back=Action(ShellCanvas,TEXT("Back"),TEXT("返回"),W-168,H-96,144,88);CastChecked<UCanvasPanelSlot>(Back->Slot)->SetZOrder(10);
    StatusText=Label(ShellCanvas,TEXT("FeatureFeedback"),PreviewFeedback,24,H-132,W-48,32,24);
    StatusText->SetVisibility(PreviewFeedback.IsEmpty()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
    bSurfaceInitialized=true;
    if(FrontendOwner)FrontendOwner->RefreshExtensionActions();
}
