# 新组件静态应用核对 v7

核对对象为各 owner 最新 asset-manifest 中的当前运行候选：**13 个唯一候选，6 个已导入且有资源绑定回读，7 个未导入/未应用**。另列一项已存在纹理的跨页复用缺口，不重复计为新图。所有候选保持 `approved:false`；未看图签视觉，未启动 UE。具体来源、canonical object、PNG/uasset SHA256 和分类见同名 JSON/CSV。

| 当前组件 | 生产 composition 字段/加载路径 | uasset / 实际运行回读 | integration 下一步 |
|---|---|---|---|
| Player 主界面装饰 | Root `NewUIMainOrnament`；`/Game/newUI/Components/Player/T_MainActionOrnament_v1` | 存在；导入回执与磁盘哈希一致；PIE04 Main 绑定断言通过 | 已应用；视觉另验 |
| Player HUD 路由图集 | Root `AddHudGlyphs` UV3×2；同目录 `T_HudRouteGlyphAtlas_v1` | 存在；哈希一致；PIE04 HUD 绑定通过 | 已应用 |
| Player 角色属性图集 | `BuildCharacterPage.AttributeGlyphAtlas` UV3×3；同目录 `T_CharacterAttributeGlyphAtlas_v1` | 存在；哈希一致；PIE04 Character 绑定通过 | 核心角色页已应用 |
| Player 背包分类图集 | `BuildInventoryPage.CategoryGlyphAtlas` UV3×2；同目录 `T_InventoryCategoryGlyphAtlas_v1` | 存在；哈希一致；PIE04 Inventory 绑定通过 | 已应用 |
| Player-c 功法图集 | `FPageAtlases.MethodsGlyphs`，builder 已消费；`RefreshPlayer` 的 `Art` 未赋值 | manifest 未导入；Player canonical 目录只有上述四个 uasset；没有目标 object/loader；无回读 | 导入 `method-loadout-glyph-atlas-v1.png`，确定 canonical 名并加载赋值 |
| Player-c 装备法宝图集 | `FPageAtlases.EquipmentGlyphs`，同为默认空指针 | 未导入；未分配最终 canonical object；无回读 | 导入 `equipment-fabao-glyph-atlas-v1.png` 并赋值 |
| Player-c 外观装饰图集 | `FPageAtlases.AppearanceOrnaments`，同为默认空指针 | 未导入；未分配最终 canonical object；无回读 | 导入 `appearance-preview-ornament-atlas-v1.png` 并赋值 |
| Player-c 复用角色属性图集 | `FPageAtlases.CharacterAttributeGlyphs`；可复用已存在的 `T_CharacterAttributeGlyphAtlas_v1` | **已导入但该 Feature composition 未引用**；核心 Character 回读不能证明 Appearance 已用 | 给 `Art.CharacterAttributeGlyphs` 赋现有纹理；无需再导入新图 |
| Activities 每日卷轴 | category `daily`；manifest `/Game/newUI/Activities/Icons/T_DailyActivityScroll_v1` | 目标 uasset 不存在；当前分类 VM/producer 没有图片字段或 loader；无回读 | 导入并补分类图片字段/brush 映射 |
| Activities 限时日历 | category `limited`；同目录 `T_LimitedActivityCalendar_v1` | 目标不存在；同上 | 导入并绑定 limited 语义 |
| Activities 常驻莲花 | category `permanent`；同目录 `T_PermanentActivityLotus_v1` | 目标不存在；同上 | 导入并绑定 permanent 语义 |
| Rank 武灵徽记 v3 | `FInkWulingOverviewData.Insignia → Emblem`；`/Game/newUI/Components/RankWuling/T_NUI_Wuling_SectInsignia_Normal_V1` | 目标不存在；consumer 已有，`RefreshRankFamily` 未给 Insignia 赋值；无回读 | 仅导入当前 v3 并赋值；排除已废弃 v1/v2 |
| Journey 竖/横地图两张 | MapCanvas Art 根据方向加载 `/Game/newUI/Textures/Journey/T_NUI_JourneyMapPortrait_V1` / `T_NUI_JourneyMapLandscape_V1` | 两个 uasset 存在且各与回执哈希一致；PIE04 `world-player-entry.json` 的 images 回读含两个 canonical resource | 两图已导入绑定；不据此声称两个方向视觉均验收 |

Activities-02 与 Economy-03 manifest 只有整页概念图，全部排除运行组件清单；其当前 UMG 纸面、文字、按钮可用原生 brush，不能因此虚报新 PNG 导入，也无须把概念图作为背景。Player-d/e 的物品图标来自实际 Inventory `Item.Icon`，没有在这些 owner manifest 中声明新的独立运行 PNG。Rank 头像为运行数据/服务资源，亦不是这次徽记候选。

owner manifest 的 imported=false 不是最终权威：前四 Player 和两张地图已被后续 integration 导入回执、磁盘哈希及 PIE 回读覆盖；其余七项有 manifest、精确目标缺失/受限目录清单、生产赋值缺失的对应证据。**582 迁移资源 + 4 Player + 2 地图 + WBP 的 589 组成，不等于新候选组件全套已应用。** 此处核对精确候选和六个 uasset，未重新全量统计目录。

证据截止以 `selected-inputs-v7.json` 为准。`source-drift-after-v7.json` 单列截止后变化；本文不批准后续 Source。PIE04 整轮 `passed:false`，Character 入口点击失败报告已保留，以上纹理回读仅代表已通过的局部断言。真实服务业务、设备和视觉验收均未完成。
