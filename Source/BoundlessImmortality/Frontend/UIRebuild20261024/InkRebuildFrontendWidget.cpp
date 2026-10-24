#include "InkRebuildFrontendWidget.h"
#include "InkRebuildTheme.h"
#include "InkRebuildFeatureHost.h"
#include "Player/InkPlayerVisibleEntryWidgets.h"
#include "Player/InkPlayerMenuPageWidgets.h"
#include "Player/InkPlayerCharacterInventoryWidgets.h"
#include "Frontend/InkAnchorStatusWidget.h"
#include "Frontend/InkMenuDirector.h"
#include "Frontend/InkSaveSubsystem.h"
#include "Frontend/WulingFrontendData.h"
#include "Frontend/UiRollout20261024/LogicCore/InkCoreAdapter.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SafeZone.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WidgetSwitcherSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "GameFramework/GameUserSettings.h"

namespace
{
const FName CorePages[]={TEXT("Main"),TEXT("HUD"),TEXT("Saves"),TEXT("Settings"),TEXT("Character"),TEXT("Inventory"),TEXT("Leaderboard")};
UTexture2D* PlayerTexture(const TCHAR* Name)
{
    const FString Package=FString(TEXT("/Game/newUI/Components/Player/"))+Name;
    return LoadObject<UTexture2D>(nullptr,*(Package+TEXT(".")+Name),nullptr,LOAD_NoWarn|LOAD_Quiet);
}
void AddHudGlyphs(UWidgetTree* Tree)
{
    auto* Atlas=PlayerTexture(TEXT("T_HudRouteGlyphAtlas_v1"));if(!Atlas)return;
    const TCHAR* Buttons[]={TEXT("OpenCharacter"),TEXT("OpenInventory"),TEXT("ToggleCultivation"),TEXT("OpenLeaderboard"),TEXT("OpenExtensions"),TEXT("ToggleHUDMenu")};
    for(int32 Cell=0;Cell<6;++Cell)
    {
        auto* Button=Cast<UInkFrontendActionButton>(Tree->FindWidget(FName(*(FString(TEXT("Rebuild_Hud_"))+Buttons[Cell]))));
        auto* Box=Button?Cast<USizeBox>(Button->GetContent()):nullptr;auto* Label=Box?Box->GetContent():nullptr;if(!Label)continue;
        Box->ClearChildren();Box->SetMinDesiredWidth(88);Box->SetMinDesiredHeight(88);
        auto* Stack=Tree->ConstructWidget<UVerticalBox>();Box->SetContent(Stack);
        auto* Icon=Tree->ConstructWidget<UImage>(UImage::StaticClass(),FName(*(FString(TEXT("NewUIHudGlyph_"))+Buttons[Cell])));
        FSlateBrush Brush;Brush.SetResourceObject(Atlas);Brush.DrawAs=ESlateBrushDrawType::Image;Brush.SetImageSize(FVector2D(44));
        Brush.SetUVRegion(FBox2D(FVector2D(float(Cell%3)/3,float(Cell/3)/2),FVector2D(float(Cell%3+1)/3,float(Cell/3+1)/2)));Icon->SetBrush(Brush);Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* IconSize=Tree->ConstructWidget<USizeBox>();IconSize->SetWidthOverride(44);IconSize->SetHeightOverride(44);IconSize->SetContent(Icon);
        Stack->AddChildToVerticalBox(IconSize)->SetHorizontalAlignment(HAlign_Center);Stack->AddChildToVerticalBox(Label)->SetHorizontalAlignment(HAlign_Center);
    }
}
InkPlayerCharacterInventoryWidgets::EProviderState PlayerState(InkCorePolicy::EState State)
{
    using E=InkPlayerCharacterInventoryWidgets::EProviderState;
    switch(State){case InkCorePolicy::EState::Data:return E::Data;case InkCorePolicy::EState::Empty:return E::Empty;case InkCorePolicy::EState::Offline:return E::Offline;case InkCorePolicy::EState::Loading:return E::Loading;case InkCorePolicy::EState::Error:return E::Error;default:return E::Unknown;}
}
FText PageTitle(FName Page)
{
    if(Page==TEXT("Saves"))return FText::FromString(TEXT("山河留迹"));
    if(Page==TEXT("Settings"))return FText::FromString(TEXT("静心调息"));
    if(Page==TEXT("Character"))return FText::FromString(TEXT("修士"));
    if(Page==TEXT("Inventory"))return FText::FromString(TEXT("行囊"));
    return FText::FromString(TEXT("山海榜"));
}
void RetireChildren(UCanvasPanel* Canvas)
{
    TArray<UWidget*> Old;
    TFunction<void(UWidget*)> Visit=[&](UWidget* Widget){if(auto* Panel=Cast<UPanelWidget>(Widget))for(auto* Child:Panel->GetAllChildren()){Visit(Child);Old.Add(Child);}};
    Visit(Canvas);Canvas->ClearChildren();for(auto* Widget:Old)Widget->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);
}
}

