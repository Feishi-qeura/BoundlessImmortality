#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "InkProgressState.h"
#include "InkAnchorStatusWidget.generated.h"

class UInkFrontendWidget;
class USizeBox;
class UTexture2D;
class UFont;
class UMaterialInstanceDynamic;
class ULiveInteractionSubsystem;
class UCultivationHealthComponent;
class UCanvasPanel;
class UInkNiagaraTitleWidget;
class UInkAnchorStatusWidget;
enum class ECultivationActionState : uint8;

/** Separate background layer so NiagaraUI children always paint above the HUD art. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkAnchorStatusArt : public UUserWidget
{
    GENERATED_BODY()
public:
    TWeakObjectPtr<UInkAnchorStatusWidget> Owner;
protected:
    virtual int32 NativePaint(const FPaintArgs&,const FGeometry&,const FSlateRect&,FSlateWindowElementList&,int32,const FWidgetStyle&,bool)const override;
};

/** Presentation only: looping actions intentionally have no completion percentage. */
struct BOUNDLESSIMMORTALITY_API FInkAnchorActionPresentation
{
    FString Label;
    float Fraction=0;
    bool bActive=false;
    bool bIndeterminate=false;
};

/** Compact local anchor HUD. It never owns gameplay, grants stats, or receives input. */
UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkAnchorStatusWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UInkAnchorStatusWidget(const FObjectInitializer& Initializer);
    UFUNCTION(BlueprintCallable,Category="Ink UI|Anchor") void Configure(UInkFrontendWidget* InFrontend);
    UFUNCTION(BlueprintCallable,Category="Ink UI|Anchor") void RefreshFromWorld();
    /** Presentation hook only. 0/0 means there is no shield; this does not absorb damage. */
    UFUNCTION(BlueprintCallable,Category="Ink UI|Anchor") void SetShieldPresentation(double Current,double Max);
    /** Supply a verified identity texture, or explicitly identify an illustration placeholder. */
    UFUNCTION(BlueprintCallable,Category="Ink UI|Anchor") void SetAvatarPresentation(UTexture2D* Texture,const FString& IdentityUrl,bool bPlaceholder=false);
    UFUNCTION(BlueprintPure,Category="Ink UI|Anchor") FString GetIdentityAvatarUrl()const{return AvatarUrl;}
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ink UI|Anchor") TSoftObjectPtr<UTexture2D> PlaceholderPortrait;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ink UI|Anchor",meta=(ClampMin="0.01",ClampMax="2")) float SmoothingSeconds=.18f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ink UI|Progress") FInkProgressSettings ProgressSettings;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Progress") float HealthTrailFraction=0;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Progress") float CultivationTrailFraction=0;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Progress") float ShieldTrailFraction=0;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Anchor") FString AnchorName;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Anchor") FText RealmName;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Anchor") double Health=0;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Anchor") double MaxHealth=0;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Anchor") double Shield=0;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Anchor") double MaxShield=0;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Anchor") double Cultivation=0;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Anchor") double RequiredCultivation=0;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Anchor") float DisplayedHealthFraction=0;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Anchor") float DisplayedCultivationFraction=0;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Anchor") float DisplayedShieldFraction=0;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Anchor") TObjectPtr<UTexture2D> AvatarTexture;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Anchor") FString ActionLabel;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Anchor") float ActionFraction=0;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Anchor") bool bActionIndeterminate=false;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Anchor") bool bAvatarPlaceholder=true;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Anchor") bool bHasAnchor=false;
    UFUNCTION(BlueprintCallable,Category="Ink UI|Anchor") void ObserveDamage(UCultivationHealthComponent* HealthComponent);
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Anchor") float DamageFlash=0;
    static float SafeFraction(double Current,double Max);
    static FVector2D ResponsiveSize(FVector2D LogicalViewport);
    /** Strict channel inset even at full health; shared with painted geometry checks. */
    static FSlateRect MeterInterior(FSlateRect Track);
    static FSlateRect AvatarFrameBounds(){return FSlateRect(8,4,96,92);}
    static FSlateRect AvatarPortraitBounds(){return FSlateRect(28,24,76,72);}
    static FSlateRect RealmTitleBounds(){return FSlateRect(4,94,156,126);}
    static FSlateRect MeterBounds(int32 Kind){return FSlateRect(154,44+32*Kind,290,56+32*Kind);}
    static FInkAnchorActionPresentation ResolveAction(ECultivationActionState State,float Elapsed,float Duration);
protected:
    virtual TSharedRef<SWidget> RebuildWidget()override;
    virtual void NativeConstruct()override;
    virtual void NativeDestruct()override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds)override;
    virtual int32 NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& CullingRect,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool bParentEnabled)const override;
private:
    friend class UInkAnchorStatusArt;
    int32 PaintStatus(const FPaintArgs&,const FGeometry&,const FSlateRect&,FSlateWindowElementList&,int32,const FWidgetStyle&,bool)const;
    TWeakObjectPtr<UInkFrontendWidget> Frontend;
    TWeakObjectPtr<ULiveInteractionSubsystem> AvatarHost;
    UPROPERTY(Transient) TObjectPtr<USizeBox> DesiredSizeBox;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> EffectsCanvas;
    UPROPERTY(Transient) TObjectPtr<UInkNiagaraTitleWidget> RealmTitle;
    UPROPERTY(Transient) TObjectPtr<UInkNiagaraTitleWidget> AvatarFrame;
    UPROPERTY(Transient) TObjectPtr<UFont> TextFont;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> PortraitFallback;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> PortraitMask;
    TWeakObjectPtr<UTexture2D> MaskedPortrait;
    /** These reflected brushes retain the generated atlas textures across GC. */
    UPROPERTY(Transient) TArray<FSlateBrush> PresentationSurfaces;
    UPROPERTY(Transient) TArray<FSlateBrush> PresentationIcons;
    FString AnchorId,AvatarUrl,RequestedAvatarId,RequestedAvatarUrl;
    bool bAvatarOverride=false,bInitializedValues=false,bActionActive=false,bDead=false;
    double DisplayedHealth=0,DisplayedCultivation=0,DisplayedShield=0;
    double NextAvatarAttempt=0;
    float AnimationTime=0;
    TWeakObjectPtr<UCultivationHealthComponent> ObservedHealth;
    UFUNCTION() void HandleDamage(double Applied,double Maximum);
    int32 LastRealmIndex=INDEX_NONE;
    double LastMaxHealth=0,LastMaxShield=0,LastRequiredCultivation=0;
    FInkProgressState HealthProgress,ShieldProgress,CultivationProgress,ActionProgress;
    UFUNCTION() void HandleAvatarReady(const FString& UserId,UTexture2D* Texture);
    void CachePresentationArt();
};
