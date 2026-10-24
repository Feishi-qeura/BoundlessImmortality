# 新 UI 运行架构与接入接口

Owner: logic-integration，唯一生产 Source/Config writer。依据 quickstart.md 最新用户授权；当前为源码阶段，用户 Editor36400/LiveCoding46212保持运行，不触发引擎、热更、Content迁移或导入。

## 默认运行根

- 新源码根：`Source/BoundlessImmortality/Frontend/UIRebuild20261024/`。
- 计划实际 Root 类：`UInkRebuildFrontendWidget : UInkFrontendWidget`。继承仅复用权威数据、既有动作/存档/库存/排行订阅及Director接口；独立构造新 WidgetTree，不复制旧可见树，不调用旧页面 polish/layout 作为新版fallback。
- `AInkMenuDirector` 的实际创建路径强制选择新 Root 分支；已序列化的旧 WidgetClass 不能让默认启动静默退回旧根。
- 新默认 WBP：`/Game/newUI/Runtime/WBP_NewUIFrontend`，Parent 为新 Root 类。未导入该 WBP 时只允许同一新体系原生 Root 类，不回退旧UI。
- 根内保留 `SafeCanvas`/`PageSwitcher` 等少量业务定位名，以维持输入/页面栈/回执挂载；这些是业务接缝，不表示复用旧构图。
- 兼容主路由仍为 Main/HUD/Saves/Settings/Character/Inventory/Leaderboard，Feature 子路由继续现有语义；结果页取消误指向通用业务页的旧别名，绑定真实结算宿主。

## 分层职责

1. **Theme/Primitives**：统一720×1280逻辑基线、24安全边距、88触控目标、字体/间距/按钮/状态容器；原style-contract只作临时颜色字体令牌，不保留旧页面结构。资产只加载 canonical `/Game/newUI`。
2. **Root/Nav/Page shell**：根树、页面框架/导航、确认层、回执层、输入隔离、横竖屏、安全区、布局版本。logic-integration独占维护。
3. **Owner page composition**：四队在本目录各自team_id下交付独立UUserWidget组合/staged-source；动态Text、真实动作和组件可重用。integration负责宿主创建、生命周期、路由、保存树及更新。
4. **Data/actions**：保留InkCore/InkJourneyAdapter、UWulingPrototypeSubsystem、存档/库存/排行的数据所有权；不在视觉组件制造奖励、经济或突破数值。
5. **Migration/editor tooling**：new WBP创建/保存、实际树/类/版本/纹理/按钮读回与截图；cleanup负责UI目录迁移/备份/无旧根证明，两者引擎串行。

## 生产源码接口决策

- 新Root提供单独presentation生命周期钩子。现UInkFrontendWidget的业务更新不删除；对旧树布局/polish/rebuild的调用改为可覆盖presentation hook，避免旧逻辑在数据刷新时重建旧外观。
- 新页面组装不能依赖旧WBP已有Root为空。新Root始终通过新版建树路径构造；保存WBP必须新Parent、新默认创建路由和可读布局版本。
- owner组合可保持独立UUserWidget；host包装负责Configure/数据mapper/Delegate/Back，UPROPERTY持有子组件，提前在NativeOnInitialized或RebuildWidget前建树。
- 原生动作优先使用现有UInkFrontendActionButton与真实Action ID，由Frontend接收。原生普通按钮通过host delegate回到原业务入口；不可只复制按钮外观而无动作。
- 根的action收集需显式遍历嵌套UUserWidget root，避免嵌入新组合后按钮未绑定/测试不可达。
- 排行保留同一persistent rows/头像请求，清旧chrome前先捕获并detach；新的分类/范围/状态/本人名次区域是新版组合。非USTRUCT的factory结果不可整块UPROPERTY。
- 成员页保留真实RecordRow_i→Rows→SelectedMember→RecordRemove链，并在新版组件内重挂同一控件；不得降级为只读姓名。
- 同条目刷新保留确认/请求态；完成/失败/退出由真实传输和路由结果解除。每帧无条件重建不作为刷新方案。

## 四队交付输入

每片交接给出：staged文件/拟target、当前hash、组件API、构图尺寸与响应规则、真实字段/Action ID、root/page/version marker、设计来源、纹理语义/尺寸/alpha/hash/approved:false、已做与未做验证。每片2–3页可独立接入，完成暂存后继续后续片。

阶段接口未齐时，先交UUserWidget组合与精确mapper缝；无需等待公共基类。integration按实际API适配，不要求各队自行修改生产Source。

## 可见验收账本

每页单列：启动route → 实例class → 保存WBP/原生树所有者 → runtime布局版本 → 本次设计/组件 → canonical纹理 → 实际截图 → 功能证据。Source接入、编译、资产存在与截图验收分别记录。

当前文档为架构决策，不表示上述新Root/WBP已经编译或运行。首批旧5纹理/2WBP/21测试证据保持历史范围，不扩展为新体系通过。

## Integration compiler/runtime checkpoint 12:54
Build-02 succeeded for the independent root/shared theme and reviewed Player helpers. Shared theme fonts intentionally use the exact suffix-preserving canonical migrated paths `/Game/newUI/Ink/Fonts/Shared/F_InkDisplay.F_InkDisplay` and `/Game/newUI/Ink/Fonts/Shared/F_InkBody.F_InkBody`; `/Game/newUI/Shared/Fonts` is a contract intent without current packages. Owner stages should consume `InkRebuildUI::Font`/palette rather than independently creating NAME_None/Latin fallbacks. Actual post-migration font load validation remains required. Do not create another font copy merely to satisfy an invented namespace.
Current owned migration Editor 27180 (first 26904 exited normally without mutation because keepalive was omitted; corrected). Preflight and migration are kept same-PID with a disk-controlled callback. Execution wrapper makes one documented world-context repair to the cleanup staged script's console call; original owner script untouched, exact source and hashes stored by integration.
