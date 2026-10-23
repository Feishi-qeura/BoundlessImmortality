#include "Frontend/UI/InkUiAssetPaths.h"
#include "InkFrontendWidget.h"
#include "InkInventoryModel.h"
#include "Wuling/WulingPrototypeSubsystem.h"
#include "Engine/GameInstance.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "InkInventoryScrollBar.h"
#include "UI/InkUiTheme.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WidgetSwitcherSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "Framework/Application/SlateApplication.h"
#include "Materials/MaterialInterface.h"
#include "Styling/CoreStyle.h"
#include "Brushes/SlateColorBrush.h"

#define LOCTEXT_NAMESPACE "InkInventory"

namespace InkInventoryUI
{
    constexpr int32 SlotsPerPage=40;
    const FVector4 Main(20,16,683,565), Detail(692,52,963,565), Tooltip(975,86,1237,298);
    const FVector4 Empty(28,667,153,791), Normal(165,667,290,791), Selected(300,666,425,791), Hover(437,666,561,791), Locked(708,667,832,791);
    const FVector4 Tabs[]={FVector4(18,579,239,656),FVector4(244,579,443,656),FVector4(450,579,651,656),FVector4(654,579,856,656),FVector4(861,579,1076,656)};
    const FVector4 CategoryIcons[]={FVector4(1152,477,1241,559),FVector4(1063,354,1153,456),FVector4(972,359,1062,454),FVector4(973,466,1062,564),FVector4(1064,464,1153,565)};
    const FName Categories[]={TEXT("All"),TEXT("Equipment"),TEXT("Material"),TEXT("Consumable"),TEXT("Treasure")};
    const FVector4 Actions[]={FVector4(17,956,221,1034),FVector4(225,956,423,1034),FVector4(423,956,622,1034),FVector4(622,956,819,1034),FVector4(824,956,1029,1034),FVector4(1033,956,1242,1034)};
    const FVector4 CloseNormal(998,1126,1077,1204),CloseHover(1077,1126,1158,1204),ClosePressed(1158,1126,1239,1204);

    template<typename T> T* Find(UWidgetTree* Tree,const TCHAR* Name){return Tree?Cast<T>(Tree->FindWidget(FName(Name))):nullptr;}
    FSlateBrush Art(UTexture2D* Texture,const FVector4& Rect,bool bSlice=false)
    {
        FSlateBrush B;B.SetResourceObject(Texture);B.DrawAs=ESlateBrushDrawType::Image;B.ImageSize=FVector2D(Rect.Z-Rect.X,Rect.W-Rect.Y);
        if(Texture)
        {
            FString N=FString::Printf(TEXT("MI_%s_%d_%d_%d_%d_%s"),*Texture->GetName(),int32(Rect.X),int32(Rect.Y),int32(Rect.Z),int32(Rect.W),bSlice?TEXT("Slice"):TEXT("Fit"));
            if(auto* M=InkUiAssets::Load<UMaterialInterface>(nullptr,*(TEXT("/Game/UI/Ink/Materials/Atlas/")+N+TEXT(".")+N),nullptr,LOAD_NoWarn|LOAD_Quiet))B.SetResourceObject(M);
            else B.SetUVRegion(FBox2f(FVector2f(Rect.X/1254.f,Rect.Y/1254.f),FVector2f(Rect.Z/1254.f,Rect.W/1254.f)));
        }
        return B;
    }
    void Place(UCanvasPanel* Canvas,UWidget* Widget,float X,float Y,float W,float H,int32 Z=0)
    {auto* S=Canvas->AddChildToCanvas(Widget);S->SetOffsets(FMargin(X,Y,W,H));S->SetZOrder(Z);}
    void Fill(UOverlaySlot* S){S->SetHorizontalAlignment(HAlign_Fill);S->SetVerticalAlignment(VAlign_Fill);}
    UImage* Image(UWidgetTree* Tree,UTexture2D* Atlas,UCanvasPanel* Canvas,FName Name,FVector4 Rect,float X,float Y,float W,float H)
    {auto* I=Tree->ConstructWidget<UImage>(UImage::StaticClass(),Name);I->SetBrush(Art(Atlas,Rect));I->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Canvas,I,X,Y,W,H);return I;}
    UInkFrontendActionButton* Button(UWidgetTree* Tree,UInkFrontendWidget* Frontend,UTexture2D* Atlas,UCanvasPanel* Canvas,FName Name,FVector4 Rect,UTextBlock* Label,float X,float Y,float W,float H)
    {
        auto* B=Tree->ConstructWidget<UInkFrontendActionButton>(UInkFrontendActionButton::StaticClass(),Name);B->Action=Name;
        FButtonStyle S;auto Brush=Art(Atlas,Rect);S.SetNormal(Brush);auto Hov=Brush;Hov.TintColor=FLinearColor(1.12f,1.06f,.90f,1);S.SetHovered(Hov);S.SetPressed(Hov);Brush.TintColor=FLinearColor(.36f,.36f,.36f,1);S.SetDisabled(Brush);S.SetNormalPadding(FMargin(0));S.SetPressedPadding(FMargin(0));B->SetStyle(S);
        if(Label){Label->SetJustification(ETextJustify::Center);Label->SetAutoWrapText(false);B->SetContent(Label);auto* Layout=Cast<UButtonSlot>(Label->Slot);Layout->SetVerticalAlignment(VAlign_Center);Layout->SetHorizontalAlignment(HAlign_Fill);Layout->SetPadding(FMargin(2));}
        Place(Canvas,B,X,Y,W,H);B->BindFrontend(Frontend);return B;
    }
    FText CategoryLabel(FName Category)
    {
        if(Category==TEXT("Equipment"))return LOCTEXT("Equipment","装备");
        if(Category==TEXT("Material"))return LOCTEXT("Material","材料");
        if(Category==TEXT("Consumable"))return LOCTEXT("Pill","丹药");
        if(Category==TEXT("Treasure"))return LOCTEXT("Treasure","法宝");
        return LOCTEXT("All","全部");
    }
    FVector4 ItemIcon(const FInkInventoryItem& Item)
    {
        if(Item.Category==TEXT("Treasure"))return FVector4(669,802,815,944);
        if(Item.Category==TEXT("Equipment"))
        {
            if(Item.EquipmentSlot==TEXT("Armor")||Item.EquipmentSlot==TEXT("Clothes"))return FVector4(188,802,335,944);
            if(Item.EquipmentSlot==TEXT("Ring"))return FVector4(349,802,497,944);
            if(Item.EquipmentSlot==TEXT("Necklace"))return FVector4(511,802,657,944);
            return FVector4(28,802,172,944);
        }
        return Item.Category==TEXT("Consumable")?CategoryIcons[3]:CategoryIcons[2];
    }
}

UInkInventoryModel* UInkFrontendWidget::GetInventoryModel()
{
    if(!InventoryModel){InventoryModel=NewObject<UInkInventoryModel>(this);InventoryModel->SetUnlockedSlotCount(DefaultUnlockedInventorySlots);InventoryModel->OnChanged.AddUniqueDynamic(this,&UInkFrontendWidget::HandleInventoryModelChanged);}
    return InventoryModel;
}

