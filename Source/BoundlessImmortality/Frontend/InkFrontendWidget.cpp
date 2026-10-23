#include "InkFrontendWidget.h"
#include "Frontend/UI/InkUiAssetPaths.h"
#include "InkAnchorStatusWidget.h"
#include "InkPresentationStyle.h"
#include "UI/InkUiTheme.h"
#include "UI/InkRewardReceiptPanel.h"
#include "Wuling/WulingPrototypeSubsystem.h"
#include "UI/InkUiResponsiveLayout.h"
#include "InkMenuDirector.h"
#include "InkSaveSubsystem.h"
#include "InkExtensionWidget.h"
#include "InkFeaturePanel.h"
#include "InkLeaderboardAvatarRequest.h"
#include "Cultivation/CultivationSubsystem.h"
#include "CultivationGameplayComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SafeZone.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/RetainerBox.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WidgetSwitcherSlot.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateColorBrush.h"

#define LOCTEXT_NAMESPACE "InkFrontend"

namespace InkFrontend
{
    const FName PageNames[]={TEXT("Main"),TEXT("HUD"),TEXT("Saves"),TEXT("Settings"),TEXT("Character"),TEXT("Inventory"),TEXT("Leaderboard")};
    const FName AttributeIds[]={TEXT("Health"),TEXT("Attack"),TEXT("Defense"),TEXT("SpiritDamage"),TEXT("Critical"),TEXT("Evasion"),TEXT("Luck"),TEXT("MoveSpeed"),TEXT("Swordsmanship"),TEXT("CultivationSpeed"),TEXT("Realm")};

    FSlateBrush AtlasBrush(UTexture2D* Texture,FVector4 Rect,bool bBox=false)
    {
        FSlateBrush Brush;Brush.SetResourceObject(Texture);Brush.DrawAs=ESlateBrushDrawType::Image;
        Brush.ImageSize=FVector2D(Rect.Z-Rect.X,Rect.W-Rect.Y);Brush.Margin=FMargin(0);
        if(Texture)
        {
            const FString Name=FString::Printf(TEXT("MI_%s_%d_%d_%d_%d_%s"),*Texture->GetName(),int32(Rect.X),int32(Rect.Y),int32(Rect.Z),int32(Rect.W),bBox?TEXT("Slice"):TEXT("Fit"));
            const FString Path=TEXT("/Game/UI/Ink/Materials/Atlas/")+Name+TEXT(".")+Name;
            if(auto* Material=InkUiAssets::Load<UMaterialInterface>(nullptr,*Path,nullptr,LOAD_NoWarn|LOAD_Quiet))
            {Brush.SetResourceObject(Material);return Brush;}
        }
        if(Texture)Brush.SetUVRegion(FBox2f(FVector2f(Rect.X/FMath::Max(1,Texture->GetSizeX()),Rect.Y/FMath::Max(1,Texture->GetSizeY())),FVector2f(Rect.Z/FMath::Max(1,Texture->GetSizeX()),Rect.W/FMath::Max(1,Texture->GetSizeY()))));
        return Brush;
    }

    template<typename T> T* Find(const UInkFrontendWidget* Widget,const TCHAR* Name)
    { return Cast<T>(Widget->GetWidgetFromName(FName(Name))); }

    template<typename T> T* Find(UWidgetTree* Tree,const TCHAR* Name)
    { return Tree?Cast<T>(Tree->FindWidget(FName(Name))):nullptr; }

    void FillCanvas(UCanvasPanelSlot* LayoutSlot,FMargin Margins=FMargin(0))
    { LayoutSlot->SetAnchors(FAnchors(0,0,1,1));LayoutSlot->SetOffsets(Margins); }

    void FillOverlay(UOverlaySlot* LayoutSlot)
    { LayoutSlot->SetHorizontalAlignment(HAlign_Fill);LayoutSlot->SetVerticalAlignment(VAlign_Fill); }

    UInkSaveSubsystem* Saves(const UUserWidget* Widget)
    { auto* Instance=Widget->GetGameInstance();return Instance?Instance->GetSubsystem<UInkSaveSubsystem>():nullptr; }

    FText ActionStateText(const ECultivationActionState State)
    {
        switch(State)
        {
        case ECultivationActionState::MeditationStart:return LOCTEXT("MeditationStart","凝神入定");
        case ECultivationActionState::MeditationLoop:return LOCTEXT("Meditating","静心修炼");
        case ECultivationActionState::MeditationEnd:return LOCTEXT("MeditationEnd","收功回神");
        case ECultivationActionState::ChargeStart:
        case ECultivationActionState::ChargeLoop:return LOCTEXT("Charging","聚气蓄势");
        case ECultivationActionState::ChargeRelease:return LOCTEXT("Releasing","运气出招");
        default:return LOCTEXT("Idle","自由行动");
        }
    }
}

void UInkFrontendActionButton::BindFrontend(UInkFrontendWidget* InFrontend)
{
    Frontend=InFrontend;
    OnClicked.AddUniqueDynamic(this,&UInkFrontendActionButton::HandleClicked);
    OnHovered.AddUniqueDynamic(this,&UInkFrontendActionButton::HandleHovered);
    OnUnhovered.AddUniqueDynamic(this,&UInkFrontendActionButton::HandleUnhovered);
    OnPressed.AddUniqueDynamic(this,&UInkFrontendActionButton::HandlePressed);
    OnReleased.AddUniqueDynamic(this,&UInkFrontendActionButton::HandleReleased);
}

void UInkFrontendActionButton::HandleClicked()
{ if(auto* Widget=Frontend.Get())Widget->TriggerAction(Action); }

void UInkFrontendActionButton::HandleHovered()
{ if(auto* Widget=Frontend.Get())if(Action.ToString().StartsWith(TEXT("InventorySelect_")))Widget->HoverInventorySlot(FCString::Atoi(*Action.ToString().Mid(16)),true); }

void UInkFrontendActionButton::HandleUnhovered()
{ HandleReleased();if(auto* Widget=Frontend.Get())if(Action.ToString().StartsWith(TEXT("InventorySelect_")))Widget->HoverInventorySlot(FCString::Atoi(*Action.ToString().Mid(16)),false); }

void UInkFrontendActionButton::HandlePressed()
{if(auto* F=Frontend.Get();F&&!F->bReducedMotion)if(auto* C=GetContent()){C->SetRenderTransformPivot(FVector2D(.5));C->SetRenderScale(FVector2D(.97));}}
void UInkFrontendActionButton::HandleReleased()
{if(auto* C=GetContent())C->SetRenderScale(FVector2D(1));}

UInkFrontendWidget::UInkFrontendWidget(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer)
{
    SetIsFocusable(true);
    // Optional art uses the existing Slate paper/button surfaces until supplied.
    PanelTexture.Reset();
    ButtonTexture.Reset();
    TitleTexture=TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/UI/Ink/Textures/Shared/T_InkTitle.T_InkTitle")));
    MeditationTexture=TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/UI/Ink/Textures/Cultivation/T_InkMeditation.T_InkMeditation")));
    AttributeIconAtlas=TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/UI/Ink/Textures/Shared/T_InkAttributeIcons.T_InkAttributeIcons")));
    ProvidedButtonAtlas=TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/UI/Ink/Textures/Shared/T_ProvidedButtons.T_ProvidedButtons")));
    ProvidedRankAtlas=TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/UI/Ink/Textures/Leaderboard/T_ProvidedRank.T_ProvidedRank")));
    ProvidedInventoryAtlas=TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/UI/Ink/Textures/Inventory/T_ProvidedInventory.T_ProvidedInventory")));
    TitleFeatherMaterial=TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/UI/Ink/Materials/Shared/M_InkTitleFeather.M_InkTitleFeather")));
    ScrollEdgeMaterial=TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/UI/Ink/Materials/Shared/M_InkScrollFade.M_InkScrollFade")));
    TextFont=TSoftObjectPtr<UFont>(FSoftObjectPath(TEXT("/Game/UI/Ink/Fonts/Shared/F_InkBody.F_InkBody")));
    PaperColor=FLinearColor::FromSRGBColor(FColor(238,233,223));
    InkColor=FLinearColor::FromSRGBColor(FColor(32,34,31));
    AccentColor=FLinearColor::FromSRGBColor(FColor(173,150,104));
    CinnabarColor=FLinearColor::FromSRGBColor(FColor(145,62,47));
}

UTextBlock* UInkFrontendWidget::AddText(UWidgetTree* Tree,UVerticalBox* Box,const FText& Text,int32 Size,FName Name)
{
    auto* Label=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);
    Label->SetText(Text);
    FSlateFontInfo Font=InkUiTypography::MakeFont(Size>=40?EInkUiTextRole::Display:EInkUiTextRole::Body,Size>0?Size:BodyFontSize);
    if(Size<40)if(auto* LoadedFont=TextFont.LoadSynchronous())Font.FontObject=LoadedFont;
    Label->SetFont(Font);Label->SetColorAndOpacity(InkColor);Label->SetAutoWrapText(true);
    Label->SetVisibility(ESlateVisibility::HitTestInvisible);
    if(Box)Box->AddChildToVerticalBox(Label)->SetPadding(FMargin(0,8));
    return Label;
}

UInkFrontendActionButton* UInkFrontendWidget::MakeButton(UWidgetTree* Tree,FName Name,FName Action,const FText& Text)
{
    auto* Button=Tree->ConstructWidget<UInkFrontendActionButton>(UInkFrontendActionButton::StaticClass(),Name);
    Button->Action=Action;
    FButtonStyle Style=InkPresentationStyle::Button();
    Style.SetNormalPadding(FMargin(18,6));Style.SetPressedPadding(FMargin(18,8,18,4));Button->SetStyle(Style);
    auto* Sizing=Tree->ConstructWidget<USizeBox>();Sizing->SetMinDesiredHeight(ButtonHeight);
    auto* TextLabel=AddText(Tree,nullptr,Text);TextLabel->SetJustification(ETextJustify::Center);
    TextLabel->SetAutoWrapText(false);
    Sizing->SetContent(TextLabel);if(auto* LabelSlot=Cast<USizeBoxSlot>(TextLabel->Slot))LabelSlot->SetVerticalAlignment(VAlign_Center);Button->SetContent(Sizing);
    if(auto* ContentSlot=Cast<UButtonSlot>(Sizing->Slot))ContentSlot->SetPadding(FMargin(0));
    Button->SetClickMethod(EButtonClickMethod::DownAndUp);Button->SetTouchMethod(EButtonTouchMethod::DownAndUp);
    return Button;
}

UBorder* UInkFrontendWidget::MakeInkCard(UWidgetTree* Tree,UWidget* Content,FMargin CardPadding,bool bSelected)
{
    auto* Outline=Tree->ConstructWidget<UBorder>();Outline->SetPadding(FMargin(bSelected?2.f:1.f));
    Outline->SetBrushColor(bSelected?AccentColor:FLinearColor(InkColor.R,InkColor.G,InkColor.B,.24f));
    auto* Paper=Tree->ConstructWidget<UBorder>();Paper->SetPadding(CardPadding);
    Paper->SetBrushColor(FLinearColor::FromSRGBColor(bSelected?FColor(231,221,201):FColor(232,228,218)));
    Paper->SetContent(Content);Outline->SetContent(Paper);
    if(auto* Material=InkUiAssets::Load<UMaterialInterface>(nullptr,TEXT("/Game/UI/Ink/MaterialInstances/Shared/MI_InkCardSoft.MI_InkCardSoft"),nullptr,LOAD_NoWarn|LOAD_Quiet))
    {
        FSlateBrush Brush;Brush.SetResourceObject(Material);Brush.DrawAs=ESlateBrushDrawType::Image;
        Outline->SetBrush(Brush);Outline->SetBrushColor(FLinearColor::White);Paper->SetBrushColor(FLinearColor::Transparent);
    }
    return Outline;
}

UBorder* UInkFrontendWidget::MakeLeaderboardRow(UWidgetTree* Tree,const FText& Rank,const FText& Name,const FText& Metric,bool bHeader)
{
    auto* Row=Tree->ConstructWidget<UHorizontalBox>();
    auto* RankSize=Tree->ConstructWidget<USizeBox>();RankSize->SetWidthOverride(76);RankSize->SetMinDesiredHeight(bHeader?50.f:64.f);
    auto* RankText=AddText(Tree,nullptr,Rank,24);RankText->SetJustification(ETextJustify::Center);RankSize->SetContent(RankText);
    if(auto* TextLayout=Cast<USizeBoxSlot>(RankText->Slot))TextLayout->SetVerticalAlignment(VAlign_Center);
    Row->AddChildToHorizontalBox(RankSize)->SetVerticalAlignment(VAlign_Center);
    auto* NameText=AddText(Tree,nullptr,Name,24);auto* NameLayout=Row->AddChildToHorizontalBox(NameText);NameLayout->SetSize(FSlateChildSize(ESlateSizeRule::Fill));NameLayout->SetVerticalAlignment(VAlign_Center);NameLayout->SetPadding(FMargin(12,0));
    auto* MetricText=AddText(Tree,nullptr,Metric,24,bHeader?FName(TEXT("LeaderboardColumns")):NAME_None);MetricText->SetJustification(ETextJustify::Right);
    auto* MetricLayout=Row->AddChildToHorizontalBox(MetricText);FSlateChildSize MetricSize(ESlateSizeRule::Fill);MetricSize.Value=.75f;MetricLayout->SetSize(MetricSize);MetricLayout->SetVerticalAlignment(VAlign_Center);MetricLayout->SetPadding(FMargin(8,0));
    auto* Card=MakeInkCard(Tree,Row,FMargin(4,4),bHeader);
    if(auto* Texture=ProvidedRankAtlas.LoadSynchronous())
    {
        Card->SetBrush(InkFrontend::AtlasBrush(Texture,bHeader?FVector4(40,935,976,987):FVector4(40,990,976,1041),true));Card->SetBrushColor(FLinearColor::White);
        if(auto* Inner=Cast<UBorder>(Card->GetContent()))Inner->SetBrushColor(FLinearColor::Transparent);
    }
    return Card;
}

