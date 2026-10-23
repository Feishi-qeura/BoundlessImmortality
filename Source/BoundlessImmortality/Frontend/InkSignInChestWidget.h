#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Tickable.h"
#include "InkSignInChestWidget.generated.h"
class UNiagaraSystemWidget;
class UInkFrontendWidget;

/** Open art deliberately shows an empty interior. Niagara is presentation only. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkSignInChestWidget : public UUserWidget, public FTickableGameObject
{
    GENERATED_BODY()
public:
    void Configure(UInkFrontendWidget* Frontend,FName Route,bool Open,bool Eligible);
    virtual void Tick(float Delta)override;
    virtual TStatId GetStatId()const override;
    virtual bool IsTickable()const override;
    virtual bool IsTickableWhenPaused()const override{return true;}
    virtual UWorld* GetTickableGameObjectWorld()const override{return GetWorld();}
    UFUNCTION(BlueprintPure) bool IsChestOpen()const{return bOpen;}
    UFUNCTION(BlueprintPure) bool IsGlowActive()const;
    virtual void ReleaseSlateResources(bool bReleaseChildren)override;
protected:
    virtual TSharedRef<SWidget> RebuildWidget()override;
    virtual void NativeConstruct()override;
    virtual void NativeDestruct()override;
    virtual int32 NativePaint(const FPaintArgs&,const FGeometry&,const FSlateRect&,FSlateWindowElementList&,int32,const FWidgetStyle&,bool)const override;
private:
    UPROPERTY(Transient) TObjectPtr<UNiagaraSystemWidget> Glow;
    UPROPERTY(Transient) TWeakObjectPtr<UInkFrontendWidget> OwnerFrontend;
    FName OwnerRoute;
    bool bOpen=false,bEligible=false,bSlateReady=false;
    void StopGlow();
};
