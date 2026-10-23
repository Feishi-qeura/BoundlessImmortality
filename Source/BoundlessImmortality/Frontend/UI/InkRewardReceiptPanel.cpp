#include "InkRewardReceiptPanel.h"
#include "InkUiTheme.h"
#include "InkUiAssetPaths.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "Dom/JsonObject.h"
#include "Materials/MaterialInterface.h"
#include "Serialization/JsonSerializer.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

namespace
{
    FString ReceiptText(const TSharedPtr<FJsonObject>& Object,const TCHAR* Key)
    {FString Value;if(Object)Object->TryGetStringField(Key,Value);return Value;}
    UCanvasPanelSlot* PlaceReceipt(UCanvasPanel* Parent,UWidget* Widget,float X,float Y,float W,float H)
    {auto* Slot=Parent->AddChildToCanvas(Widget);Slot->SetPosition({X,Y});Slot->SetSize({W,H});return Slot;}
    void SizeReceipt(UWidget* Widget,float X,float Y,float W,float H)
    {if(Widget)if(auto* Slot=Cast<UCanvasPanelSlot>(Widget->Slot)){Slot->SetPosition({X,Y});Slot->SetSize({W,H});}}
}

void UInkRewardReceiptPanel::NativeOnInitialized()
{
    Super::NativeOnInitialized();SetIsFocusable(true);
    Root=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("RewardOverlay"));WidgetTree->RootWidget=Root;
    auto* Backdrop=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("RewardBackdrop"));
    Backdrop->SetBrushColor(FLinearColor(.01f,.02f,.015f,.58f));
    auto* BackdropSlot=Root->AddChildToCanvas(Backdrop);BackdropSlot->SetAnchors(FAnchors(0,0,1,1));BackdropSlot->SetOffsets(FMargin(0));
    Card=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("RewardCard"));
    auto* CardSlot=Root->AddChildToCanvas(Card);CardSlot->SetAnchors(FAnchors(.5f,.5f));CardSlot->SetAlignment({.5f,.5f});CardSlot->SetSize({632,600});
    auto* Paper=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("RewardPaper"));
    FSlateBrush Brush;Brush.DrawAs=ESlateBrushDrawType::RoundedBox;Brush.TintColor=InkUiTypography::Theme()->PaperTint;
    Brush.OutlineSettings.CornerRadii=FVector4(12,12,12,12);Brush.OutlineSettings.Color=InkUiTypography::Theme()->GoldTint;Brush.OutlineSettings.Width=2;
    Brush.OutlineSettings.RoundingType=ESlateBrushRoundingType::FixedRadius;
    if(auto* Material=InkUiAssets::Load<UMaterialInterface>(nullptr,TEXT("/Game/UI/Ink/Materials/Shared/M_InkPaperCloud.M_InkPaperCloud"),nullptr,LOAD_NoWarn|LOAD_Quiet))
    {Brush.DrawAs=ESlateBrushDrawType::Image;Brush.SetResourceObject(Material);Brush.TintColor=FLinearColor::White;}
    Paper->SetBrush(Brush);Paper->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* PaperSlot=Card->AddChildToCanvas(Paper);PaperSlot->SetAnchors(FAnchors(0,0,1,1));PaperSlot->SetOffsets(FMargin(0));
    auto Label=[&](FName Name,EInkUiTextRole Role,int32 Size)
    {auto* Text=WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);Text->SetFont(InkUiTypography::MakeFont(Role,Size));Text->SetColorAndOpacity(InkUiTypography::Theme()->InkTint);Text->SetAutoWrapText(true);return Text;};
    Title=Label(TEXT("RewardTitle"),EInkUiTextRole::Display,44);PlaceReceipt(Card,Title,24,18,580,64);
    Source=Label(TEXT("RewardSource"),EInkUiTextRole::Body,26);PlaceReceipt(Card,Source,24,88,580,48);
    auto* Line=WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("RewardRule"));Line->SetBrushColor(InkUiTypography::Theme()->GoldTint);Rule=Line;PlaceReceipt(Card,Rule,24,140,584,1);
    Items=WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("RewardItems"));Items->SetAllowOverscroll(false);Items->SetScrollbarThickness({6,6});Items->SetClipping(EWidgetClipping::ClipToBoundsAlways);PlaceReceipt(Card,Items,24,156,584,330);
    Note=Label(TEXT("RewardConfirmedNote"),EInkUiTextRole::Secondary,22);Note->SetText(FText::FromString(TEXT("已存入 · 关闭返回原页面")));PlaceReceipt(Card,Note,24,510,380,64);
    Close=WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(),TEXT("RewardClose"));
    FButtonStyle Style=Close->GetStyle();FSlateBrush ButtonBrush;ButtonBrush.DrawAs=ESlateBrushDrawType::RoundedBox;
    ButtonBrush.TintColor=InkUiTypography::Theme()->GoldTint;ButtonBrush.OutlineSettings.CornerRadii=FVector4(6,6,6,6);
    Style.SetNormal(ButtonBrush);auto Hover=ButtonBrush;Hover.TintColor=FLinearColor(.68f,.46f,.20f,1);Style.SetHovered(Hover);Style.SetPressed(Hover);Close->SetStyle(Style);
    auto* CloseText=Label(TEXT("RewardCloseText"),EInkUiTextRole::Button,28);CloseText->SetText(FText::FromString(TEXT("关闭")));CloseText->SetJustification(ETextJustify::Center);
    Close->AddChild(CloseText);CastChecked<UButtonSlot>(CloseText->Slot)->SetVerticalAlignment(VAlign_Center);CastChecked<UButtonSlot>(CloseText->Slot)->SetHorizontalAlignment(HAlign_Center);
    Close->OnClicked.AddDynamic(this,&ThisClass::CloseReceipt);PlaceReceipt(Card,Close,456,500,152,88);
    Populate();
}

