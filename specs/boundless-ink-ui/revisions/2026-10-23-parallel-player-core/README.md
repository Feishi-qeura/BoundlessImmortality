# 玩家核心页面分组交付

状态：设计候选交付；未获本轮逐页视觉审核。范围：主菜单、主 HUD、角色属性、背包、突破准备。仅本目录新增文件；不改公共契约、公共清单、公共说明或其他 revision。

## 获批资源保护核查

- `home-v2` 页面与 `home-components-trim-v2-png` 在根 `contracts/asset-manifest.yaml` 中为 `approved: true`，另有对应用户批准证据。此分组只引用，不复制、不编辑、不覆盖。
- 当前角色属性与背包按契约为 `preserved`，`delivery: preserve-runtime` 且页面视觉 `approval.approved: false`。portrait-refactor README 和代码交接亦明确保护角色属性、背包、排行榜几何/控件/业务绑定。本分组不生成替换稿；提供明确的字段关系与审阅说明。
- 突破需区别“运行设计/实现已记录”和“页面图资产获批”：portrait-refactor 的 `screen-contract-proposed.yaml` 将突破列为已实现并有 `approval.json`，但根契约仍是 `new_planned`，根资产清单未登记突破 PNG。已有 `assets/pages/breakthrough-v1.png` 文件，不等于清单或用户批准。本分组不修改/覆盖它，新增独立 `breakthrough-preparation-v1.png`，标记候选。
- 主菜单本身在根 screen contract 仍标 `preserved`、`approval.approved: false`；本分组只给出现有已实现布局的描述，不将实现交接误作新视觉批准。

## 流程与入口

```text
application-start → Main 主菜单 → HUD
HUD底部角色 → CharacterProfile（角色属性/功法/丹田/六侧装备槽/评级）
HUD底部背包 → Inventory（沿用原200槽、默认20解锁、每页40格与原操作）
角色面板当前突破资格/缺口 → BreakthroughPrepare
BreakthroughPrepare 的材料“查看来源” → 仅由服务返回的来源路由；不足/来源为空时保留在页内并禁用提交
```

回退按导航栈返回原入口。角色页不接管背包物品操作；突破 `InventoryVm` 只读取可用实例与数量，选择/预留结果由权威服务确认后刷新角色资格与库存。不能从 UI 槽位重算 Q、倍率、成长范围或天劫概率。

## 页面说明

天劫独立现场候选另见 `tribulation-live-v1.png`：只显示服务端阶段、助力事实、截止/剩余时间、概率和结果；不客户端累加重复助力或在截止后补报。准备/冻结/提交/断线/结局时助力动作不可执行。成功和失败区域是互斥状态模板，仅按权威 `TribulationVm.outcome` 呈现一种。

### main 主菜单

- 沿用现有启动页人物、题字和水墨场景，不再做背景替换或新增登录/选服。开始游戏进入 HUD；继续先进入原存档选择流程；空存档时禁用继续并展示原因；设置/退出维持次级入口及退出确认。
- 现有 portrait-refactor 实现记录：竖屏主操作落在底部，设置/退出置于次级行；横屏标题在左、菜单在右；继续仍打开原存档选择。它不是运行验证报告，本分组未重测。

### home 主 HUD

- 主场景与镜头保留。720×1280：顶部左侧身份状态带；右上活动、签到、仙途纵列；中央角色保护区；右侧排行入口；底部五项角色/背包/修炼/排行/菜单。
- 五项位置、实际状态值、HUD v2 页面与 v4 组件包均使用已批准资源。只描述入口连接，不复制静态图或对获批素材再生图。
- portrait-refactor 的竖屏纠正记录把状态带定为约440×128，右侧入口热区至少88×88，底部dock高104。宽屏按锚点重排，禁止整体拉伸。

### character 角色属性（保护页）

- 原布局、丹田、功法装备槽、评级框、动态选项、绑定与操作全部保留。标题/返回上方；中段沿用原属性信息结构；不添加总战力、不套用外部案例立绘卡片。
- `AnchorVm.attributes` 为角色属性；`MethodVm.equipped` 为功法；`EquipmentVm.slots` 为装备槽；`BreakthroughVm.availability` 仅供入口资格/缺口提示。
- 状态包括空槽、锁槽、长属性、未配置。当前视觉批准为否，不能据本分组重绘或将既有界面标成新版候选。

### inventory 背包（保护页）

- 保留原200物理槽、默认20解锁、40格分页、搜索/分类、新标/数量、滚动、多选、分页、详情与出售/分解/整理操作。原 hover 蓝、selected 金、网格、材质和框架不变。
- `InventoryVm.capacity/items/filters/page/selection` 为数据入口。突破页只消费权威材料实例与数量；若发生服务确认后的材料消耗/预留，回到背包须刷新同一权威库存；不能乐观伪扣。
- 状态包括空仓、搜索无结果、满仓、锁槽、绑定过期/托管权限。破坏性批量动作继续二次确认。

### breakthrough 突破准备（新增视觉候选）

