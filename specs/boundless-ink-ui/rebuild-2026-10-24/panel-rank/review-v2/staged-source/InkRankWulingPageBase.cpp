#include "InkRankWulingPageBase.h"

#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"
#include "Blueprint/WidgetTree.h"

UInkRankWulingCallbackButton::UInkRankWulingCallbackButton()
{
    OnClicked.AddUniqueDynamic(this, &UInkRankWulingCallbackButton::HandleClicked);
}

void UInkRankWulingCallbackButton::HandleClicked()
{
    if (Action)
    {
        Action();
    }
}

void UInkRankWulingPageBase::SetSharedThemeFonts(UFont* InTitleFont, UFont* InBodyFont)
{
    SharedTitleFont = InTitleFont;
    SharedBodyFont = InBodyFont;
    RebuildForThemeChange();
}

bool UInkRankWulingPageBase::EnsureStableRoot()
{
    if (!WidgetTree)
    {
        return false;
    }
    if (StableRoot && Content)
    {
        return true;
    }

    StableRoot = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    if (UVerticalBoxSlot* ContentSlot = StableRoot->AddChildToVerticalBox(Content))
    {
        FSlateChildSize Fill(ESlateSizeRule::Fill);
        ContentSlot->SetSize(Fill);
    }
    StableRoot->SetClipping(EWidgetClipping::ClipToBounds);
    WidgetTree->RootWidget = StableRoot;
    return true;
}

FSlateFontInfo UInkRankWulingPageBase::MakeThemeFont(int32 Size, bool bTitle) const
{
    const UFont* Font = bTitle ? SharedTitleFont.Get() : SharedBodyFont.Get();
    return Font ? FSlateFontInfo(Font, static_cast<float>(Size))
                : FSlateFontInfo(FCoreStyle::GetDefaultFont().ToSharedPtr(), static_cast<float>(Size));
}