void UInkFrontendWidget::SetInventoryItems(const TArray<FInkInventoryItem>& Items)
{const int32 Rejected=GetInventoryModel()->SetItems(Items,false);SelectedInventoryEntry.Invalidate();HoveredInventoryEntry.Invalidate();InventoryPageIndex=0;RefreshInventoryFromModel();if(Rejected>0)SetPageFeedback(FText::Format(LOCTEXT("ImportOverflow","背包空间不足，{0} 件物品未能放入"),Rejected));}

int32 UInkFrontendWidget::AddInventoryItem(const FInkInventoryItem& Item,int32 Quantity)
{const int32 Added=GetInventoryModel()->AddItem(Item,Quantity);RefreshInventoryFromModel();return Added;}

void UInkFrontendWidget::SetInventoryUnlockedSlots(int32 Count)
{GetInventoryModel()->SetUnlockedSlotCount(Count);RefreshInventoryFromModel();}

void UInkFrontendWidget::RefreshInventoryFromModel(){if(WidgetTree)RefreshInventory();}

void UInkFrontendWidget::HandleInventoryModelChanged(){if(!bInventoryReadingHover)bInventoryModelDirty=true;}

FReply UInkFrontendWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& KeyEvent)
{
    if(CurrentPage==TEXT("Inventory")&&KeyEvent.GetKey()==EKeys::Escape)
    {
        if(SelectedInventoryEntry.IsValid()){SelectedInventoryEntry.Invalidate();RemoveInventoryWindow(TEXT("InventoryDetailPanel"));RefreshInventory();}
        else TriggerAction(TEXT("Back"));
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(Geometry,KeyEvent);
}

void UInkFrontendWidget::RebuildInventoryPage(UWidgetTree* Tree)
{
    if(!Tree)return;
    auto* Page=InkInventoryUI::Find<UCanvasPanel>(Tree,TEXT("Inventory"));
    if(Page)
    {
        TArray<UWidget*> Old;
        TFunction<void(UWidget*)> Collect=[&](UWidget* W){if(auto* P=Cast<UPanelWidget>(W))for(auto* C:P->GetAllChildren()){Collect(C);Old.Add(C);}};
        Collect(Page);Page->ClearChildren();
        for(auto* W:Old)W->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);
    }
    else
    {
        auto* Switcher=InkInventoryUI::Find<UWidgetSwitcher>(Tree,TEXT("PageSwitcher"));if(!Switcher)return;
        Page=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("Inventory"));
        auto* S=Cast<UWidgetSwitcherSlot>(Switcher->AddChild(Page));S->SetHorizontalAlignment(HAlign_Fill);S->SetVerticalAlignment(VAlign_Fill);
    }
    BuildInventorySurface(Tree,Page);
}

