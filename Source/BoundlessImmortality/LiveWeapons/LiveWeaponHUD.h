#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LiveWeaponTypes.h"
#include "Frontend/InkProgressState.h"
#include "LiveWeaponHUD.generated.h"
class UCultivationSubsystem;
class UTexture2D;
class ALiveMonster;
UCLASS()
class BOUNDLESSIMMORTALITY_API ALiveWeaponHUD : public AHUD
{
    GENERATED_BODY()
public:
    // All participants receive a nameplate; no distance or overlap culling.
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float NameplateScale=.75f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) TObjectPtr<UFont> NameFont;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) int32 DrawnNameplates=0;
    UPROPERTY(Transient,BlueprintReadWrite,Category="Presentation|Diagnostics") bool bCollectNameplateDiagnostics=false;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Presentation|Diagnostics") TMap<FString,FVector2D> PaintedNameplateSizes;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Presentation|Diagnostics") TMap<FString,float> PaintedNameplateDistances;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) bool bGMPanelOpen=true;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FString GMFeedback;
    virtual void DrawHUD()override;
    virtual void NotifyHitBoxClick(FName BoxName)override;
    virtual void EndPlay(const EEndPlayReason::Type Reason)override;
private:
    void DrawGMPanel();
    TArray<FLiveWeaponLabelSample> Samples;
    TArray<int32> VisibleLabels;
    double LayoutReadyAt=0;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> RealmBanners;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> RealmChart;
    UPROPERTY(Transient) TObjectPtr<UTexture2D> WeaponTitles;
    TWeakObjectPtr<UCultivationSubsystem> Cultivation;
    FInkProgressState AnchorHealthProgress,AnchorCultivationProgress;
    FInkProgressContext AnchorProgressContext;
    TMap<TWeakObjectPtr<ALiveMonster>,FInkProgressState> MonsterProgress;
    FString Announcement;
    double AnnouncementUntil=0;
    UFUNCTION() void HandleAnnouncement(const FString& Text);
};
