# 可复现页面提示词与制作说明

## 主菜单候选 v1

工具提示词原文：

```text
Create one portrait mobile game main menu UI concept for an original Chinese xianxia cultivation game. Canvas around 720x1280, near 9:16, orthographic full-screen UI only, no device frame. This is a text-only generated design candidate and must not imitate any existing game screenshot. Quiet premium Chinese ink-and-paper art direction: warm ivory paper, deep ink green, muted jade, restrained antique gold, sparse misty ink mountains only as subtle header/footer framing. No character illustration and no background scene; preserve ample clean negative space for a project-owned title/character to be inserted later. Composition: top third has a large empty title/calligraphy placement area, central area remains mostly open for the existing project character artwork (do not draw a person), lower third has a vertical stack of two large readable main controls labeled 开始游戏 and 继续游戏, with continue visually secondary. Beneath, a small secondary row with 存档, 设置, 退出 as restrained text/icon affordances. Continue should read as potentially disabled but show no fake save data or invented status. Keep generous 24-unit safe margins and thumb-reachable controls at least 88x88. Use only the exact short Chinese labels given, rendered cleanly; leave title and character areas blank. Simple refined paper panels, crisp hierarchy, no elaborate decorative frames. Ensure whole composition fits with no clipping. Candidate concept only, not production art.
```

反向限制：不使用任何项目本地图像作为参考；不复制线上案例图或商标；不绘制角色，避免替换受保护场景；不增加选服/登录；不添加存档时间或状态伪数据。最终运行图须将保留的项目标题、人物和场景以独立层恢复，此图不是整页背景替换稿。

记录：生成并保存 `main-menu-v1.png`。人工检查显示纵向主按钮与次级入口清楚，但生成器添加了大面积抽象山水纸底，虽无角色/标题内容，不能覆盖项目现有背景。作为仅布局候选供审阅；后续组合须沿用现有背景资产。实测941×1672 RGB，SHA256 `aa8924f782a39cd15cc86153242e77faea863a885b7570dc990644138bb8f400`；approved=false。

## 主菜单 / HUD / 角色 / 背包

这些页面在本分工里属于保留/已有获批基准，不触发生图：

- **主菜单**：保留项目当前启动页题字、人物、场景与已存在按钮动作；仅复述 portrait-refactor `hud-menu-report.md` 的已实现排布。禁止本地或线上截图做图像条件、禁止重画已保留人物或场景、禁止虚构入口。
- **HUD**：复用 `home-v2` 及 v4 已批准组件包原路径；竖屏采用已记录左上身份带、右上三入口纵列、中心保护区、排行右侧入口、底部五导航。禁止调用图像生成覆盖或再造既有批准资源。
- **角色**：保护原丹田/功法/装备槽/评级布局和绑定。若后续经明确批准制作独立概念图，必须直接导出当前运行页面的结构描述而非引用资产图；本次不生图。
- **背包**：保护原网格/材质/框架/操作。不得将已有 `inventory-v1.png` 当作批准证据或生图参考；本次不生图。

## 突破准备候选 v1

工具提示词原文：

```text
Generate a polished portrait mobile game UI mockup, exactly one screen, Chinese cultivation xianxia breakthrough preparation page. Canvas 720x1280, no device frame. Respect generous safe margins 24 logical units, with top content beginning below a 24-unit system safe area and a fixed thumb-reachable footer. A restrained premium Chinese ink-and-paper UI: warm ivory xuan paper, deep ink-green text, muted jade dividers, very small antique-gold accents, sparse brush-ink mountain ornament only in the header/footer. Clear flat orthographic interface, high readability, generous row spacing, simple refined hierarchy, no elaborate background scene. Header: back arrow, title 境界突破, a thin current realm → target realm line. Top focal card: 境界品质 section with an empty placeholder meter and blank numeric areas for server-fed Q score, quality and growth range; clearly separate quality from tribulation. Scrollable middle: four compact material rows labeled 核心, 护道, 精华, 奇珍, each with a neutral empty icon well, required/owned quantity placeholders and a small 查看来源 text affordance. Below, a readiness summary with blank status fields, then a distinct small panel 天劫概率（独立字段） with empty value placeholder and explanatory line. Fixed bottom paper summary strip for exact cost placeholder and a full width muted antique-gold primary button 准备突破. Add an unobtrusive disabled reason placeholder below/near button. Use only these short accurate Chinese labels; no other generated text. This is a design candidate only, not an in-game screenshot. No fake numbers, item names, success rate, or price. Text areas crisp and typeset-looking. Ensure all elements fit within the canvas and no content is clipped.
```

反向限制：不使用任何项目本地图像作为设计参考；不将线上图像素材放入参考；不复制品牌/角色；不出现未经配置的倍率、品质、材料名、货币、价格、成功概率；不把Q分与天劫概率合并；不输出设备外框、透视、乱码、密集小字或赌场转盘。动态中文值仅占位，运行时应换成字体资产 TextBlock。

记录：生成工具返回 `breakthrough-preparation-v1.png`，实际 941×1672 RGB（比例约0.563），近似9:16而非精确720×1280。人工打开检查：布局完整，四类槽、评分与天劫分开，固定主按钮可见；文字基本可辨。图片未裁切到严格参考尺寸，页面底部按钮下说明较小，正式使用需排版适配。状态 `candidate`, `approved=false`。

## 天劫现场候选 v1

最终提示词：

