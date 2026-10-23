#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LiveWeaponCombatModel.h"
#include "LiveWeaponTrajectory.h"
#include "Cultivation/CultivationNameplate.h"
#include "LiveWeaponSwarm.generated.h"
class UInstancedStaticMeshComponent;
class ULiveTitleEffectsComponent;
class ULiveWeaponDefinition;
class ULiveWeaponSkill;
class ALiveMonster;
class APawn;
UENUM(BlueprintType)
enum class ELiveWeaponFlightPhase:uint8 { Wander,Hover,Strike,Drift,Returning,Alert };
/** Local visual demonstration. Gameplay remains the default for every existing swarm. */
UENUM(BlueprintType)
enum class ELiveWeaponPresentationMode:uint8 { Gameplay,Idle,Alert };
struct FLiveWeaponVisualState
{
    using EPhase=ELiveWeaponFlightPhase;
    FVector Position=FVector::ZeroVector,Velocity=FVector::ZeroVector;
    FLiveBezierFlight Path;
    float Age=0,OrbitAngle=0;
    float HoldRemaining=0;
    float RecoveryRemaining=0;
    float ReferenceFloorZ=0,PathFloorStartZ=0,PathFloorEndZ=0;
    float RingRadiusOffset=0,RingHeightOffset=0;
    float RadiusDriftPhase=0,HeightDriftPhase=0,RadiusDriftRate=0,HeightDriftRate=0;
    // Independent, smooth tempo. Baseline is never multiplied cumulatively.
    float TempoBias=0,TempoPhase=0,TempoFrequency=1;
    float SpeedScale(double Time)const{return FMath::Clamp(1.f+TempoBias+.25f*FMath::Sin(float(Time)*TempoFrequency+TempoPhase),.5f,1.5f);}
    bool bNeedsRecovery=false;
    bool bIdleEvacuating=false;
    FVector HoverOrigin=FVector::ZeroVector,FollowReference=FVector::ZeroVector;
    FQuat RenderRotation=FQuat::Identity,IdleRotation=FQuat::Identity;
    EPhase Phase=EPhase::Wander;
    bool bPathValid=false,bContinueCombo=false,bPendingArrow=false;
    int32 InstanceIndex=0;
    int32 ComboHits=0;
    uint32 AttackToken=0;
    FRandomStream Random;
    TWeakObjectPtr<ALiveMonster> Target;
    TArray<TWeakObjectPtr<ALiveMonster>,TInlineAllocator<4>> HitTargets;
    FName SkillId;
};
struct FLiveArrowVisual
{
    FLiveBezierFlight Path;
    FVector Position=FVector::ZeroVector,Velocity=FVector::ZeroVector;
    FLiveWeaponHandle Owner;
    uint32 AttackToken=0;
    TWeakObjectPtr<ALiveMonster> Target;
    FName SkillId;
    float Age=0,AfterHit=0;
    bool bActive=false,bHit=false;
};
UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API ALiveWeaponSwarm : public AActor
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Presentation|Nameplates") FCultivationNameplateDistanceSettings NameplateDistanceSettings;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Presentation|Titles") TObjectPtr<ULiveTitleEffectsComponent> TitleEffects;
    ALiveWeaponSwarm();
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="预警圆环",meta=(DisplayName="最小半径",ClampMin="1",Units="cm",ToolTip="预警圆环目标位置的最小水平半径，单位厘米。每把剑灵在最小与最大半径之间独立缓慢变化；两值相等时关闭径向变化。实际跟随位置仍受返回过程和人物避让约束。")) float AlertRadiusMin=200;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="预警圆环",meta=(DisplayName="最大半径",ClampMin="1",Units="cm",ToolTip="预警圆环目标位置的最大水平半径，单位厘米。调大使圆环分布向外扩展；若小于最小半径，运行时会自动按两值排序。")) float AlertRadiusMax=260;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="预警圆环",meta=(DisplayName="最低高度",Units="cm",ToolTip="预警圆环目标位置相对角色中心的最低高度，单位厘米。每把剑灵在最低与最高高度之间独立缓慢变化，差值就是圆环的竖直厚度；最终仍遵守模型安全高度。")) float AlertHeightMin=100;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="预警圆环",meta=(DisplayName="最高高度",Units="cm",ToolTip="预警圆环目标位置相对角色中心的最高高度，单位厘米。两高度相等时去掉上下分布变化；若填反，运行时自动排序。")) float AlertHeightMax=160;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="待机活动",meta=(DisplayName="最小活动半径",ClampMin="1",Units="cm",ToolTip="待机出生点和闲游目标的最小水平半径，单位厘米。与最大活动半径共同决定待机散布范围，不再随剑灵人数自动扩大；目标与路径还会执行人物避让。")) float IdleRadiusMin=170;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="待机活动",meta=(DisplayName="最大活动半径",ClampMin="1",Units="cm",ToolTip="待机出生点和闲游目标的最大水平半径，单位厘米。调大使剑灵散布更远；这是目标区间，飞行曲线、人物避让和移动跟随过程可能超出该区间。")) float IdleRadiusMax=679.2836f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="待机活动",meta=(DisplayName="最低活动高度",Units="cm",ToolTip="待机目标相对角色中心的最低高度，单位厘米。每把剑灵独立选取目标，通过连续曲线移动；低于模型安全高度的目标会被抬高。")) float IdleHeightMin=90;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="待机活动",meta=(DisplayName="最高活动高度",Units="cm",ToolTip="待机目标相对角色中心的最高高度，单位厘米。与最低活动高度共同决定上下散布；曲线拱起和人物屏幕避让可能将实际位置抬到目标区间之外。")) float IdleHeightMax=400;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="待机避让",meta=(DisplayName="启用人物屏幕避让",ToolTip="为待机出生点、闲游目标及路径检查当前镜头下的角色屏幕区域，并计入武器尺寸。与世界空间的 XYZ 避让同时生效；镜头或角色移动时会重新检查。无本地视口时仅执行世界空间避让。")) bool bAvoidCharacterOnScreen=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="待机避让",meta=(DisplayName="人物屏幕留白",ClampMin="0",ToolTip="角色屏幕包围范围外额外保留的边距，单位像素。调大可让待机剑灵在画面上离角色更远；武器模型的投影尺寸会额外计入。")) float CharacterScreenPadding=12;
    virtual void Serialize(FArchive& Ar)override;
    virtual void PostLoad()override;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ToolTip="武器配置列表。引用剑、斧、枪、弓各自的 DA_Weapon 数据资产；模型、伤害、攻击范围、飞行参数和技能从对应武器类型的配置读取。建议每种武器类型只配置一份。")) TArray<TObjectPtr<ULiveWeaponDefinition>> Definitions;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,meta=(ToolTip="武器实例化网格组件列表，依次对应剑、斧、枪、弓。用于批量绘制剑灵；模型和材质请优先在 Definitions 引用的数据资产中配置。")) TArray<TObjectPtr<UInstancedStaticMeshComponent>> WeaponMeshes;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,meta=(ToolTip="箭矢实例池的网格组件。弓型剑灵发射的箭矢由此批量绘制；这是运行时表现组件。")) TObjectPtr<UInstancedStaticMeshComponent> ArrowMesh;
    UPROPERTY(BlueprintReadWrite,Category="Compatibility",meta=(DeprecatedProperty,DeprecationMessage="旧配置仅用于加载迁移，请使用预警圆环和待机活动中的最小/最大区间。",ToolTip="旧版兼容字段，仅用于将旧资产转换为新的最小与最大区间。")) float FormationRadius=170;
    UPROPERTY(BlueprintReadWrite,Category="Compatibility",meta=(DeprecatedProperty,DeprecationMessage="旧配置仅用于加载迁移，请使用预警圆环和待机活动中的最小/最大区间。",ToolTip="旧版兼容字段，仅用于将旧资产转换为新的最小与最大区间。")) float FloatHeight=170;
    // Legacy asset value retained; the visual transforms now update every rendered frame.
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(DeprecatedProperty,DeprecationMessage="该参数已停用，武器表现现在逐帧更新。",ToolTip="旧版表现刷新频率，单位次/秒。该参数已停用，仅保留旧资产兼容；当前武器位置和朝向逐帧更新，修改此值不会改变刷新频率。")) float PresentationHz=30;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="待机活动",meta=(DisplayName="最短悬停时间",ClampMin="1",ClampMax="5",Units="s",ToolTip="闲游悬停时长下限，单位秒。每次悬停在此值与 Idle Hold Max 之间随机取值；调大可延长最短停留时间，配置范围为 1 至 5 秒。")) float IdleHoldMin=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="待机活动",meta=(DisplayName="最长悬停时间",ClampMin="1",ClampMax="5",Units="s",ToolTip="闲游悬停时长上限，单位秒。与 Idle Hold Min 共同决定每次随机悬停时间；调大可增加较长停留，建议不小于下限，配置范围为 1 至 5 秒。")) float IdleHoldMax=5;
    UPROPERTY(BlueprintReadWrite,Category="Compatibility",meta=(DeprecatedProperty,DeprecationMessage="旧配置仅用于加载迁移，请使用预警圆环和待机活动中的最小/最大区间。",ToolTip="旧版兼容字段，仅用于将旧资产转换为新的最小与最大区间。")) float IdleHeightSpread=300;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="待机活动",meta=(DisplayName="跟随刷新距离",ToolTip="悬停时刷新跟随路径的角色移动距离，单位厘米。角色相对上次跟随参考点超过此距离后重新闲游寻位；调小会更频繁地响应移动，运行时最低按 30 厘米计算。")) float FollowRefreshDistance=150;
    // World-space ellipsoid radii around the character, in cm; independent of the camera.
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="待机避让",meta=(DisplayName="人物避让半径 XYZ",ClampMin="1",UIMin="100",UIMax="250",Units="cm",ToolTip="人物周围避让椭球的 X、Y、Z 半径，单位厘米，按世界坐标轴计算。调大使闲游和警戒目标位置离人物更远；这是空间避让范围，不是随镜头缩放的屏幕矩形。")) FVector CharacterAvoidanceExtent=FVector(150,150,150);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="预警圆环",meta=(DisplayName="预警检测范围",ClampMin="0",Units="cm",ToolTip="警戒查询的基础范围，单位厘米。附近存在存活怪物时剑灵进入警戒；最终查询范围会取此值、Default Attack Range 和各武器 Attack Range 的最大值，因此设为 0 不会单独关闭警戒。")) float AlertDetectionRange=1500;
    UPROPERTY(BlueprintReadWrite,Category="Compatibility",meta=(DeprecatedProperty,DeprecationMessage="旧配置仅用于加载迁移，请使用预警圆环和待机活动中的最小/最大区间。",ToolTip="旧版兼容字段，仅用于将旧资产转换为新的最小与最大区间。")) float AlertRingRadius=230;
    // Full radial width and full vertical thickness of the orbit band.
    UPROPERTY(BlueprintReadWrite,Category="Compatibility",meta=(DeprecatedProperty,DeprecationMessage="旧配置仅用于加载迁移，请使用预警圆环和待机活动中的最小/最大区间。",ToolTip="旧版兼容字段，仅用于将旧资产转换为新的最小与最大区间。")) float AlertRingWidth=60;
    UPROPERTY(BlueprintReadWrite,Category="Compatibility",meta=(DeprecatedProperty,DeprecationMessage="旧配置仅用于加载迁移，请使用预警圆环和待机活动中的最小/最大区间。",ToolTip="旧版兼容字段，仅用于将旧资产转换为新的最小与最大区间。")) float AlertRingThickness=60;
    UPROPERTY(BlueprintReadWrite,Category="Compatibility",meta=(DeprecatedProperty,DeprecationMessage="旧配置仅用于加载迁移，请使用预警圆环和待机活动中的最小/最大区间。",ToolTip="旧版兼容字段，仅用于将旧资产转换为新的最小与最大区间。")) float AlertRingHeight=130;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="预警圆环",meta=(DisplayName="环绕基准角速度",ClampMin="0",Units="deg/s",ToolTip="警戒环绕的基准角速度，单位度/秒。调大使剑灵更快绕人物旋转；各剑灵还叠加独立的 0.5 至 1.5 倍动态速度。设为 0 停止推进环绕角度，仍可跟随移动中的角色。")) float AlertOrbitSpeed=75;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Combat",meta=(ClampMin="0",Units="s",ToolTip="攻击返回后的警戒恢复时间下限，单位秒。剑灵返回人物附近后才推进恢复计时，恢复完成才能再次出击；与 Post Return Alert Max 共同决定随机时长。")) float PostReturnAlertMin=2;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Combat",meta=(ClampMin="0",Units="s",ToolTip="攻击返回后的警戒恢复时间上限，单位秒。调大可能延长两轮出击之间的停留；建议不小于 Post Return Alert Min，实际等待还受返回距离和攻击间隔影响。")) float PostReturnAlertMax=4;
    // Mathematical height margin above character/target foot reference, without scene queries.
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Flight",meta=(ClampMin="0",Units="cm",ToolTip="飞行轨迹额外保留的高度余量，单位厘米。实际安全高度还包含武器模型尺寸；调大可让剑灵飞得更高。高度参考来自角色脚底和目标位置，不执行场景地形射线检测。")) float FlightHeightMargin=12;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Presentation",meta=(ToolTip="是否让武器和箭矢实例投射阴影。在游戏开始时应用；关闭可减少大量剑灵的阴影渲染开销，但会失去对应阴影。")) bool bWeaponCastShadows=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ToolTip="找不到对应武器数据资产时使用的默认攻击范围，单位厘米。正常情况下优先使用该武器的 Attack Range；此值也参与警戒查询范围的最大值计算。")) float DefaultAttackRange=800;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,meta=(ToolTip="运行时累计成功命中次数，包含首击和连击追加命中。仅供观察统计，不是攻击次数配置。")) int64 TotalAttacks=0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,meta=(ToolTip="运行时累计发起的首轮攻击次数，包括近战出击和成功发射箭矢；发起攻击不代表已经命中。")) int64 TotalPrimaryLaunched=0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,meta=(ToolTip="运行时累计连击追加命中次数，不包含每轮攻击的第一次命中。用于观察追击效果。")) int64 TotalFollowUpHits=0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,meta=(ToolTip="运行时观察到的单轮最大连击命中数，包含首击。它是统计结果；连击上限请在对应武器数据资产的 Max Combo Hits 中调整。")) int32 MaxComboHitsObserved=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ToolTip="是否自动寻找目标并发起新的攻击。关闭后不再自动出击，但已经发起的攻击、连击、返回和闲游仍会继续推进。")) bool bAutomaticAttacks=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ToolTip="是否绘制攻击命中盒的调试线框，便于检查飞行攻击判定范围。仅影响调试显示，不会创建真实碰撞组件，也不会改变伤害。")) bool bDrawAttackHitboxes=false;
    UFUNCTION(BlueprintCallable) void SetAnchor(APawn* Pawn);
    UFUNCTION(BlueprintCallable,Category="剑灵|菜单演示") void SetPresentationMode(ELiveWeaponPresentationMode Mode);
    UFUNCTION(BlueprintPure,Category="剑灵|菜单演示") ELiveWeaponPresentationMode GetPresentationMode()const{return PresentationMode;}
    // Only consumed outside Gameplay; existing maps retain their original trajectories and transforms.
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="剑灵|菜单演示",meta=(ClampMin="0",ClampMax="1")) float PresentationPathScale=1.f;
    UPROPERTY(Transient) FRotator PresentationMeshRotation=FRotator::ZeroRotator;
    UPROPERTY(Transient) FVector PresentationMeshCenter=FVector::ZeroVector;
    UPROPERTY(Transient) bool bConstrainPresentationHeight=false;
    /** Relative to anchor. The full rotated mesh, not only its center, stays in this band. */
    UPROPERTY(Transient) FVector2D PresentationHeightRange=FVector2D(-70,85);
    FLiveWeaponHandle JoinParticipant(const FLiveParticipantProfile& Profile);
    UFUNCTION(BlueprintCallable) bool AddParticipant(const FLiveParticipantProfile& Profile){return JoinParticipant(Profile).IsValid();}
    UFUNCTION(BlueprintCallable) bool SwitchParticipantWeapon(const FString& Id,ELiveWeaponType Type);
    UFUNCTION(BlueprintCallable) bool GrantSkill(const FString& Id,FName SkillId){return Combat.GrantSkill(Id,SkillId);}
    UFUNCTION(BlueprintCallable) void ResetParticipants();
    UFUNCTION(BlueprintCallable) bool RemoveParticipant(const FString& Id);
    UFUNCTION(BlueprintPure) int32 GetParticipantCount()const;
    UFUNCTION(BlueprintPure) FString GetProgressionOwnerId(const FString& Id)const;
    UFUNCTION(BlueprintPure) FString GetPersonalSwordId()const{return PersonalSwordId;}
    UFUNCTION(BlueprintPure) int32 GetRenderedCount()const;
    UFUNCTION(BlueprintPure) int32 GetActiveArrowCount()const{return Arrows.Num()-FreeArrows.Num();}
    /** Actual outgoing attacks, excluding menu demonstrations, idle and alert flight. */
    UFUNCTION(BlueprintPure,Category="Combat") bool HasActiveAttacks()const;
    UFUNCTION(BlueprintPure) FVector GetWeaponPosition(const FString& Id)const;
    UFUNCTION(BlueprintPure) FVector GetWeaponVelocity(const FString& Id)const;
    UFUNCTION(BlueprintPure) float GetIdleHoldRemaining(const FString& Id)const;
    UFUNCTION(BlueprintPure) ELiveWeaponFlightPhase GetWeaponPhase(const FString& Id)const;
    UFUNCTION(BlueprintPure) float GetRecoveryRemaining(const FString& Id)const;
    UFUNCTION(BlueprintPure) float GetWeaponSpeedMultiplier(const FString& Id)const;
    UFUNCTION(BlueprintPure) float GetAttackFrequencyMultiplier(const FString& Id)const;
    UFUNCTION(BlueprintPure) float GetMinimumFollowSpeed()const{return PlayerFollowSpeed;}
    UFUNCTION(BlueprintPure) bool IsInsideCharacterReadabilityArea(FVector Position,float Padding=1)const;
    UFUNCTION(BlueprintPure,Category="剑灵|待机避让") bool IsInsideCharacterScreenArea(FVector Position)const;
    FBox2D GetCharacterScreenBounds(class APlayerController* PC)const;
    static FVector2D KeepNameplateOutside(const FVector2D& Position,const FVector2D& Size,const FBox2D& Protected);
    static FVector2D PlaceNameplate(const FVector2D& Position,const FVector2D& Size,const FBox2D& Character,const FBox2D& Panel,const FVector2D& Viewport);
    static bool ShouldContinueCombo(ELiveWeaponType Type,int32 Hits,float Chance,int32 Maximum,FRandomStream& Random);
    void GetPresentationSamples(TArray<FLiveWeaponLabelSample>& Out)const;
    const FLiveWeaponRoster& GetRoster()const{return Roster;}
    APawn* GetAnchor()const{return Anchor.Get();}
    bool AcquireAlertLease(const FString& ParticipantId,const FString& SourceId,double ExpiresUtc,int64 SessionGeneration);
    void ReleaseAlertLease(const FString& ParticipantId,const FString& SourceId);
    void ReleaseAlertLeasesForGeneration(int64 SessionGeneration);
    int32 GetAlertLeaseCount(const FString& ParticipantId)const;
    double GetSimulationTime()const{return Clock;}
    virtual void Tick(float DeltaSeconds)override;