void UInkFrontendWidget::BuildInventorySurface(UWidgetTree* Tree,UCanvasPanel* Page)
{
    using namespace InkInventoryUI;
    auto* Atlas=ProvidedInventoryAtlas.LoadSynchronous();
    auto* Scale=Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(),TEXT("InventoryScale"));Scale->SetStretch(EStretch::ScaleToFit);
    auto* Outer=Page->AddChildToCanvas(Scale);Outer->SetAnchors(FAnchors(0,0,1,1));Outer->SetOffsets(FMargin(24));
    auto* DesignSize=Tree->ConstructWidget<USizeBox>();DesignSize->SetWidthOverride(1440);DesignSize->SetHeightOverride(950);Scale->SetContent(DesignSize);
    auto* Canvas=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("InventoryShell"));DesignSize->SetContent(Canvas);
    auto Image=[&](UCanvasPanel* C,FName Name,FVector4 Rect,float X,float Y,float W,float H)
    {auto* I=Tree->ConstructWidget<UImage>(UImage::StaticClass(),Name);I->SetBrush(Art(Atlas,Rect));I->SetVisibility(ESlateVisibility::HitTestInvisible);Place(C,I,X,Y,W,H);return I;};
    auto Label=[&](UCanvasPanel* C,FName Name,const FText& Text,int32 Font,float X,float Y,float W,float H)
    {auto* L=AddText(Tree,nullptr,Text,Font,Name);Place(C,L,X,Y,W,H);return L;};
    auto Button=[&](UCanvasPanel* C,FName Name,FVector4 Rect,const FText& Text,float X,float Y,float W,float H)
    {
        auto* B=Tree->ConstructWidget<UInkFrontendActionButton>(UInkFrontendActionButton::StaticClass(),Name);B->Action=Name;
        FButtonStyle S;auto NormalBrush=Art(Atlas,Rect);S.SetNormal(NormalBrush);auto Highlight=NormalBrush;Highlight.TintColor=FLinearColor(1.12f,1.06f,.90f,1);S.SetHovered(Highlight);S.SetPressed(Highlight);
        auto Disabled=NormalBrush;Disabled.TintColor=FLinearColor(.36f,.36f,.36f,1);S.SetDisabled(Disabled);S.SetNormalPadding(FMargin(0));S.SetPressedPadding(FMargin(0));B->SetStyle(S);
        // This atlas already contains the button lettering. Keep the original art.
        Place(C,B,X,Y,W,H);B->BindFrontend(this);return B;
    };
    Image(Canvas,TEXT("InventoryMainFrame"),Main,20,20,1020,1020.f*549.f/663.f);
    auto* TopClose=Button(Canvas,TEXT("InventoryCloseIcon"),CloseNormal,FText::GetEmpty(),956,182,44,44);
    auto CS=TopClose->GetStyle();CS.SetHovered(Art(Atlas,CloseHover));CS.SetPressed(Art(Atlas,ClosePressed));TopClose->SetStyle(CS);
    Label(Canvas,TEXT("InventoryCount"),FText::GetEmpty(),18,520,202,180,34);
    Button(Canvas,TEXT("InventoryGrid"),FVector4(1037,1043,1102,1111),FText::GetEmpty(),724,198,38,40);
    Button(Canvas,TEXT("InventoryList"),FVector4(1104,1043,1166,1111),FText::GetEmpty(),780,198,38,40);
    Button(Canvas,TEXT("InventorySortDirection"),FVector4(1173,1043,1211,1111),FText::GetEmpty(),836,198,30,40);
    auto* Scroll=Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("InventoryScroll"));Scroll->SetScrollBarVisibility(ESlateVisibility::Collapsed);Place(Canvas,Scroll,74,242,888,364);
    auto ScrollStyle=Scroll->GetWidgetStyle();ScrollStyle.SetTopShadowBrush(FSlateNoResource());ScrollStyle.SetBottomShadowBrush(FSlateNoResource());Scroll->SetWidgetStyle(ScrollStyle);
    Image(Canvas,TEXT("InventoryScrollTrack"),FVector4(1198,584,1238,951),968,242,34,350);
    auto* Handle=Tree->ConstructWidget<UInkInventoryScrollBar>(UInkInventoryScrollBar::StaticClass(),TEXT("InventoryScrollThumb"));
    Handle->TargetScrollBox=Scroll;Handle->SetThickness(FVector2D(24,24));Handle->SetPadding(FMargin(0));
    FScrollBarStyle HandleStyle;
    const FSlateNoResource NoRail;
    HandleStyle.SetVerticalBackgroundImage(NoRail);HandleStyle.SetVerticalTopSlotImage(NoRail);HandleStyle.SetVerticalBottomSlotImage(NoRail);
    auto HandleArt=Art(Atlas,FVector4(1158,685,1186,804));
    HandleStyle.SetNormalThumbImage(HandleArt);HandleStyle.SetHoveredThumbImage(HandleArt);HandleStyle.SetDraggedThumbImage(HandleArt);
    Handle->SetWidgetStyle(HandleStyle);Place(Canvas,Handle,973,300,24,228);
    auto* Grid=Tree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(),TEXT("InventoryItemList"));Grid->SetSlotPadding(FMargin(3));Cast<UScrollBoxSlot>(Scroll->AddChild(Grid))->SetHorizontalAlignment(HAlign_Center);
    Label(Canvas,TEXT("InventoryEmptyState"),FText::GetEmpty(),20,106,550,800,30);
    for(int32 I=0;I<5;++I)
    {
        const FName N(*(TEXT("Filter")+Categories[I].ToString()));auto* B=Button(Canvas,N,Tabs[I],FText::GetEmpty(),92+I*174,610,166,50);
        auto* Content=Tree->ConstructWidget<UCanvasPanel>();B->SetContent(Content);
        InkInventoryUI::Image(Tree,Atlas,Content,FName(*(TEXT("InventoryCategoryIcon_")+Categories[I].ToString())),CategoryIcons[I],10,8,28,34);
        if(auto* BS=Cast<UButtonSlot>(Content->Slot)){BS->SetHorizontalAlignment(HAlign_Fill);BS->SetVerticalAlignment(VAlign_Fill);BS->SetPadding(FMargin(0));}
    }
    Button(Canvas,TEXT("InventoryPrevious"),FVector4(17,1041,222,1110),LOCTEXT("Previous","上一页"),96,665,174,48);
    auto* PageLabel=Label(Canvas,TEXT("InventoryPageNumber"),FText::GetEmpty(),22,290,675,174,36);PageLabel->SetJustification(ETextJustify::Center);
    Button(Canvas,TEXT("InventoryNext"),FVector4(229,1041,434,1110),LOCTEXT("Next","下一页"),484,665,174,48);
    Button(Canvas,TEXT("InventoryMultiSelect"),FVector4(840,864,906,933),FText::GetEmpty(),758,672,34,34);
    Label(Canvas,TEXT("InventoryMultiSelectLabel"),LOCTEXT("MultiSelect","多选"),20,808,677,130,34);
    Button(Canvas,TEXT("InventorySell"),Actions[2],LOCTEXT("Sell","出售"),145,735,200,60);
    Button(Canvas,TEXT("InventoryDecompose"),Actions[3],LOCTEXT("Decompose","分解"),435,735,200,60);
    Button(Canvas,TEXT("InventorySort"),Actions[4],LOCTEXT("Sort","整理"),725,735,200,60);
    Label(Canvas,TEXT("InventoryFeedback"),FText::GetEmpty(),18,84,810,922,32)->SetColorAndOpacity(CinnabarColor);
    RefreshInventory(Tree);
}
void UInkFrontendWidget::RemoveInventoryWindow(FName Name)
{
    if(Name==TEXT("InventoryDetailPanel")&&WidgetTree)if(auto* Dim=WidgetTree->FindWidget(TEXT("InventoryDetailDim"))){Dim->RemoveFromParent();Dim->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);}
    if(!WidgetTree)return;auto* Window=WidgetTree->FindWidget(Name);if(!Window)return;
    TArray<UWidget*> Old;TFunction<void(UWidget*)> Collect=[&](UWidget* W){if(auto* P=Cast<UPanelWidget>(W))for(auto* C:P->GetAllChildren()){Collect(C);Old.Add(C);}};
    Collect(Window);Window->RemoveFromParent();Old.Add(Window);for(auto* W:Old)W->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);
    BindTree();
}

void UInkFrontendWidget::EnsureInventoryDetailWindow()
{
    using namespace InkInventoryUI;if(!WidgetTree||Find<UCanvasPanel>(WidgetTree,TEXT("InventoryDetailPanel")))return;
    auto* Shell=Find<UCanvasPanel>(WidgetTree,TEXT("InventoryShell"));if(!Shell)return;auto* Tree=WidgetTree.Get();auto* Atlas=ProvidedInventoryAtlas.LoadSynchronous();
    if(bInventoryPortrait){auto* Dim=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("InventoryDetailDim"));Dim->SetBrushColor(FLinearColor(0,0,0,.58f));Place(Shell,Dim,0,0,720,1220,4);}
    auto* Panel=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("InventoryDetailPanel"));Place(Shell,Panel,bInventoryPortrait?109:1070,bInventoryPortrait?152:112,334,610,5);
    if(bInventoryPortrait){Panel->SetRenderTransformPivot(FVector2D::ZeroVector);Panel->SetRenderScale(FVector2D(1.5f));}
    Panel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    Image(Tree,Atlas,Panel,TEXT("InventoryDetailFrame"),Detail,0,0,320,320.f*513.f/271.f);
    Image(Tree,Atlas,Panel,TEXT("InventoryDetailIcon"),Empty,42,82,88,88);
    auto Label=[&](FName N,const FText& T,int32 Font,float X,float Y,float W,float H)
    {auto* L=AddText(Tree,nullptr,T,Font,N);Place(Panel,L,X,Y,W,H);return L;};
    auto Make=[&](FName N,FVector4 R,const FText& T,float X,float Y,float W,float H)
    {return Button(Tree,this,Atlas,Panel,N,R,nullptr,X,Y,W,H);};
    Label(TEXT("InventoryDetailName"),FText::GetEmpty(),23,150,78,144,60);
    Label(TEXT("InventoryDetailType"),FText::GetEmpty(),20,150,141,144,32);
    Label(TEXT("InventoryDetailCount"),FText::GetEmpty(),18,36,174,270,28);
    Label(TEXT("InventoryDescriptionHeading"),LOCTEXT("Description","物品描述"),20,36,223,250,30);
    auto* Description=Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("InventoryDescriptionScroll"));Place(Panel,Description,42,266,238,136);Description->AddChild(AddText(Tree,nullptr,FText::GetEmpty(),20,TEXT("InventoryDetail")));
    auto* QuantityFrame=Image(Tree,Atlas,Panel,TEXT("InventoryQuantityFrame"),FVector4(91,1185,238,1242),70,420,180,34);
    QuantityFrame->SetBrush(Art(Atlas,FVector4(91,1185,238,1242),true));
    Make(TEXT("InventoryQuantityMinus"),FVector4(32,1185,91,1242),FText::GetEmpty(),30,420,34,34);
    Make(TEXT("InventoryQuantityPlus"),FVector4(240,1185,294,1242),FText::GetEmpty(),256,420,34,34);
    Label(TEXT("InventoryQuantity"),FText::GetEmpty(),21,70,422,180,32)->SetJustification(ETextJustify::Center);
    const FVector4 Badges[]={FVector4(847,759,916,831),FVector4(918,759,987,831),FVector4(991,759,1061,831),FVector4(1062,757,1137,833)};
    const TCHAR* Presets[]={TEXT("1"),TEXT("10"),TEXT("99"),TEXT("Max")};
    for(int32 I=0;I<4;++I)
    {
        const FString S=Presets[I];auto* B=Make(FName(*(TEXT("InventoryQuantityPreset_")+S)),Badges[I],I==3?LOCTEXT("AllQuantity","全"):FText::FromString(S),35+I*64,462,44,44);
        auto* Radio=Tree->ConstructWidget<UImage>(UImage::StaticClass(),FName(*(TEXT("InventoryQuantityRadio_")+S)));Radio->SetBrush(Art(Atlas,FVector4(988,864,1059,933)));Radio->SetVisibility(ESlateVisibility::HitTestInvisible);
        Place(Panel,Radio,81+I*64,477,14,14);
    }
    Make(TEXT("InventoryUse"),Actions[0],LOCTEXT("Use","使用"),16,512,138,40);
    Make(TEXT("InventoryEquip"),Actions[1],LOCTEXT("Equip","装备"),166,512,138,40);
    auto* CloseLabel=Make(TEXT("InventoryClose"),Actions[5],LOCTEXT("CloseDetails","关闭"),75,559,170,38);if(auto* Text=Cast<UTextBlock>(CloseLabel->GetContent()))Text->SetColorAndOpacity(PaperColor);
    BindTree();
}