FName UInkRebuildFrontendWidget::GetRuntimePageLayoutVersion(FName Page)const
{if(const auto* Host=Cast<UInkRebuildFeatureHost>(ExtensionPages.FindRef(Page).Get()))return Host->GetLayoutVersion();return PageVersions.FindRef(Page);}
UCanvasPanel* UInkRebuildFrontendWidget::GetPageContentCanvas(FName Page)const
{return PageBodies.FindRef(Page);}

void UInkRebuildFrontendWidget::BuildDefaultTree(UWidgetTree* Tree)
{
    if(!Tree)return;
    if(Tree->FindWidget(TEXT("NewUIRoot_r1"))&&!PageCanvases.IsEmpty())return;
    if(Tree->RootWidget)
    {
        TArray<UWidget*> Old;Tree->GetAllWidgets(Old);Tree->RootWidget=nullptr;
        for(auto* Widget:Old)Widget->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);
    }
    PageCanvases.Reset();PageBodies.Reset();PageComponents.Reset();PageVersions.Reset();ContentKeys.Reset();
    auto* Root=Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(),TEXT("NewUIRoot_r1"));Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);Tree->RootWidget=Root;
    auto* Safe=Tree->ConstructWidget<USafeZone>(USafeZone::StaticClass(),TEXT("NewUIDeviceSafeZone"));Safe->SetSidesToPad(true,true,true,true);
    auto* SafeLayout=Root->AddChildToOverlay(Safe);SafeLayout->SetHorizontalAlignment(HAlign_Fill);SafeLayout->SetVerticalAlignment(VAlign_Fill);
    SafeRoot=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("SafeCanvas"));SafeRoot->SetVisibility(ESlateVisibility::SelfHitTestInvisible);Safe->SetContent(SafeRoot);
    auto* Switcher=Tree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(),TEXT("PageSwitcher"));
    auto* SwitcherLayout=SafeRoot->AddChildToCanvas(Switcher);SwitcherLayout->SetAnchors(FAnchors(0,0,1,1));SwitcherLayout->SetOffsets(FMargin(0));
    for(FName Page:CorePages)
    {
        auto* Canvas=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),FName(*(TEXT("NewUIPage_")+Page.ToString())));Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        auto* PageLayout=CastChecked<UWidgetSwitcherSlot>(Switcher->AddChild(Canvas));PageLayout->SetHorizontalAlignment(HAlign_Fill);PageLayout->SetVerticalAlignment(VAlign_Fill);PageCanvases.Add(Page,Canvas);
        if(Page==TEXT("HUD")){PageBodies.Add(Page,Canvas);InkPlayerVisibleEntryWidgets::BuildHudSurface(Tree,this,Canvas,nullptr,FText());AddHudGlyphs(Tree);PageVersions.Add(Page,TEXT("newui-r1-player-hud"));continue;}
        if(Page==TEXT("Main"))
        {
            InkRebuildUI::Card(Tree,Canvas,TEXT("NewUIMainTitleFrame"),FVector4(24,24,672,160));
            InkRebuildUI::Text(Tree,Canvas,TEXT("NewUIMainTitle"),FText::FromString(TEXT("武灵")),InkRebuildUI::ETextRole::Display,FVector4(48,60,624,88),64,true);
            if(auto* Ornament=PlayerTexture(TEXT("T_MainActionOrnament_v1")))
            {auto* Image=Tree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("NewUIMainOrnament"));Image->SetBrushFromTexture(Ornament);Image->SetVisibility(ESlateVisibility::HitTestInvisible);InkRebuildUI::Place(Canvas,Image,FVector4(24,24,160,160),2);}
            auto* Body=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("NewUIMainActionsSlot"));PageBodies.Add(Page,Body);InkRebuildUI::Place(Canvas,Body,FVector4(24,906,672,350));
            MountPageComponent(Page,InkPlayerVisibleEntryWidgets::BuildMainMenuSurface(Tree,this,false,FText::FromString(TEXT("存档状态读取中"))),TEXT("newui-r1-player-main"));continue;
        }
        auto* Background=InkRebuildUI::Card(Tree,Canvas,FName(*(TEXT("NewUIBackground_")+Page.ToString())),FVector4(0,0,720,1280));Background->SetBrush(InkRebuildUI::Surface(InkRebuildUI::Background(),0,false));
        InkRebuildUI::Text(Tree,Canvas,FName(*(TEXT("NewUIHeading_")+Page.ToString())),PageTitle(Page),InkRebuildUI::ETextRole::Title,FVector4(24,24,540,80));
        InkRebuildUI::Action(Tree,Canvas,this,FName(*(TEXT("NewUIBack_")+Page.ToString())),TEXT("Back"),FText::FromString(TEXT("返回")),FVector4(608,24,88,88));
        if(Page==TEXT("Inventory"))
        {
            auto* Search=Tree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(),TEXT("NewUIInventorySearch"));
            Search->SetHintText(FText::FromString(TEXT("搜索物品")));
            auto InputStyle=Search->GetWidgetStyle();InputStyle.SetFont(InkRebuildUI::Font(InkRebuildUI::ETextRole::Body));
            const auto InputBackground=InkRebuildUI::Surface(InkRebuildUI::Paper());
            InputStyle.SetBackgroundImageNormal(InputBackground).SetBackgroundImageHovered(InputBackground).SetBackgroundImageFocused(InputBackground);
            InputStyle.SetForegroundColor(InkRebuildUI::Ink());Search->SetWidgetStyle(InputStyle);
            Search->SetText(FText::FromString(InventorySearchText));Search->OnTextChanged.AddUniqueDynamic(this,&ThisClass::HandleNewInventorySearch);
            InkRebuildUI::Place(Canvas,Search,FVector4(24,136,672,88));
        }
        InkRebuildUI::Text(Tree,Canvas,FName(*(TEXT("NewUIFeedback_")+Page.ToString())),FText(),InkRebuildUI::ETextRole::Secondary,FVector4(24,1232,672,40));
        auto* Scroll=Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),FName(*(TEXT("NewUIScroll_")+Page.ToString())));Scroll->SetAllowOverscroll(false);Scroll->SetScrollbarThickness(FVector2D(6));InkRebuildUI::Place(Canvas,Scroll,FVector4(24,136,672,1096));
        auto* Extent=Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),FName(*(TEXT("NewUIExtent_")+Page.ToString())));Extent->SetWidthOverride(660);Extent->SetMinDesiredHeight(1096);Scroll->AddChild(Extent);
        auto* Body=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),FName(*(TEXT("NewUIBody_")+Page.ToString())));Extent->SetContent(Body);PageBodies.Add(Page,Body);
        PageVersions.Add(Page,TEXT("newui-r1-awaiting-owner-page"));
        InkRebuildUI::Text(Tree,Body,FName(*(TEXT("NewUIComponentPending_")+Page.ToString())),FText::FromString(TEXT("新版页面组件正在接入")),InkRebuildUI::ETextRole::Body,FVector4(24,24,600,96));
    }
    BuildConfirmation(Tree);LayoutPages({720,1280});
}

