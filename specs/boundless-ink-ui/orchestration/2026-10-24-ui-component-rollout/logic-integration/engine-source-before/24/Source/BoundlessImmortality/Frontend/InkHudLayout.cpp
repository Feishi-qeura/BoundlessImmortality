#include "InkFrontendWidget.h"
#include "InkAnchorStatusWidget.h"
#include "InkPresentationStyle.h"
#include "InkHomeArt.h"
#include "Frontend/UiRollout20261024/LogicJourney/InkIntegratedActivityPanels.h"
#include "Components/ScaleBox.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UI/InkUiResponsiveLayout.h"
#include "UI/InkUiTheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/SizeBox.h"
#include "Components/ButtonSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Materials/MaterialInterface.h"

void UInkFrontendWidget::EnsureExtensionPages()
{
    if(IsDesignTime()||!WidgetTree||bRuntimeHudBuilt)return;
    auto* HUD=Cast<UCanvasPanel>(GetWidgetFromName(TEXT("HUDPage")));if(!HUD)return;
    bRuntimeHudBuilt=true;
    auto* LaunchCountdown=CreateWidget<UInkIntegratedLaunchCountdown>(GetOwningPlayer());
    LaunchCountdown->Rename(TEXT("RolloutHUDLaunchCountdown"),WidgetTree);
    HUD->AddChildToCanvas(LaunchCountdown)->SetZOrder(4);LaunchCountdown->Configure(this);
    if(!AnchorStatusWidget)
    {
        AnchorStatusWidget=CreateWidget<UInkAnchorStatusWidget>(GetOwningPlayer());
        AnchorStatusWidget->Configure(this);
        HUD->AddChildToCanvas(AnchorStatusWidget)->SetAutoSize(true);
    }
    auto* Identity=MakeButton(WidgetTree,TEXT("HUD_AnchorIdentity"),TEXT("ShowAnchorIdentity"),FText::GetEmpty());
    FSlateBrush IdentityClear;IdentityClear.DrawAs=ESlateBrushDrawType::NoDrawType;
    FButtonStyle IdentityStyle;IdentityStyle.SetNormal(IdentityClear).SetHovered(IdentityClear).SetPressed(IdentityClear).SetDisabled(IdentityClear);
    Identity->SetStyle(IdentityStyle);Identity->SetToolTipText(FText::GetEmpty());
    HUD->AddChildToCanvas(Identity)->SetZOrder(2);
    auto IconButton=[&](UInkFrontendActionButton* B,const TCHAR* Caption,int32 Tile,float Width,float IconSize,bool Center,FName LabelName=NAME_None)
    {
        FSlateBrush Clear;Clear.DrawAs=ESlateBrushDrawType::RoundedBox;Clear.TintColor=Tile<0?FLinearColor(.025f,.04f,.04f,.88f):Center?FLinearColor(.06f,.13f,.11f,.94f):FLinearColor::Transparent;
        Clear.OutlineSettings.CornerRadii=FVector4(12,12,12,12);Clear.OutlineSettings.RoundingType=ESlateBrushRoundingType::FixedRadius;
        Clear.OutlineSettings.Width=Center?1.5f:0.f;Clear.OutlineSettings.Color=FLinearColor(.65f,.49f,.23f,1);
        auto Hover=Clear;Hover.TintColor=FLinearColor(.03f,.16f,.22f,.4f);
        auto Pressed=Clear;Pressed.TintColor=FLinearColor(.36f,.21f,.06f,.5f);
        FButtonStyle Style;Style.SetNormal(Clear).SetHovered(Hover).SetPressed(Pressed).SetDisabled(Clear);
        Style.SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0,2,0,-2));B->SetStyle(Style);
        auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(Width);Size->SetHeightOverride(Tile<0?88:104);B->SetContent(Size);
        auto* BS=CastChecked<UButtonSlot>(Size->Slot);BS->SetPadding(FMargin(0));BS->SetHorizontalAlignment(HAlign_Center);BS->SetVerticalAlignment(VAlign_Bottom);
        auto* C=WidgetTree->ConstructWidget<UCanvasPanel>();Size->SetContent(C);C->SetVisibility(ESlateVisibility::HitTestInvisible);
        IconSize=FMath::Min(IconSize,Center?64.f:Tile<0?40.f:48.f);
        const bool Top=Tile<0;const float IconY=Top?6:FMath::Max(0.f,64-IconSize);
        const FName IconName(Center?TEXT("HUDCultivationIcon"):Tile==16?TEXT("HUDRoleIcon"):Tile==1?TEXT("HUDBagIcon"):Tile==2?TEXT("HUDRankIcon"):Tile==3?TEXT("HUDMenuIcon"):TEXT(""));
        auto* Icon=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),IconName);
        const FName ArtId=Top?(Tile==-3?TEXT("home-icon-activity"):Tile==-2?TEXT("home-icon-sign-in"):TEXT("home-icon-journey")):
            Tile==16?TEXT("home-avatar-portrait"):Tile==1?TEXT("home-icon-bag"):Tile==2?TEXT("home-icon-rank"):Tile==3?TEXT("home-icon-menu"):TEXT("home-icon-cultivate");
        Icon->SetBrush(InkHomeArt::Brush(ArtId));
        auto* IS=C->AddChildToCanvas(Icon);const auto IconRect=InkHomeArt::ArtBounds(ArtId,FSlateRect((Width-IconSize)*.5f,IconY,(Width+IconSize)*.5f,IconY+IconSize));
        IS->SetPosition(IconRect.GetTopLeft());IS->SetSize(IconRect.GetSize());IS->SetZOrder(1);
        if(Tile==16)
        {
            auto* Frame=WidgetTree->ConstructWidget<UImage>();Frame->SetBrush(InkHomeArt::Brush(TEXT("home-avatar-frame")));
            const auto R=InkHomeArt::ArtBounds(TEXT("home-avatar-frame"),FSlateRect((Width-IconSize)*.5f-4,IconY-4,(Width+IconSize)*.5f+4,IconY+IconSize+4));
            auto* FS=C->AddChildToCanvas(Frame);FS->SetPosition(R.GetTopLeft());FS->SetSize(R.GetSize());FS->SetZOrder(2);
        }
        auto* Text=WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),LabelName);
        auto LabelFont=InkUiTypography::MakeFont(EInkUiTextRole::HudLabel,24);LabelFont.OutlineSettings.OutlineSize=1;LabelFont.OutlineSettings.OutlineColor=InkPresentationStyle::Ink();Text->SetFont(LabelFont);Text->SetText(FText::FromString(Caption));Text->SetAutoWrapText(false);
        Text->SetColorAndOpacity(FLinearColor::FromSRGBColor(FColor(246,223,167)));Text->SetJustification(ETextJustify::Center);
        auto* TS=C->AddChildToCanvas(Text);TS->SetPosition(FVector2D(0,Top?50:66));TS->SetSize(FVector2D(Width,32));TS->SetZOrder(2);
        B->SetToolTipText(FText::GetEmpty());B->BindFrontend(this);
    };
    auto* Grid=Cast<UUniformGridPanel>(GetWidgetFromName(TEXT("HUDNavigation")));if(!Grid)return;
    const FName MenuActions[]={TEXT("OpenSaves"),TEXT("OpenSettings"),TEXT("ReturnToMainMenu")};
    const TCHAR* MenuLabels[]={TEXT("存档"),TEXT("设置"),TEXT("返回主菜单")};
    TArray<UInkFrontendActionButton*> MenuButtons;
    for(int I=0;I<3;++I)
    {
        const FName Name(*(TEXT("HUD_")+MenuActions[I].ToString()));auto* B=Cast<UInkFrontendActionButton>(GetWidgetFromName(Name));
        if(!B)B=MakeButton(WidgetTree,Name,MenuActions[I],FText::FromString(MenuLabels[I]));
        B->RemoveFromParent();MenuButtons.Add(B);
    }
    const FName Actions[]={TEXT("OpenCharacter"),TEXT("OpenInventory"),TEXT("ToggleCultivation"),TEXT("OpenLeaderboard"),TEXT("ToggleHUDMenu")};
    const TCHAR* Labels[]={TEXT("角色"),TEXT("背包"),TEXT("修炼"),TEXT("排行"),TEXT("菜单")};
    const int32 Tiles[]={16,1,4,2,3};
    TArray<UInkFrontendActionButton*> Buttons;
    for(int I=0;I<5;++I)
    {
        auto* B=Cast<UInkFrontendActionButton>(GetWidgetFromName(FName(*(TEXT("HUD_")+Actions[I].ToString()))));
        if(!B)B=MakeButton(WidgetTree,FName(*(TEXT("HUD_")+Actions[I].ToString())),Actions[I],FText::FromString(Labels[I]));
        B->RemoveFromParent();Buttons.Add(B);
    }
    Grid->ClearChildren();Grid->SetSlotPadding(FMargin(0));
    for(int I=0;I<5;++I)
    {
        IconButton(Buttons[I],Labels[I],Tiles[I],I==2?128:104,I==2?CultivateVisualSize:I==0?80:104,I==2,I==2?FName(TEXT("HUDCultivationLabel")):NAME_None);
        auto* S=Grid->AddChildToUniformGrid(Buttons[I],0,I);S->SetHorizontalAlignment(HAlign_Fill);S->SetVerticalAlignment(VAlign_Fill);
    }
    auto* Veil=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("HUDNavigationVeil"));
    // Reuse the feathered ink silhouette locally. No light full-width fog sits over the world.
    FSlateBrush VB;VB.DrawAs=ESlateBrushDrawType::RoundedBox;VB.TintColor=FLinearColor(.022f,.032f,.03f,.94f);VB.OutlineSettings.CornerRadii=FVector4(12,12,12,12);VB.OutlineSettings.RoundingType=ESlateBrushRoundingType::FixedRadius;VB.OutlineSettings.Width=1;VB.OutlineSettings.Color=FLinearColor(.38f,.31f,.18f,.8f);
    Veil->SetBrush(VB);Veil->SetVisibility(ESlateVisibility::HitTestInvisible);HUD->AddChildToCanvas(Veil)->SetZOrder(-1);
    auto* Cloud=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("HUDBottomCloud"));
    Cloud->SetBrush(InkHomeArt::Brush(TEXT("home-bottom-cloud")));Cloud->SetColorAndOpacity(FLinearColor(.025f,.035f,.032f,.65f));Cloud->SetVisibility(ESlateVisibility::Collapsed);HUD->AddChildToCanvas(Cloud)->SetZOrder(0);
    if(FParse::Param(FCommandLine::Get(),TEXT("GWLocalTest")))
    {
        auto* Preview=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("HUDPreviewPaper"));
        Preview->SetBrush(InkHomeArt::Brush(TEXT("home-preview-paper")));Preview->SetColorAndOpacity(FLinearColor(.08f,.09f,.07f,1));Preview->SetVisibility(ESlateVisibility::HitTestInvisible);HUD->AddChildToCanvas(Preview)->SetZOrder(1);
        auto* Caption=WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),TEXT("HUDPreviewCaption"));
        Caption->SetText(FText::FromString(TEXT("视觉预览 · 本地模拟")));Caption->SetFont(InkUiTypography::MakeFont(EInkUiTextRole::Secondary,16));
        Caption->SetJustification(ETextJustify::Center);Caption->SetColorAndOpacity(InkPresentationStyle::Paper());Caption->SetVisibility(ESlateVisibility::HitTestInvisible);
        HUD->AddChildToCanvas(Caption)->SetZOrder(2);
    }
    const FName Shortcuts[]={TEXT("OpenActivities"),TEXT("OpenSignIn"),TEXT("OpenExtensions"),TEXT("OpenLaunchEvent"),TEXT("OpenWuling")};
    const TCHAR* ShortcutLabels[]={TEXT("活动"),TEXT("签到"),TEXT("仙途"),TEXT("开服"),TEXT("武灵")};
    for(int I=0;I<5;++I)
    {
        const FName Action=Shortcuts[I];
        const FName Name(*(TEXT("HUD_")+Action.ToString()));auto* B=Cast<UInkFrontendActionButton>(GetWidgetFromName(Name));
        if(!B)B=MakeButton(WidgetTree,Name,Action,FText::GetEmpty());
        if(!B->GetParent())HUD->AddChildToCanvas(B);
        IconButton(B,ShortcutLabels[I],I<3?I-3:I==3?-3:-1,88,56,false);
        if(I>=3)B->SetToolTipText(FText::FromString(I==3?TEXT("开服活动"):TEXT("武灵山门")));
    }
    auto* Menu=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("HUDRuntimeMenu"));
    FSlateBrush Paper=InkPresentationStyle::IdentityBackground();
    Menu->SetBrush(Paper);Menu->SetPadding(FMargin(8));Menu->SetVisibility(ESlateVisibility::Collapsed);HUD->AddChildToCanvas(Menu)->SetZOrder(20);
    auto* Body=WidgetTree->ConstructWidget<UVerticalBox>();Menu->SetContent(Body);
    for(int I=0;I<3;++I)
    {
        auto* B=MenuButtons[I];
        auto* S=WidgetTree->ConstructWidget<USizeBox>();S->SetHeightOverride(88);B->SetContent(S);
        auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetFont(InkUiTypography::MakeFont(EInkUiTextRole::Button));T->SetText(FText::FromString(MenuLabels[I]));T->SetColorAndOpacity(InkUiTypography::Theme()->InkTint);T->SetJustification(ETextJustify::Center);S->SetContent(T);
        auto Style=InkPresentationStyle::Button();Style.SetNormalPadding(FMargin(0));Style.SetPressedPadding(FMargin(0,2,0,0));B->SetStyle(Style);
        CastChecked<UButtonSlot>(S->Slot)->SetVerticalAlignment(VAlign_Center);Body->AddChildToVerticalBox(B);B->BindFrontend(this);
    }
    LastHudSafeSize=FVector2D::ZeroVector;UpdateAdaptiveHudLayout();
}