void UInkFrontendWidget::EnsureInventoryTooltipWindow()
{
    using namespace InkInventoryUI;if(!WidgetTree||Find<UCanvasPanel>(WidgetTree,TEXT("InventoryTooltip")))return;
    auto* Shell=Find<UCanvasPanel>(WidgetTree,TEXT("InventoryShell"));if(!Shell)return;auto* Tree=WidgetTree.Get();auto* Atlas=ProvidedInventoryAtlas.LoadSynchronous();
    auto* Tip=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("InventoryTooltip"));Place(Shell,Tip,0,0,310,310.f*212.f/262.f,20);Tip->SetVisibility(ESlateVisibility::HitTestInvisible);
    Image(Tree,Atlas,Tip,TEXT("InventoryTooltipFrame"),Tooltip,0,0,310,310.f*212.f/262.f);
    Image(Tree,Atlas,Tip,TEXT("InventoryTooltipIcon"),Empty,36,42,76,76);
    auto Label=[&](FName N,int32 Font,float X,float Y,float W,float H){auto* L=AddText(Tree,nullptr,FText::GetEmpty(),Font,N);L->SetColorAndOpacity(PaperColor);Place(Tip,L,X,Y,W,H);return L;};
    Label(TEXT("InventoryTooltipName"),22,128,36,162,56);Label(TEXT("InventoryTooltipType"),20,128,92,162,30);
    Label(TEXT("InventoryTooltipDescription"),20,24,150,264,78)->SetClipping(EWidgetClipping::ClipToBounds);
}

void UInkFrontendWidget::HandleInventorySearchChanged(const FText& Text)
{InventorySearchText=Text.ToString();InventoryPageIndex=0;HoveredInventoryEntry.Invalidate();RefreshInventory();}