void UInkRebuildFrontendWidget::MountPageComponent(FName Page,UWidget* Component,FName Version)
{
    auto* Body=PageBodies.FindRef(Page).Get();if(!Body||!Component)return;
    if(Component->GetParent())Component->RemoveFromParent();RetireChildren(Body);
    auto* Layout=Body->AddChildToCanvas(Component);Layout->SetAnchors(FAnchors(0,0,1,1));Layout->SetOffsets(FMargin(0));
    PageComponents.Add(Page,Component);PageVersions.Add(Page,Version);
}

void UInkRebuildFrontendWidget::BuildConfirmation(UWidgetTree* Tree)
{
    ConfirmationRoot=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("ConfirmationLayer"));auto* Layer=SafeRoot->AddChildToCanvas(ConfirmationRoot);Layer->SetAnchors(FAnchors(0,0,1,1));Layer->SetOffsets(FMargin(0));Layer->SetZOrder(1000);
    auto* Dim=InkRebuildUI::Card(Tree,ConfirmationRoot,TEXT("NewUIConfirmDim"),FVector4(0,0,720,1280));Dim->SetBrush(InkRebuildUI::Surface(FLinearColor(0,0,0,.55f),0,false));
    auto* Paper=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("ConfirmationPaper"));InkRebuildUI::Place(ConfirmationRoot,Paper,FVector4(60,420,600,420),2);
    InkRebuildUI::Card(Tree,Paper,TEXT("NewUIConfirmSurface"),FVector4(0,0,600,420));
    InkRebuildUI::Text(Tree,Paper,TEXT("ConfirmationTitle"),FText(),InkRebuildUI::ETextRole::Section,FVector4(24,24,552,64));
    InkRebuildUI::Text(Tree,Paper,TEXT("ConfirmationMessage"),FText(),InkRebuildUI::ETextRole::Body,FVector4(24,112,552,148));
    InkRebuildUI::Action(Tree,Paper,this,TEXT("CancelPending"),TEXT("CancelPending"),FText::FromString(TEXT("取消")),FVector4(24,308,264,88));
    InkRebuildUI::Action(Tree,Paper,this,TEXT("ConfirmPending"),TEXT("ConfirmPending"),FText::FromString(TEXT("确认")),FVector4(312,308,264,88),true);
    ConfirmationRoot->SetVisibility(ESlateVisibility::Collapsed);
}