void UInkFrontendWidget::BuildDefaultTree(UWidgetTree* Tree)
{
    if(!Tree||Tree->RootWidget)return;
    auto* Root=Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(),TEXT("InkRoot"));
    Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);Tree->RootWidget=Root;
    auto* Safe=Tree->ConstructWidget<USafeZone>(USafeZone::StaticClass(),TEXT("DeviceSafeZone"));Safe->SetSidesToPad(true,true,true,true);
    InkFrontend::FillOverlay(Root->AddChildToOverlay(Safe));
    auto* Canvas=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("SafeCanvas"));Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);Safe->SetContent(Canvas);
    auto* Pages=Tree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(),TEXT("PageSwitcher"));
    Pages->SetVisibility(ESlateVisibility::SelfHitTestInvisible);InkFrontend::FillCanvas(Canvas->AddChildToCanvas(Pages));

    auto AddPage=[&](UWidget* Widget){auto* LayoutSlot=Cast<UWidgetSwitcherSlot>(Pages->AddChild(Widget));LayoutSlot->SetHorizontalAlignment(HAlign_Fill);LayoutSlot->SetVerticalAlignment(VAlign_Fill);};
    auto Paper=[&](FName Name)
    {
        auto* Border=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),Name);Border->SetPadding(FMargin(28));
        FSlateBrush Brush;Brush.DrawAs=ESlateBrushDrawType::Box;Brush.Margin=FMargin(.1f);Brush.TintColor=PaperColor;
        if(auto* Texture=PanelTexture.LoadSynchronous()){Brush.SetResourceObject(Texture);Brush.TintColor=FLinearColor::White;}
        if(auto* Texture=ProvidedRankAtlas.LoadSynchronous())Brush=InkFrontend::AtlasBrush(Texture,FVector4(28,15,815,443),true);
        Border->SetBrush(Brush);return Border;
    };
    auto AddRowButton=[&](UHorizontalBox* Row,const TCHAR* Name,const TCHAR* Action,const FText& Text)
    {
        auto* Button=MakeButton(Tree,Name,Action,Text);auto* LayoutSlot=Row->AddChildToHorizontalBox(Button);
        LayoutSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));LayoutSlot->SetPadding(FMargin(4));return Button;
    };
    auto AddPageFrame=[&](const TCHAR* Name,const FText& Title,const FText& Intro)
    {
        auto* PageCanvas=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),FName(Name));AddPage(PageCanvas);
        auto* Border=Paper(FName(*(FString(Name)+TEXT("Paper"))));InkFrontend::FillCanvas(PageCanvas->AddChildToCanvas(Border),FMargin(20));
        if(bUsePageSafePadding)Border->SetPadding(PageSafePadding);
        auto* Layout=Tree->ConstructWidget<UVerticalBox>();Border->SetContent(Layout);
        const bool bRank=FCString::Strcmp(Name,TEXT("Leaderboard"))==0,bInventory=FCString::Strcmp(Name,TEXT("Inventory"))==0;
        if((bRank&&ProvidedRankAtlas.LoadSynchronous())||(bInventory&&ProvidedInventoryAtlas.LoadSynchronous()))
        {
            auto* Art=Tree->ConstructWidget<UImage>(UImage::StaticClass(),bRank?FName(TEXT("LeaderboardArtHeading")):FName(TEXT("InventoryArtHeading")));
            Art->SetBrush(InkFrontend::AtlasBrush(bRank?ProvidedRankAtlas.LoadSynchronous():ProvidedInventoryAtlas.LoadSynchronous(),bRank?FVector4(16,450,436,584):FVector4(14,0,683,131)));
            auto* ArtScale=Tree->ConstructWidget<UScaleBox>();ArtScale->SetStretch(EStretch::ScaleToFit);ArtScale->SetContent(Art);
            auto* ArtSize=Tree->ConstructWidget<USizeBox>();ArtSize->SetHeightOverride(bRank?130:100);ArtSize->SetContent(ArtScale);Layout->AddChildToVerticalBox(ArtSize);
        }
        else {auto* Heading=AddText(Tree,Layout,Title,44);Heading->SetColorAndOpacity(InkColor);}
        if(!Intro.IsEmpty()){auto* Description=AddText(Tree,Layout,Intro,24);Description->SetColorAndOpacity(FLinearColor(InkColor.R,InkColor.G,InkColor.B,.72f));}
        auto* Rule=Tree->ConstructWidget<UBorder>();Rule->SetBrushColor(AccentColor);Rule->SetPadding(FMargin(0));
        auto* RuleSize=Tree->ConstructWidget<USizeBox>();RuleSize->SetHeightOverride(2);RuleSize->SetContent(Rule);Layout->AddChildToVerticalBox(RuleSize)->SetPadding(FMargin(0,14));
        auto* Scroll=Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),FName(*(FString(Name)+TEXT("Scroll"))));
        Scroll->SetScrollBarVisibility(ESlateVisibility::Visible);
        UWidget* ScrollSurface=Scroll;
        if(auto* Material=ScrollEdgeMaterial.LoadSynchronous())
        {
            auto* Fade=Tree->ConstructWidget<URetainerBox>();Fade->SetContent(Scroll);Fade->SetEffectMaterial(Material);Fade->SetTextureParameter(TEXT("Texture"));ScrollSurface=Fade;
        }
        auto* ScrollSlot=Layout->AddChildToVerticalBox(ScrollSurface);ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        auto* Body=Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),FName(*(FString(Name)+TEXT("Content"))));Scroll->AddChild(Body);
        auto* Feedback=AddText(Tree,Layout,FText::GetEmpty(),23,FName(*(FString(Name)+TEXT("Feedback"))));Feedback->SetColorAndOpacity(CinnabarColor);
        auto* Back=MakeButton(Tree,FName(*(FString(Name)+TEXT("Back"))),TEXT("Back"),LOCTEXT("Back","返回"));
        Layout->AddChildToVerticalBox(Back)->SetPadding(FMargin(0,8,0,0));return Body;
    };

    // Keep the middle of the main menu clear for the live meditation camera.
    auto* Main=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("MainPage"));Main->SetVisibility(ESlateVisibility::SelfHitTestInvisible);AddPage(Main);
    auto* TitleScale=Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(),TEXT("MainTitleScale"));TitleScale->SetStretch(EStretch::ScaleToFit);
    auto* TitleImage=Tree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("MainTitleImage"));TitleImage->SetBrushFromTexture(TitleTexture.LoadSynchronous(),true);TitleImage->SetVisibility(ESlateVisibility::HitTestInvisible);TitleScale->SetContent(TitleImage);
    auto* TitleSlot=Main->AddChildToCanvas(TitleScale);TitleSlot->SetAnchors(FAnchors(.12f,.02f,.88f,.21f));TitleSlot->SetOffsets(FMargin(0));
    auto* MenuScroll=Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("MainMenuScroll"));MenuScroll->SetScrollBarVisibility(ESlateVisibility::Collapsed);
    auto* MenuSlot=Main->AddChildToCanvas(MenuScroll);MenuSlot->SetAnchors(FAnchors(.15f,.62f,.85f,.98f));MenuSlot->SetOffsets(FMargin(0));
    auto* Menu=Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("MainMenuButtons"));MenuScroll->AddChild(Menu);
    const FName MainActions[]={TEXT("StartGame"),TEXT("OpenSaves"),TEXT("OpenSettings"),TEXT("RequestQuit")};
    const FText MainLabels[]={LOCTEXT("StartGame","开始游戏"),LOCTEXT("ContinueMenuSaves","继续游戏"),LOCTEXT("Settings","设置"),LOCTEXT("MenuExit","退出")};
    for(int32 I=0;I<4;++I)
    {
        auto* Button=MakeButton(Tree,FName(*(FString(TEXT("Main_"))+MainActions[I].ToString())),MainActions[I],MainLabels[I]);
        // 64 content + 12 style padding + 8 row spacing = 84 units per menu entry.
        if(auto* Sizing=Cast<USizeBox>(Button->GetContent()))
        {
            Sizing->SetMinDesiredHeight(FMath::Max(64.f,ButtonHeight-16.f));
            if(auto* ContentSlot=Cast<UButtonSlot>(Sizing->Slot))ContentSlot->SetPadding(FMargin(0));
        }
        Menu->AddChildToVerticalBox(Button)->SetPadding(FMargin(0,4));
    }
    AddText(Tree,Menu,FText::GetEmpty(),23,TEXT("MainFeedback"))->SetVisibility(ESlateVisibility::Collapsed);

    auto* HUD=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("HUDPage"));HUD->SetVisibility(ESlateVisibility::SelfHitTestInvisible);AddPage(HUD);
    auto* HUDButtons=Tree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(),TEXT("HUDNavigation"));HUDButtons->SetSlotPadding(FMargin(5));
    auto* HUDSlot=HUD->AddChildToCanvas(HUDButtons);HUDSlot->SetAnchors(FAnchors(0,1,1,1));HUDSlot->SetOffsets(FMargin(20,-220,20,200));
    const FName HUDActions[]={TEXT("OpenCharacter"),TEXT("OpenInventory"),TEXT("OpenLeaderboard"),TEXT("OpenSaves"),TEXT("OpenSettings"),TEXT("ReturnToMainMenu")};
    const FText HUDLabels[]={LOCTEXT("Character","角色"),LOCTEXT("Inventory","背包"),LOCTEXT("Leaderboard","排行榜"),LOCTEXT("HUDSaves","存档"),LOCTEXT("HUDSettings","设置"),LOCTEXT("ReturnMain","返回主菜单")};
    for(int32 I=0;I<6;++I){auto* Button=MakeButton(Tree,FName(*(FString(TEXT("HUD_"))+HUDActions[I].ToString())),HUDActions[I],HUDLabels[I]);auto* LayoutSlot=HUDButtons->AddChildToUniformGrid(Button,I/3,I%3);LayoutSlot->SetHorizontalAlignment(HAlign_Fill);LayoutSlot->SetVerticalAlignment(VAlign_Fill);}

    auto* SavesBody=AddPageFrame(TEXT("Saves"),LOCTEXT("SaveHeading","山河留迹"),LOCTEXT("SaveIntro","保存此刻的行程，续写下一段修行。"));
    AddText(Tree,SavesBody,LOCTEXT("SaveScope","本地存档记录当前场景与角色位置。"),24);
    auto* SaveList=Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("SaveSlotList"));SavesBody->AddChildToVerticalBox(SaveList);

    auto* SettingsBody=AddPageFrame(TEXT("Settings"),LOCTEXT("SettingsHeading","静心调息"),LOCTEXT("SettingsIntro","随心调整，让修行更从容。"));
    AddText(Tree,SettingsBody,LOCTEXT("Quality","画面品质"),32);
    auto* QualityRow=Tree->ConstructWidget<UHorizontalBox>();SettingsBody->AddChildToVerticalBox(QualityRow);
    AddRowButton(QualityRow,TEXT("QualityLow"),TEXT("QualityLow"),LOCTEXT("Low","流畅"));AddRowButton(QualityRow,TEXT("QualityMedium"),TEXT("QualityMedium"),LOCTEXT("Medium","均衡"));AddRowButton(QualityRow,TEXT("QualityHigh"),TEXT("QualityHigh"),LOCTEXT("High","精致"));
    AddText(Tree,SettingsBody,FText::GetEmpty(),24,TEXT("QualityValue"));
    AddText(Tree,SettingsBody,LOCTEXT("FrameRate","帧率上限"),32);
    auto* FrameRow=Tree->ConstructWidget<UHorizontalBox>();SettingsBody->AddChildToVerticalBox(FrameRow);
    AddRowButton(FrameRow,TEXT("Frame30"),TEXT("Frame30"),LOCTEXT("FPS30","30 帧"));AddRowButton(FrameRow,TEXT("Frame60"),TEXT("Frame60"),LOCTEXT("FPS60","60 帧"));
    AddText(Tree,SettingsBody,FText::GetEmpty(),24,TEXT("FrameValue"));

    auto* CharacterBody=AddPageFrame(TEXT("Character"),LOCTEXT("CharacterHeading","观己"),LOCTEXT("CharacterIntro","内观其心，外察其行。"));
    AddText(Tree,CharacterBody,LOCTEXT("CurrentCharacter","当前角色"),34,TEXT("CharacterName"));
    auto* MeditationSize=Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("CharacterMeditationSize"));MeditationSize->SetWidthOverride(220);MeditationSize->SetHeightOverride(220);
    auto* MeditationScale=Tree->ConstructWidget<UScaleBox>();MeditationScale->SetStretch(EStretch::ScaleToFit);
    auto* MeditationImage=Tree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("CharacterMeditationImage"));MeditationImage->SetBrushFromTexture(MeditationTexture.LoadSynchronous(),true);MeditationImage->SetVisibility(ESlateVisibility::HitTestInvisible);MeditationScale->SetContent(MeditationImage);MeditationSize->SetContent(MeditationScale);
    auto* MeditationLayout=CharacterBody->AddChildToVerticalBox(MeditationSize);MeditationLayout->SetHorizontalAlignment(HAlign_Center);MeditationLayout->SetPadding(FMargin(0,4,0,8));
    AddText(Tree,CharacterBody,FText::GetEmpty(),24,TEXT("CharacterLiveStatus"));
    AddText(Tree,CharacterBody,LOCTEXT("UnknownAttributeHint","— 表示暂无属性数据"),21,TEXT("CharacterAttributeHint"));
    auto* AttributeList=Tree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(),TEXT("CharacterAttributeList"));AttributeList->SetSlotPadding(FMargin(4));CharacterBody->AddChildToVerticalBox(AttributeList);
    auto* DisciplineCards=Tree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(),TEXT("DisciplineCards"));DisciplineCards->SetSlotPadding(FMargin(5));CharacterBody->AddChildToVerticalBox(DisciplineCards)->SetPadding(FMargin(0,16,0,6));
    const TCHAR* DisciplineIds[]={TEXT("Inner"),TEXT("Movement"),TEXT("Martial")};
    const FText DisciplineLabels[]={LOCTEXT("Inner","内功"),LOCTEXT("Movement","轻功"),LOCTEXT("Martial","武绝")};
    for(int32 I=0;I<3;++I)
    {
        auto* CardBody=Tree->ConstructWidget<UVerticalBox>();auto* Heading=AddText(Tree,CardBody,DisciplineLabels[I],28);Heading->SetJustification(ETextJustify::Center);
        auto* List=Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),FName(*(FString(TEXT("Discipline_"))+DisciplineIds[I])));CardBody->AddChildToVerticalBox(List);
        auto* CardSize=Tree->ConstructWidget<USizeBox>();CardSize->SetMinDesiredHeight(150);CardSize->SetContent(MakeInkCard(Tree,CardBody,FMargin(10)));
        auto* CardLayout=DisciplineCards->AddChildToUniformGrid(CardSize,0,I);CardLayout->SetHorizontalAlignment(HAlign_Fill);CardLayout->SetVerticalAlignment(VAlign_Fill);
    }
    AddText(Tree,CharacterBody,LOCTEXT("Manual","秘籍"),30);
    auto* ManualSlots=Tree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(),TEXT("Discipline_Manual"));ManualSlots->SetSlotPadding(FMargin(4));CharacterBody->AddChildToVerticalBox(ManualSlots);

    RebuildInventoryPage(Tree);

    auto* LeaderboardBody=AddPageFrame(TEXT("Leaderboard"),LOCTEXT("LeaderboardHeading","仙途留名"),LOCTEXT("LeaderboardIntro","各有所长，皆有其名。"));
    auto* RankTabs=Tree->ConstructWidget<UHorizontalBox>();LeaderboardBody->AddChildToVerticalBox(RankTabs);
    AddRowButton(RankTabs,TEXT("RankAnchor"),TEXT("RankAnchor"),LOCTEXT("RankAnchor","主播"));AddRowButton(RankTabs,TEXT("RankCultivation"),TEXT("RankCultivation"),LOCTEXT("RankCultivation","修炼速度"));AddRowButton(RankTabs,TEXT("RankDamage"),TEXT("RankDamage"),LOCTEXT("RankDamage","伤害"));
    LeaderboardBody->AddChildToVerticalBox(MakeLeaderboardRow(Tree,LOCTEXT("RankColumn","名次"),LOCTEXT("NameColumn","名称"),LOCTEXT("ValueColumn","数值"),true))->SetPadding(FMargin(0,12,0,4));
    auto* RankList=Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("LeaderboardList"));LeaderboardBody->AddChildToVerticalBox(RankList);

    auto* Confirmation=Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(),TEXT("ConfirmationLayer"));InkFrontend::FillOverlay(Root->AddChildToOverlay(Confirmation));
    auto* Dim=Tree->ConstructWidget<UBorder>();Dim->SetBrushColor(FLinearColor(0,0,0,.52f));InkFrontend::FillOverlay(Confirmation->AddChildToOverlay(Dim));
    auto* DialogSafe=Tree->ConstructWidget<USafeZone>();InkFrontend::FillOverlay(Confirmation->AddChildToOverlay(DialogSafe));
    auto* DialogCanvas=Tree->ConstructWidget<UCanvasPanel>();DialogSafe->SetContent(DialogCanvas);
    auto* DialogPaper=Paper(TEXT("ConfirmationPaper"));DialogPaper->SetPadding(FMargin(44,54));auto* DialogSlot=DialogCanvas->AddChildToCanvas(DialogPaper);DialogSlot->SetAnchors(FAnchors(.08f,.26f,.92f,.74f));DialogSlot->SetOffsets(FMargin(0));
    auto* DialogScroll=Tree->ConstructWidget<UScrollBox>();DialogPaper->SetContent(DialogScroll);auto* Dialog=Tree->ConstructWidget<UVerticalBox>();DialogScroll->AddChild(Dialog);
    AddText(Tree,Dialog,LOCTEXT("ConfirmHeading","请确认"),36,TEXT("ConfirmationTitle"));AddText(Tree,Dialog,FText::GetEmpty(),28,TEXT("ConfirmationMessage"));
    Dialog->AddChildToVerticalBox(MakeButton(Tree,TEXT("ConfirmPending"),TEXT("ConfirmPending"),LOCTEXT("Confirm","确认")))->SetPadding(FMargin(0,14,0,6));
    Dialog->AddChildToVerticalBox(MakeButton(Tree,TEXT("CancelPending"),TEXT("CancelPending"),LOCTEXT("Cancel","取消")));
    Confirmation->SetVisibility(ESlateVisibility::Collapsed);
    Pages->SetActiveWidgetIndex(0);
    // Save the initial empty-state structure too, so it is visible and editable in the WBP tree.
    RebuildLeaderboardSurfaces(Tree);RefreshCharacter(Tree);RefreshInventory(Tree);RefreshLeaderboard(Tree);
}