void UInkFrontendWidget::RefreshInventory(UWidgetTree* SourceTree)
{
    using namespace InkInventoryUI;
    UWidgetTree* Tree=SourceTree?SourceTree:WidgetTree.Get();auto* Grid=Find<UUniformGridPanel>(Tree,TEXT("InventoryItemList"));if(!Grid)return;
    bInventoryModelDirty=false;
    auto* Model=GetInventoryModel();auto* Atlas=ProvidedInventoryAtlas.LoadSynchronous();
    for(auto It=InventoryBatchSelection.CreateIterator();It;++It)if(!Model->FindEntry(*It))It.RemoveCurrent();
    // Detach the old cells before constructing stable names used by Blueprint and automation.
    TArray<UWidget*> Old;TFunction<void(UWidget*)> Collect=[&](UWidget* W){if(auto* P=Cast<UPanelWidget>(W))for(auto* C:P->GetAllChildren()){Collect(C);Old.Add(C);}};
    Collect(Grid);Grid->ClearChildren();for(auto* W:Old)W->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);
    const FString Query=InventorySearchText.TrimStartAndEnd();
    TArray<int32> Slots;const bool bFiltered=InventoryCategory!=TEXT("All")||!Query.IsEmpty();
    if(!bFiltered){for(int32 I=0;I<Model->GetTotalSlotCount();++I)Slots.Add(I);}
    else for(const auto& E:Model->GetEntries())
    {
        if(InventoryCategory!=TEXT("All")&&E.Item.Category!=InventoryCategory)continue;
        if(!Query.IsEmpty()&&!E.Item.Name.ToString().Contains(Query)&&!E.Item.ItemId.ToString().Contains(Query))continue;
        Slots.Add(E.SlotIndex);
    }
    const bool bNoMatches=bFiltered&&Slots.IsEmpty();
    if(bFiltered)
    {
        Slots.Sort();
        // Category views keep a complete page of empty cells even when no
        // matching entry exists. Negative indices never refer to model entries.
        const int32 PaddedCount=FMath::Max(SlotsPerPage,FMath::DivideAndRoundUp(Slots.Num(),SlotsPerPage)*SlotsPerPage);
        while(Slots.Num()<PaddedCount)Slots.Add(-1-Slots.Num());
    }
    const int32 Pages=FMath::Max(1,FMath::DivideAndRoundUp(Slots.Num(),SlotsPerPage));InventoryPageIndex=FMath::Clamp(InventoryPageIndex,0,Pages-1);
    Grid->SetMinDesiredSlotWidth(bInventoryListView?850:88);Grid->SetMinDesiredSlotHeight(bInventoryListView?76:86);
    const int32 Start=InventoryPageIndex*SlotsPerPage,End=FMath::Min(Start+SlotsPerPage,Slots.Num());
    for(int32 N=Start;N<End;++N)
    {
        const int32 Index=Slots[N];const auto* Entry=Model->FindEntryAtSlot(Index);const bool bLocked=Index>=Model->GetUnlockedSlotCount()||(Index<0&&N>=Model->GetUnlockedSlotCount());
        auto* Button=Tree->ConstructWidget<UInkFrontendActionButton>(UInkFrontendActionButton::StaticClass(),FName(*FString::Printf(TEXT("InventorySlot_%d"),Index)));Button->Action=FName(*FString::Printf(TEXT("InventorySelect_%d"),Index));
        FButtonStyle S;S.SetNormal(Art(Atlas,bLocked?Locked:Entry?(Entry->EntryId==SelectedInventoryEntry?Selected:Normal):Empty));S.SetHovered(Art(Atlas,Entry?Hover:(bLocked?Locked:Empty)));S.SetPressed(Art(Atlas,Entry?Selected:Empty));S.SetDisabled(Art(Atlas,Locked));S.SetNormalPadding(FMargin(0));S.SetPressedPadding(FMargin(0));Button->SetStyle(S);
        // Locked cells retain the locked artwork and cannot receive pointer input.
        if(bLocked){Button->SetIsEnabled(false);Button->SetVisibility(ESlateVisibility::HitTestInvisible);}
        auto* Content=Tree->ConstructWidget<UCanvasPanel>();Button->SetContent(Content);if(auto* BS=Cast<UButtonSlot>(Content->Slot)){BS->SetPadding(FMargin(0));BS->SetHorizontalAlignment(HAlign_Fill);BS->SetVerticalAlignment(VAlign_Fill);}
        if(bInventoryListView)
        {
            InkInventoryUI::Image(Tree,Atlas,Content,FName(*FString::Printf(TEXT("InventoryListFrame_%d"),Index)),bLocked?Locked:Entry?(Entry->EntryId==SelectedInventoryEntry?Selected:Normal):Empty,0,0,72,72);
            const FSlateColorBrush Clear(FLinearColor::Transparent);S.SetNormal(Clear);S.SetHovered(Clear);S.SetPressed(Clear);S.SetDisabled(Clear);Button->SetStyle(S);
        }
        if(Entry)
        {
            auto* Icon=Tree->ConstructWidget<UImage>(UImage::StaticClass(),FName(*FString::Printf(TEXT("InventoryIcon_%d"),Index)));
            if(auto* T=Entry->Item.Icon.LoadSynchronous())Icon->SetBrushFromTexture(T,true);else Icon->SetBrush(Art(Atlas,ItemIcon(Entry->Item)));Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
            Place(Content,Icon,bInventoryPortrait&&!bInventoryListView?12:9,7,bInventoryListView?52:(bInventoryPortrait?80:64),bInventoryListView?52:(bInventoryPortrait?80:64));
            if(bInventoryListView){auto* Name=AddText(Tree,nullptr,Entry->Item.Name,25);Place(Content,Name,80,18,bInventoryPortrait?360:600,42);}
            auto* New=Tree->ConstructWidget<UImage>(UImage::StaticClass(),FName(*FString::Printf(TEXT("InventoryNew_%d"),Index)));New->SetBrush(Art(Atlas,FVector4(869,666,971,742)));Place(Content,New,0,0,44,32,5);New->SetVisibility(Entry->bIsNew?ESlateVisibility::HitTestInvisible:ESlateVisibility::Collapsed);
            auto* Count=Tree->ConstructWidget<UOverlay>();auto* Badge=Tree->ConstructWidget<UImage>();Badge->SetBrush(Art(Atlas,FVector4(987,669,1107,737)));Fill(Count->AddChildToOverlay(Badge));
            auto* Number=AddText(Tree,nullptr,FText::AsNumber(Entry->Item.Count),bInventoryPortrait?20:(bInventoryListView?16:13));Number->SetColorAndOpacity(PaperColor);Number->SetJustification(ETextJustify::Center);auto* NS=Count->AddChildToOverlay(Number);NS->SetHorizontalAlignment(HAlign_Fill);NS->SetVerticalAlignment(VAlign_Center);Place(Content,Count,bInventoryListView?(bInventoryPortrait?476:750):(bInventoryPortrait?43:26),bInventoryListView?24:(bInventoryPortrait?77:58),56,24,6);Count->SetVisibility(ESlateVisibility::HitTestInvisible);
            if(Entry->bEquipped){auto* Equipped=AddText(Tree,nullptr,LOCTEXT("Equipped","已装备"),bInventoryPortrait?18:(bInventoryListView?15:12));Equipped->SetColorAndOpacity(AccentColor);Place(Content,Equipped,bInventoryListView?(bInventoryPortrait?370:655):4,bInventoryListView?24:(bInventoryPortrait?55:45),74,25,7);}
            if(bInventoryMultiSelect)
            {
                const auto Rect=InventoryBatchSelection.Contains(Entry->EntryId)?FVector4(913,864,980,933):FVector4(840,864,906,933);
                InkInventoryUI::Button(Tree,this,Atlas,Content,FName(*FString::Printf(TEXT("InventoryToggle_%d"),Index)),Rect,nullptr,bInventoryListView?(bInventoryPortrait?526:808):4,bInventoryListView?24:60,20,20);
            }
        }
        auto* Size=Tree->ConstructWidget<USizeBox>();Size->SetWidthOverride(bInventoryListView?(bInventoryPortrait?550:850):(bInventoryPortrait?104:82));Size->SetHeightOverride(bInventoryListView?(bInventoryPortrait?92:72):(bInventoryPortrait?104:82));Size->SetContent(Button);
        const int32 Local=N-Start;auto* GS=Grid->AddChildToUniformGrid(Size,bInventoryListView?Local:Local/InventoryGridColumns,bInventoryListView?0:Local%InventoryGridColumns);GS->SetHorizontalAlignment(HAlign_Center);GS->SetVerticalAlignment(VAlign_Center);Button->BindFrontend(this);
    }
    auto Text=[&](const TCHAR* N,const FText& T){if(auto* L=Find<UTextBlock>(Tree,N))L->SetText(T);};
    Text(TEXT("InventoryCount"),FText::Format(LOCTEXT("Capacity","{0} / {1} 格"),Model->GetOccupiedSlotCount(),Model->GetUnlockedSlotCount()));
    Text(TEXT("InventoryPageNumber"),FText::Format(LOCTEXT("Page","{0} / {1}"),InventoryPageIndex+1,Pages));
    Text(TEXT("InventoryEmptyState"),bNoMatches?LOCTEXT("NoMatch","没有符合条件的物品"):FText::GetEmpty());
    if(auto* B=Find<UInkFrontendActionButton>(Tree,TEXT("InventoryPrevious")))B->SetIsEnabled(InventoryPageIndex>0);
    if(auto* B=Find<UInkFrontendActionButton>(Tree,TEXT("InventoryNext")))B->SetIsEnabled(InventoryPageIndex+1<Pages);
    for(int32 I=0;I<5;++I)if(auto* B=Find<UInkFrontendActionButton>(Tree,*(TEXT("Filter")+Categories[I].ToString())))
    {auto S=B->GetStyle();auto BArt=Art(Atlas,Tabs[I]);BArt.TintColor=InventoryCategory==Categories[I]?FLinearColor(1.12f,1.03f,.82f,1):FLinearColor::White;S.SetNormal(BArt);BArt.TintColor=FLinearColor(1.16f,1.08f,.88f,1);S.SetHovered(BArt);B->SetStyle(S);}
    auto Check=[&](const TCHAR* N,bool Checked,bool Radio=false){if(auto* B=Find<UInkFrontendActionButton>(Tree,N)){auto S=B->GetStyle();auto R=Radio?(Checked?FVector4(1061,864,1134,933):FVector4(988,864,1059,933)):(Checked?FVector4(913,864,980,933):FVector4(840,864,906,933));S.SetNormal(Art(Atlas,R));S.SetHovered(Art(Atlas,R));S.SetPressed(Art(Atlas,R));B->SetStyle(S);}};
    Check(TEXT("InventoryMultiSelect"),bInventoryMultiSelect);
    RefreshInventoryDetails(Tree);if(!SourceTree){BindTree();UpdateInventoryTooltip();}
}