void UInkRewardReceiptPanel::SetReceiptJson(const FString& Json)
{
    Receipt.Reset();ReceiptId.Reset();bClosed=false;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Receipt);
    ReceiptId=ReceiptText(Receipt,TEXT("receipt_id"));Populate();
}

void UInkRewardReceiptPanel::Populate()
{
    if(!Items)return;
    Items->ClearChildren();Items->ScrollToStart();
    Title->SetText(FText::FromString(TEXT("获得奖励")));
    Source->SetText(FText::FromString(TEXT("来源 · ")+ReceiptText(Receipt,TEXT("source_label"))));
    const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;if(!Receipt||!Receipt->TryGetArrayField(TEXT("items"),Rows))return;
    int32 Index=0;
    for(const auto& Value:*Rows)
    {
        if(!Value.IsValid()||Value->Type!=EJson::Object)continue;
        const auto Item=Value->AsObject();double Quantity=0;if(!Item||!Item->TryGetNumberField(TEXT("quantity"),Quantity)||Quantity<=0)continue;
        const FString Id=ReceiptText(Item,TEXT("item_id"));
        auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),FName(*FString::Printf(TEXT("RewardRow_%d"),Index++)));
        CastChecked<UScrollBoxSlot>(Items->AddChild(Row))->SetPadding(FMargin(0,8,0,12));
        auto* IconSize=WidgetTree->ConstructWidget<USizeBox>();IconSize->SetWidthOverride(80);IconSize->SetHeightOverride(88);
        auto* Icon=WidgetTree->ConstructWidget<UImage>();Icon->SetVisibility(ESlateVisibility::HitTestInvisible);IconSize->SetContent(Icon);
        FString IconPath=TEXT("/Game/UI/Ink/MaterialInstances/Inventory/MI_T_ProvidedInventory_972_359_1062_454_Fit");
        if(Id.StartsWith(TEXT("EQ_")))IconPath=TEXT("/Game/UI/Ink/MaterialInstances/Inventory/MI_T_ProvidedInventory_28_802_172_944_Fit");
        else if(Id.StartsWith(TEXT("FABAO"))||Id.StartsWith(TEXT("COSMETIC")))IconPath=TEXT("/Game/UI/Ink/MaterialInstances/Inventory/MI_T_ProvidedInventory_669_802_815_944_Fit");
        else if(Id.StartsWith(TEXT("PROP")))IconPath=TEXT("/Game/UI/Ink/MaterialInstances/Inventory/MI_T_ProvidedInventory_973_466_1062_564_Fit");
        if(auto* Material=InkUiAssets::Load<UMaterialInterface>(nullptr,*IconPath,nullptr,LOAD_NoWarn|LOAD_Quiet))Icon->SetBrushFromMaterial(Material);
        Row->AddChildToHorizontalBox(IconSize)->SetPadding(FMargin(0,0,18,0));
        auto* Details=WidgetTree->ConstructWidget<UVerticalBox>();auto* DetailsSlot=Row->AddChildToHorizontalBox(Details);DetailsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));DetailsSlot->SetVerticalAlignment(VAlign_Center);
        auto* Name=WidgetTree->ConstructWidget<UTextBlock>();Name->SetText(FText::FromString(ReceiptText(Item,TEXT("name"))));Name->SetFont(InkUiTypography::MakeFont(EInkUiTextRole::Body,28));Name->SetColorAndOpacity(InkUiTypography::Theme()->InkTint);Name->SetAutoWrapText(true);Details->AddChildToVerticalBox(Name);
        auto* Count=WidgetTree->ConstructWidget<UTextBlock>();Count->SetText(FText::FromString(FString::Printf(TEXT("× %s"),*FText::AsNumber(Quantity).ToString())));Count->SetFont(InkUiTypography::MakeFont(EInkUiTextRole::Data,27));Count->SetColorAndOpacity(InkUiTypography::Theme()->GoldTint);Details->AddChildToVerticalBox(Count)->SetPadding(FMargin(0,8,0,0));
    }
}

void UInkRewardReceiptPanel::Layout(const FVector2D& Size)
{
    if(!Card)return;const float W=FMath::Min(680.f,FMath::Max(240.f,float(Size.X)-48.f));
    const float H=FMath::Min(640.f,FMath::Max(320.f,float(Size.Y)-48.f));
    CastChecked<UCanvasPanelSlot>(Card->Slot)->SetSize({W,H});
    SizeReceipt(Title,24,18,W-48,64);SizeReceipt(Source,24,88,W-48,48);SizeReceipt(Rule,24,140,W-48,1);
    SizeReceipt(Items,24,156,W-48,FMath::Max(48.f,H-280));SizeReceipt(Note,24,H-108,FMath::Max(80.f,W-224),88);
    Note->SetText(FText::FromString(W<480?TEXT("已存入"):TEXT("已存入 · 关闭返回原页面")));
    SizeReceipt(Close,W-176,H-112,152,88);
}

void UInkRewardReceiptPanel::NativeTick(const FGeometry& Geometry,float DeltaTime)
{
    Super::NativeTick(Geometry,DeltaTime);const auto Size=Geometry.GetLocalSize();
    if(!Size.Equals(LastSize,.5f)&&Size.X>0&&Size.Y>0){LastSize=Size;Layout(Size);}
}

void UInkRewardReceiptPanel::CloseReceipt()
{
    if(bClosed)return;bClosed=true;OnClosed.Broadcast();
}

FReply UInkRewardReceiptPanel::NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if(Event.GetKey()==EKeys::Escape){CloseReceipt();return FReply::Handled();}
    return Super::NativeOnKeyDown(Geometry,Event);
}