void UInkRebuildFrontendWidget::LayoutPages(FVector2D Size)
{
    if(Size.X<=0||Size.Y<=0||!WidgetTree)return;LastSafeSize=Size;
    const bool Wide=Size.X/Size.Y>=1.25;const float W=Size.X,H=Size.Y,BodyW=W-48;
    auto Move=[&](FName Name,FVector4 Bounds){if(auto* Widget=GetWidgetFromName(Name))if(auto* Layout=Cast<UCanvasPanelSlot>(Widget->Slot))Layout->SetOffsets(FMargin(Bounds.X,Bounds.Y,Bounds.Z,Bounds.W));};
    Move(TEXT("NewUIMainTitleFrame"),FVector4(24,24,Wide?W*.52f:BodyW,160));Move(TEXT("NewUIMainTitle"),FVector4(48,60,Wide?W*.52f-48:BodyW-48,88));
    Move(TEXT("NewUIMainActionsSlot"),Wide?FVector4(W*.60f,H*.25f,W*.40f-24,FMath::Max(350.f,H*.72f-24)):FVector4(24,H-374,BodyW,350));
    for(FName Page:CorePages)
    {
        if(Page==TEXT("Main")||Page==TEXT("HUD"))continue;
        Move(FName(*(TEXT("NewUIBackground_")+Page.ToString())),FVector4(0,0,W,H));Move(FName(*(TEXT("NewUIHeading_")+Page.ToString())),FVector4(24,24,W-168,80));Move(FName(*(TEXT("NewUIBack_")+Page.ToString())),FVector4(W-112,24,88,88));
        if(Page==TEXT("Leaderboard"))if(auto* Heading=GetWidgetFromName(TEXT("NewUIHeading_Leaderboard")))Heading->SetVisibility(ESlateVisibility::Collapsed);
        const float BodyY=Page==TEXT("Inventory")?240.f:Page==TEXT("Leaderboard")?24.f:136.f;
        Move(FName(*(TEXT("NewUIScroll_")+Page.ToString())),FVector4(24,BodyY,BodyW,H-BodyY-72));
        Move(FName(*(TEXT("NewUIFeedback_")+Page.ToString())),FVector4(24,H-64,BodyW,40));
        if(Page==TEXT("Inventory"))Move(TEXT("NewUIInventorySearch"),FVector4(24,136,BodyW,88));
        if(auto* Extent=Cast<USizeBox>(GetWidgetFromName(FName(*(TEXT("NewUIExtent_")+Page.ToString())))))
        {
            Extent->SetWidthOverride(BodyW-12);const auto* Component=PageComponents.FindRef(Page).Get();const float Desired=Component?float(Component->GetDesiredSize().Y):0.f;Extent->SetHeightOverride(FMath::Max(H-160,Desired>0?Desired:Extent->GetHeightOverride()));
        }
    }
    Move(TEXT("NewUIConfirmDim"),FVector4(0,0,W,H));const float DialogW=FMath::Min(600.f,W-48);
    Move(TEXT("ConfirmationPaper"),FVector4((W-DialogW)*.5f,(H-420)*.5f,DialogW,420));Move(TEXT("NewUIConfirmSurface"),FVector4(0,0,DialogW,420));
    Move(TEXT("ConfirmationTitle"),FVector4(24,24,DialogW-48,64));Move(TEXT("ConfirmationMessage"),FVector4(24,112,DialogW-48,148));
    Move(TEXT("CancelPending"),FVector4(24,308,(DialogW-72)*.5f,88));Move(TEXT("ConfirmPending"),FVector4(36+(DialogW-48)*.5f,308,(DialogW-72)*.5f,88));
}