void UInkFrontendWidget::RefreshInventoryDetails(UWidgetTree* Tree)
{
    using namespace InkInventoryUI;auto* Model=GetInventoryModel();const auto* Entry=Model->FindEntry(SelectedInventoryEntry);auto* Atlas=ProvidedInventoryAtlas.LoadSynchronous();
    if(!Entry){SelectedInventoryEntry.Invalidate();if(Tree==WidgetTree.Get())RemoveInventoryWindow(TEXT("InventoryDetailPanel"));}
    else if(Tree==WidgetTree.Get()&&CurrentPage==TEXT("Inventory"))EnsureInventoryDetailWindow();
    if(Entry)
    {
        InventoryActionQuantity=FMath::Clamp(InventoryActionQuantity,1,Entry->Item.Count);
        if(auto* I=Find<UImage>(Tree,TEXT("InventoryDetailIcon"))){if(auto* T=Entry->Item.Icon.LoadSynchronous())I->SetBrushFromTexture(T,true);else I->SetBrush(Art(Atlas,ItemIcon(Entry->Item)));}
        auto Set=[&](const TCHAR* N,const FText& T){if(auto* L=Find<UTextBlock>(Tree,N))L->SetText(T);};
        Set(TEXT("InventoryDetailName"),Entry->Item.Name);Set(TEXT("InventoryDetailType"),CategoryLabel(Entry->Item.Category));Set(TEXT("InventoryDetailCount"),FText::Format(LOCTEXT("Owned","拥有数量：{0}"),Entry->Item.Count));Set(TEXT("InventoryDetail"),Entry->Item.Description);Set(TEXT("InventoryQuantity"),FText::AsNumber(InventoryActionQuantity));
        for(const TCHAR* Preset:{TEXT("1"),TEXT("10"),TEXT("99"),TEXT("Max")})if(auto* Radio=Find<UImage>(Tree,*(FString(TEXT("InventoryQuantityRadio_"))+Preset)))Radio->SetBrush(Art(Atlas,InventoryQuantityPreset==FName(Preset)?FVector4(1061,864,1134,933):FVector4(988,864,1059,933)));
    }
    auto Enable=[&](const TCHAR* N,bool b){if(auto* B=Find<UInkFrontendActionButton>(Tree,N))B->SetIsEnabled(b);};
    Enable(TEXT("InventoryUse"),Entry&&Model->CanUseEntry(Entry->EntryId));Enable(TEXT("InventoryEquip"),Entry&&(Entry->bEquipped||Model->CanEquipEntry(Entry->EntryId)));
    bool CanSell=Entry&&Entry->Item.bSellable&&!Entry->bEquipped,CanDecompose=Entry&&Entry->Item.bDecomposable&&!Entry->bEquipped;
    if(bInventoryMultiSelect&&!InventoryBatchSelection.IsEmpty())
    {CanSell=false;CanDecompose=false;for(auto Id:InventoryBatchSelection)if(const auto* E=Model->FindEntry(Id)){CanSell|=E->Item.bSellable&&!E->bEquipped;CanDecompose|=E->Item.bDecomposable&&!E->bEquipped;}}
    Enable(TEXT("InventorySell"),CanSell);Enable(TEXT("InventoryDecompose"),CanDecompose);
    Enable(TEXT("InventoryQuantityMinus"),Entry&&InventoryActionQuantity>1);Enable(TEXT("InventoryQuantityPlus"),Entry&&InventoryActionQuantity<Entry->Item.Count);
}

void UInkFrontendWidget::HoverInventorySlot(int32 SlotIndex,bool bHovered)
{
    const auto* E=GetInventoryModel()->FindEntryAtSlot(SlotIndex);
    if(E&&bInventoryListView)if(auto* Frame=InkInventoryUI::Find<UImage>(WidgetTree,*FString::Printf(TEXT("InventoryListFrame_%d"),SlotIndex)))Frame->SetBrush(InkInventoryUI::Art(ProvidedInventoryAtlas.LoadSynchronous(),bHovered?InkInventoryUI::Hover:(E->EntryId==SelectedInventoryEntry?InkInventoryUI::Selected:InkInventoryUI::Normal)));
    if(bHovered&&E)
    {HoveredInventoryEntry=E->EntryId;bInventoryReadingHover=true;GetInventoryModel()->MarkSeen(E->EntryId);bInventoryReadingHover=false;if(auto* W=GetWidgetFromName(FName(*FString::Printf(TEXT("InventoryNew_%d"),SlotIndex))))W->SetVisibility(ESlateVisibility::Collapsed);}
    else if(!bHovered&&E&&HoveredInventoryEntry==E->EntryId)HoveredInventoryEntry.Invalidate();
    UpdateInventoryTooltip();
}

void UInkFrontendWidget::UpdateInventoryTooltip()
{
    using namespace InkInventoryUI;if(!WidgetTree)return;
    if(auto* Handle=Find<UInkInventoryScrollBar>(WidgetTree,TEXT("InventoryScrollThumb")))Handle->SynchronizeScrollPosition();
    const auto* E=GetInventoryModel()->FindEntry(HoveredInventoryEntry);const bool bShow=E&&CurrentPage==TEXT("Inventory")&&!(bInventoryPortrait&&SelectedInventoryEntry.IsValid());
    if(!bShow){RemoveInventoryWindow(TEXT("InventoryTooltip"));return;}
    EnsureInventoryTooltipWindow();auto* Tip=Find<UCanvasPanel>(WidgetTree,TEXT("InventoryTooltip"));if(!Tip)return;
    if(auto* L=Find<UTextBlock>(WidgetTree,TEXT("InventoryTooltipName")))L->SetText(E->Item.Name);
    if(auto* L=Find<UTextBlock>(WidgetTree,TEXT("InventoryTooltipType")))L->SetText(CategoryLabel(E->Item.Category));
    if(auto* L=Find<UTextBlock>(WidgetTree,TEXT("InventoryTooltipDescription")))L->SetText(E->Item.Description);
    if(auto* I=Find<UImage>(WidgetTree,TEXT("InventoryTooltipIcon"))){if(auto* T=E->Item.Icon.LoadSynchronous())I->SetBrushFromTexture(T,true);else I->SetBrush(Art(ProvidedInventoryAtlas.LoadSynchronous(),ItemIcon(E->Item)));}
    if(auto* Shell=Find<UCanvasPanel>(WidgetTree,TEXT("InventoryShell")))if(auto* S=Cast<UCanvasPanelSlot>(Tip->Slot))if(FSlateApplication::IsInitialized())
    {const FVector2D P=Shell->GetCachedGeometry().AbsoluteToLocal(FSlateApplication::Get().GetCursorPos());S->SetPosition(FVector2D(FMath::Clamp(P.X+20.,30.,bInventoryPortrait?380.:1090.),FMath::Clamp(P.Y+22.,30.,bInventoryPortrait?900.:650.)));}
}