TSharedRef<SWidget> UInkFrontendWidget::RebuildWidget()
{
    if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this,TEXT("InkWidgetTree"));
    BuildDefaultTree(WidgetTree);
    if(!WidgetTree->FindWidget(TEXT("InventoryShell")))RebuildInventoryPage(WidgetTree);
    if(!WidgetTree->FindWidget(TEXT("RankHUDTitleEffect_1")))RebuildLeaderboardSurfaces(WidgetTree);
    return Super::RebuildWidget();
}

void UInkFrontendWidget::NativeOnInitialized()
{ Super::NativeOnInitialized();if(WidgetTree){BuildDefaultTree(WidgetTree);if(!WidgetTree->FindWidget(TEXT("InventoryShell")))RebuildInventoryPage(WidgetTree);if(!WidgetTree->FindWidget(TEXT("RankHUDTitleEffect_1")))RebuildLeaderboardSurfaces(WidgetTree);}EnsureExtensionPages();EnsureLeaderboardControls();BindTree(); }

void UInkFrontendWidget::NativePreConstruct()
{
    Super::NativePreConstruct();BindTree();ApplyPageSafePadding();ApplyProvidedArt();
    if(IsDesignTime())ShowPage(DesignerPreviewPage);
}

void UInkFrontendWidget::NativeConstruct()
{
    Super::NativeConstruct();BindTree();ApplyPageSafePadding();ApplyProvidedArt();
    if(auto* Settings=UGameUserSettings::GetGameUserSettings())if(Settings->GetFrameRateLimit()>0&&GEngine)GEngine->bUseFixedFrameRate=false;
    ShowPage(CurrentPage);
}

void UInkFrontendWidget::NativeDestruct()
{
    for(const auto& Pair:LeaderboardAvatarRequests)if(Pair.Value)Pair.Value->Cancel();LeaderboardAvatarRequests.Reset();
    for(auto Button:ActionButtons)if(Button)Button->BindFrontend(nullptr);
    ActionButtons.Reset();Super::NativeDestruct();
}

void UInkFrontendWidget::Configure(AInkMenuDirector* InDirector)
{
    Director=InDirector;EnsureExtensionPages();EnsureLeaderboardControls();BindTree();ShowPage(Director&&Director->bMenuActive?TEXT("Main"):TEXT("HUD"));
}

void UInkFrontendWidget::BindTree()
{
    ActionButtons.Reset();if(!WidgetTree)return;
    TArray<UWidget*> Widgets;WidgetTree->GetAllWidgets(Widgets);
    for(auto* Widget:Widgets)if(auto* Button=Cast<UInkFrontendActionButton>(Widget)){Button->BindFrontend(this);ActionButtons.Add(Button);}
    for(const auto& Entry:ExtensionPages)if(Entry.Key==CurrentPage&&Entry.Value&&Entry.Value->WidgetTree)
    {
        TFunction<void(UUserWidget*)> BindNested=[&](UUserWidget* User){if(!User||!User->WidgetTree)return;TArray<UWidget*> Children;User->WidgetTree->GetAllWidgets(Children);for(auto* Child:Children){if(auto* Button=Cast<UInkFrontendActionButton>(Child)){Button->BindFrontend(this);ActionButtons.Add(Button);}if(auto* Nested=Cast<UUserWidget>(Child);Nested&&Nested!=User)BindNested(Nested);}};
        BindNested(Entry.Value);
    }
}

bool UInkFrontendWidget::RevealFrontendAction(FName Action)
{
    for(auto B:ActionButtons)if(B&&B->Action==Action)
    {
        for(auto* Parent=B->GetParent();Parent;Parent=Parent->GetParent())if(auto* Scroll=Cast<UScrollBox>(Parent))
        {Scroll->ScrollWidgetIntoView(B,false,EDescendantScrollDestination::IntoView);return true;}
        return true;
    }
    return false;
}

void UInkFrontendWidget::ShowPage(FName Page)
{
    if(CurrentPage==TEXT("Draw")&&Page!=TEXT("Draw"))DrawPreviewWheel.Skip();
    EnsureFeaturePage(Page);
    int32 Index=INDEX_NONE;for(int32 I=0;I<UE_ARRAY_COUNT(InkFrontend::PageNames);++I)if(InkFrontend::PageNames[I]==Page){Index=I;break;}
    UInkExtensionWidget* Extension=ExtensionPages.FindRef(Page);
    if(Index==INDEX_NONE&&!Extension)return;
    if(Index!=INDEX_NONE&&ExtensionPages.Contains(CurrentPage))
    {
        for(const auto& Pair:ExtensionPages)if(Pair.Value)Pair.Value->RemoveFromParent();
        ExtensionPages.Reset();FeaturePanelStack.Reset();
    }
    if(Page!=TEXT("Inventory")){SelectedInventoryEntry.Invalidate();RemoveInventoryWindow(TEXT("InventoryDetailPanel"));RemoveInventoryWindow(TEXT("InventoryTooltip"));}
    CurrentPage=Page;
    if(auto* Menu=GetWidgetFromName(TEXT("HUDRuntimeMenu")))Menu->SetVisibility(ESlateVisibility::Collapsed);
    if(auto* Cultivate=GetWidgetFromName(TEXT("HUD_ToggleCultivation")))Cultivate->SetVisibility(ESlateVisibility::Visible);
    // Extension pages are created before gameplay; refresh their real identity
    // and attributes on opening instead of retaining the construction snapshot.
    if(Extension)Extension->Configure(this,Page);
    HoveredInventoryEntry.Invalidate();UpdateInventoryTooltip();
    if(auto* Scroll=InkFrontend::Find<UScrollBox>(this,*(Page.ToString()+TEXT("Scroll"))))Scroll->SetScrollOffset(0);
    if(auto* Pages=InkFrontend::Find<UWidgetSwitcher>(this,TEXT("PageSwitcher"))){if(Extension)Pages->SetActiveWidget(Extension);else Pages->SetActiveWidgetIndex(Index);}
    SetQuitConfirmation(false);SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    if(Director&&!IsDesignTime())Director->SetFrontendInputBlocked(Page!=TEXT("HUD")||RewardReceiptPanel!=nullptr);
    RefreshPage();
    if(!IsDesignTime()&&IsInViewport()&&Page!=TEXT("HUD"))SetKeyboardFocus();
}

void UInkFrontendWidget::RefreshPage()
{
    if(CurrentPage==TEXT("Character"))RefreshCharacter();
    else if(CurrentPage==TEXT("Inventory"))RefreshInventory();
    else if(CurrentPage==TEXT("Leaderboard"))RefreshLeaderboard();
    else if(CurrentPage==TEXT("Saves"))RefreshSaves();
    else if(CurrentPage==TEXT("Settings"))RefreshSettings();
    if(auto* Continue=InkFrontend::Find<UInkFrontendActionButton>(this,TEXT("Main_ContinueGame")))
    { auto* Save=InkFrontend::Saves(this);Continue->SetIsEnabled(Save&&Save->FindLatestSlot()>=0); }
    if(auto* State=InkFrontend::Find<UTextBlock>(this,TEXT("MainContinueState")))
    {
        const auto* Save=InkFrontend::Saves(this);
        State->SetText(Save&&Save->FindLatestSlot()>=0?LOCTEXT("ChooseContinueSlot","继续游戏 · 选择已保存的行程"):LOCTEXT("ContinueNoSlot","暂无存档 · 入世后可保存行程"));
        State->SetColorAndOpacity(InkPresentationStyle::Paper());State->SetJustification(ETextJustify::Center);
    }
    RefreshLiveStatus();BindTree();ApplyProvidedArt();ApplyPageSafePadding();
}