void UInkRebuildFrontendWidget::RefreshMainAndHud()
{
    auto* Saves=GetGameInstance()?GetGameInstance()->GetSubsystem<UInkSaveSubsystem>():nullptr;const bool CanContinue=Saves&&Saves->FindLatestSlot()!=INDEX_NONE;
    if(auto* Button=Cast<UInkFrontendActionButton>(GetWidgetFromName(TEXT("Rebuild_Main_ContinueGame"))))Button->SetIsEnabled(CanContinue);
    if(auto* Label=Cast<UTextBlock>(GetWidgetFromName(TEXT("Rebuild_MainContinueStatus"))))Label->SetText(FText::FromString(CanContinue?TEXT("继续已保存的行程"):TEXT("暂无有效存档")));
    if(!AnchorStatusWidget&&GetGameInstance()&&GetOwningPlayer())
    {AnchorStatusWidget=CreateWidget<UInkAnchorStatusWidget>(this);if(AnchorStatusWidget){AnchorStatusWidget->Configure(this);AnchorStatusWidget->SetVisibility(ESlateVisibility::Collapsed);}}
    if(AnchorStatusWidget)AnchorStatusWidget->RefreshFromWorld();
    auto* Service=WulingFrontendData::Service(this);const FText Status=FText::FromString(Service?Service->GetStatus():TEXT("服务未启用"));
    InkPlayerVisibleEntryWidgets::RefreshHudStatus(WidgetTree,AnchorStatusWidget,Status);
    if(auto* Label=Cast<UTextBlock>(GetWidgetFromName(TEXT("Rebuild_HudCultivationLabel"))))Label->SetText(FText::FromString(AnchorStatusWidget&&!AnchorStatusWidget->ActionLabel.IsEmpty()?AnchorStatusWidget->ActionLabel:TEXT("修炼")));
}

