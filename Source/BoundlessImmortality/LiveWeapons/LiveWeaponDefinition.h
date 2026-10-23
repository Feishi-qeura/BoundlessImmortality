#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LiveWeaponTypes.h"
#include "LiveWeaponSkill.h"
#include "LiveWeaponDefinition.generated.h"
class UStaticMesh;
class UMaterialInterface;
USTRUCT(BlueprintType)
struct FLiveSkillEntry
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ToolTip="技能唯一标识。用于技能解锁、抽取和冷却记录，必须与授予剑灵的技能标识一致；不同技能应使用不同标识。")) FName SkillId;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0",ToolTip="技能相对抽取权重。只在已解锁、冷却结束且满足释放条件的候选技能中比较；调大使本技能更容易被选中，设为 0 时不参与抽取。权重不是最终触发概率。")) float Weight=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0",ClampMax="1",ToolTip="技能被权重抽中后实际触发的概率，范围 0 至 1，例如 0.2 表示 20%。判定失败时本轮按普通攻击处理；实际触发还要求技能已解锁、冷却完成且满足释放条件。")) float TriggerChance=.2f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0",ToolTip="技能成功执行后的冷却时长，单位秒，按参与者和技能分别记录。调大使同一剑灵更少重复触发该技能；执行失败不提交冷却。")) float Cooldown=3;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ToolTip="技能实现类，可选择继承 LiveWeaponSkill 的蓝图或原生类。CanExecute 判断能否释放，Execute 执行技能；成功返回后提交冷却。仅填写技能标识并不会自动实现技能效果。")) TSubclassOf<ULiveWeaponSkill> SkillClass;
};
UCLASS(BlueprintType)
class BOUNDLESSIMMORTALITY_API ULiveWeaponDefinition : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ToolTip="本配置对应的武器类型：Sword 为剑，Axe 为斧，Spear 为枪，Bow 为弓。Swarm 按该类型寻找配置；剑和枪支持连击追击，弓使用箭矢攻击。")) ELiveWeaponType WeaponType=ELiveWeaponType::Sword;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ToolTip="该武器类型使用的静态网格模型。所有使用此配置的剑灵共享模型并通过实例化批量绘制；模型原点和尺寸也会影响表现及安全飞行高度。")) TObjectPtr<UStaticMesh> Mesh;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ToolTip="武器模型第 0 个材质槽的覆盖材质。留空时不覆盖该槽；其他材质槽仍使用网格组件原有设置。在游戏开始时从本配置应用。")) TObjectPtr<UMaterialInterface> Material;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ToolTip="武器模型在本地 X、Y、Z 方向的缩放倍率，无单位。调大使模型更大，也会提高根据模型包围范围计算的安全飞行高度；攻击命中盒由 Attack Box Half Extent 单独配置。")) FVector MeshScale=FVector(.12,.12,.8);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="0",ToolTip="普通攻击每次成功命中的基础伤害。连击的追加命中也使用此值；技能实现可通过 BaseDamage 读取该基础值并自行处理技能伤害。")) float Damage=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="1",ToolTip="该武器发起首轮攻击的目标范围，单位厘米，按角色到怪物的水平距离判断。调大可选择更远的目标；连击搜索使用 Chain Search Radius，此值也会扩大 Swarm 的警戒查询范围。")) float AttackRange=800;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ClampMin="1.2",ToolTip="自动攻击的基础间隔，单位秒，配置最低为 1.2 秒。调小通常使出手更频繁；实际间隔还受每个剑灵 0.8 至 1.2 倍的频率波动，以及攻击、返回和警戒恢复状态限制。")) float AttackInterval=1.2f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Flight",meta=(ClampMin="100",ToolTip="攻击飞行的基准速度，单位厘米/秒，用于近战出击和箭矢的轨迹构建。近战调大通常可缩短接敌时间，实际速度仍受曲线和加速度限制，并叠加剑灵 0.5 至 1.5 倍动态速度；箭矢不叠加该倍率。警戒环绕角速度在 Swarm 的 Alert Orbit Speed 中调整。")) float FlightSpeed=1900;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Flight",meta=(ClampMin="100",ToolTip="攻击飞行轨迹的基准加速度限制，单位厘米/秒的平方。调大允许更快地加减速和改变飞行方向；近战轨迹内部还会应用平滑系数，因此它不是最终瞬时加速度。")) float FlightAcceleration=12000;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Flight",meta=(ToolTip="飞行命中盒的 X、Y、Z 半尺寸，单位厘米，完整尺寸为各轴数值的两倍；近战武器和该类型发射的箭矢都使用此配置，判定时 Z 轴沿本次移动方向。调大使命中更宽松；模型缩放不会自动修改此值。")) FVector AttackBoxHalfExtent=FVector(14,14,32);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Flight",meta=(ClampMin="0",ClampMax="1",ToolTip="成功命中后继续连击追击的概率，范围 0 至 1，例如 0.5 表示 50%。仅剑和枪参与此判定；继续追击还要求未达到 Max Combo Hits 且能找到有效目标。设为 1 也不保证一定打满连击。")) float FollowUpChance=.5f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Flight",meta=(ClampMin="1",ClampMax="4",ToolTip="剑和枪单轮连击的最大命中次数，包含第一次命中，范围 1 至 4。设为 1 时不追加追击；实际次数还受 Follow Up Chance 和有效目标影响，斧和弓不使用近战连击。")) int32 MaxComboHits=4;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Flight",meta=(ClampMin="1",ToolTip="连击追击的目标搜索半径，单位厘米，以剑灵当前位置为中心。调大可衔接更远的目标；只搜索仍存活且本轮尚未命中的目标，不改变首轮攻击的 Attack Range。")) float ChainSearchRadius=900;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,meta=(ToolTip="该武器可使用的技能配置列表。每项包含技能标识、抽取权重、触发概率、冷却和实现类；参与者仍需先解锁对应技能，才会进入自动攻击的技能候选池。")) TArray<FLiveSkillEntry> Skills;
};