void UInkFrontendWidget::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{
    Super::NativeTick(Geometry,DeltaSeconds);StatusRefreshTime+=DeltaSeconds;
    UpdateAdaptiveSurfaceLayout();UpdateAdaptiveHudLayout();UpdateInventoryAdaptiveLayout();UpdateLeaderboardAdaptiveLayout();UpdateGameplayLeaderboard(DeltaSeconds);
    LeaderboardAvatarPoll-=DeltaSeconds;if(LeaderboardAvatarPoll<=0){LeaderboardAvatarPoll=.3f;UpdateLeaderboardAvatars();}
    if(CurrentPage==TEXT("Inventory")){if(bInventoryModelDirty)RefreshInventory();UpdateInventoryTooltip();}
    if(CurrentPage==TEXT("Main"))UpdateAdaptiveMainLayout();
    if(!PendingConfirmationAction.IsNone())UpdateConfirmationLayout();
    if(StatusRefreshTime>=.35f){StatusRefreshTime=0;RefreshLiveStatus();UpdateRewardReceipts();}
}

void UInkFrontendWidget::UpdateConfirmationLayout()
{
    auto* Canvas=GetWidgetFromName(TEXT("SafeCanvas"));auto* Paper=InkFrontend::Find<UBorder>(this,TEXT("ConfirmationPaper"));
    auto* Layout=Paper?Cast<UCanvasPanelSlot>(Paper->Slot):nullptr;if(!Canvas||!Layout)return;
    const FVector2D Size=Canvas->GetCachedGeometry().GetLocalSize();if(Size.X<=0||Size.Y<=0)return;
    const float Width=FMath::Min(Size.X*.84f,640.f),Height=FMath::Min(Size.Y*.88f,640.f);
    Layout->SetAnchors(FAnchors(0));Layout->SetAlignment(FVector2D::ZeroVector);
    Layout->SetOffsets(FMargin((Size.X-Width)*.5f,(Size.Y-Height)*.5f,Width,Height));
    Paper->SetPadding(FMargin(Width*.10f,Height*.22f,Width*.10f,Height*.12f));
    for(const TCHAR* Name:{TEXT("ConfirmPending"),TEXT("CancelPending")})
        if(auto* Button=InkFrontend::Find<UInkFrontendActionButton>(this,Name))
        {
            if(auto* Row=Cast<UVerticalBoxSlot>(Button->Slot))Row->SetHorizontalAlignment(HAlign_Center);
            if(auto* Sizing=Cast<USizeBox>(Button->GetContent()))
            {Sizing->SetMinDesiredHeight(0);Sizing->SetWidthOverride(240);Sizing->SetHeightOverride(84);}
        }
}

void UInkFrontendWidget::UpdateAdaptiveSurfaceLayout()
{
    if(!bAdaptiveSurfaceLayout||!WidgetTree)return;
    auto* Canvas=GetWidgetFromName(TEXT("SafeCanvas"));if(!Canvas)return;
    FVector2D Size=Canvas->GetCachedGeometry().GetLocalSize();
    if(Size.X<=0||Size.Y<=0){if(!IsDesignTime())return;Size=FVector2D(720,1280);}
    const bool bLandscape=Size.X>Size.Y*1.12f;
    const float Width=FMath::Min(Size.X-40.f,bLandscape?900.f:720.f),Height=FMath::Min(Size.Y-40.f,1280.f);
    if(auto* Attributes=InkFrontend::Find<UUniformGridPanel>(this,TEXT("CharacterAttributeList")))
        for(int32 I=0;I<Attributes->GetChildrenCount();++I)if(auto* Cell=Cast<UUniformGridSlot>(Attributes->GetChildAt(I)->Slot))
        {const int32 Columns=bLandscape?2:1;Cell->SetRow(I/Columns);Cell->SetColumn(I%Columns);}
    for(const TCHAR* Page:{TEXT("Saves"),TEXT("Settings"),TEXT("Character"),TEXT("Leaderboard")})
    {
        const FString Prefix(Page);
        auto* Paper=InkFrontend::Find<UBorder>(this,*(Prefix+TEXT("Paper")));if(!Paper)continue;
        if(Prefix==TEXT("Saves")||Prefix==TEXT("Settings"))
        {
            // These two routes reflow independently of the three preserved surfaces below.
            const bool Wide=InkUiLayout::IsLandscape(Size);
            const float FrameWidth=FMath::Min(Size.X-48.f,Wide?1100.f:720.f);
            if(auto* PresentationSlot=Cast<UCanvasPanelSlot>(Paper->Slot))
            {PresentationSlot->SetAnchors(FAnchors(0));PresentationSlot->SetAlignment(FVector2D(0));PresentationSlot->SetOffsets(FMargin((Size.X-FrameWidth)*.5f,24,FrameWidth,Size.Y-48));}
            Paper->SetPadding(FMargin(24));
            if(auto* Body=Cast<UVerticalBox>(Paper->GetContent()))
            {
                if(auto* Heading=Cast<UTextBlock>(Body->GetChildAt(0)))Heading->SetFont(InkUiTypography::MakeFont(EInkUiTextRole::Display,36));
                if(auto* Intro=Cast<UTextBlock>(Body->GetChildAt(1)))Intro->SetVisibility(Size.Y<760?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
            }
            const TCHAR* GridName=Prefix==TEXT("Saves")?TEXT("SavePresentationGrid"):TEXT("SettingsPresentationGrid");
            if(auto* Grid=InkFrontend::Find<UGridPanel>(this,GridName))
                for(int32 I=0;I<Grid->GetChildrenCount();++I)if(auto* Cell=Cast<UGridSlot>(Grid->GetChildAt(I)->Slot))
                {Cell->SetRow(Wide?0:I);Cell->SetColumn(Wide?I:0);Cell->SetVerticalAlignment(VAlign_Top);Cell->SetPadding(FMargin(8));Grid->SetColumnFill(0,1);Grid->SetColumnFill(1,Wide&&Prefix!=TEXT("Settings")?1:0);}
            if(Prefix==TEXT("Settings"))
            {
                if(auto* Sections=InkFrontend::Find<UGridPanel>(this,TEXT("SettingsSectionsLayout")))
                {
                    Sections->SetColumnFill(0,1);Sections->SetColumnFill(1,Wide?1:0);
                    for(int32 I=0;I<Sections->GetChildrenCount();++I)if(auto* Cell=Cast<UGridSlot>(Sections->GetChildAt(I)->Slot))
                    {Cell->SetRow(Wide?I/2:I);Cell->SetColumn(Wide?I%2:0);Cell->SetColumnSpan(Wide&&I==2?2:1);if(Wide&&I==2)Cell->SetColumn(0);}
                }
                for(const TCHAR* Name:{TEXT("SettingsQualityOptions"),TEXT("SettingsFrameOptions")})if(auto* Options=InkFrontend::Find<UUniformGridPanel>(this,Name))
                {
                    const float OptionWidth=Wide?(FrameWidth-96)/2:FrameWidth-80;
                    const int32 Columns=OptionWidth>=420?3:2;
                    for(int32 I=0;I<Options->GetChildrenCount();++I)if(auto* Cell=Cast<UUniformGridSlot>(Options->GetChildAt(I)->Slot))
                    {Cell->SetRow(I/Columns);Cell->SetColumn(I%Columns);}
                }
            }
            if(auto* Feedback=InkFrontend::Find<UTextBlock>(this,*(Prefix+TEXT("Feedback"))))Feedback->SetVisibility(Feedback->GetText().IsEmpty()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
            for(const FString Name:{Prefix+TEXT("Back"),FString(TEXT("ApplySettings"))})if(auto* Button=InkFrontend::Find<UInkFrontendActionButton>(this,*Name))
                if(auto* Sizing=Cast<USizeBox>(Button->GetContent())){Sizing->ClearWidthOverride();Sizing->SetMinDesiredHeight(88);Sizing->SetHeightOverride(88);}
            continue;
        }
        if(auto* Layout=Cast<UCanvasPanelSlot>(Paper->Slot))
        {
            Layout->SetAnchors(FAnchors(0));Layout->SetAlignment(FVector2D::ZeroVector);
            Layout->SetOffsets(FMargin((Size.X-Width)*.5f,(Size.Y-Height)*.5f,Width,Height));
        }
        if(bUsePageSafePadding)Paper->SetPadding(bLandscape?FMargin(56,76,56,56):PageSafePadding);
        if(auto* Body=Cast<UVerticalBox>(Paper->GetContent()))
        {
            // Headers and footer have explicit budgets; the scroll viewport gets
            // the remaining height, rather than losing it to portrait spacing.
            if(auto* Header=Cast<USizeBox>(Body->GetChildAt(0)))
                Header->SetHeightOverride(bLandscape?64.f:(Prefix==TEXT("Leaderboard")?130.f:100.f));
            if(auto* Heading=Cast<UTextBlock>(Body->GetChildAt(0)))
            {auto Font=Heading->GetFont();Font.Size=bLandscape?34:44;Heading->SetFont(Font);}
            if(auto* Intro=Cast<UTextBlock>(Body->GetChildAt(1)))
            {auto Font=Intro->GetFont();Font.Size=bLandscape?22:24;Intro->SetFont(Font);}
            for(int32 I=0;I<FMath::Min(3,Body->GetChildrenCount());++I)
                if(auto* Row=Cast<UVerticalBoxSlot>(Body->GetChildAt(I)->Slot))Row->SetPadding(FMargin(0,bLandscape?4:8));
        }
        if(auto* Feedback=InkFrontend::Find<UTextBlock>(this,*(Prefix+TEXT("Feedback"))))
            Feedback->SetVisibility(Feedback->GetText().IsEmpty()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
        if(auto* Back=InkFrontend::Find<UInkFrontendActionButton>(this,*(Prefix+TEXT("Back"))))
        {
            if(auto* Row=Cast<UVerticalBoxSlot>(Back->Slot))Row->SetHorizontalAlignment(HAlign_Center);
            if(auto* Sizing=Cast<USizeBox>(Back->GetContent()))
            {Sizing->SetMinDesiredHeight(0);Sizing->SetWidthOverride(240);Sizing->SetHeightOverride(72);}
        }
        if(auto* Scroll=InkFrontend::Find<UScrollBox>(this,*(Prefix+TEXT("Scroll"))))
        {
            // Reset only when the effective frame size changes; normal scrolling
            // and data refreshes retain their position.
            const FIntPoint NewSize(FMath::RoundToInt(Width),FMath::RoundToInt(Height));
            auto* Previous=LastPageFrameSizes.Find(FName(Page));
            if(!Previous||*Previous!=NewSize){Scroll->SetScrollOffset(0);LastPageFrameSizes.Add(FName(Page),NewSize);}
        }
    }
    if(!GetWidgetFromName(TEXT("InventoryShell")))if(auto* Grid=InkFrontend::Find<UUniformGridPanel>(this,TEXT("InventoryItemList")))
    {
        if(auto* Row=Cast<UVerticalBoxSlot>(Grid->Slot))Row->SetHorizontalAlignment(HAlign_Center);
        const int32 Columns=bLandscape?6:4;
        for(int32 I=0;I<Grid->GetChildrenCount();++I)if(auto* Cell=Cast<UUniformGridSlot>(Grid->GetChildAt(I)->Slot))
        {Cell->SetRow(I/Columns);Cell->SetColumn(I%Columns);}
    }
    if(!GetWidgetFromName(TEXT("InventoryShell")))if(auto* Count=InkFrontend::Find<UTextBlock>(this,TEXT("InventoryCount")))
    {
        auto Font=Count->GetFont();Font.Size=bLandscape?22:24;Count->SetFont(Font);
        if(auto* Row=Cast<UVerticalBoxSlot>(Count->Slot))Row->SetPadding(FMargin(0,bLandscape?0:8));
    }
    if(auto* List=InkFrontend::Find<UVerticalBox>(this,TEXT("LeaderboardList")))if(auto* Outer=Cast<UBorder>(List->GetChildAt(0)))
        if(auto* Inner=Cast<UBorder>(Outer->GetContent()))if(auto* Empty=Cast<USizeBox>(Inner->GetContent()))Empty->SetMinDesiredHeight(bLandscape?180.f:230.f);
    for(auto Button:ActionButtons)if(Button)
    {
        if(auto* Shell=GetWidgetFromName(TEXT("RankAnchorShell"));Shell&&Button->IsChildOf(Shell))continue;
        const FString Action=Button->Action.ToString();
        if(Button->GetName().StartsWith(TEXT("HUD_"))||Button->Action==TEXT("HUDRankView"))continue;
        if(Action.StartsWith(TEXT("Quality"))||Action.StartsWith(TEXT("Frame"))||Action.StartsWith(TEXT("SelectSave_"))||Action.StartsWith(TEXT("SaveSlot_"))||Action.StartsWith(TEXT("LoadSlot_"))||Action.StartsWith(TEXT("DeleteSlot_"))||Action==TEXT("ToggleReducedMotion"))
        {
            if(auto* Sizing=Cast<USizeBox>(Button->GetContent())){Sizing->ClearWidthOverride();Sizing->SetMinDesiredHeight(88);Sizing->SetHeightOverride(88);}
            if(auto* PresentationSlot=Cast<UVerticalBoxSlot>(Button->Slot))PresentationSlot->SetHorizontalAlignment(HAlign_Fill);
            continue;
        }
        const bool bTab=Action.StartsWith(TEXT("Filter"))||Action.StartsWith(TEXT("Rank"));
        const bool bSetting=Action.StartsWith(TEXT("Quality"))||Action.StartsWith(TEXT("Frame"));
        const bool bSave=Action.StartsWith(TEXT("SaveSlot_"))||Action.StartsWith(TEXT("LoadSlot_"))||Action.StartsWith(TEXT("DeleteSlot_"));
        auto* Sizing=Cast<USizeBox>(Button->GetContent());if(!Sizing)continue;
        if(bTab||bSetting||bSave)
        {
            auto Style=Button->GetStyle();Style.SetNormalPadding(FMargin(0));Style.SetPressedPadding(FMargin(0));Button->SetStyle(Style);
            float ButtonWidth=bTab?(Action==TEXT("RankCultivation")?208.f:(Action.StartsWith(TEXT("Rank"))?128.f:112.f)):(bSave?140.f:(Action.StartsWith(TEXT("Frame"))?180.f:148.f));
            Sizing->SetWidthOverride(ButtonWidth);Sizing->SetMinDesiredHeight(0);Sizing->SetHeightOverride(bLandscape?(bTab?60.f:56.f):64.f);
            if(bTab)if(auto* Label=Cast<UTextBlock>(Sizing->GetContent())){auto Font=Label->GetFont();Font.Size=24;Label->SetFont(Font);}
            if(auto* Row=Cast<UHorizontalBoxSlot>(Button->Slot))
            {Row->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));if(bTab)Row->SetPadding(FMargin(2,bLandscape?2:4));}
            if(auto* Row=Cast<UVerticalBoxSlot>(Button->GetParent()?Button->GetParent()->Slot:nullptr))Row->SetHorizontalAlignment(HAlign_Center);
        }
    }
}

void UInkFrontendWidget::UpdateAdaptiveMainLayout()
{
    if(IsDesignTime())return;
    auto* Menu=InkFrontend::Find<UScrollBox>(this,TEXT("MainMenuScroll"));
    auto* Body=InkFrontend::Find<UVerticalBox>(this,TEXT("MainMenuButtons"));
    auto* Canvas=GetWidgetFromName(TEXT("SafeCanvas"));
    auto* MenuLayout=Menu?Cast<UCanvasPanelSlot>(Menu->Slot):nullptr;if(!MenuLayout||!Canvas||!Body)return;
    if(!bAdaptiveMainLayout)
    {
        if(bMainMenuLayoutAdapted&&bMainMenuLayoutCached)MenuLayout->SetLayout(AuthoredMainMenuLayout);
        bMainMenuLayoutAdapted=false;LastMainLayoutRatio=-1;return;
    }
    const FVector2D SafeSize=Canvas->GetCachedGeometry().GetLocalSize();if(SafeSize.X<=0||SafeSize.Y<=0)return;
    if(!GetWidgetFromName(TEXT("MainSecondaryActions")))
    {
        // Upgrade saved WBP trees as well as freshly generated trees.
        auto* Secondary=WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("MainSecondaryActions"));
        for(const TCHAR* Name:{TEXT("Main_OpenSettings"),TEXT("Main_RequestQuit")})
            if(auto* Button=GetWidgetFromName(Name))
            {
                Button->RemoveFromParent();auto* PresentationSlot=Secondary->AddChildToHorizontalBox(Button);
                PresentationSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));PresentationSlot->SetPadding(FMargin(4,0));
            }
        Body->AddChildToVerticalBox(Secondary)->SetPadding(FMargin(0,8));
        AddText(WidgetTree,Body,FText::GetEmpty(),20,TEXT("MainContinueState"));
        RefreshPage();
    }
    if(!bMainMenuLayoutCached){AuthoredMainMenuLayout=MenuLayout->GetLayout();bMainMenuLayoutCached=true;}
    const float Ratio=SafeSize.X/SafeSize.Y;
    const bool bLandscape=InkUiLayout::IsLandscape(SafeSize);
    const float Width=FMath::Min(SafeSize.X-48.f,bLandscape?400.f:480.f);
    const float Height=FMath::Min(SafeSize.Y-48.f,360.f);
    const float X=bLandscape?SafeSize.X-24-Width:(SafeSize.X-Width)*.5f;
    const float Y=bLandscape?(SafeSize.Y-Height)*.5f:SafeSize.Y-24-Height;
    FAnchorData ResponsiveLayout=AuthoredMainMenuLayout;
    ResponsiveLayout.Anchors=FAnchors(0);ResponsiveLayout.Alignment=FVector2D::ZeroVector;
    ResponsiveLayout.Offsets=FMargin(X,Y,Width,Height);
    for(auto Button:ActionButtons)if(Button&&Button->GetName().StartsWith(TEXT("Main_")))
    {
        const bool Primary=Button->Action==TEXT("StartGame")||Button->Action==TEXT("OpenSaves");
        if(auto* Sizing=Cast<USizeBox>(Button->GetContent())){Sizing->SetMinDesiredHeight(0);Sizing->SetHeightOverride(Primary?96:88);}
        if(auto* Row=Cast<UVerticalBoxSlot>(Button->Slot))Row->SetPadding(FMargin(0,4));
    }
    MenuLayout->SetLayout(ResponsiveLayout);bMainMenuLayoutAdapted=true;LastMainLayoutRatio=Ratio;
    if(auto* Title=GetWidgetFromName(TEXT("MainTitleScale")))if(auto* PresentationSlot=Cast<UCanvasPanelSlot>(Title->Slot))
    {
        PresentationSlot->SetAnchors(bLandscape?FAnchors(.08f,.02f,.56f,.32f):FAnchors(.12f,.02f,.88f,.21f));PresentationSlot->SetOffsets(FMargin(0));
    }
}

