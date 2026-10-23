#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InkUiStyleEditorLibrary.generated.h"
class UFont;
class UFontFace;

UCLASS()
class BOUNDLESSIMMORTALITYEDITOR_API UInkUiStyleEditorLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    /** Creates only new, tool-owned assets; existing faces/fonts are retained. */
    UFUNCTION(BlueprintCallable,Category="Ink UI|Editor") static TArray<UObject*> ImportRefactorFonts();
    /** Exact missing-glyph ranges, calculated from source cmap tables by the import tool. */
    UFUNCTION(BlueprintCallable,Category="Ink UI|Editor") static bool SetMissingGlyphRanges(UFont* Font,UFontFace* Face,const TArray<int32>& InclusivePairs);
    /** Font references only. Does not replace WidgetTree or touch unrelated defaults. */
    UFUNCTION(BlueprintCallable,Category="Ink UI|Editor") static FString MigrateRefactorTypography();
    /** Explicit UI-only relocation for native CDO-referenced assets; retains redirectors. */
    UFUNCTION(BlueprintCallable,Category="Ink UI|Editor") static bool RelocateUiAsset(UObject* Asset,const FString& DestinationPackage);
    UFUNCTION(BlueprintCallable,Category="Ink UI|Editor") static void FixUiRedirectors();
};
