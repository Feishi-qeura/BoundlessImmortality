#include "InkUiTheme.h"
#include "Frontend/UI/InkUiAssetPaths.h"
#include "Engine/Font.h"
#include "Styling/CoreStyle.h"

UInkUiThemeData::UInkUiThemeData()
{
    PaperTint=FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("F3EBDD")));
    InkTint=FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("202823")));
    GoldTint=FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("B88B45")));
}

const UInkUiThemeData* InkUiTypography::Theme()
{
    static TWeakObjectPtr<UInkUiThemeData> Cached;
    if(!Cached.IsValid())Cached=InkUiAssets::Load<UInkUiThemeData>(nullptr,TEXT("/Game/newUI/Ink/Data/Shared/DA_InkUITheme_v1.DA_InkUITheme_v1"),nullptr,LOAD_NoWarn|LOAD_Quiet);
    return Cached.IsValid()?Cached.Get():GetDefault<UInkUiThemeData>();
}

FSlateFontInfo InkUiTypography::MakeFont(EInkUiTextRole Role,int32 SizeOverride)
{
    const auto* T=Theme();
    UFont* Font=Role==EInkUiTextRole::Display?T->DisplayFont.Get():Role==EInkUiTextRole::Data?T->DataFont.Get():T->BodyFont.Get();
    const int32 Size=SizeOverride>0?SizeOverride:Role==EInkUiTextRole::Display?T->DisplaySize:Role==EInkUiTextRole::Section?T->SectionSize:Role==EInkUiTextRole::Button?T->ButtonSize:Role==EInkUiTextRole::Data?T->DataSize:Role==EInkUiTextRole::Secondary?T->SecondarySize:Role==EInkUiTextRole::HudLabel?T->HudLabelSize:T->BodySize;
    auto Info=FCoreStyle::GetDefaultFontStyle("Regular",Size*T->SlatePointScale);
    if(Font)Info.FontObject=Font;
    return Info;
}