void UInkFrontendWidget::ApplyProvidedArt()
{
    if(!WidgetTree)return;
    // Older saved WBP trees retain the direct-resume row. The supplied menu has
    // one Continue entry, routed to the existing save selection page.
    if(auto* OldContinue=GetWidgetFromName(TEXT("Main_ContinueGame")))OldContinue->SetVisibility(ESlateVisibility::Collapsed);
    if(auto* Status=GetWidgetFromName(TEXT("HUDStatusPaper")))Status->SetVisibility(ESlateVisibility::Collapsed);
    if(auto* Image=InkFrontend::Find<UImage>(this,TEXT("CharacterMeditationImage")))
    {
        // A WBP saved before asynchronous texture compilation may serialize a
        // zero desired size. Give ScaleToFit a stable square source geometry.
        auto Brush=Image->GetBrush();Brush.ImageSize=FVector2D(1024,1024);Image->SetBrush(Brush);
    }
    if(auto* Image=InkFrontend::Find<UImage>(this,TEXT("MainTitleImage")))if(auto* Material=TitleFeatherMaterial.LoadSynchronous())
    {
        auto Brush=Image->GetBrush();Brush.SetResourceObject(Material);Brush.DrawAs=ESlateBrushDrawType::Image;
        Brush.ImageSize=FVector2D(1672,941);Image->SetBrush(Brush);
    }
    for(const TCHAR* Name:{TEXT("LeaderboardPaper"),TEXT("CharacterPaper"),TEXT("SavesPaper"),TEXT("SettingsPaper"),TEXT("ConfirmationPaper")})
        if(auto* Paper=InkFrontend::Find<UBorder>(this,Name))Paper->SetBrush(InkPresentationStyle::Surface(InkPresentationStyle::ESurface::Card,FVector2D(720,1100)));
    if(auto* Heading=InkFrontend::Find<UImage>(this,TEXT("InventoryArtHeading")))if(auto* Texture=ProvidedInventoryAtlas.LoadSynchronous())
        Heading->SetBrush(InkFrontend::AtlasBrush(Texture,FVector4(14,0,683,131)));
    if(auto* Dialog=InkFrontend::Find<UBorder>(this,TEXT("ConfirmationPaper")))Dialog->SetPadding(FMargin(44,54));
    for(auto Button:ActionButtons)if(Button&&!Button->Action.ToString().StartsWith(TEXT("Item_")))
    {
        if(Button->Action.ToString().StartsWith(TEXT("Inventory"))||Button->Action.ToString().StartsWith(TEXT("Filter"))||Button->Action.ToString().StartsWith(TEXT("Ext_"))||Button->Action==TEXT("RankClose"))continue;
        if(Button->Action.ToString().StartsWith(TEXT("SelectSave_")))continue;
        if(auto* Shell=GetWidgetFromName(TEXT("RankAnchorShell"));Shell&&Button->IsChildOf(Shell))continue;
        if(Button->GetName().StartsWith(TEXT("HUD_"))||Button->Action==TEXT("HUDRankView"))continue;
        const bool bBaked=Button->GetName().StartsWith(TEXT("Main_"));
        const bool Primary=Button->Action==TEXT("StartGame")||Button->Action==TEXT("ContinueGame")||(bBaked&&Button->Action==TEXT("OpenSaves"))||Button->Action.ToString().StartsWith(TEXT("LoadSlot_"))||Button->Action==TEXT("ApplySettings");
        auto Style=InkPresentationStyle::Button(false,Primary);Style.SetNormalPadding(FMargin(0));Style.SetPressedPadding(FMargin(0));Button->SetStyle(Style);
        if(auto* Sizing=Cast<USizeBox>(Button->GetContent()))
        {
            if(auto* ContentLayout=Cast<UButtonSlot>(Sizing->Slot))ContentLayout->SetPadding(FMargin(0));
            if(auto* Label=Cast<UTextBlock>(Sizing->GetContent()))
            {
                Label->SetAutoWrapText(false);Label->SetRenderOpacity(1.f);
                if(bBaked&&Button->Action==TEXT("OpenSaves"))Label->SetText(LOCTEXT("ContinueMenuSaves","继续游戏"));
                if(bBaked&&Button->Action==TEXT("RequestQuit"))Label->SetText(LOCTEXT("MenuExit","退出"));
                Label->SetColorAndOpacity(Primary?InkPresentationStyle::Paper():InkPresentationStyle::Ink());
                if(Button->Action.ToString().StartsWith(TEXT("DeleteSlot_")))Label->SetColorAndOpacity(CinnabarColor);
                Label->SetFont(InkPresentationStyle::ButtonFont(Label->GetFont(),Primary));
                if(Button->GetName().StartsWith(TEXT("HUD_"))){auto Font=Label->GetFont();Font.Size=24;Label->SetFont(Font);}
            }
        }
    }
}

void UInkFrontendWidget::ApplyPageSafePadding()
{
    if(!bUsePageSafePadding||!WidgetTree)return;
    // Only these named paper frames participate; Main/HUD and their Designer layout stay untouched.
    const TCHAR* Frames[]={TEXT("SavesPaper"),TEXT("SettingsPaper"),TEXT("CharacterPaper"),TEXT("LeaderboardPaper")};
    for(const TCHAR* Frame:Frames)
        if(auto* Paper=InkFrontend::Find<UBorder>(this,Frame))Paper->SetPadding(PageSafePadding);
    // Compact category tabs must keep Chinese labels on one line inside the portrait paper.
    for(auto Button:ActionButtons)if(Button&&Button->Action.ToString().StartsWith(TEXT("Rank")))
    {
        if(auto* Shell=GetWidgetFromName(TEXT("RankAnchorShell"));Shell&&Button->IsChildOf(Shell))continue;
        if(auto* Sizing=Cast<USizeBox>(Button->GetContent()))if(auto* Label=Cast<UTextBlock>(Sizing->GetContent()))
        {
            auto Font=Label->GetFont();Font.Size=28;Label->SetFont(Font);Label->SetAutoWrapText(false);
            Sizing->SetMinDesiredHeight(0);Sizing->SetHeightOverride(64);
            auto Style=Button->GetStyle();Style.SetNormalPadding(FMargin(8,6));Style.SetPressedPadding(FMargin(8,8,8,4));Button->SetStyle(Style);
            if(auto* ContentLayout=Cast<UButtonSlot>(Sizing->Slot))ContentLayout->SetPadding(FMargin(0));
            if(auto* TabLayout=Cast<UHorizontalBoxSlot>(Button->Slot))
            {
                FSlateChildSize Weight(ESlateSizeRule::Fill);Weight.Value=Button->Action==TEXT("RankCultivation")?1.8f:1.f;
                TabLayout->SetSize(Weight);TabLayout->SetPadding(FMargin(2,4));
            }
        }
    }
}

