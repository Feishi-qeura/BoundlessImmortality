# 突破准备

## 页面定位
主播修士的大境界突破准备页。让玩家在提交前看懂目标境界、资格材料、可选品质材料、预计成长与材料来源。

## 视觉约束
继承 `contracts/style-contract.yaml` 与已批准 HUD v2；竖屏 9:16、720×1280逻辑画布。宣纸浅底、墨绿正文、青灰区分材料组、旧金主动作和品质强调；左上柔光，局部墨山只做页眉与页尾装饰。标题马善政体；动态文案由引擎排版，不能把图中文字直接当运行时素材。手机 SafeZone 24 单位，按钮热区≥88×88。

## 布局与组件
顶部：返回、标题“境界突破”、当前境界→目标境界。
上部核心焦点：预计突破品质等级和 0–100 进度；预计属性倍率范围使用大数字占位，无抽奖式转盘。
中段：纵向四类材料卡，显示类别名、图标占位、数量、分值摘要和“查看来源”；核心材料视觉必需，其余槽位只呈现为已开放的可选项。材料缺口用文本与朱砂小印提示。
下段：突破准备度、洞府/经脉等状态简表；分隔区域以“天劫概率（独立计算）”作轻量说明，禁止混入品质评分。
底部固定旧金色主按钮“开始突破”，按钮上方是消耗确认摘要；无条件时清楚展示禁用原因。

## 状态
预览以核心材料已满足、可选材料未装入为示意；同时留出核心缺失、配置失败、类别未开放和提交中反馈。不得把护道、精华、珍宝绘成必需条件，也不显示虚构数值或实际成功率。

## 图像生成提示词
Create one polished portrait 9:16, 720x1280 concept mockup for a breakthrough preparation page in a Chinese xianxia mobile RPG. Match the approved restrained ink-and-paper UI: warm ivory paper, deep ink-green text, muted jade dividers, small antique-gold accents, sparse mountain ink only at header/footer, soft top-left light, generous readable spacing, SafeZone 24 and thumb-reachable controls. Show a fixed header with back and current-to-next realm; a clear expected-quality summary with a 0–100 normalized score placeholder and a narrow growth-range placeholder; a vertical list of at most four material categories, each with a source-link affordance. Make the core qualification material the only required category. Guardian, essence and rare-treasure categories must visibly read as optional and may be hidden for early realms. Show readiness separately. Tribulation chance belongs in its own small section and must never appear to add quality points. Fixed bottom primary action with a cost/confirmation summary. Use only accurate short Chinese labels if legible; leave values and dynamic descriptions blank for later typesetting. Straight-on orthographic interface, no phone frame.

## 反向限制
No existing game screenshots, no local reference images, no Western fantasy interface, no neon/glass UI, no casino wheel, no random-success framing, no duplicate currencies, no fake item names or prices, no gibberish text, no logos/watermarks, no character illustration covering data, no fake success-rate implication, no dense unreadable miniature text, no angled device perspective.

## 验收记录
`breakthrough-v1.png` 已审查：页面层级与品质/天劫分区可见，但“集齐全部材料后，可进行突破”把三类可选材料表现成必需条件，且素材槽数量可能被误读为四类各需四件。原图保留且未批准；下一候选使用 `breakthrough-v2.png`，明确“核心资格必需、其他按境界可选”，并减少误导性数字。运行数值仍从 v1.4 权威规则读取。
