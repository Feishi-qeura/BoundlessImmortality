#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "InkRankWulingPageBase.generated.h"

class UFont;
class UVerticalBox;

/** A native button whose dynamic click event dispatches a retained native action. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkRankWulingCallbackButton final : public UButton
{
    GENERATED_BODY()
public:
    UInkRankWulingCallbackButton();
    void SetAction(TFunction<void()>&& InAction) { Action = MoveTemp(InAction); }
private:
    UFUNCTION()
    void HandleClicked();
    TFunction<void()> Action;
};

/** Shared stable root and injected newUI typography for the six Rank/Wuling pages. */
UCLASS(Abstract)
class BOUNDLESSIMMORTALITY_API UInkRankWulingPageBase : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="newUI|Theme")
    void SetSharedThemeFonts(UFont* InTitleFont, UFont* InBodyFont);
    FSlateFontInfo MakeThemeFont(int32 Size, bool bTitle = false) const;
protected:
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> StableRoot = nullptr;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> Content = nullptr;
    UPROPERTY(Transient) TObjectPtr<UFont> SharedTitleFont = nullptr;
    UPROPERTY(Transient) TObjectPtr<UFont> SharedBodyFont = nullptr;

    bool EnsureStableRoot();
    virtual void RebuildForThemeChange() PURE_VIRTUAL(UInkRankWulingPageBase::RebuildForThemeChange,);
};