void UInkFrontendWidget::UpdateInventoryAdaptiveLayout()
{
    using namespace InkInventoryUI;if(!WidgetTree||!bAdaptiveSurfaceLayout)return;
    auto* Safe=GetWidgetFromName(TEXT("SafeCanvas"));auto* Scale=Find<UScaleBox>(WidgetTree,TEXT("InventoryScale"));if(!Safe||!Scale)return;
    const auto SafeSize=Safe->GetCachedGeometry().GetLocalSize();if(SafeSize.X<=0||SafeSize.Y<=0)return;
    // Add the existing search action to the runtime shell without rewriting the authored WBP.
    if(!Find<UEditableTextBox>(WidgetTree,TEXT("InventorySearch")))if(auto* Shell=Find<UCanvasPanel>(WidgetTree,TEXT("InventoryShell")))
    {
        auto* Search=WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(),TEXT("InventorySearch"));
        Search->SetHintText(LOCTEXT("SearchHint","搜索物品"));Search->SetText(FText::FromString(InventorySearchText));
        auto Style=Search->GetWidgetStyle();Style.SetFont(InkUiTypography::MakeFont(EInkUiTextRole::Body));
        FSlateBrush Background;Background.DrawAs=ESlateBrushDrawType::RoundedBox;Background.TintColor=PaperColor;
        Background.OutlineSettings.CornerRadii=FVector4(4,4,4,4);Background.OutlineSettings.RoundingType=ESlateBrushRoundingType::FixedRadius;
        Background.OutlineSettings.Color=InkColor;Background.OutlineSettings.Width=1;
        Style.SetBackgroundImageNormal(Background).SetBackgroundImageHovered(Background).SetBackgroundImageFocused(Background);
        Style.SetForegroundColor(InkColor);Search->SetWidgetStyle(Style);
        Place(Shell,Search,106,192,380,44,2);
        Search->OnTextChanged.AddUniqueDynamic(this,&ThisClass::HandleInventorySearchChanged);
    }
    const bool Portrait=SafeSize.X<SafeSize.Y;
    if(bInventoryLayoutInitialized&&Portrait==bInventoryPortrait)return;
    bInventoryLayoutInitialized=true;bInventoryPortrait=Portrait;InventoryGridColumns=Portrait?4:8;
    if(auto* Size=Cast<USizeBox>(Scale->GetContent())){Size->SetWidthOverride(Portrait?720:1440);Size->SetHeightOverride(Portrait?1220:950);}
    auto Move=[&](const TCHAR* Name,float X,float Y,float W,float H)
    {if(auto* Widget=GetWidgetFromName(Name))if(auto* LayoutSlot=Cast<UCanvasPanelSlot>(Widget->Slot))LayoutSlot->SetOffsets(FMargin(X,Y,W,H));};
    auto* Atlas=ProvidedInventoryAtlas.LoadSynchronous();
    if(auto* Frame=Find<UImage>(WidgetTree,TEXT("InventoryMainFrame")))Frame->SetBrush(Art(Atlas,Main,Portrait));
    Move(TEXT("InventoryMainFrame"),20,20,Portrait?680:1020,Portrait?1170:1020.f*549.f/663.f);
    Move(TEXT("InventoryCloseIcon"),Portrait?626:956,Portrait?94:182,44,44);
    Move(TEXT("InventoryCount"),Portrait?64:520,Portrait?140:202,Portrait?260:180,34);
    Move(TEXT("InventoryGrid"),Portrait?432:724,Portrait?137:198,38,40);Move(TEXT("InventoryList"),Portrait?490:780,Portrait?137:198,38,40);
    Move(TEXT("InventorySortDirection"),Portrait?550:836,Portrait?137:198,30,40);
    Move(TEXT("InventorySearch"),Portrait?64:106,Portrait?184:192,Portrait?560:380,Portrait?64:44);
    Move(TEXT("InventoryScroll"),Portrait?58:74,Portrait?264:242,Portrait?570:888,Portrait?592:364);
    Move(TEXT("InventoryScrollTrack"),Portrait?640:968,Portrait?264:242,34,Portrait?592:350);
    Move(TEXT("InventoryScrollThumb"),Portrait?645:973,Portrait?322:300,24,Portrait?470:228);
    Move(TEXT("InventoryEmptyState"),Portrait?60:106,Portrait?780:550,Portrait?560:800,30);
    for(int32 I=0;I<5;++I)
    {
        const FString Name=TEXT("Filter")+Categories[I].ToString();
        Move(*Name,Portrait?float(58+(I%3)*198):float(92+I*174),Portrait?float(882+(I/3)*60):610,Portrait?188:166,50);
    }
    Move(TEXT("InventoryPrevious"),Portrait?58:96,Portrait?1010:665,174,48);
    Move(TEXT("InventoryPageNumber"),Portrait?247:290,Portrait?1015:675,174,36);
    Move(TEXT("InventoryNext"),Portrait?446:484,Portrait?1010:665,174,48);
    Move(TEXT("InventoryMultiSelect"),Portrait?460:758,Portrait?958:672,34,34);Move(TEXT("InventoryMultiSelectLabel"),Portrait?505:808,Portrait?960:677,130,34);
    Move(TEXT("InventorySell"),Portrait?58:145,Portrait?1080:735,Portrait?176:200,60);
    Move(TEXT("InventoryDecompose"),Portrait?257:435,Portrait?1080:735,Portrait?176:200,60);
    Move(TEXT("InventorySort"),Portrait?456:725,Portrait?1080:735,Portrait?176:200,60);
    Move(TEXT("InventoryFeedback"),Portrait?58:84,Portrait?1150:810,Portrait?580:922,32);
    for(const TCHAR* Name:{TEXT("InventoryCount"),TEXT("InventoryPageNumber"),TEXT("InventoryMultiSelectLabel"),TEXT("InventoryFeedback")})
        if(auto* T=Find<UTextBlock>(WidgetTree,Name)){auto Font=T->GetFont();Font.Size=Portrait?24:(FCString::Strcmp(Name,TEXT("InventoryPageNumber"))==0?22:18);T->SetFont(Font);}
    if(SelectedInventoryEntry.IsValid())RemoveInventoryWindow(TEXT("InventoryDetailPanel"));
    if(CurrentPage==TEXT("Inventory"))RefreshInventory();else bInventoryModelDirty=true;
}