void UInkFrontendWidget::UpdateAdaptiveHudLayout()
{
    if(!bRuntimeHudBuilt)return;
    auto* Safe=GetWidgetFromName(TEXT("SafeCanvas"));if(!Safe)return;
    const FVector2D Size=Safe->GetCachedGeometry().GetLocalSize();if(Size.X<=0||Size.Y<=0)return;
    const FVector2D Tokens(InkUiTypography::Theme()->OuterPadding,InkUiTypography::Theme()->MinHitSize);
    if(Size.Equals(LastHudSafeSize,.5)&&LastHudBottomInset==ExtraStreamBottomInset&&Tokens==LastHudLayoutTokens)return;
    LastHudSafeSize=Size;LastHudBottomInset=ExtraStreamBottomInset;LastHudLayoutTokens=Tokens;
    const auto R=InkUiLayout::ResolveHud(Size,ExtraStreamBottomInset);
    auto Place=[](UWidget* W,FSlateRect Rect)
    {
        if(W)if(auto* S=Cast<UCanvasPanelSlot>(W->Slot)){S->SetAutoSize(false);S->SetAnchors(FAnchors(0));S->SetAlignment(FVector2D(0));S->SetPosition(Rect.GetTopLeft());S->SetSize(Rect.GetSize());}
    };
    Place(AnchorStatusWidget,R.Identity);Place(GetWidgetFromName(TEXT("HUDNavigation")),R.Navigation);Place(GetWidgetFromName(TEXT("HUDRuntimeMenu")),R.RuntimeMenu);
    if(auto* HudNavigation=Cast<UUniformGridPanel>(GetWidgetFromName(TEXT("HUDNavigation"))))
    {
        const float CellWidth=R.Navigation.GetSize().X/5.f;
        for(int32 I=0;I<HudNavigation->GetChildrenCount();++I)if(auto* Button=Cast<UInkFrontendActionButton>(HudNavigation->GetChildAt(I)))
            if(auto* Sizing=Cast<USizeBox>(Button->GetContent()))
            {
                Sizing->SetWidthOverride(CellWidth);Sizing->SetHeightOverride(R.Navigation.GetSize().Y);
                if(auto* ArtCanvas=Cast<UCanvasPanel>(Sizing->GetContent()))for(auto* Child:ArtCanvas->GetAllChildren())
                    if(auto* HudChildSlot=Cast<UCanvasPanelSlot>(Child->Slot))
                    {
                        if(auto* Icon=Cast<UImage>(Child))
                        {
                            const float Extent=FMath::Min(CellWidth-4,I==2?64.f:I==0?(Icon->GetFName()==TEXT("HUDRoleIcon")?44.f:52.f):48.f);
                            const auto Rect=InkHomeArt::Fit(Icon->GetBrush().ImageSize,FSlateRect((CellWidth-Extent)*.5f,64-Extent,(CellWidth+Extent)*.5f,64));
                            HudChildSlot->SetPosition(Rect.GetTopLeft());HudChildSlot->SetSize(Rect.GetSize());
                        }
                        else if(Cast<UTextBlock>(Child))HudChildSlot->SetSize(FVector2D(CellWidth,32));
                    }
            }
    }
    Place(GetWidgetFromName(TEXT("HUD_AnchorIdentity")),R.Identity);
    Place(GetWidgetFromName(TEXT("HUDBottomCloud")),InkHomeArt::ArtBounds(TEXT("home-bottom-cloud"),FSlateRect(R.Navigation.Left-12,R.Navigation.Bottom-58,R.Navigation.Right+12,R.Navigation.Bottom-8)));
    const float PreviewX=(R.Navigation.Left+R.Navigation.Right)*.5f;
    Place(GetWidgetFromName(TEXT("HUDPreviewPaper")),InkHomeArt::ArtBounds(TEXT("home-preview-paper"),FSlateRect(PreviewX-132,R.Navigation.Top-52,PreviewX+132,R.Navigation.Top-8)));
    Place(GetWidgetFromName(TEXT("HUDPreviewCaption")),FSlateRect(PreviewX-120,R.Navigation.Top-44,PreviewX+120,R.Navigation.Top-14));
    Place(GetWidgetFromName(TEXT("HUDNavigationVeil")),FSlateRect(R.Navigation.Left-4,R.Navigation.Top-4,R.Navigation.Right+4,R.Navigation.Bottom+4));
    const TCHAR* Top[]={TEXT("HUD_OpenActivities"),TEXT("HUD_OpenSignIn"),TEXT("HUD_OpenExtensions"),TEXT("HUD_OpenLaunchEvent"),TEXT("HUD_OpenWuling")};
    for(int I=0;I<5;++I)
    {
        const auto Rect=R.TopActions[I];
        Place(GetWidgetFromName(Top[I]),Rect);
    }
    const auto LaunchRect=R.TopActions[3];
    Place(GetWidgetFromName(TEXT("RolloutHUDLaunchCountdown")),FSlateRect(LaunchRect.Left-212,LaunchRect.Top,LaunchRect.Left-12,LaunchRect.Top+80));
}