FReply UInkFrontendWidget::NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& KeyEvent)
{
    if(KeyEvent.GetKey()==EKeys::Escape||KeyEvent.GetKey()==EKeys::Virtual_Gamepad_Back.GetVirtualKey())
    {
        if(RewardReceiptPanel)RewardReceiptPanel->CloseReceipt();
        else if(!PendingConfirmationAction.IsNone())SetQuitConfirmation(false);
        else if(auto* Menu=GetWidgetFromName(TEXT("HUDRuntimeMenu"));CurrentPage==TEXT("HUD")&&Menu&&Menu->GetVisibility()!=ESlateVisibility::Collapsed)ExecuteAction(TEXT("ToggleHUDMenu"));
        else if(auto* Page=ExtensionPages.FindRef(CurrentPage).Get())Page->HandleAction(Page->SelectedEntry.IsEmpty()?FName(TEXT("Back")):FName(TEXT("List")));
        else if(CurrentPage!=TEXT("Main"))TriggerAction(TEXT("Back"));
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(Geometry,KeyEvent);
}

void UInkFrontendWidget::RefreshLiveStatus()
{
    auto* Pawn=Director&&Director->bMenuActive?Director->GetPresentationPawn():GetOwningPlayerPawn();
    FText StateText=LOCTEXT("AwaitCharacter","等待角色");
    if(Pawn)
    {
        if(auto* Cultivation=Pawn->FindComponentByClass<UCultivationGameplayComponent>())StateText=InkFrontend::ActionStateText(Cultivation->State);
        else StateText=LOCTEXT("CurrentLocation","行于天地间");
    }
    if(auto* HUD=InkFrontend::Find<UTextBlock>(this,TEXT("HUDLiveStatus")))HUD->SetText(StateText);
    if(auto* Label=InkFrontend::Find<UTextBlock>(this,TEXT("HUDCultivationLabel")))
    {
        const auto* Ability=Pawn?Pawn->FindComponentByClass<UCultivationGameplayComponent>():nullptr;
        const bool Meditating=Ability&&(Ability->State==ECultivationActionState::MeditationStart||Ability->State==ECultivationActionState::MeditationLoop);
        Label->SetText(FText::FromString(Meditating?TEXT("收功"):TEXT("修炼")));
    }
    if(auto* Character=InkFrontend::Find<UTextBlock>(this,TEXT("CharacterLiveStatus")))
    {
        if(Pawn)Character->SetText(FText::Format(LOCTEXT("CharacterStatus","状态：{0}"),StateText));
        else Character->SetText(StateText);
    }
}

void UInkFrontendWidget::SetCharacterData(const FText& Name,const TArray<FInkCharacterAttribute>& Attributes,const TArray<FInkDisciplineEntry>& Disciplines)
{
    CharacterName=Name;CharacterAttributes=Attributes;CharacterDisciplines=Disciplines;if(CurrentPage==TEXT("Character"))RefreshCharacter();
    TArray<TObjectPtr<UInkExtensionWidget>> LivePanels;ExtensionPages.GenerateValueArray(LivePanels);
    for(const auto& Item:LivePanels)if(auto* Panel=Cast<UInkFeaturePanel>(Item))Panel->RefreshLiveData();
}

void UInkFrontendWidget::SetLeaderboardEntries(const TArray<FInkLeaderboardEntry>& Entries)
{ SetLeaderboardEntriesForScope(Entries,TEXT("Local")); }

void UInkFrontendWidget::SetLeaderboardEntriesForScope(const TArray<FInkLeaderboardEntry>& Entries,FName Scope)
{if(Scope!=TEXT("Global")&&Scope!=TEXT("Local"))return;auto& Target=Scope==TEXT("Global")?GlobalLeaderboardEntries:LeaderboardEntries;if(InkLeaderboard::SameEntries(Target,Entries))return;Target=Entries;RefreshLeaderboard();}
void UInkFrontendWidget::SetGlobalLeaderboardConnection(bool bConnected,const FText& Status)
{if(bGlobalLeaderboardConnected==bConnected&&GlobalLeaderboardStatus.EqualTo(Status))return;bGlobalLeaderboardConnected=bConnected;GlobalLeaderboardStatus=Status;if(!bConnected)GlobalLeaderboardEntries.Reset();RefreshLeaderboard();}

void UInkFrontendWidget::RefreshCharacter(UWidgetTree* SourceTree)
{
    UWidgetTree* Tree=SourceTree?SourceTree:WidgetTree.Get();if(!Tree)return;
    if(!SourceTree&&!Tree->FindWidget(TEXT("CharacterIdentityRow")))
    {
        auto* Name=InkFrontend::Find<UTextBlock>(Tree,TEXT("CharacterName"));
        auto* Portrait=InkFrontend::Find<USizeBox>(Tree,TEXT("CharacterMeditationSize"));
        auto* Status=InkFrontend::Find<UTextBlock>(Tree,TEXT("CharacterLiveStatus"));
        if(auto* Parent=Name?Cast<UVerticalBox>(Name->GetParent()):nullptr;Parent&&Portrait&&Status)
        {
            auto* Identity=Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("CharacterIdentityRow"));
            // Re-add the retained children so the live Slate slots are rebuilt.
            // UPanelWidget::ReplaceChild alone does not update existing Slate slots.
            const auto Existing=Parent->GetAllChildren();
            Parent->ClearChildren();
            {
                Portrait->SetWidthOverride(96);Portrait->SetHeightOverride(96);
                Identity->AddChildToHorizontalBox(Portrait)->SetPadding(FMargin(0,0,14,0));
                auto* Labels=Tree->ConstructWidget<UVerticalBox>();
                auto* Layout=Identity->AddChildToHorizontalBox(Labels);Layout->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Layout->SetVerticalAlignment(VAlign_Center);
                Labels->AddChildToVerticalBox(Name);Labels->AddChildToVerticalBox(Status);
                auto Font=Name->GetFont();Font.Size=28;Name->SetFont(Font);
                Font=Status->GetFont();Font.Size=18;Status->SetFont(Font);
                for(auto* Child:Existing)
                {
                    if(Child==Name)Parent->AddChildToVerticalBox(Identity)->SetPadding(FMargin(0,4,0,8));
                    else if(Child!=Portrait&&Child!=Status)Parent->AddChildToVerticalBox(Child);
                }
            }
        }
    }
    if(!SourceTree&&!GetWidgetFromName(TEXT("CharacterBreakthrough")))
        if(auto* Box=Cast<UVerticalBox>(GetWidgetFromName(TEXT("CharacterContent"))))
        {
            auto* B=MakeButton(Tree,TEXT("CharacterBreakthrough"),TEXT("AnchorBreakthrough"),FText::FromString(TEXT("消耗材料 · 突破境界")));Box->AddChildToVerticalBox(B);B->BindFrontend(this);
        }
    if(auto* Name=InkFrontend::Find<UTextBlock>(Tree,TEXT("CharacterName")))
    {
        Name->SetText(CharacterName.IsEmpty()?LOCTEXT("CurrentCharacter","当前角色"):CharacterName);
        Name->SetColorAndOpacity(InkPresentationStyle::Gold());Name->SetJustification(ETextJustify::Left);
    }
    if(auto* Hint=InkFrontend::Find<UTextBlock>(Tree,TEXT("CharacterAttributeHint")))Hint->SetVisibility(CharacterAttributes.IsEmpty()?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
    if(auto* Grid=InkFrontend::Find<UUniformGridPanel>(Tree,TEXT("CharacterAttributeList")))
    {
        TArray<UWidget*> Old;TFunction<void(UWidget*)> Collect=[&](UWidget* W){if(auto* P=Cast<UPanelWidget>(W))for(auto* C:P->GetAllChildren()){Collect(C);Old.Add(C);}};
        Collect(Grid);Grid->ClearChildren();for(auto* W:Old)W->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);
        TArray<FInkCharacterAttribute> Attributes=CharacterAttributes;
        const auto* Safe=Tree->FindWidget(TEXT("SafeCanvas"));const auto SafeSize=Safe?Safe->GetCachedGeometry().GetLocalSize():FVector2D(720,1280);
        const int32 Columns=SafeSize.X>SafeSize.Y*1.12f?2:1;
        if(Attributes.IsEmpty())
        {
            const FText Labels[]={LOCTEXT("Health","血量"),LOCTEXT("Attack","攻击"),LOCTEXT("Defense","防御"),LOCTEXT("SpiritDamage","剑灵增伤"),LOCTEXT("Critical","会心一击"),LOCTEXT("Evasion","闪避"),LOCTEXT("Luck","幸运"),LOCTEXT("MoveSpeed","移速"),LOCTEXT("Swordsmanship","剑法"),LOCTEXT("CultivationSpeedAttribute","修炼速度"),LOCTEXT("Realm","境界")};
            for(int32 I=0;I<UE_ARRAY_COUNT(Labels);++I){FInkCharacterAttribute Attribute;Attribute.AttributeId=InkFrontend::AttributeIds[I];Attribute.Label=Labels[I];Attribute.Value=LOCTEXT("UnknownValue","—");Attributes.Add(Attribute);}
        }
        for(int32 I=0;I<Attributes.Num();++I)
        {
            auto* Row=Tree->ConstructWidget<UHorizontalBox>();
            int32 IconIndex=INDEX_NONE;
            for(int32 J=0;J<UE_ARRAY_COUNT(InkFrontend::AttributeIds);++J)if(Attributes[I].AttributeId==InkFrontend::AttributeIds[J]){IconIndex=J;break;}
            auto* CustomIcon=Attributes[I].Icon.LoadSynchronous();
            auto* IconTexture=CustomIcon?CustomIcon:(IconIndex!=INDEX_NONE?AttributeIconAtlas.LoadSynchronous():nullptr);
            using EIcon=InkPresentationStyle::EIcon;
            const EIcon SharedIcons[]={EIcon::Health,EIcon::Sword,EIcon::Shield,EIcon::Action,EIcon::Critical,EIcon::Movement,EIcon::Jade,EIcon::Movement,EIcon::Sword,EIcon::Cultivation,EIcon::Experience};
            FSlateBrush IconBrush;
            const bool bStockAtlas=AttributeIconAtlas.ToSoftObjectPath().GetAssetName()==TEXT("T_InkAttributeIcons");
            if(!CustomIcon&&(bStockAtlas||!IconTexture)&&IconIndex!=INDEX_NONE)IconBrush=InkPresentationStyle::Icon(SharedIcons[IconIndex],FVector2D(42,42));
            if(!IconBrush.GetResourceObject()&&IconTexture)
            {
                IconBrush.SetResourceObject(IconTexture);IconBrush.DrawAs=ESlateBrushDrawType::Image;IconBrush.ImageSize=FVector2D(42,42);
                if(!CustomIcon)IconBrush.SetUVRegion(FBox2f(FVector2f((IconIndex%4)/4.f,(IconIndex/4)/3.f),FVector2f((IconIndex%4+1)/4.f,(IconIndex/4+1)/3.f)));
            }
            if(IconBrush.GetResourceObject())
            {
                auto* Icon=Tree->ConstructWidget<UImage>(UImage::StaticClass(),MakeUniqueObjectName(Tree,UImage::StaticClass(),FName(*FString::Printf(TEXT("AttributeIcon_%d"),I))));
                Icon->SetBrush(IconBrush);Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
                auto* IconSize=Tree->ConstructWidget<USizeBox>();IconSize->SetWidthOverride(42);IconSize->SetHeightOverride(42);IconSize->SetContent(Icon);
                auto* IconLayout=Row->AddChildToHorizontalBox(IconSize);IconLayout->SetPadding(FMargin(0,0,12,0));IconLayout->SetVerticalAlignment(VAlign_Center);
            }
            auto* TextColumn=Tree->ConstructWidget<UVerticalBox>();auto* TextLayout=Row->AddChildToHorizontalBox(TextColumn);TextLayout->SetSize(FSlateChildSize(ESlateSizeRule::Fill));TextLayout->SetVerticalAlignment(VAlign_Center);
            auto* Label=AddText(Tree,TextColumn,Attributes[I].Label,21,FName(*FString::Printf(TEXT("CharacterAttributeLabel_%d"),I)));Label->SetAutoWrapText(false);Label->SetColorAndOpacity(InkPresentationStyle::Gold());
            auto* Value=AddText(Tree,TextColumn,Attributes[I].Value.IsEmpty()?LOCTEXT("UnknownValue","—"):Attributes[I].Value,25,FName(*FString::Printf(TEXT("CharacterAttributeValue_%d"),I)));Value->SetJustification(ETextJustify::Left);Value->SetAutoWrapText(false);Value->SetColorAndOpacity(InkPresentationStyle::Ink());
            if(auto* LabelSlot=Cast<UVerticalBoxSlot>(Label->Slot))LabelSlot->SetPadding(FMargin(0,1));
            if(auto* ValueSlot=Cast<UVerticalBoxSlot>(Value->Slot))ValueSlot->SetPadding(FMargin(0,1));
            auto* Card=Tree->ConstructWidget<UBorder>();Card->SetBrush(InkPresentationStyle::Surface(InkPresentationStyle::ESurface::Card,FVector2D(440,88)));Card->SetPadding(FMargin(16,8));Card->SetContent(Row);
            auto* Height=Tree->ConstructWidget<USizeBox>();Height->SetMinDesiredHeight(88);Height->SetContent(Card);
            auto* CellLayout=Grid->AddChildToUniformGrid(Height,I/Columns,I%Columns);CellLayout->SetHorizontalAlignment(HAlign_Fill);CellLayout->SetVerticalAlignment(VAlign_Fill);
        }
    }
    const TCHAR* Categories[]={TEXT("Inner"),TEXT("Movement"),TEXT("Martial")};
    for(const auto* Category:Categories)if(auto* List=Cast<UVerticalBox>(Tree->FindWidget(FName(*(FString(TEXT("Discipline_"))+Category)))))
    {
        List->ClearChildren();int32 Count=0;
        for(const auto& Entry:CharacterDisciplines)if(Entry.Category==FName(Category))
        {
            auto* Name=AddText(Tree,List,Entry.Name,23);Name->SetJustification(ETextJustify::Center);
            if(!Entry.Detail.IsEmpty()){auto* Detail=AddText(Tree,List,Entry.Detail,20);Detail->SetJustification(ETextJustify::Center);}++Count;
        }
        if(Count==0){auto* Empty=AddText(Tree,List,LOCTEXT("NoDiscipline","尚未获得"),22);Empty->SetJustification(ETextJustify::Center);}
    }
    if(auto* Manuals=InkFrontend::Find<UUniformGridPanel>(Tree,TEXT("Discipline_Manual")))
    {
        Manuals->ClearChildren();TArray<const FInkDisciplineEntry*> Entries;
        for(const auto& Entry:CharacterDisciplines)if(Entry.Category==TEXT("Manual"))Entries.Add(&Entry);
        for(int32 I=0;I<5;++I)
        {
            auto* Body=Tree->ConstructWidget<UVerticalBox>();const auto* Entry=Entries.IsValidIndex(I)?Entries[I]:nullptr;
            auto* Name=AddText(Tree,Body,Entry?Entry->Name:LOCTEXT("Manual","秘籍"),22);Name->SetJustification(ETextJustify::Center);
            auto* Detail=AddText(Tree,Body,Entry?Entry->Detail:LOCTEXT("ManualEmpty","尚未装备"),16);Detail->SetJustification(ETextJustify::Center);
            if(!Entry)Detail->SetAutoWrapText(false);
            auto* Sizing=Tree->ConstructWidget<USizeBox>();Sizing->SetWidthOverride(84);Sizing->SetMinDesiredHeight(136);Sizing->SetContent(MakeInkCard(Tree,Body,FMargin(8),Entry!=nullptr));
            auto* CellLayout=Manuals->AddChildToUniformGrid(Sizing,0,I);CellLayout->SetHorizontalAlignment(HAlign_Fill);CellLayout->SetVerticalAlignment(VAlign_Fill);
        }
    }
    if(!SourceTree)RefreshLiveStatus();
}

