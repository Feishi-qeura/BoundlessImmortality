#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "LiveWeaponTypes.h"
#include "Frontend/InkProgressState.h"
#include "LiveGMOverlay.generated.h"

class ALiveWeaponSwarm;
class ALiveMonster;
class ULiveAudienceAdapterComponent;
class UTextBlock;
class UVerticalBox;
class ULiveGMOverlayWidget;
class UCultivationSubsystem;
class UTexture2D;
class ULiveGMTestSession;
class ULiveGMControlPanel;

// Opt-in testing overlay for an existing game. It never owns possession, input or cameras.
UCLASS()
class BOUNDLESSIMMORTALITY_API ALiveGMOverlay : public AActor
{
    GENERATED_BODY()
public:
    ALiveGMOverlay();
    UPROPERTY(EditAnywhere) TSubclassOf<ALiveWeaponSwarm> SwarmClass;
    UPROPERTY(EditAnywhere) TSubclassOf<ALiveMonster> MonsterClass;
    UPROPERTY(EditAnywhere) TObjectPtr<UFont> NameFont;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<ULiveAudienceAdapterComponent> Audience;
    UPROPERTY(Transient,VisibleAnywhere,BlueprintReadOnly) TObjectPtr<ALiveWeaponSwarm> Swarm;
    UPROPERTY(Transient,VisibleAnywhere,BlueprintReadOnly) TObjectPtr<ULiveGMOverlayWidget> Panel;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="GM") TObjectPtr<ULiveGMTestSession> TestSession;
    UFUNCTION(BlueprintCallable) bool ExecuteGMAction(FName Action);
    UFUNCTION(BlueprintPure) bool IsLocalSimulation() const;
    FString Feedback;
    virtual void Tick(float DeltaSeconds) override;
protected:
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    TWeakObjectPtr<APawn> Anchor;
    TArray<TWeakObjectPtr<ALiveMonster>> TestMonsters;
    UPROPERTY(Transient) TObjectPtr<ULiveAudienceAdapterComponent> OwnedAudience;
    bool bBorrowingGameplaySystems=false;
};

UCLASS()
class BOUNDLESSIMMORTALITY_API ULiveGMActionButton : public UButton
{
    GENERATED_BODY()
public:
    void BindAction(ALiveGMOverlay* InHost,FName InAction);
    FName Action;
private:
    TWeakObjectPtr<ALiveGMOverlay> Host;
    UFUNCTION() void ApplyAction();
};

UCLASS()
class BOUNDLESSIMMORTALITY_API ULiveGMOverlayWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    void Configure(ALiveGMOverlay* InHost);
    UPROPERTY(Transient,BlueprintReadOnly,Category="GM") TObjectPtr<ULiveGMControlPanel> Controls;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool bPanelOpen=false;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 DrawnNameplates=0;
    UPROPERTY(Transient,BlueprintReadWrite) bool bCollectNameplateDiagnostics=false;
    UPROPERTY(Transient,BlueprintReadOnly) TMap<FString,FVector2D> PaintedNameplateAnchors;
    UPROPERTY(Transient,BlueprintReadOnly) TMap<FString,FVector2D> PaintedRealmBadgeCenters;
    UPROPERTY(Transient,BlueprintReadOnly) TMap<FString,FVector2D> PaintedWeaponRowCenters;
    UPROPERTY(Transient,BlueprintReadOnly) TMap<FString,FVector2D> PaintedRealmRowCenters;
    UPROPERTY(Transient,BlueprintReadOnly) TMap<FString,FVector2D> PaintedNameplateSizes;
    UPROPERTY(Transient,BlueprintReadOnly) TMap<FString,float> PaintedNameplateDistances;
    UPROPERTY(Transient,BlueprintReadOnly) FString Announcement;
    UPROPERTY(Transient,BlueprintReadOnly) float AnnouncementRemaining=0;
    UPROPERTY(Transient,BlueprintReadOnly) bool bDrawCultivationPresentation=true;
    UPROPERTY(Transient) TArray<TObjectPtr<ULiveGMActionButton>> ActionButtons;
    void TogglePanel();
protected:
    virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
    virtual void NativeDestruct()override;
    virtual int32 NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& CullingRect,FSlateWindowElementList& DrawElements,int32 LayerId,const FWidgetStyle& Style,bool bParentEnabled) const override;
private:
    TWeakObjectPtr<ALiveGMOverlay> Host;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Status;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Feedback;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> Body;
    UPROPERTY(Transient) TObjectPtr<class UBorder> DebugPanel;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> RealmBanners;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> RealmChart;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> WeaponTitles;
    TWeakObjectPtr<UCultivationSubsystem> Cultivation;
    mutable FInkProgressState AnchorHealthProgress,AnchorCultivationProgress;
    mutable FInkProgressContext AnchorProgressContext;
    mutable TMap<TWeakObjectPtr<ALiveMonster>,FInkProgressState> MonsterProgress;
    UFUNCTION() void HandleAnnouncement(const FString& Text);
    UTextBlock* MakeText(const FString& Text,int32 Size=13);
};
