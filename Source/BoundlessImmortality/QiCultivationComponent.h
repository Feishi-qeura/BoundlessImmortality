#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "Containers/Ticker.h"
#include "QiCultivationComponent.generated.h"
class UNiagaraComponent;
class UNiagaraSystem;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMeshComponent;
UENUM(BlueprintType)
enum class EQiElement : uint8
{
    White UMETA(DisplayName="白·炼气"), Fire UMETA(DisplayName="红·火"), Wind UMETA(DisplayName="绿·风"),
    Earth UMETA(DisplayName="黄·土"), Water UMETA(DisplayName="蓝·水"), Ice UMETA(DisplayName="青蓝·冰"), Lightning UMETA(DisplayName="紫·雷")
};
USTRUCT(BlueprintType)
struct FQiOrbSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="真气", meta=(ClampMin="0",ClampMax="64",DisplayName="每轮数量")) int32 Count=8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="真气", meta=(ClampMin="20",ClampMax="2000",Units="cm",DisplayName="生成半径")) float Range=240;
};
/** 修炼视觉控制。本地表现，不授予经验，不同步网络状态。 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(Cultivation), meta=(BlueprintSpawnableComponent))
class BOUNDLESSIMMORTALITY_API UQiCultivationComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UQiCultivationComponent();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼") TObjectPtr<UNiagaraSystem> QiSystem;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼") TObjectPtr<UMaterialInterface> AuraMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼") FKey ToggleKey;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼") bool bEnableKeyboard=true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼") FVector ConvergenceOffset=FVector(0,0,15);
    /** 仅编辑器显示；不会令游戏自动开始修炼。需开启视口实时显示。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|预览", meta=(DisplayName="编辑器预览")) bool bPreviewInEditor=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|真气配置", meta=(DisplayName="白·炼气")) FQiOrbSettings WhiteQi;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|真气配置", meta=(DisplayName="红·火灵根")) FQiOrbSettings FireQi;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|真气配置", meta=(DisplayName="绿·风灵根")) FQiOrbSettings WindQi;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|真气配置", meta=(DisplayName="黄·土灵根")) FQiOrbSettings EarthQi;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|真气配置", meta=(DisplayName="蓝·水灵根")) FQiOrbSettings WaterQi;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|真气配置", meta=(DisplayName="青蓝·冰灵根")) FQiOrbSettings IceQi;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|真气配置", meta=(DisplayName="紫·雷灵根")) FQiOrbSettings LightningQi;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|真气配置", meta=(ClampMin="1",ClampMax="20",Units="s",DisplayName="汇聚时长")) float Duration=5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|真气配置", meta=(ClampMin="0",ClampMax="1000",Units="cm",DisplayName="生成高度")) float SpawnHeight=100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|真气配置", meta=(ClampMin="8",ClampMax="120",Units="cm",DisplayName="光球尺寸")) float OrbSize=44;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|描边", meta=(ClampMin="0",ClampMax="12",Units="cm",DisplayName="火焰外扩大小")) float AuraSize=1.5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|描边", meta=(ClampMin="0.05",ClampMax="3",DisplayName="描边宽度")) float AuraWidth=1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|描边", meta=(ClampMin="0",ClampMax="5",DisplayName="微光强度")) float AuraIntensity=1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="修炼|描边", meta=(ClampMin="0",ClampMax="5",DisplayName="火焰流动速度")) float AuraSpeed=1;
    UPROPERTY(Transient, DuplicateTransient, VisibleAnywhere, BlueprintReadOnly, Category="修炼|状态") TObjectPtr<UNiagaraComponent> Effect;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="修炼|状态") TArray<int32> LiveCounts;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="修炼|状态") bool bCultivating=false;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="修炼|状态") bool bDraining=false;
    /** 开始本地汇聚；重复调用不重置，不执行网络同步。 */
    UFUNCTION(BlueprintCallable, Category="修炼") void StartCultivation();
    /** 停止生成；现有粒子继续吸附，全部结束后描边淡出。不执行 RPC。 */
    UFUNCTION(BlueprintCallable, Category="修炼") void StopCultivation();
    /** 生命周期/编辑器清理：立即释放余粒与描边。 */
    UFUNCTION(BlueprintCallable, Category="修炼") void CancelCultivation();
    /** 本地切换开始/停止，可由输入动作调用，不执行 RPC。 */
    UFUNCTION(BlueprintCallable, Category="修炼") void ToggleCultivation();
    /** 保存一类配置，支持构造脚本与运行时。Type=0白1火2风3土4水5冰6雷；Count=每轮数量，Range=厘米。本地视觉，无 RPC。 */
    UFUNCTION(BlueprintCallable, Category="修炼") void ConfigureQi(int32 Type,int32 Count,float Range);
    /** 设置指定灵根数量并应用，支持构造脚本，不影响其他灵根。本地视觉，无 RPC。 */
    UFUNCTION(BlueprintCallable, Category="修炼|真气配置", meta=(DisplayName="设置真气数量",Keywords="Set Qi Count Quantity")) void SetQiCount(EQiElement Element,int32 Count);
    /** 设置指定灵根半径（厘米），支持构造脚本。本地视觉，无 RPC。 */
    UFUNCTION(BlueprintCallable, Category="修炼|真气配置", meta=(DisplayName="设置真气范围",Keywords="Set Qi Range")) void SetQiRange(EQiElement Element,float Range);
    UFUNCTION(BlueprintPure, Category="修炼|真气配置", meta=(DisplayName="获取真气数量")) int32 GetQiCount(EQiElement Element)const;
    /** 应用配置而不重启粒子；编辑器和运行时均可调用，无 RPC。 */
    UFUNCTION(BlueprintCallable, CallInEditor, Category="修炼|预览", meta=(DisplayName="应用真气配置")) void ApplySettings();
    /** 编辑器中重新开始预览；游戏中不改变修炼状态，无 RPC。 */
    UFUNCTION(BlueprintCallable, CallInEditor, Category="修炼|预览", meta=(DisplayName="重新开始预览")) void RestartPreview();
    /** 本地描边配置：Size=外扩厘米，Width=宽度，Intensity=亮度，Speed=流动速度；支持构造脚本，无 RPC。 */
    UFUNCTION(BlueprintCallable, Category="修炼|描边", meta=(DisplayName="设置修炼描边")) void SetAuraSettings(float Size,float Width,float Intensity,float Speed);
protected:
    virtual void OnRegister() override;
    virtual void OnUnregister() override;
    virtual void PreSave(FObjectPreSaveContext SaveContext) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float Delta,ELevelTick TickType,FActorComponentTickFunction* Function) override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
#endif
private:
#if WITH_EDITOR
    FTSTicker::FDelegateHandle PreviewTicker;
    void TickEditorPreview(float Delta);
#endif
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Aura;
    UPROPERTY(Transient) TArray<TObjectPtr<UMeshComponent>> AuraMeshes;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInterface>> PreviousOverlays;
    float AuraFadeRemaining=0;
    int32 LastParticleTotal=0;
    void AdvanceDrain(float Delta);
    FQiOrbSettings* Settings(int32 Type);
    const FQiOrbSettings* Settings(int32 Type)const;
    void EnsureEffect();
    void ReleaseEffect();
    void AttachAura();
    void UpdateAura();
    bool IsPreviewWorld()const;
};