void UInkRebuildFrontendWidget::RefreshLocalPage(FName Page)
{
    if(Page!=TEXT("Saves")&&Page!=TEXT("Settings")&&Page!=TEXT("Character")&&Page!=TEXT("Inventory"))return;
    UWidget* Content=nullptr;FString Key;
    using EProvider=InkPlayerCharacterInventoryWidgets::EProviderState;
    const bool PlayerDataPage=Page==TEXT("Character")||Page==TEXT("Inventory");
    const auto View=PlayerDataPage?InkCore::Read(this,GetPresentationDirector()&&!GetPresentationDirector()->bMenuActive):InkCore::FView();
    if(Page==TEXT("Character"))
    {
        const auto& Attributes=GetPresentedCharacterAttributes();const auto& Disciplines=GetPresentedCharacterDisciplines();
        const FText Realm=AnchorStatusWidget?AnchorStatusWidget->RealmName:FText();
        Key=GetPresentedCharacterName().ToString()+TEXT("|")+Realm.ToString()+FString::Printf(TEXT("|%d|%d|"),int32(View.CharacterState),View.Transport.Busy)+View.Status;
        for(const auto& Value:Attributes)Key+=TEXT("|")+Value.AttributeId.ToString()+Value.Label.ToString()+Value.Value.ToString();
        for(const auto& Value:Disciplines)Key+=TEXT("|")+Value.Category.ToString()+Value.Name.ToString()+Value.Detail.ToString()+FString::FromInt(Value.QualityTier);
        if(ContentKeys.FindRef(Page)==Key)return;RetireChildren(PageBodies.FindRef(Page));
        Content=InkPlayerCharacterInventoryWidgets::BuildCharacterPage(WidgetTree,this,GetPresentedCharacterName(),Realm,Attributes,Disciplines,PlayerTexture(TEXT("T_CharacterAttributeGlyphAtlas_v1")),
            PlayerState(View.CharacterState),View.Transport.Busy?FText::FromString(TEXT("请求处理中")):FText());
    }
    else if(Page==TEXT("Inventory"))
    {
        auto* Model=GetInventoryModel();const auto* Selected=Model->FindEntry(SelectedInventoryEntry);
        InventoryActionQuantity=Selected?FMath::Clamp(InventoryActionQuantity,1,FMath::Max(1,Selected->Item.Count)):1;
        if(!Selected)SelectedInventoryEntry.Invalidate();
        const bool CapacityKnown=!View.WulingAuthority&&!Model->IsAuthoritative();
        int32 VisibleCount=0;
        if(CapacityKnown&&InventoryCategory==TEXT("All")&&InventorySearchText.IsEmpty())VisibleCount=Model->GetTotalSlotCount();
        else for(const auto& Entry:Model->GetEntriesView())
            if((InventoryCategory==TEXT("All")||Entry.Item.Category==InventoryCategory)&&
                (InventorySearchText.IsEmpty()||Entry.Item.Name.ToString().Contains(InventorySearchText,ESearchCase::IgnoreCase)||Entry.Item.ItemId.ToString().Contains(InventorySearchText,ESearchCase::IgnoreCase)))++VisibleCount;
        InventoryPageIndex=FMath::Clamp(InventoryPageIndex,0,FMath::Max(0,FMath::DivideAndRoundUp(VisibleCount,40)-1));
        Key=FString::Printf(TEXT("%s|%s|%s|%d|%d|%d|%d|%d|%d|%.0f|"),*SelectedInventoryEntry.ToString(),*InventoryCategory.ToString(),*InventorySearchText,InventoryPageIndex,InventoryActionQuantity,Model->GetUnlockedSlotCount(),int32(View.InventoryState),View.Transport.Busy,CapacityKnown,LastSafeSize.X)+View.Status;
        for(const auto& Entry:Model->GetEntriesView())
        {FString Serialized;FInkInventoryEntry::StaticStruct()->ExportText(Serialized,&Entry,nullptr,nullptr,PPF_None,nullptr);Key+=TEXT("|")+Serialized;}
        if(ContentKeys.FindRef(Page)==Key)return;RetireChildren(PageBodies.FindRef(Page));
        auto Provider=PlayerState(View.InventoryState);if(Provider==EProvider::Empty&&CapacityKnown)Provider=EProvider::Data;
        Content=InkPlayerCharacterInventoryWidgets::BuildInventoryPage(WidgetTree,this,Model,SelectedInventoryEntry,InventoryCategory,InventorySearchText,InventoryPageIndex,InventoryActionQuantity,CapacityKnown,Model->GetUnlockedSlotCount(),PlayerTexture(TEXT("T_InventoryCategoryGlyphAtlas_v1")),Provider,View.Transport.Busy?FText::FromString(TEXT("请求处理中")):FText(),FMath::Max(112.f,LastSafeSize.X-60),View.Transport.Busy);
    }
    else if(Page==TEXT("Saves"))
    {
        auto* Saves=GetGameInstance()?GetGameInstance()->GetSubsystem<UInkSaveSubsystem>():nullptr;const auto Slots=Saves?Saves->GetSlots():TArray<FInkSaveSlotInfo>();
        const bool CanSave=GetPresentationDirector()&&!GetPresentationDirector()->bMenuActive;
        Key=CanSave?TEXT("game"):TEXT("menu");for(const auto& SaveInfo:Slots)Key+=FString::Printf(TEXT("|%d|%d|%d|%lld|%s"),SaveInfo.SlotIndex,SaveInfo.bExists,SaveInfo.bValid,SaveInfo.SavedAt.GetTicks(),*SaveInfo.ValidationError);
        if(ContentKeys.FindRef(Page)==Key)return;
        RetireChildren(PageBodies.FindRef(Page));
        Content=InkPlayerMenuPageWidgets::BuildSavesBody(WidgetTree,this,Slots,CanSave,FText::FromString(TEXT("进入游戏后可保存当前行程")));
    }
    else
    {
        auto* Settings=UGameUserSettings::GetGameUserSettings();const int32 Quality=Settings?Settings->GetOverallScalabilityLevel():INDEX_NONE;const float Cap=Settings?Settings->GetFrameRateLimit():0;
        Key=FString::Printf(TEXT("%d|%.3f|%d"),Quality,Cap,bReducedMotion);if(ContentKeys.FindRef(Page)==Key)return;
        RetireChildren(PageBodies.FindRef(Page));
        Content=InkPlayerMenuPageWidgets::BuildSettingsBody(WidgetTree,this,Quality,Cap,bReducedMotion);
    }
    MountPageComponent(Page,Content,FName(*(TEXT("newui-r1-player-")+Page.ToString().ToLower())));ContentKeys.Add(Page,Key);
}