bool UInkFrontendWidget::ExecuteInventoryAction(FName Action)
{
    using namespace InkInventoryUI;const FString A=Action.ToString();
    if(A.StartsWith(TEXT("Filter")))
    {const FName C(*A.Mid(6));for(const auto& Category:Categories)if(C==Category){InventoryCategory=C;InventoryPageIndex=0;HoveredInventoryEntry.Invalidate();RefreshInventory();return true;}return false;}
    if(!A.StartsWith(TEXT("Inventory")))return false;
    auto* Model=GetInventoryModel();
    if(A.StartsWith(TEXT("InventorySelect_")))
    {
        const int32 SlotIndex=FCString::Atoi(*A.Mid(16));const auto* E=Model->FindEntryAtSlot(SlotIndex);
        SelectedInventoryEntry=E?E->EntryId:FGuid();InventoryActionQuantity=1;InventoryQuantityPreset=TEXT("1");RefreshInventoryDetails(WidgetTree);
        for(auto B:ActionButtons)if(B&&B->Action.ToString().StartsWith(TEXT("InventorySelect_")))
        {const int32 Index=FCString::Atoi(*B->Action.ToString().Mid(16));const auto* Other=Model->FindEntryAtSlot(Index);if(Other){const auto Brush=Art(ProvidedInventoryAtlas.LoadSynchronous(),Other->EntryId==SelectedInventoryEntry?Selected:Normal);if(bInventoryListView){if(auto* Frame=Find<UImage>(WidgetTree,*FString::Printf(TEXT("InventoryListFrame_%d"),Index)))Frame->SetBrush(Brush);}else{auto Style=B->GetStyle();Style.SetNormal(Brush);B->SetStyle(Style);}}}
        return true;
    }
    if(Action==TEXT("InventoryCloseIcon")){TriggerAction(TEXT("Back"));return true;}
    if(Action==TEXT("InventoryClose")||Action==TEXT("InventoryDetailClose"))
    {SelectedInventoryEntry.Invalidate();RemoveInventoryWindow(TEXT("InventoryDetailPanel"));RefreshInventory();return true;}
    if(Action==TEXT("InventoryMultiSelect")){bInventoryMultiSelect=!bInventoryMultiSelect;InventoryBatchSelection.Reset();}
    else if(A.StartsWith(TEXT("InventoryToggle_")))
    {
        if(bInventoryMultiSelect)if(const auto* E=Model->FindEntryAtSlot(FCString::Atoi(*A.Mid(16))))
        {if(InventoryBatchSelection.Contains(E->EntryId))InventoryBatchSelection.Remove(E->EntryId);else InventoryBatchSelection.Add(E->EntryId);}
    }
    else if(Action==TEXT("InventoryPrevious"))--InventoryPageIndex;
    else if(Action==TEXT("InventoryNext"))++InventoryPageIndex;
    else if(Action==TEXT("InventoryGrid"))bInventoryListView=false;
    else if(Action==TEXT("InventoryList"))bInventoryListView=true;
    else if(Action==TEXT("InventorySort")||Action==TEXT("InventorySortDirection"))
    {if(Action==TEXT("InventorySortDirection"))bInventorySortAscending=!bInventorySortAscending;Model->SortItems(bInventorySortAscending);InventoryPageIndex=0;}
    else if(Action==TEXT("InventoryQuantityMinus")||Action==TEXT("InventoryQuantityPlus"))
    {InventoryActionQuantity+=Action==TEXT("InventoryQuantityPlus")?1:-1;InventoryQuantityPreset=TEXT("Custom");RefreshInventoryDetails(WidgetTree);return true;}
    else if(A.StartsWith(TEXT("InventoryQuantityPreset_")))
    {InventoryQuantityPreset=FName(*A.Mid(24));if(const auto* E=Model->FindEntry(SelectedInventoryEntry))InventoryActionQuantity=A.EndsWith(TEXT("Max"))?E->Item.Count:FCString::Atoi(*A.Mid(24));RefreshInventoryDetails(WidgetTree);return true;}
    else if(Action==TEXT("InventoryUse")||Action==TEXT("InventorySell")||Action==TEXT("InventoryDecompose")||Action==TEXT("InventoryEquip"))
    {
        if(Model->IsAuthoritative())
        {
            auto* W=GetGameInstance()?GetGameInstance()->GetSubsystem<UWulingPrototypeSubsystem>():nullptr;
            const auto* E=Model->FindEntry(SelectedInventoryEntry);
            if(!W||!E||bInventoryMultiSelect){SetPageFeedback(LOCTEXT("AuthoritySingle","请选中一项物品后提交，批量操作尚未开放"));return true;}
            if(Action==TEXT("InventoryDecompose")){SetPageFeedback(LOCTEXT("AuthorityNoDecompose","此规则档未开放分解"));return true;}
            auto Args=MakeShared<FJsonObject>();Args->SetStringField(TEXT("item_id"),E->Item.ItemId.ToString());Args->SetNumberField(TEXT("count"),FMath::Clamp(InventoryActionQuantity,1,E->Item.Count));
            FString Json;FJsonSerializer::Serialize(Args,TJsonWriterFactory<>::Create(&Json));
            const FString Op=Action==TEXT("InventoryEquip")?(E->bEquipped?TEXT("Unequip"):TEXT("Equip")):(Action==TEXT("InventorySell")?TEXT("Recycle"):(E->Item.ItemId.ToString().StartsWith(TEXT("BOOK_"))?TEXT("LearnMethod"):TEXT("UseProp")));
            SetPageFeedback(FText::FromString(W->SubmitPrototypeCommand(Op,Json)?TEXT("请求已记录，等待服务确认"):W->GetStatus()));return true;
        }
        int32 Changed=0;
        if(bInventoryMultiSelect&&!InventoryBatchSelection.IsEmpty()&&(Action==TEXT("InventorySell")||Action==TEXT("InventoryDecompose")))
        {
            const TArray<FGuid> SelectedIds=InventoryBatchSelection.Array();
            for(auto Id:SelectedIds)if(const auto* E=Model->FindEntry(Id)){const int32 Count=E->Item.Count;Changed+=Action==TEXT("InventorySell")?Model->SellItem(Id,Count):Model->DecomposeItem(Id,Count);}
            InventoryBatchSelection.Reset();
        }
        else if(Action==TEXT("InventoryUse"))Changed=Model->UseItem(SelectedInventoryEntry,InventoryActionQuantity);
        else if(Action==TEXT("InventorySell"))Changed=Model->SellItem(SelectedInventoryEntry,InventoryActionQuantity);
        else if(Action==TEXT("InventoryDecompose"))Changed=Model->DecomposeItem(SelectedInventoryEntry,InventoryActionQuantity);
        else if(const auto* E=Model->FindEntry(SelectedInventoryEntry))Changed=E->bEquipped?Model->UnequipItem(E->EntryId):Model->EquipItem(E->EntryId);
        SetPageFeedback(Changed?LOCTEXT("Applied","操作完成"):LOCTEXT("Unavailable","当前物品无法执行此操作"));
    }
    else return true;
    HoveredInventoryEntry.Invalidate();RefreshInventory();
    if(auto* Scroll=Find<UScrollBox>(WidgetTree,TEXT("InventoryScroll")))Scroll->SetScrollOffset(0);
    return true;
}
#undef LOCTEXT_NAMESPACE