- 页面见 `breakthrough-preparation-v1.png`。竖屏逻辑画布720×1280，顶部为导航与当前→目标境界；中段先品质预估，再显示最多四类材料，之后准备度；天劫区另行展示，底部固定消耗摘要和准备突破按钮。中段滚动，页头和底部固定。
- 输入严格来自 `BreakthroughVm` 与 `InventoryVm`：`current_stage`, `target_stage`, `slots[]`, `q`, `quality`, `growth_preview`, `readiness`, `sources[]`。评分来自服务 `q_normalized`（0–100）和 `quality_band`；成长使用 `growth_min/max`。天劫仅显示独立 `tribulation_chance`。画面中的空占位不代表实际数值。
- 核心材料缺失/配置不完整时禁用准备突破并说明原因；早期未开放类别隐藏；材料不足查看真实来源；加载、断线、提交中、预留过期均保留返回。确认前显示权威消耗摘要；提交中锁定提交并保留输入。确认后显示待对账状态，不自行判定结果。
- 不纳入旧稿 ×2.5–×5.0 倍率、虚构材料名或成功率；不混合品质分与天劫概率。横屏目标沿用契约列表+详情，品质/成长卡置右，材料列表置左；需后续审核具体视觉。

## 在线案例与可核查研究来源

只研究官方公开介绍所说明的竖屏习惯和信息分组，不下载/用案例图片作生成参考，也不复制其商标、角色、版式细节。

1. [TapTap 官方入驻《六界仙尊》](https://www.taptap.cn/app/226073)：页面明确称其为“竖屏挂机修仙手游”，并介绍多角色、装备及道具体系。可参考竖屏主操作集中、养成入口分组的品类预期；Q萌风格不作为本项目美术方向。页面抓取显示更新日期2026-05-15。
2. [《完美世界：诸神之战》官方研发手记：竖版简化操作](https://wmzs.wanmei.com/news/gamenews/20211026/233913.shtml)：官方介绍竖版主界面可隐藏、收回、展开，强调清爽与直观；支持将低频入口折叠并保留展开触达。原站本次搜索摘要可读，网页打开器返回内部错误，故只记录摘要可核查链接，不宣称逐图审阅。
3. [《诛仙手游》官方界面介绍](https://zx.wanmei.com/startegy/rm_slg/20160728/185034.shtml)：官方介绍角色头像、气血/法力信息及背包入口，支持“身份与状态同区、背包有明确快捷入口”的传统移动 MMORPG 信息结构观察。来源年代较早，仅作入口语义参考。

## 文件清单与资产状态

| 文件 | 用途 | 状态 |
|---|---|---|
| `README.md` | 范围、保护基线、流程关系、研究来源与验证 | 本分组交付 |
| `screen-layouts.md` | 五页区域规格、状态与关系 | 设计稿，未审核 |
| `prompts.md` | 可复现突破出图提示词及保护页文本规格 | 提示词/记录 |
| `breakthrough-preparation-v1.png` | 新突破竖屏页面图 | 候选，禁止视作批准资产 |
| `main-menu-v1.png` | 新主菜单竖屏布局候选 | 候选；不能替换受保护场景 |
| `tribulation-live-v1.png` | 独立天劫现场页面候选 | 候选；动态字段留空 |
| `asset-manifest-proposed.yaml` | 供后续合并的独立候选登记，不改公共清单 | proposed；approved=false |
| `validation.md` | 实际文件像素/哈希与结构检查结果 | 本地设计包检查 |

## 待合并公共条目

- `screen-contract.yaml`: 主菜单应保留现实现有 portrait/landscape 明确布局；角色和背包注明具体保护字段及突破入口关系；突破记录视觉 `candidate-review`，同时保留其已有运行实现批准证据。不可仅以 runtime implementation 作为页面 PNG 批准证据。
- `asset-manifest.yaml`: 增加本分组突破与主菜单候选，版本1、实际测量941×1672、RGB、相对路径本 revision、SHA256 见 `asset-manifest-proposed.yaml`、source_tool 为 built-in-image_gen、approved=false。获批页面/组件资源保持原始路径与哈希。
- 公共 style/screen 决定没有阻止本稿：两张图实际941×1672，接近9:16；排版逻辑仍基于720×1280。需视觉审核决定其UI是否接受以及是否要求重新生成严格比例版。

## 阻塞与边界

- 本分工未运行 UE 构建、PIE、Cook、设备检查或在运行时逐一测试这些入口；引用 portrait-refactor README/报告中的既有证据但不代称为本次验证。
- 根契约和 portrait-refactor 提案对突破状态不一致，待主任务合并时统一运行实现状态与页面视觉批准状态。
- 图像生成返回941×1672（约0.563:1），符合9:16近似稿但不是严格720×1280；如需精确逻辑画布需批准后另做排版/尺寸裁切决策。当前候选仅用于审阅构图。
- 根资产清单没有主菜单、背包、突破既有图片登记项；保留页不应凭磁盘存在的 PNG 推断批准。