void UInkFrontendWidget::RefreshSaves()
{
    auto* List=InkFrontend::Find<UVerticalBox>(this,TEXT("SaveSlotList"));if(!List||!WidgetTree)return;
    int32 Selected=0;
    if(auto* Choice=InkFrontend::Find<UWidgetSwitcher>(this,TEXT("SaveDetailSwitcher")))Selected=Choice->GetActiveWidgetIndex();
    auto* Grid=InkFrontend::Find<UGridPanel>(this,TEXT("SavePresentationGrid"));
    auto* Details=InkFrontend::Find<UWidgetSwitcher>(this,TEXT("SaveDetailSwitcher"));
    if(Grid)Grid->ClearChildren();if(Details)Details->ClearChildren();
    List->ClearChildren();
    auto* Save=InkFrontend::Saves(this);TArray<FInkSaveSlotInfo> Slots;
    if(Save)Slots=Save->GetSlots();
    else for(int32 I=0;I<UInkSaveSubsystem::ManualSlotCount;++I){FInkSaveSlotInfo SaveInfo;SaveInfo.SlotIndex=I;Slots.Add(SaveInfo);}
    Selected=FMath::Clamp(Selected,0,FMath::Max(0,Slots.Num()-1));
    const bool bCanSave=Save&&Director&&!Director->bMenuActive&&GetOwningPlayerPawn();
    if(!Grid)Grid=WidgetTree->ConstructWidget<UGridPanel>(UGridPanel::StaticClass(),TEXT("SavePresentationGrid"));List->AddChildToVerticalBox(Grid);
    auto* Selectors=WidgetTree->ConstructWidget<UVerticalBox>();Grid->AddChildToGrid(Selectors,0,0)->SetHorizontalAlignment(HAlign_Fill);
    if(!Details)Details=WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(),TEXT("SaveDetailSwitcher"));Grid->AddChildToGrid(Details,1,0)->SetHorizontalAlignment(HAlign_Fill);
    for(int32 I=0;I<Slots.Num();++I)
    {
        const auto& Info=Slots[I];
        auto* Select=MakeButton(WidgetTree,NAME_None,FName(*FString::Printf(TEXT("SelectSave_%d"),I)),FText::Format(LOCTEXT("SaveSlotPicker","存档 {0} · {1}"),FText::AsNumber(Info.SlotIndex+1),Info.bValid?LOCTEXT("SaveReady","可读取"):Info.bExists?LOCTEXT("SaveInvalid","不可读取"):LOCTEXT("SaveEmpty","空位")));
        Select->SetStyle(InkPresentationStyle::Button(I==Selected));
        Selectors->AddChildToVerticalBox(Select)->SetPadding(FMargin(0,4));
        auto* Body=WidgetTree->ConstructWidget<UVerticalBox>();
        auto* DetailSlot=CastChecked<UWidgetSwitcherSlot>(Details->AddChild(Body));DetailSlot->SetHorizontalAlignment(HAlign_Fill);DetailSlot->SetVerticalAlignment(VAlign_Top);
        AddText(WidgetTree,Body,FText::Format(LOCTEXT("SaveSlotTitle","存档 {0}"),FText::AsNumber(Info.SlotIndex+1)),32);
        if(!Info.bExists)AddText(WidgetTree,Body,LOCTEXT("EmptySlot","尚未留迹"),24);
        else if(!Info.bValid)AddText(WidgetTree,Body,LOCTEXT("UnavailableSlot","此存档暂不可读取，可删除后重新保存。"),24);
        else
        {
            AddText(WidgetTree,Body,FText::Format(LOCTEXT("SavedDate","上次保存：{0}"),FText::AsDateTime(Info.SavedAt,EDateTimeStyle::Short,EDateTimeStyle::Short)),24);
            AddText(WidgetTree,Body,FText::Format(LOCTEXT("PlayedMinutes","修行时长：{0} 分钟"),FText::AsNumber(FMath::Max(0,FMath::FloorToInt(Info.PlayedSeconds/60.0)))),24);
        }
        auto Add=[&](const TCHAR* Prefix,const FText& Label,bool bEnabled,bool Primary)
        {
            auto* Button=MakeButton(WidgetTree,NAME_None,FName(*FString::Printf(TEXT("%s_%d"),Prefix,Info.SlotIndex)),Label);Button->SetIsEnabled(bEnabled);Button->SetStyle(InkPresentationStyle::Button(false,Primary));
            Body->AddChildToVerticalBox(Button)->SetPadding(FMargin(0,4));return Button;
        };
        Add(TEXT("LoadSlot"),LOCTEXT("LoadSelectedSlot","读取所选存档"),Save&&Info.bValid,true);
        Add(TEXT("SaveSlot"),Info.bExists?LOCTEXT("OverwriteSlot","覆盖此存档"):LOCTEXT("SaveSlot","保存当前行程"),bCanSave,false);
        auto* Delete=Add(TEXT("DeleteSlot"),LOCTEXT("DeleteSlot","删除此存档"),Save&&Info.bExists,false);
        CastChecked<UVerticalBoxSlot>(Delete->Slot)->SetPadding(FMargin(0,24,0,4));
        if(auto* Sizing=Cast<USizeBox>(Delete->GetContent()))if(auto* Label=Cast<UTextBlock>(Sizing->GetContent()))Label->SetColorAndOpacity(CinnabarColor);
    }
    Details->SetActiveWidgetIndex(Selected);
    if(!bCanSave)AddText(WidgetTree,List,LOCTEXT("SaveGameplayOnly","进入游戏后可保存当前行程。"),24);
    BindTree();
}

void UInkFrontendWidget::RefreshSettings()
{
    if(!GetWidgetFromName(TEXT("SettingsPresentationGrid")))
    {
        auto* Scroll=InkFrontend::Find<UScrollBox>(this,TEXT("SettingsScroll"));
        auto* Body=Scroll?Cast<UVerticalBox>(Scroll->GetChildAt(0)):nullptr;
        if(Body)
        {
            // Reparent the original controls so actions and loaded user settings remain authoritative.
            TArray<UWidget*> QualityControls,FrameControls;
            for(const TCHAR* Name:{TEXT("QualityLow"),TEXT("QualityMedium"),TEXT("QualityHigh"),TEXT("QualityValue")})if(auto* W=GetWidgetFromName(Name))QualityControls.Add(W);
            for(const TCHAR* Name:{TEXT("Frame30"),TEXT("Frame60"),TEXT("FrameValue")})if(auto* W=GetWidgetFromName(Name))FrameControls.Add(W);
            auto* Reduced=GetWidgetFromName(TEXT("ToggleReducedMotion"));
            Body->ClearChildren();
            auto* Grid=WidgetTree->ConstructWidget<UGridPanel>(UGridPanel::StaticClass(),TEXT("SettingsPresentationGrid"));Body->AddChildToVerticalBox(Grid);
            auto* Groups=WidgetTree->ConstructWidget<UGridPanel>(UGridPanel::StaticClass(),TEXT("SettingsSectionsLayout"));Grid->AddChildToGrid(Groups,0,0)->SetHorizontalAlignment(HAlign_Fill);
            int32 SectionIndex=0;
            auto Group=[&](FName GridName,const FText& Title,const TArray<UWidget*>& Controls)
            {
                auto* Section=WidgetTree->ConstructWidget<UVerticalBox>();auto* SectionLayout=Groups->AddChildToGrid(Section,SectionIndex++,0);SectionLayout->SetPadding(FMargin(0,0,12,20));SectionLayout->SetHorizontalAlignment(HAlign_Fill);SectionLayout->SetVerticalAlignment(VAlign_Top);
                AddText(WidgetTree,Section,Title,32);
                auto* Options=WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(),GridName);Options->SetSlotPadding(FMargin(4,6));Section->AddChildToVerticalBox(Options)->SetHorizontalAlignment(HAlign_Fill);
                int32 Index=0;for(auto* Control:Controls)
                {
                    Control->RemoveFromParent();
                    if(Cast<UInkFrontendActionButton>(Control)){auto* Cell=Options->AddChildToUniformGrid(Control,0,Index++);Cell->SetHorizontalAlignment(HAlign_Fill);Cell->SetVerticalAlignment(VAlign_Fill);}
                    else Section->AddChildToVerticalBox(Control)->SetPadding(FMargin(4,8));
                }
            };
            Group(TEXT("SettingsQualityOptions"),LOCTEXT("Quality","画面品质"),QualityControls);Group(TEXT("SettingsFrameOptions"),LOCTEXT("FrameRate","帧率上限"),FrameControls);
            auto* Motion=WidgetTree->ConstructWidget<UVerticalBox>();auto* MotionLayout=Groups->AddChildToGrid(Motion,2,0);MotionLayout->SetHorizontalAlignment(HAlign_Fill);MotionLayout->SetPadding(FMargin(0,8));
            AddText(WidgetTree,Motion,LOCTEXT("MotionHeading","动态效果"),32);
            if(!Reduced)Reduced=MakeButton(WidgetTree,TEXT("ToggleReducedMotion"),TEXT("ToggleReducedMotion"),FText::GetEmpty());
            Reduced->RemoveFromParent();Motion->AddChildToVerticalBox(Reduced);
            AddText(WidgetTree,Motion,LOCTEXT("SettingsImmediate","选项即时生效并保存。可按设备性能选择适合的画面品质。"),24);
            if(auto* Paper=InkFrontend::Find<UBorder>(this,TEXT("SettingsPaper")))if(auto* Frame=Cast<UVerticalBox>(Paper->GetContent()))
            {
                auto* Apply=MakeButton(WidgetTree,TEXT("ApplySettings"),TEXT("ApplySettings"),LOCTEXT("ApplySettings","应用设置"));
                auto* Footer=WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("SettingsFixedActions"));
                if(auto* Back=GetWidgetFromName(TEXT("SettingsBack"))){Back->RemoveFromParent();auto* BS=Footer->AddChildToHorizontalBox(Back);BS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));BS->SetPadding(FMargin(0,0,8,0));}
                auto* AS=Footer->AddChildToHorizontalBox(Apply);auto Fill=FSlateChildSize(ESlateSizeRule::Fill);Fill.Value=2;AS->SetSize(Fill);
                Frame->AddChildToVerticalBox(Footer)->SetPadding(FMargin(0,8,0,0));
            }
        }
    }
    if(auto* B=InkFrontend::Find<UInkFrontendActionButton>(this,TEXT("ToggleReducedMotion")))
        if(auto* Size=Cast<USizeBox>(B->GetContent()))if(auto* Text=Cast<UTextBlock>(Size->GetContent()))Text->SetText(FText::FromString(bReducedMotion?TEXT("减少动效：开启"):TEXT("减少动效：关闭")));
    auto* Settings=UGameUserSettings::GetGameUserSettings();
    if(auto* Quality=InkFrontend::Find<UTextBlock>(this,TEXT("QualityValue")))
    {
        const int32 Value=Settings?Settings->GetOverallScalabilityLevel():-1;
        const FText Label=Value==0?LOCTEXT("Low","流畅"):Value==1?LOCTEXT("Medium","均衡"):Value==2?LOCTEXT("High","精致"):LOCTEXT("QualityCustom","自定义");
        Quality->SetText(FText::Format(LOCTEXT("CurrentQuality","当前：{0}"),Label));
    }
    if(auto* Frame=InkFrontend::Find<UTextBlock>(this,TEXT("FrameValue")))
    {
        const float Limit=Settings?Settings->GetFrameRateLimit():0;
        const float EffectiveLimit=Limit>0?Limit:(GEngine&&GEngine->bUseFixedFrameRate?GEngine->FixedFrameRate:0);
        Frame->SetText(EffectiveLimit>0?FText::Format(LOCTEXT("CurrentFrameLimit","当前上限：{0} 帧"),FText::AsNumber(FMath::RoundToInt(EffectiveLimit))):LOCTEXT("UnlimitedFrames","当前：不限制"));
    }
}

void UInkFrontendWidget::TriggerAction(FName Action)
{
    if(IsDesignTime())return;
    if(!PendingConfirmationAction.IsNone()&&Action!=TEXT("ConfirmPending")&&Action!=TEXT("CancelPending"))return;
    ExecuteAction(Action);OnActionTriggered.Broadcast(Action);
}