```text
Create one portrait mobile game UI mockup, a dedicated Chinese xianxia tribulation live-event page, 720x1280 logical canvas approximate 9:16, no phone frame. Generate from this text only; do not use or imitate any existing game screenshot. Premium restrained Chinese ink-and-paper visual system: warm ivory paper, deep ink green typography, muted jade structure, small antique-gold highlights, sparse lightning-cloud ink motif confined to a compact header and footer. This page is a live authoritative event dashboard, not a preparation page. Header: back, title 天劫现场, current event phase placeholder. A prominent three-stage vertical timeline with labels 第一重, 第二重, 第三重 and neutral pending/current/completed visual state markers, but no fake completion. A large central remaining-time panel with blank placeholder only. A compact server-fed support summary with fields 已助力人数, 本人状态, 截止时间 all shown as blank placeholders; no probability numbers or fabricated participants. A clearly separated outcome/reconciliation panel with neutral placeholder text 状态以服务端结果为准. Bottom fixed primary action area contains a large simple button label 助力 (only if eligible) and a secondary disabled/reason placeholder; explicitly leave action unavailable during preparation, frozen, submitting, disconnected and resolved states. Make room for three distinct phase labels 准备, 天劫进行, 揭示, but do not invent durations. Show outcome slot labels 成功结果 / 失败结果 as empty conditional states, not simultaneously active; do not add rewards, costs, loot or probabilities. Readable Chinese labels, spacious rows, 24-unit safe margin, touch targets at least 88x88, scrollable middle, fixed header/footer. No person, no warrior figure, no weapon art, no brand, no gibberish, no numerical percentages, no countdown digits. Flat straight-on UI concept.
```

反向限制：不引用本地图片或线上 UI 图；不填概率/助力人数/截止时间/倒计时/奖励/扣修/材料消耗；不把 UI 端重算当服务结果；成功与失败结果卡互斥显示；当前阶段状态由服务端确定。实际规则的时长、概率与失败处理按权威配置和 `TribulationVm`，不把数值硬编码入静态图。

记录：生成并人工查看 `tribulation-live-v1.png`。实测941×1672 RGB；SHA256 `67ca77b4f40973471cb88931ad4e2d305d3b10be13fe177202b91e301767897f`。三阶段、剩余时间占位、助力摘要、服务端结局和底部动作区可辨；成功/失败并列卡仅代表互斥模板区域，运行态仅展示与权威 `outcome` 相符的一种。approved=false。

## 修订版 v2 候选提示词

### 主菜单 v2
```text
Create one portrait 9:16 Chinese xianxia cultivation game main menu UI concept, 720x1280 target, no device frame. Text-only prompt, do not reference any local image or existing game screenshot. Premium restrained ink-and-paper style: warm ivory, ink green, muted jade, limited antique gold. At the top create a compact clear title hierarchy: a large blank title lettering area and one smaller subtitle line area. In the center place a clearly bounded, tall, empty character-display window occupying roughly half the screen height, with subtle jade outline and soft paper-mist interior; this is an explicit placeholder for the real project character to be inserted later. Do not draw any person, silhouette, weapon, architecture or scene inside it. At the lower area place two large thumb-friendly controls, primary 开始游戏 and secondary 继续游戏, followed by a compact secondary row 存档 / 设置 / 退出. Keep labels legible, 24-unit safe margins, consistent spacing, and calm contrast. Do not show save slots, timestamps or fictional state. Background should be mostly plain warm paper with only minimal corner ink texture, not a landscape scene. Candidate UI layout only.
```

### 突破准备 v2（空配置态）
```text
Create one portrait 9:16 Chinese xianxia breakthrough preparation UI, 720x1280 target, no device frame. Text-only prompt; no reference images. Restrained warm ivory xuan paper, ink-green text, muted jade dividers, sparse antique-gold accents. Header back and title 境界突破; current/target realm fields are blank placeholders. A quality panel has blank placeholders for Q score, quality band and growth range. Show four material rows 核心 / 护道 / 精华 / 奇珍 with empty icon wells and blank owned/required values. Add a readiness section marked 配置状态：等待配置. Add separate 天劫概率 field left blank and labeled 由服务端规则提供. This is explicitly an empty-data/config-unavailable state: fixed bottom action 准备突破 is visibly disabled, muted, and accompanied by clear reason text 等待突破配置，暂不可操作. The cost summary is blank and says 待配置. No active gold button, no fake numbers, costs, material names, probabilities, rewards, multiplier or success implication. Scrollable middle, fixed header/footer, 24-unit safe margins, touch targets at least 88x88. Flat orthographic UI, crisp Chinese labels.
```

### 天劫现场 v2（单一运行态）
```text
Create one portrait 9:16 Chinese xianxia live tribulation UI, 720x1280 target, no device frame. Text-only prompt, no screenshots or local image references. Restrained ink-and-paper style, ivory paper, ink green, jade, tiny antique gold; minimal thunder-cloud decoration only in header. Show exactly one current live event state, not a preparation/results montage. Header back and 天劫现场 with a blank server phase field. A three-stage timeline 第一重 / 第二重 / 第三重 has neutral uncompleted markers and no implied completed stage. Show a blank server remaining-time field. Include clearly labeled server-fed fields 基础成功率, 助力加成, 连续失败保底, 本次成功率, all blank placeholders, and distinct from one another. Include different eligible helpers count, viewer's own assistance status, and server deadline as blank fields. A large action area has one conditional button slot labeled 助力，仅当服务端确认资格且阶段为进行中可启用; show a neutral rule note that the action is unavailable outside the active assistance window, while frozen or submitting. Include one status panel 对账状态：等待服务端结果. Do not show success and failure outcome cards together; no outcome panel content in this live state. No fake values, rewards, loot, item costs, characters or percentage numbers. Fixed header and footer, scrollable center, 24-unit safe margins, 88x88 minimum touch areas, readable orthographic UI.
```

提示词均为纯文字条件；最终画面不引用本地素材。v1 保留为历史候选，v2 单独登记并标记 approved=false。
