# 角色属性评级与品质边框

## 范围

角色属性面板移除突破材料。生命、剑灵攻击、防御、剑灵伤害、会心一击、闪避、实际移速上限、修炼速度显示 D、C、B、A、S、SS、SSS 七级纯字母图标。幸运、剑法为预留，不评级；修为进度不评级。生命评级输入为生命上限，受伤不会降低评级。输入使用结构化数值，缺失、负数、NaN 与无穷值隐藏评级，不解析显示文字。

## 策划依据与规则

依据 `../2026-10-21-wuling-resource-v1.4/03-数值策划案.md`、`15-分类物品与武灵经济扩展策划案.md`、`tables/realm_targets.csv`、`tables/items-v1.4/equipment_items.csv`。这是新增的展示评级规则，不修改战斗、成长或掉落数值。概率与剑灵伤害的七档切分为本次新增的展示标准。

练气 0–8，筑基 9–12，结晶 13–15，金丹 16–21，元婴 22，化神 23，元神 24–25，炼虚 26–30，大乘 31，飞升 32，混元无极 33。同一大境界内所有小境界使用相同阈值。

生命、攻击、防御、移速、修炼速度设本大境界裸装起点 B 与可达上端 U，七级下限依次为：

| D | C | B | A | S | SS | SSS |
|---|---|---|---|---|---|---|
| 0 | 0.8B | B | B+0.25(U−B) | B+0.50(U−B) | B+0.75(U−B) | B+0.95(U−B) |

达到下限即进入该级。U 不低于 B×1.01，避免常数属性阈值重叠。

v1.4 的生命基准为 `1000×(1+0.15r)`，攻击 `10×(1+0.12r)`，防御 `20×(1+0.10r)`。B 用本大境界第一个 r，U 用最后一个 r，再乘以 `1+大境界序号×0.01` 的累计突破品质参考及本境界可解锁的最高装备加成。大境界序号从零计数，累计最多 +10%，不反复相乘。

装备在大境界序号 0/3/6/9 解锁 T1/T2/T3/T4；四级生命加成 3/5/8/12%，攻击 2/3/5/8%，防御 3/4/6/9%，移速 2/3/4/5%。使用实际可解锁档位，不用无法在早期达到的全局上限。移速 B 为角色未加成行走上限（单位厘米/秒），U 为该基准乘可解锁鞋履加成。v1.4 修炼速度 B=1，U=1.2，包含灵泉 +20%，不虚构预留功法与活动增益。

旧规则使用运行中的 Rules.BaseAttributes 与 AttributeGrowthPerRealm：基准 `Base×(1+growth×r)`，仍取本大境界固定起止点；默认配置的完整阈值单列为 LegacyDefault。若旧配置裸装防御为零，采用策划防御曲线作评级参考，实际零防御评级 D。旧规则修炼速度 B/U 均乘实际配置 Rules.MeditationPerSecond，U 另乘 1.2，避免自定义打坐倍率使裸装基准评级偏移。规则模式来自当前实际权威模式，不改变现有存档或战斗属性。

会心与闪避的七档概率下限为 0%、1%、3%、6%、10%、15%、20%。剑灵伤害下限为同大境界攻击 B 的 0%、5%、10%、20%、35%、50%、75%。这三项仍在每个大境界表中独立列出；概率阈值相同，剑灵伤害阈值随大境界调整。

`attribute-rating-thresholds.csv` 由运行时评级库直接导出，包含两种规则 × 11 大境界 × 8 正式属性，共 176 行。`export-evidence.json` 记录策划源文件哈希，并验证同大境界小阶阈值一致。运行时蓝图可调用 InkAttributeRatingLibrary 的 Thresholds/Evaluate；面板悬浮评级字母可查看本属性全部下限。

## 图标与边框

图标源文件在 `SourceAssets/UI/CharacterRating20261022/T_Rating_*.png`，使用内置 image_gen 逐个生成和编辑；最终提示词见同目录 `imagegen-letter-prompts.json`。仅保留字母，外部及字母孔洞均为透明背景，无四边形徽章或底纹。导入资源 `/Game/UI/Ink/CharacterRating20261022/T_Rating_*`。

功法、轻功、内功、武神使用实际装备品质 QualityTier（1–6），未知品质与未装备不发光，锁定功法无粒子。主修取权威 loadout.methods[main].level，其他取已供给的 InkDisciplineEntry.QualityTier。未接入业务数据时不虚构已装备状态。预览可显式启用 bPreviewQualities，默认关闭。

边框颜色由 CharacterProfilePanel.QualityColors 配置；边框宽度 `(0.9+0.42×品质)×布局缩放`，柔光半径 `(2+0.8×品质)×布局缩放`，强度 `0.75+0.1×品质`。连续材质提供完整发光边框，Niagara 粒子仅沿本物品框边缘流动，无汇聚线。减少动态效果时停止 Niagara，保留静态发光边框。隐藏与关闭面板停止粒子。丹田沿用境界颜色与金丹光环。

## 验证入口

- `Tools/InkFrontend/create_character_rating_assets.py`：导入七图标与生成连续边框材质。
- `Tools/InkFrontend/export_attribute_rating_rules.py`：从运行时库导出完整阈值表。
- `Tools/InkFrontend/verify_character_rating_pie.py`：真实渲染 PIE 的三级分辨率、评级边界、实际品质、卸装、粒子边界与生命周期。
- `BoundlessImmortality.Frontend.AttributeRating.MajorRealmAndBoundaries`：两种规则的 11 大境界、七级边界及小阶恒定性原生测试。

验证报告在 `Saved/Codex/CharacterRating20261022`。Windows PIE 与原生编译验证不等于移动设备验证。