void UInkRebuildFrontendWidget::HandleNewInventorySearch(const FText& Text)
{InventorySearchText=Text.ToString();InventoryPageIndex=0;RefreshRebuiltPresentation();}

void UInkRebuildFrontendWidget::RefreshRebuiltPresentation()
{
    if(bRefreshing||!WidgetTree)return;TGuardValue<bool> Guard(bRefreshing,true);RefreshMainAndHud();RefreshLocalPage(CurrentPage);RefreshNewLeaderboard();PollNewRankAvatars();LayoutPages(LastSafeSize.IsNearlyZero()?FVector2D(720,1280):LastSafeSize);RebindPresentationActions();
}
void UInkRebuildFrontendWidget::TickRebuiltPresentation(const FGeometry& Geometry,float DeltaSeconds)
{
    const auto Size=SafeRoot?SafeRoot->GetCachedGeometry().GetLocalSize():Geometry.GetLocalSize();if(!Size.Equals(LastSafeSize,1.f))LayoutPages(Size);
    RefreshElapsed+=DeltaSeconds;if(RefreshElapsed>=.35f){RefreshElapsed=0;RefreshRebuiltPresentation();}
    if(AnchorStatusWidget)
    {
        const float Health=UInkAnchorStatusWidget::SafeFraction(AnchorStatusWidget->Health,AnchorStatusWidget->MaxHealth),Qi=UInkAnchorStatusWidget::SafeFraction(AnchorStatusWidget->Cultivation,AnchorStatusWidget->RequiredCultivation);
        HealthFraction=bReducedMotion?Health:FMath::FInterpTo(HealthFraction,Health,DeltaSeconds,8);CultivationFraction=bReducedMotion?Qi:FMath::FInterpTo(CultivationFraction,Qi,DeltaSeconds,8);
        if(auto* Meter=Cast<UProgressBar>(GetWidgetFromName(TEXT("Rebuild_HudHealthMeter"))))Meter->SetPercent(HealthFraction);
        if(auto* Meter=Cast<UProgressBar>(GetWidgetFromName(TEXT("Rebuild_HudCultivationMeter"))))Meter->SetPercent(CultivationFraction);
    }
}
bool UInkRebuildFrontendWidget::HandleRebuiltNavigation(FName Action)
{
    const FString Id=Action.ToString();if(!Id.StartsWith(TEXT("Open")))return false;const FName Page(*Id.Mid(4));
    if(!PageCanvases.Contains(Page))return false;ShowPage(Page);return true;
}
void UInkRebuildFrontendWidget::SetRebuiltFeedback(const FText& Message)
{
    LatestFeedback=Message;const FName Target=CurrentPage==TEXT("Main")?FName(TEXT("Rebuild_MainFeedback")):CurrentPage==TEXT("HUD")?FName(TEXT("Rebuild_HudActivityStatus")):FName(*(TEXT("NewUIFeedback_")+CurrentPage.ToString()));
    if(auto* Label=Cast<UTextBlock>(GetWidgetFromName(Target))){Label->SetText(Message);Label->SetVisibility(Message.IsEmpty()?ESlateVisibility::Collapsed:ESlateVisibility::HitTestInvisible);}
}

UInkExtensionWidget* UInkRebuildFrontendWidget::CreateRebuiltFeaturePage(FName Page)
{
    if(!UInkRebuildFeatureHost::SupportsRoute(Page)||!GetOwningPlayer())return nullptr;
    return CreateWidget<UInkRebuildFeatureHost>(GetOwningPlayer());
}
FName UInkRebuildFrontendWidget::ResolveRebuiltFeatureRoute(FName Page,const FString& Context)const
{return UInkRebuildFeatureHost::CanonicalRoute(Page,Context);}
