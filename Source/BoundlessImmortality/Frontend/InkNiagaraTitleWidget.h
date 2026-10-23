#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "Tickable.h"
#include "InkNiagaraTitleWidget.generated.h"
class UNiagaraSystemWidget;
class UImage;
class UTextBlock;
class UTexture2D;
UENUM(BlueprintType)
enum class EInkNiagaraTitleKind : uint8 { Honor, Realm, RealmIcon, AvatarFrame };

/** Same source artwork and authoritative IDs in HUD, leaderboards and sword labels. */
namespace InkNiagaraTitlePresentation
{
    BOUNDLESSIMMORTALITY_API FSlateBrush HonorBrush(FName TitleId);
    BOUNDLESSIMMORTALITY_API FSlateBrush RealmBrush(int32 RealmIndex);
    BOUNDLESSIMMORTALITY_API FSlateBrush RealmIconBrush(int32 RealmIndex);
    BOUNDLESSIMMORTALITY_API FSlateBrush AvatarFrameBrush();
    BOUNDLESSIMMORTALITY_API int32 QualityForTitleId(FName TitleId);
    BOUNDLESSIMMORTALITY_API int32 QualityForRealmIndex(int32 RealmIndex);
    BOUNDLESSIMMORTALITY_API float ParticleRateForQuality(int32 Quality);
    BOUNDLESSIMMORTALITY_API bool ShouldAnimate(bool Visible,bool ReducedMotion,bool HasPlayer,bool PaintedRecently);
}

/** CPU particles rendered by NiagaraUIRenderer, with static art retained in reduced motion. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkNiagaraTitleWidget : public UUserWidget, public FTickableGameObject
{
    GENERATED_BODY()
public:
    UInkNiagaraTitleWidget(const FObjectInitializer& ObjectInitializer);
    void Configure(EInkNiagaraTitleKind Kind,const FText& Text,int32 Quality,const FSlateBrush& Plaque=FSlateBrush());
    UFUNCTION(BlueprintCallable) void SetReducedMotion(bool Reduced);
    UFUNCTION(BlueprintCallable) void SetPresentationVisible(bool Visible);
    UFUNCTION(BlueprintCallable) void SetEffectStyle(FLinearColor Tint,float Intensity);
    UFUNCTION(BlueprintPure) bool IsEffectActive()const;
    UFUNCTION(BlueprintPure) int32 GetParticleCount()const;
    UFUNCTION(BlueprintPure) UNiagaraSystemWidget* GetEffectWidget()const{return Effect;}
    UFUNCTION(BlueprintPure) int32 GetEffectQuality()const{return Quality;}
    virtual void Tick(float Delta)override;
    virtual TStatId GetStatId()const override;
    virtual bool IsTickable()const override;
    virtual bool IsTickableWhenPaused()const override{return true;}
    virtual UWorld* GetTickableGameObjectWorld()const override{return GetWorld();}
    virtual void ReleaseSlateResources(bool bReleaseChildren)override;
protected:
    virtual TSharedRef<SWidget> RebuildWidget()override;
    virtual void NativeConstruct()override;
    virtual void NativeDestruct()override;
    virtual int32 NativePaint(const FPaintArgs&,const FGeometry&,const FSlateRect&,FSlateWindowElementList&,int32,const FWidgetStyle&,bool)const override;
private:
    UPROPERTY(Transient) TObjectPtr<UNiagaraSystemWidget> Effect;
    UPROPERTY(Transient) TObjectPtr<UImage> Artwork;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Caption;
    UPROPERTY() FSlateBrush Plaque;
    UPROPERTY() FText Label;
    UPROPERTY() EInkNiagaraTitleKind Kind=EInkNiagaraTitleKind::Honor;
    UPROPERTY() int32 Quality=0;
    bool bReducedMotion=false,bPresentationVisible=true,bActivationRequested=false,bSlateReady=false;
    FLinearColor EffectTint=FLinearColor::White;
    float EffectIntensity=1;
    mutable double LastPaintTime=-1;
    mutable FVector2D LastPaintSize=FVector2D(130,30);
    void StopEffect();
    void ApplyConfiguration();
};

/** One projected group owns titles, avatar, frame and name with a single DPI/distance transform. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkNiagaraNameplateWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Configure(FName TitleId,const FText& HonorText,int32 RealmIndex,const FText& RealmText,bool ReducedMotion);
    void SetIdentity(const FString& Id,const FString& DisplayName,UTexture2D* AvatarTexture,int32 AvatarFrame=0);
    void StopEffects();
    void SetEffectStyle(FLinearColor Tint,float Intensity,float RealmAdvancePulse=0);
    TArray<UNiagaraSystemWidget*> GetEffects()const;
    UFUNCTION(BlueprintPure,Category="Presentation|Diagnostics") FString GetParticipantId()const{return ParticipantId;}
    UFUNCTION(BlueprintPure,Category="Presentation|Diagnostics") UImage* GetAvatarImage()const{return Avatar;}
    UFUNCTION(BlueprintPure,Category="Presentation|Diagnostics") UInkNiagaraTitleWidget* GetAvatarEffect()const{return Frame;}
    UFUNCTION(BlueprintPure,Category="Presentation|Diagnostics") UTextBlock* GetIdentityText()const{return Identity;}
protected:
    virtual TSharedRef<SWidget> RebuildWidget()override;
    virtual void NativeDestruct()override;
private:
    UPROPERTY(Transient) TObjectPtr<UInkNiagaraTitleWidget> Honor;
    UPROPERTY(Transient) TObjectPtr<UInkNiagaraTitleWidget> Realm;
    UPROPERTY(Transient) TObjectPtr<UInkNiagaraTitleWidget> Icon;
    UPROPERTY(Transient) TObjectPtr<UInkNiagaraTitleWidget> Frame;
    UPROPERTY(Transient) TObjectPtr<UImage> Avatar;
    UPROPERTY(Transient) TObjectPtr<UImage> AvatarBorder;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Identity;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> CachedAvatar;
    FString ParticipantId,CachedDisplayName;
    int32 CachedAvatarFrame=0;
    bool bIdentityApplied=false;
    FName CachedTitle;
    FText CachedHonor,CachedRealm;
    int32 CachedRealmIndex=0;
    bool bCachedReduced=false;
    FLinearColor CachedTint=FLinearColor::White;
    float CachedIntensity=1,CachedPulse=0;
};