protected:
    virtual void BeginPlay()override;
    virtual void EndPlay(const EEndPlayReason::Type Reason)override;
private:
    friend class FLiveSpatialMigrationTest;
    void MigrateLegacySpatialRanges(const ALiveWeaponSwarm* Defaults);
    FLiveWeaponRoster Roster;
    struct FAlertLease{double ExpiresUtc=0;int64 Generation=0;};
    TMap<FString,TMap<FString,FAlertLease>> AlertLeases;
    int64 HighestAlertGeneration=0;
    bool WantsAlert(int32 Slot)const;
    FLiveWeaponCombatModel Combat{Roster};
    TWeakObjectPtr<APawn> Anchor;
    FString PersonalSwordId;
    FRandomStream DamageRandom{9911};
    void EnsurePersonalSword();
    TArray<FLiveWeaponVisualState> Visuals;
    TArray<int32> RenderSlots[4];
    TArray<FLiveArrowVisual> Arrows;
    TArray<int32> FreeArrows;
    double Clock=0;
    TArray<FTransform> PresentationTransforms[4],ArrowPresentationTransforms;
    bool bArrowsNeedRenderUpdate=false,bPerspectiveView=false;
    bool bNearbyThreat=false;
    ELiveWeaponPresentationMode PresentationMode=ELiveWeaponPresentationMode::Gameplay;
    FVector ViewCenter=FVector::ZeroVector,ViewLocation=FVector::ZeroVector,ViewForward=FVector::ForwardVector,ViewRight=FVector::RightVector,ViewUp=FVector::UpVector;
    float ViewDepth=1;
    float PlayerFollowSpeed=1200;
    FMatrix IdleViewProjection=FMatrix::Identity,IdleInverseViewProjection=FMatrix::Identity;
    FIntRect IdleViewRect;
    FBox2D IdleScreenBounds=FBox2D(ForceInit);
    float IdlePixelsPerUnit=1,IdleWeaponRadius=40;
    bool bHasIdleView=false;
    UPROPERTY(Transient) TMap<FName,TObjectPtr<ULiveWeaponSkill>> SkillObjects;
    ULiveWeaponDefinition* Definition(ELiveWeaponType Type)const;
    FVector FormationPosition(int32 Index)const;
    bool IsCurrent(FLiveWeaponHandle Handle)const;
    ALiveMonster* FindPrimaryTarget(ELiveWeaponType Type,const FVector& From,const TArray<TWeakObjectPtr<ALiveMonster>>& Candidates)const;
    void BeginWander(int32 Slot,bool bEvacuate=false);
    void BeginHover(int32 Slot);
    void RefreshReadabilityView();
    FVector2D ReadabilityOffset(const FVector& Position)const;
    FVector SafeIdlePosition(const FVector& Position,bool bForIdle=false)const;
    bool IsIdlePositionBlocked(const FVector& Position,float Padding=1)const;
    float IdleScreenWeaponMargin(const FVector& Position)const;
    FVector AlertPosition(int32 Slot)const;
    void BeginReturn(int32 Slot,bool bAfterAttack);
    void CompleteReturn(int32 Slot);
    void AdvanceAlert(int32 Slot,float DeltaSeconds);
    void AdvanceReturn(int32 Slot,float DeltaSeconds);
    float AnchorFootZ()const;
    float WeaponHeightMargin(ELiveWeaponType Type)const;
    void ShapeFlightHeight(int32 Slot,float EndFloorZ);
    void BeginStrike(int32 Slot,ALiveMonster* Target,FName SkillId,bool bNewCombo);
    void BeginDrift(int32 Slot,bool bContinue);
    void AdvanceWeapon(int32 Slot,float DeltaSeconds);
    void AdvanceArrows(float DeltaSeconds);
    bool ApplyImpact(FLiveWeaponHandle ParticipantHandle,ALiveMonster* Target,FName SkillId,bool bFollowUp);
    bool StartArrow(int32 Slot,ALiveMonster* Target,FName SkillId);
    void ReleaseArrow(int32 Index);
    void UpdatePresentation(float DeltaSeconds);
    FTransform WeaponTransform(const FQuat& Rotation,const FVector& Position,const FVector& Scale)const;
};
