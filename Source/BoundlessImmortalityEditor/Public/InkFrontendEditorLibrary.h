#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InputCoreTypes.h"
#include "InkFrontendEditorLibrary.generated.h"
class UInkFrontendWidget;
class UTexture2D;
class ULiveGMOverlayWidget;
class UButton;
UCLASS()
class BOUNDLESSIMMORTALITYEDITOR_API UInkFrontendEditorLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static FString VerifyDevelopmentMutationRefusal(UObject* WorldContext);
    UFUNCTION(BlueprintCallable,Category="newUI|Editor") static bool ClickLabeledButton(UInkFrontendWidget* Widget,const FText& Caption);
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool ClickGMAction(ULiveGMOverlayWidget* Widget,FName Action);
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static UObject* CreateFrontendWidgetAsset(const FString& PackagePath,FName PreviewPage);
    /** Creates only the new default asset, with the independent rebuilt parent. */
    UFUNCTION(BlueprintCallable,Category="newUI|Editor") static UObject* CreateRebuiltFrontendWidgetAsset(const FString& PackagePath);
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool RefreshGeneratedFrontendAsset(const FString& PackagePath,FName PreviewPage);
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool ClickAction(UInkFrontendWidget* Widget,FName Action);
    /** Isolated local-test only; follows Slate hit testing for delivered ordinary buttons. */
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool ClickRolloutButton(UInkFrontendWidget* Widget,UButton* Button);
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool PressFrontendKey(UInkFrontendWidget* Widget,FKey Key);
    /** Dispatches without moving focus, for focused editable-text keyboard routing checks. */
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool PressFocusedFrontendKey(FKey Key);
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static FString DescribeWidget(UInkFrontendWidget* Widget);
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static TArray<int32> GetUniformGridColumns(UInkFrontendWidget* Widget,FName Name);
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool SetPIEViewportSize(int32 Width,int32 Height);
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool BeginFrontendPIE(int32 Width,int32 Height);
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool CaptureFrontendViewport(const FString& Filename);
    /** Test-only transient padding inside the existing SafeZone; never saves assets. */
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool SetFrontendSafePadding(UInkFrontendWidget* Widget,FMargin Padding);
    /** Only permitted in the explicit isolated local-test process. */
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool SetFrontendTestAnchorName(UInkFrontendWidget* Widget,const FString& Name);
    /** Editor-only cached identity fixture. No network, gameplay grants or saved assets. */
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool SetFrontendTestAnchorAvatar(UInkFrontendWidget* Widget,const FString& Url,UTexture2D* Texture);
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool ScrollToFrontendWidget(UInkFrontendWidget* Widget,FName Name);
    /** Inventory-only migration: preserves every authored page outside Inventory. */
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool RefreshInventoryFrontendAsset(const FString& PackagePath);
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool RefreshLeaderboardFrontendAsset(const FString& PackagePath);
    /** Moves the Slate pointer through the same hit-test path as a physical mouse. */
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool HoverAction(UInkFrontendWidget* Widget,FName Action);
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool MoveFrontendPointerOutside(UInkFrontendWidget* Widget);
    /** Focuses the search box, selects existing input, then dispatches Slate character events. */
    UFUNCTION(BlueprintCallable,Category="newUI|Editor") static bool TypeFrontendText(UInkFrontendWidget* Widget,FName Name,const FString& Text);
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool TypeInventorySearch(UInkFrontendWidget* Widget,const FString& Text);
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static FString DescribeInventory(UInkFrontendWidget* Widget);
    /** Routes a press, drag and release to an independently authored scrollbar. */
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool DragInventoryScrollThumb(UInkFrontendWidget* Widget,float FromFraction,float ToFraction);
    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool DragFrontendScrollThumb(UInkFrontendWidget* Widget,FName ThumbName,float FromFraction,float ToFraction);
};