void UInkFrontendWidget::ExecuteAction(FName Action,bool bConfirmed)
{
    if(Action.ToString().StartsWith(TEXT("SelectSave_")))
    {
        if(auto* Details=InkFrontend::Find<UWidgetSwitcher>(this,TEXT("SaveDetailSwitcher")))Details->SetActiveWidgetIndex(FCString::Atoi(*Action.ToString().Mid(11)));
        RefreshSaves();ApplyProvidedArt();return;
    }
    if(Action==TEXT("ApplySettings"))
    {
        if(auto* Settings=UGameUserSettings::GetGameUserSettings()){Settings->ApplyNonResolutionSettings();Settings->SaveSettings();RefreshSettings();SetPageFeedback(LOCTEXT("SettingsSaved","设置已应用并保存。"));}
        else SetPageFeedback(LOCTEXT("SettingsUnavailable","设置暂不可用。"));
        return;
    }
    if(Action==TEXT("ToggleReducedMotion")){bReducedMotion=!bReducedMotion;if(bReducedMotion)DrawPreviewWheel.Skip();RefreshSettings();return;}
    if(Action==TEXT("ShowAnchorIdentity"))
    {
        const FString Name=AnchorStatusWidget?AnchorStatusWidget->AnchorName:FString();
        ShowConfirmation(LOCTEXT("AnchorIdentityTitle","主播身份"),Name.IsEmpty()?LOCTEXT("AnchorIdentityMissing","身份尚未同步"):FText::FromString(Name),TEXT("DismissAnchorIdentity"));
        return;
    }
    if(Action==TEXT("DismissAnchorIdentity")){SetKeyboardFocus();return;}
    if(Action==TEXT("ToggleHUDMenu"))
    {
        if(auto* Menu=GetWidgetFromName(TEXT("HUDRuntimeMenu")))
        {
            const bool Open=Menu->GetVisibility()==ESlateVisibility::Collapsed;
            Menu->SetVisibility(Open?ESlateVisibility::SelfHitTestInvisible:ESlateVisibility::Collapsed);
            if(Director)Director->SetFrontendInputBlocked(Open);
            if(Open)SetKeyboardFocus();
            else if(auto* Button=GetWidgetFromName(TEXT("HUD_ToggleHUDMenu")))Button->SetKeyboardFocus();
        }
        return;
    }
    if(Action==TEXT("ToggleCultivation"))
    {
        if(auto* Pawn=GetOwningPlayerPawn())if(auto* Ability=Pawn->FindComponentByClass<UCultivationGameplayComponent>())Ability->RequestCultivationToggle();
        RefreshLiveStatus();return;
    }
    if(Action==TEXT("AnchorBreakthrough"))
    {
        auto* Cultivation=GetGameInstance()?GetGameInstance()->GetSubsystem<UCultivationSubsystem>():nullptr;
        const bool Done=Cultivation&&Cultivation->TryAnchorBreakthrough();
        SetPageFeedback(FText::FromString(Done?TEXT("境界突破成功"):TEXT("尚未满足突破条件：需要足够修为与本层突破材料。")));RefreshCharacter();return;
    }
    const FString ExtensionAction=Action.ToString();
    if(ExtensionAction.StartsWith(TEXT("Ext_"))){if(auto* Page=ExtensionPages.FindRef(CurrentPage).Get())Page->HandleAction(FName(*ExtensionAction.Mid(4)));return;}
    if(Action==TEXT("RankLocal")||Action==TEXT("RankGlobal")){LeaderboardScope=Action==TEXT("RankGlobal")?TEXT("Global"):TEXT("Local");RefreshLeaderboard();return;}
    if(Action==TEXT("CancelPending")){SetQuitConfirmation(false);SetKeyboardFocus();return;}
    if(Action==TEXT("ConfirmPending"))
    {
        const FName ConfirmedAction=PendingConfirmationAction;SetQuitConfirmation(false);
        if(!ConfirmedAction.IsNone())ExecuteAction(ConfirmedAction,true);return;
    }
    if(Action==TEXT("StartGame"))
    {
        if(!Director||!Director->StartGame())SetPageFeedback(LOCTEXT("StartFailed","暂时无法进入游戏，请稍后再试。"));
        return;
    }
    if(Action==TEXT("ReturnToMainMenu"))
    {
        if(Director)Director->ReturnToMainMenu();return;
    }
    if(Action==TEXT("Back"))
    {
        if(CurrentPage==TEXT("HUD")){if(Director)Director->ReturnToMainMenu();}
        else ShowPage(Director&&Director->bMenuActive?TEXT("Main"):TEXT("HUD"));
        return;
    }
    const FString ActionString=Action.ToString();
    if(Action==TEXT("OpenCharacter")){OpenFeaturePanel(TEXT("CharacterProfile"),FString());return;}
    if(ActionString.StartsWith(TEXT("Open")))
    {const FName Page(*ActionString.Mid(4));if(!OpenFeaturePanel(Page,FString()))ShowPage(Page);return;}
    if(Action==TEXT("RequestQuit")||Action==TEXT("QuitGame"))
    {
        if(!bConfirmed)ShowConfirmation(LOCTEXT("QuitTitle","暂别仙途"),LOCTEXT("QuitBody","确定退出游戏？尚未保存的行程将不会保留。"),TEXT("QuitGame"));
        else UKismetSystemLibrary::QuitGame(this,GetOwningPlayer(),EQuitPreference::Quit,false);
        return;
    }
    if(Action==TEXT("ContinueGame"))
    {
        auto* Save=InkFrontend::Saves(this);const int32 SlotIndex=Save?Save->FindLatestSlot():INDEX_NONE;
        if(SlotIndex<0){SetPageFeedback(LOCTEXT("NoContinueSave","暂无可继续的存档。"));return;}
        ExecuteAction(FName(*FString::Printf(TEXT("LoadSlot_%d"),SlotIndex)));return;
    }
    if(ActionString.StartsWith(TEXT("SaveSlot_"))||ActionString.StartsWith(TEXT("LoadSlot_"))||ActionString.StartsWith(TEXT("DeleteSlot_")))
    {
        FString Prefix,Number;if(!ActionString.Split(TEXT("_"),&Prefix,&Number)||!Number.IsNumeric())return;
        const int32 Index=FCString::Atoi(*Number);if(!UInkSaveSubsystem::IsValidSlotIndex(Index))return;
        auto* Save=InkFrontend::Saves(this);
        if(!Save){SetPageFeedback(LOCTEXT("SaveUnavailable","本地存档尚未就绪。"));return;}
        const auto Slots=Save->GetSlots();const auto* Info=Slots.FindByPredicate([Index](const FInkSaveSlotInfo& SaveInfo){return SaveInfo.SlotIndex==Index;});
        if(Prefix==TEXT("SaveSlot"))
        {
            if(!Director||Director->bMenuActive){SetPageFeedback(LOCTEXT("SaveGameplayOnly","进入游戏后可保存当前行程。"));return;}
            if(Info&&Info->bExists&&!bConfirmed){ShowConfirmation(LOCTEXT("OverwriteTitle","覆盖旧迹"),FText::Format(LOCTEXT("OverwriteBody","确定覆盖存档 {0}？原有记录将被当前行程替换。"),FText::AsNumber(Index+1)),Action);return;}
            const bool bSaved=Save->SaveSlot(Index,GetOwningPlayerPawn());RefreshSaves();
            SetPageFeedback(bSaved?LOCTEXT("Saved","行程已保存。"):FText::FromString(Save->LastError));return;
        }
        if(Prefix==TEXT("LoadSlot"))
        {
            if(!Info||!Info->bValid){SetPageFeedback(LOCTEXT("LoadInvalid","此存档无法读取。"));return;}
            const bool bWasMenu=Director&&Director->bMenuActive;
            if(!Director||(bWasMenu&&!Director->StartGame())){SetPageFeedback(LOCTEXT("StartFailed","暂时无法进入游戏，请稍后再试。"));return;}
            if(!Save->LoadSlot(Index,GetOwningPlayerPawn()))
            {
                const FText Error=FText::FromString(Save->LastError);
                if(bWasMenu)Director->ReturnToMainMenu();ShowPage(TEXT("Saves"));SetPageFeedback(Error);return;
            }
            ShowPage(TEXT("HUD"));return;
        }
        if(Prefix==TEXT("DeleteSlot"))
        {
            if(!Info||!Info->bExists){SetPageFeedback(LOCTEXT("EmptySlot","尚未留迹"));return;}
            if(!bConfirmed){ShowConfirmation(LOCTEXT("DeleteTitle","抹去旧迹"),FText::Format(LOCTEXT("DeleteBody","确定删除存档 {0}？删除后无法恢复。"),FText::AsNumber(Index+1)),Action);return;}
            const bool bDeleted=Save->DeleteSlot(Index);RefreshSaves();
            SetPageFeedback(bDeleted?LOCTEXT("Deleted","存档已删除。"):FText::FromString(Save->LastError));return;
        }
    }
    if(Action==TEXT("QualityLow")||Action==TEXT("QualityMedium")||Action==TEXT("QualityHigh")||Action==TEXT("Frame30")||Action==TEXT("Frame60"))
    {
        auto* Settings=UGameUserSettings::GetGameUserSettings();if(!Settings){SetPageFeedback(LOCTEXT("SettingsUnavailable","设置暂不可用。"));return;}
        if(ActionString.StartsWith(TEXT("Quality")))Settings->SetOverallScalabilityLevel(Action==TEXT("QualityLow")?0:Action==TEXT("QualityMedium")?1:2);
        else {Settings->SetFrameRateLimit(Action==TEXT("Frame30")?30.f:60.f);if(GEngine)GEngine->bUseFixedFrameRate=false;}
        Settings->ApplyNonResolutionSettings();Settings->SaveSettings();RefreshSettings();SetPageFeedback(LOCTEXT("SettingsSaved","设置已应用并保存。"));return;
    }
    if(ExecuteInventoryAction(Action))return;
    if(Action==TEXT("HUDRankView")){bGameplayLeaderboardRequested=true;return;}
    if(Action==TEXT("HUDRankClose")){bGameplayLeaderboardRequested=false;return;}
    if(Action==TEXT("HUDRankSpeed")||Action==TEXT("HUDRankDamage")){RequestedGameplayLeaderboard=Action==TEXT("HUDRankDamage")?TEXT("Damage"):TEXT("CultivationSpeed");bGameplayLeaderboardRequested=true;return;}
    if(Action==TEXT("RankClose")){ShowPage(TEXT("HUD"));return;}
    if(Action==TEXT("RankAnchor")||Action==TEXT("RankCultivation")||Action==TEXT("RankDamage"))
    { LeaderboardCategory=Action==TEXT("RankAnchor")?FName(TEXT("Anchor")):Action==TEXT("RankCultivation")?FName(TEXT("CultivationSpeed")):FName(TEXT("Damage"));RefreshLeaderboard();return; }
}

void UInkFrontendWidget::SetFeedback(const FText& Message){SetPageFeedback(Message);}

UTextBlock* UInkFrontendWidget::EnsureHUDFeedback()
{
    if(!WidgetTree)return nullptr;
    if(auto* Existing=GetWidgetFromName(TEXT("HUDFeedback")))return Cast<UTextBlock>(Existing);
    auto* HUD=InkFrontend::Find<UCanvasPanel>(this,TEXT("HUDPage"));if(!HUD)return nullptr;

    // Add only the missing runtime feedback area; an authored HUDFeedback is always reused.
    auto* Text=AddText(WidgetTree,nullptr,FText::GetEmpty(),23,TEXT("HUDFeedback"));
    Text->SetColorAndOpacity(CinnabarColor);
    RuntimeHUDFeedbackPanel=MakeInkCard(WidgetTree,Text,FMargin(16,12));
    RuntimeHUDFeedbackPanel->SetVisibility(ESlateVisibility::Collapsed);
    auto* FeedbackLayout=HUD->AddChildToCanvas(RuntimeHUDFeedbackPanel);
    FeedbackLayout->SetAnchors(FAnchors(0,0,1,0));
    FeedbackLayout->SetOffsets(FMargin(22,132,22,120));
    FeedbackLayout->SetZOrder(1);
    return Text;
}

void UInkFrontendWidget::SetPageFeedback(const FText& Message)
{
    auto* Text=Cast<UTextBlock>(GetWidgetFromName(FName(*(CurrentPage.ToString()+TEXT("Feedback")))));
    if(!Text&&CurrentPage==TEXT("HUD")&&!Message.IsEmpty())Text=EnsureHUDFeedback();
    if(Text)
    {
        Text->SetText(Message);
        if(CurrentPage==TEXT("Main"))Text->SetVisibility(Message.IsEmpty()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
        if(CurrentPage==TEXT("HUD")&&RuntimeHUDFeedbackPanel&&Text->IsChildOf(RuntimeHUDFeedbackPanel))
            RuntimeHUDFeedbackPanel->SetVisibility(Message.IsEmpty()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);
    }
}

void UInkFrontendWidget::SetQuitConfirmation(bool bVisible)
{
    if(auto* Layer=GetWidgetFromName(TEXT("ConfirmationLayer")))Layer->SetVisibility(bVisible?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    if(!bVisible)PendingConfirmationAction=NAME_None;
    if(Director&&!IsDesignTime())Director->SetFrontendInputBlocked(bVisible||CurrentPage!=TEXT("HUD"));
}

void UInkFrontendWidget::ShowConfirmation(const FText& Title,const FText& Message,FName ConfirmAction)
{
    PendingConfirmationAction=ConfirmAction;
    if(auto* Text=InkFrontend::Find<UTextBlock>(this,TEXT("ConfirmationTitle")))Text->SetText(Title);
    if(auto* Text=InkFrontend::Find<UTextBlock>(this,TEXT("ConfirmationMessage")))Text->SetText(Message);
    SetQuitConfirmation(true);
    if(auto* Button=InkFrontend::Find<UInkFrontendActionButton>(this,TEXT("CancelPending")))Button->SetKeyboardFocus();
}

bool UInkFrontendWidget::IsQuitConfirmationVisible() const
{ return PendingConfirmationAction==TEXT("QuitGame")&&GetWidgetFromName(TEXT("ConfirmationLayer"))&&GetWidgetFromName(TEXT("ConfirmationLayer"))->GetVisibility()==ESlateVisibility::Visible; }

#undef LOCTEXT_NAMESPACE
