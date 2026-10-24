# 运行组件应用增量 v8

**13/13 唯一候选已导入；缺导入0，所选 Source 中缺引用0；新增7项的实际运行 brush 回读仍 pending。** 另有共享属性图集的 Appearance 复用回读 pending，不重复计新候选。只核对这13个精确 uasset，未全量扫描596项目录、未看图签视觉、未运行UE。

新增七项的 PNG SHA256、uasset SHA256 与 `remaining-component-import-result.json` 逐项一致，回执 passed/AssetRegistry readback 均为true。旧六项 uasset 哈希与v7一致。生产字段、完整 canonical object 和逐项哈希见同名 JSON/CSV；以下路径均为 `/Game/newUI/` 下的包名。

| 候选 | 当前生产字段/consumer | canonical 包路径 | 磁盘/运行证据 |
|---|---|---|---|
| 主界面装饰 | Root `NewUIMainOrnament` | `Components/Player/T_MainActionOrnament_v1` | 存在；历史 PIE04/05 Main绑定通过 |
| HUD路由图集 | `AddHudGlyphs` | `Components/Player/T_HudRouteGlyphAtlas_v1` | 存在；历史 PIE04/05 HUD绑定通过 |
| 角色属性图集 | `BuildCharacterPage.AttributeGlyphAtlas` | `Components/Player/T_CharacterAttributeGlyphAtlas_v1` | 存在；历史核心 Character绑定通过 |
| 背包分类图集 | `BuildInventoryPage.CategoryGlyphAtlas` | `Components/Player/T_InventoryCategoryGlyphAtlas_v1` | 存在；历史 Inventory绑定通过 |
| 功法图集 | `FPageAtlases.MethodsGlyphs → BuildMethodsPage` | `Components/Player/T_MethodLoadoutGlyphAtlas_v1` | 新导入哈希通过；Source赋值；live brush pending |
| 装备法宝图集 | `FPageAtlases.EquipmentGlyphs → BuildEquipmentPage` | `Components/Player/T_EquipmentFabaoGlyphAtlas_v1` | 同上 |
| 外观装饰图集 | `FPageAtlases.AppearanceOrnaments → BuildAppearancePage` | `Components/Player/T_AppearancePreviewOrnamentAtlas_v1` | 同上 |
| 每日卷轴 | `CategoryBadge(daily) → UImage.SetBrushFromTexture` | `Activities/Icons/T_DailyActivityScroll_v1` | 新导入哈希通过；实际调用点在分类卡；live brush pending |
| 限时日历 | `CategoryBadge(limited)` | `Activities/Icons/T_LimitedActivityCalendar_v1` | 同上 |
| 常驻莲花 | `CategoryBadge(permanent)` | `Activities/Icons/T_PermanentActivityLotus_v1` | 同上 |
| 武灵徽记v3 | `FInkWulingOverviewData.Insignia → Emblem` | `Components/RankWuling/T_NUI_Wuling_SectInsignia_Normal_V1` | 新导入哈希通过；Source赋值；live brush pending |
| 竖地图 | Journey MapCanvas Art | `Textures/Journey/T_NUI_JourneyMapPortrait_V1` | 存在；历史 PIE04 world images含canonical资源 |
| 横地图 | Journey MapCanvas Art | `Textures/Journey/T_NUI_JourneyMapLandscape_V1` | 同上；不扩称两个方向视觉均通过 |
| 已有属性图集复用，非第14候选 | `Art.CharacterAttributeGlyphs → BuildAppearancePage` | 复用 `Components/Player/T_CharacterAttributeGlyphAtlas_v1` | 当前Source已赋值；Appearance实际brush回读pending |

V7-ASSET-01 分拆为：**import 静态关闭、reference Source关闭、runtime仍未关闭**。这里的 Source 引用指明确的生产加载/赋值与consumer调用，不把 AssetRegistry 存在冒充可见UI绑定。

PIE05 在15:22结束，新增七项在15:33导入；该 attempt 不可能证明这七项的后续应用。PIE05虽经过 EquipmentFabao/Methods/Appearance 等路由，不能因此追认为新图已用。当前所选 verifier 仍只有前四Player纹理的显式绑定检查，后续 verifier 已变化，单列 drift，不据此声称执行通过。

596为integration报告的当前目录数，构成为582迁移+4Player+2地图+WBP+7新增；它不是13候选的全部运行验收。整页概念图、Rank废弃v1/v2、失败候选、参考图继续排除；所有视觉批准仍为false。
