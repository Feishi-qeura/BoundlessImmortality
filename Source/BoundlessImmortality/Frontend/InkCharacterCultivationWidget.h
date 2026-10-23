#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Tickable.h"
#include "InkCharacterCultivationWidget.generated.h"

class UNiagaraSystemWidget;
class UCanvasPanel;
class UImage;
class UMaterialInstanceDynamic;
class UInkFrontendWidget;

UENUM(BlueprintType)
enum class EInkDantianStage : uint8 { Qi, Foundation, Crystal, GoldenCore, NascentSoul };

USTRUCT(BlueprintType)
struct FInkCharacterSlotEffect
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector2D Position=FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FVector2D FrameSize=FVector2D(100,104);
    UPROPERTY(EditAnywhere,BlueprintReadWrite) FLinearColor Color=FLinearColor::White;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bUnlocked=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bEquipped=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 QualityTier=0;
};

namespace InkCharacterCultivation
{
    BOUNDLESSIMMORTALITY_API EInkDantianStage StageForRealm(int32 Realm);
    BOUNDLESSIMMORTALITY_API FLinearColor StageColor(EInkDantianStage Stage);
    BOUNDLESSIMMORTALITY_API FVector2D BorderPoint(FVector2D Center,FVector2D Size,float Angle);
}

/** CPU NiagaraUI particles on slot borders, with an independent realm-colored dantian. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkCharacterCultivationWidget : public UUserWidget, public FTickableGameObject
{
    GENERATED_BODY()
public:
    void Configure(UInkFrontendWidget* Owner,FVector2D Size,const TArray<FInkCharacterSlotEffect>& Nodes,
        FVector2D Target,int32 Realm,FLinearColor Color,float Speed,float ParticleSize,float Radius);
    UFUNCTION(BlueprintPure) TArray<UNiagaraSystemWidget*> GetEffects()const;
    UFUNCTION(BlueprintPure) EInkDantianStage GetDantianStage()const{return InkCharacterCultivation::StageForRealm(RealmIndex);}
    UFUNCTION(BlueprintPure) FLinearColor GetDantianColor()const{return CoreColor;}
    virtual void Tick(float DeltaTime)override;
    virtual bool IsTickable()const override;
    virtual bool IsTickableWhenPaused()const override{return true;}
    virtual UWorld* GetTickableGameObjectWorld()const override{return GetWorld();}
    virtual TStatId GetStatId()const override;
protected:
    virtual TSharedRef<SWidget> RebuildWidget()override;
    virtual void NativeDestruct()override;
    virtual int32 NativePaint(const FPaintArgs&,const FGeometry&,const FSlateRect&,FSlateWindowElementList&,int32,const FWidgetStyle&,bool)const override;
private:
    UPROPERTY(Transient) TArray<TObjectPtr<UNiagaraSystemWidget>> Borders;
    UPROPERTY(Transient) TArray<TObjectPtr<UImage>> Frames;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> FrameMaterials;
    UPROPERTY(Transient) TObjectPtr<UNiagaraSystemWidget> Core;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Canvas;
    UPROPERTY(Transient) TObjectPtr<UImage> Glow;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> GlowMaterial;
    UPROPERTY(Transient) TWeakObjectPtr<UInkFrontendWidget> Frontend;
    TArray<FInkCharacterSlotEffect> Sources;
    FVector2D Extent=FVector2D(620,600),Dantian=FVector2D(310,380);
    FLinearColor CoreColor=FLinearColor(.08f,.42f,1,1);
    int32 RealmIndex=0;
    float BorderSpeed=.42f,MoteSize=2.6f,CoreRadius=18;
    bool bRequested=false;
    mutable double LastPaint=-1;
    void StopEffects();
    void UpdateGeometry();
};
