#include "InkRebuildTheme.h"
#include "Frontend/InkFrontendWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Styling/CoreStyle.h"

namespace InkRebuildUI
{
namespace
{
FLinearColor Color(const TCHAR* Hex){return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));}
}
FLinearColor Ink(){return Color(TEXT("1B2925"));}
FLinearColor Muted(){return Color(TEXT("52594F"));}
FLinearColor Paper(){return Color(TEXT("FBF7EE"));}
FLinearColor Background(){return Color(TEXT("F4ECDD"));}
FLinearColor Primary(){return Color(TEXT("173E37"));}
FLinearColor Jade(){return Color(TEXT("326F63"));}
FLinearColor Gold(){return Color(TEXT("9A7334"));}
FLinearColor Warning(){return Color(TEXT("9C4336"));}

FSlateFontInfo Font(ETextRole Role,int32 LogicalSize)
{
    const bool Display=Role==ETextRole::Display||Role==ETextRole::Title;
    const int32 Default=Role==ETextRole::Display?64:Role==ETextRole::Title?48:Role==ETextRole::Section?32:Role==ETextRole::Button?30:Role==ETextRole::Secondary?24:Role==ETextRole::Hud?26:28;
    const int32 Minimum=Display?32:Role==ETextRole::Secondary?24:26;
    const int32 Size=FMath::Max(Minimum,LogicalSize>0?LogicalSize:Default);
    static TWeakObjectPtr<UFont> DisplayFont,BodyFont;
    auto& Cached=Display?DisplayFont:BodyFont;
    if(!Cached.IsValid())Cached=LoadObject<UFont>(nullptr,Display
        ?TEXT("/Game/newUI/Ink/Fonts/Shared/F_InkDisplay.F_InkDisplay")
        :TEXT("/Game/newUI/Ink/Fonts/Shared/F_InkBody.F_InkBody"),nullptr,LOAD_NoWarn|LOAD_Quiet);
    // Existing composite fonts include the Noto CJK missing-glyph fallback; the
    // exact migrated packages are mandatory in engine acceptance before PIE.
    auto Result=FCoreStyle::GetDefaultFontStyle("Regular",FMath::RoundToInt(Size*.75f));
    if(Cached.IsValid())Result.FontObject=Cached.Get();
    return Result;
}

FSlateBrush Surface(FLinearColor Fill,float Radius,bool bOutline)
{
    FSlateBrush Brush;Brush.DrawAs=ESlateBrushDrawType::RoundedBox;Brush.TintColor=Fill;
    Brush.OutlineSettings.CornerRadii=FVector4(Radius,Radius,Radius,Radius);
    Brush.OutlineSettings.RoundingType=ESlateBrushRoundingType::FixedRadius;
    Brush.OutlineSettings.Width=bOutline?1.f:0.f;Brush.OutlineSettings.Color=Color(TEXT("B6A382"));
    return Brush;
}
FButtonStyle ButtonStyle(bool bPrimary,bool bSelected)
{
    FButtonStyle Style;
    Style.SetNormal(Surface(bPrimary||bSelected?Primary():Paper(),4));
    Style.SetHovered(Surface(bPrimary||bSelected?Jade():Background(),4));
    Style.SetPressed(Surface(bPrimary||bSelected?Primary():Background(),4));
    Style.SetDisabled(Surface(Background(),4));
    Style.SetNormalPadding(FMargin(0));Style.SetPressedPadding(FMargin(0,1,0,-1));
    return Style;
}
UCanvasPanelSlot* Place(UCanvasPanel* Parent,UWidget* Child,FVector4 Bounds,int32 Z)
{
    if(!Parent||!Child)return nullptr;
    auto* Layout=Parent->AddChildToCanvas(Child);Layout->SetOffsets(FMargin(Bounds.X,Bounds.Y,Bounds.Z,Bounds.W));Layout->SetZOrder(Z);return Layout;
}
UTextBlock* Text(UWidgetTree* Tree,UCanvasPanel* Parent,FName Name,const FText& Value,ETextRole Role,FVector4 Bounds,int32 LogicalSize,bool bCentered)
{
    if(!Tree||!Parent)return nullptr;
    auto* Label=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);Label->SetText(Value);Label->SetFont(Font(Role,LogicalSize));
    Label->SetColorAndOpacity(Role==ETextRole::Secondary?Muted():Ink());Label->SetAutoWrapText(true);
    Label->SetJustification(bCentered?ETextJustify::Center:ETextJustify::Left);Label->SetVisibility(ESlateVisibility::HitTestInvisible);
    Place(Parent,Label,Bounds,3);return Label;
}
UBorder* Card(UWidgetTree* Tree,UCanvasPanel* Parent,FName Name,FVector4 Bounds,bool bEmphasized)
{
    if(!Tree||!Parent)return nullptr;
    auto* Panel=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),Name);Panel->SetBrush(Surface(bEmphasized?Primary():Paper()));
    Panel->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Parent,Panel,Bounds,0);return Panel;
}
UInkFrontendActionButton* Action(UWidgetTree* Tree,UCanvasPanel* Parent,UInkFrontendWidget* Owner,FName Name,FName ActionId,const FText& Caption,FVector4 Bounds,bool bPrimary,bool bSelected)
{
    if(!Tree||!Parent||!Owner)return nullptr;
    auto* Button=Tree->ConstructWidget<UInkFrontendActionButton>(UInkFrontendActionButton::StaticClass(),Name);
    Button->Action=ActionId;Button->BindFrontend(Owner);Button->SetStyle(ButtonStyle(bPrimary,bSelected));
    auto* CaptionText=Tree->ConstructWidget<UTextBlock>();CaptionText->SetText(Caption);CaptionText->SetFont(Font(ETextRole::Button));
    CaptionText->SetColorAndOpacity(bPrimary||bSelected?Paper():Ink());CaptionText->SetJustification(ETextJustify::Center);CaptionText->SetAutoWrapText(true);
    CaptionText->SetVisibility(ESlateVisibility::HitTestInvisible);Button->SetContent(CaptionText);
    auto* ContentLayout=CastChecked<UButtonSlot>(CaptionText->Slot);ContentLayout->SetHorizontalAlignment(HAlign_Fill);ContentLayout->SetVerticalAlignment(VAlign_Center);ContentLayout->SetPadding(FMargin(12,4));
    Bounds.Z=FMath::Max(Bounds.Z,TouchSize);Bounds.W=FMath::Max(Bounds.W,TouchSize);Place(Parent,Button,Bounds,5);return Button;
}
}
