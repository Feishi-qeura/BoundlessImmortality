# Batch-2 接入前只读复核

2026-10-24，Asia/Hong_Kong。结论：**四队当前产物均不宜原样直接接入**。确认两项独立编译阻塞：Player include 路径、Activities 的未限定 `ESurface`。另有宿主路由、成员管理选择链、实时刷新等具体接入问题。没有运行 UHT/UBT/UE、测试或图片评审；下列“确认”指源码/公开头声明可核对，不表示编译或运行已经复现。

文件定位约定：P = `panel-player/batch-2/`；R = `panel-rank/batch-2/`；J = `panel-journey/batch-2/`；A = `panel-activities/batch-2/`，均相对于本次 orchestration 根。每队两份 staged `.h/.cpp` 均已全文读取；读取了三个实际存在的 handoff，以及各队 API/host-callsite/merge 建议。**A/handoff.md 在复核时不存在**；已改读 A/status.json、merge-plan.md、field-action-mapping.md，未把缺失交接当作已读。

| 队 | 确认的编译/API阻塞 | 具体接入/业务问题 | 建议 |
|---|---|---|---|
| Player | PP-01 | PP-02 时间双重时区转换；PP-03 现有自定义值展示丢失 | 修 include；保留 UTC 格式化与实时设置值 |
| Rank | 暂存函数本体未发现确定编译阻塞 | PR-01 只读名册替换管理列表会断开成员选择/移除链 | 保留真实 RecordRow 控件及管理区，或增加明确管理入口 |
| Journey | 未发现确定的公开 API/反射声明错误 | PJ-02 与当前路由类型不兼容；PJ-01 生命周期风险；PJ-03 同条目刷新重置确认/请求态 | 提供 feature-panel 宿主包装；提前构造树；分离数据刷新和交互状态 |
| Activities | PA-01 `ESurface` 未解析 | PA-02 三类页面缺有效实时刷新；PA-03 真实数组当数字读 | 限定 enum；补基于真实状态的刷新；按返回数组计数 |

## Player

### PP-01 [P1，已证实 include 搜索不匹配] 新目录中使用了 Frontend 根的短 include

- `P/staged-source/Frontend/UiRollout20261024/Player/InkPlayerMenuPageWidgets.h:5`：`#include "InkSaveSubsystem.h"`。
- 同目录 `.cpp:3-4,14`：`"InkFrontendWidget.h"`、`"InkPresentationStyle.h"`、`"UI/InkUiTheme.h"`。
- 这些文件实际在 `Source/BoundlessImmortality/Frontend/`，不在未来新类同级目录。当前 `BoundlessImmortality.Build.cs:12` 只把 ModuleDirectory 加入私有路径；实际已生成 `Intermediate/Build/Win64/x64/UnrealEditor/Development/BoundlessImmortality/BoundlessImmortality.Shared.rsp:45,340` 也只有模块根/Source 根，没有 Frontend include path。
- 因此按交接目标原样提升后，独立 translation unit 无法解析这些短路径。这是搜索路径事实，未实际调用编译器。
- **最小修正**：分别使用 `Frontend/InkSaveSubsystem.h`、`Frontend/InkFrontendWidget.h`、`Frontend/InkPresentationStyle.h`、`Frontend/UI/InkUiTheme.h`；不要为这两个文件扩大整个模块的全局 include 路径。自身同级头仍用原短名。

### PP-02 [P2，已证实违反当前公开格式化契约] 保存时间先转本地又交给默认时区格式化

- 暂存 `.cpp:69,80` 先 `Info.SavedAt.ToLocalTime()`，再 `FText::AsDateTime(LocalTime,... )`，未指定 invariant timezone。
- 当前真实宿主 `InkFrontendWidget.cpp:1026` 直接格式化 `Info.SavedAt`。本机 UE5.8 `Engine/Source/Runtime/Core/Public/Internationalization/Text.h:468-476` 明确要求本地时间戳传 `FText::GetInvariantTimeZone()`，否则会再次转换时区。
- 影响：非 UTC 环境保存日期/时间可能再偏移一次；具体用户系统/文化设置下的最终显示未运行。
- **最小修正**：直接 `AsDateTime(Info.SavedAt,...)`，沿用现有 UTC 输入行为；若确实保留 ToLocalTime，显式传 invariant timezone，二选一。

### PP-03 [P2，已证实新树没有旧实时字段] 自定义画质/实际帧率从可见内容中消失

- 暂存 `.cpp:162-171` 只创建三种画质和30/60选项，custom/unlimited 留空选中；没有 `QualityValue`、`FrameValue` 文本。
- `P/host-integration-proposal.md` 的替换方案在 `Body->ClearChildren()` 后只插入新 helper；其“保留后续 value 更新”仍指当前 `InkFrontendWidget.cpp:1132-1139` 的旧字段名。该 pass 不能给新 helper 增加不存在的字段。
- 对已有 quality=3、有效帧率45/120或不限帧的用户，新页只能看到无人选中的30/60，原来实际值标签丢失；这是信息回退，不能把它当成默认30或60。
- **最小修正**：在 helper 中保留并返回/命名两个实时值文本，传入真实 effective frame（不先坍缩为0）和当前 quality；让原刷新更新实际可见控件。Back 在当前默认树中是外层 frame 的兄弟控件（宿主 cpp:248-250），本次没有证据认定它会被 Body->ClearChildren 删除。

## Rank

### PR-01 [P1，已证实替换建议会断开既有管理操作] 仅保留姓名和分页，不足以替换 WulingGuild 管理列表

- `R/staged-source/InkRankWulingCompositeViews.cpp:243-256` 名册行只构造 TextBlock；函数参数 `.h:63-68` 只接受 PagingActionWidgets，没有成员行选择 delegate/user id 或既有 RecordRow 控件入口。
- `R/host-callsite-suggestions.md:25-29` 建议用于现有 `WulingGuild`，只传 Previous/Next，将管理 mutation 留给原 handler。
- 真实宿主 `UI/InkWulingRecordsPanel.cpp:59-62` 通过可点击 `RecordRow_i` 写入 SelectedMember，才能启用 RecordRemove；cpp:123,126 把选中 user_id 发送给服务。若用新名册替换可见旧列表，这个必要选择链就没有入口，保留 handler 不能保持功能。
- **最小修正**：复用并重挂载当前真实 RecordRow 控件、Rows 数据及移除/权限控制；或把新名册作为独立只读区域，并保留显式“组织管理”入口进入原完整页面。不要把当前管理路由无声降级为只读姓名名单。若原管理列表仍完整可见，问题不触发；此项阻止的是建议中的替换方式，不是只读 factory 本身编译。

额外接入说明（不计作已发生故障）：`.h:18-24` 的 FRankChrome 是普通 C++ struct，非 USTRUCT。`host-callsite-suggestions.md:7,11` 所说 transient host references 不能直接写成 `UPROPERTY(Transient) FRankChrome`；应使用宿主独立 UPROPERTY 字段，或正式反射结构，或明确由当前 WidgetTree 根保活的原生引用并在重建时重置。当前没有新宿主字段，不能声称已发现 UHT 错误。

榜单重挂载本身暂未发现必然导致订阅/头像失效：factory `.cpp:73-81,146-152` 保留同一个 PersistentRows/名称/子树，没有清空或复制 rows。当前头像代码仍通过 `RankAnchorScroll` 和原行名定位（宿主 `InkLeaderboardWidget.cpp:498-547`）。**但实际接入不能调用旧 RebuildLeaderboardSurfaces 的 Clear(Page) 后再去找已脱离树的 scroll**；要先捕获并保留旧树，再重挂载，并协调现有 adaptive layout。缓存/网络请求/返回重开后的存活仍需后续聚焦运行验证，不能由此报告推导已通过。

## Journey

### PJ-01 [P1，公开生命周期支持的源码风险；未运行复现] 首次 RootWidget 在 NativeConstruct 才创建

- `J/staged-source/InkJourneyBatch2Widgets.cpp:51-61,83-97,160-171` 的三个原生类都只在 `NativeConstruct` 中构造并替换 RootWidget，没有 NativeOnInitialized/RebuildWidget 阶段的树构造。
- 本机 UE5.8 `UMG/Public/Blueprint/UserWidget.h:525-528` 明确 Construct 在底层 Slate 构造之后调用，并可因移入/移出重复触发；cpp 当前又每次无条件换根。对于交接声称不依赖已保存 WBP 的原生类，首次建 Slate 时根可能仍为空；重复挂载也可能换掉已绑定的根/控件。
- **最小修正**：把一次性原生树构造放 NativeOnInitialized，或 RebuildWidget 在调用 Super 前构造；用已构建检查保护根，Construct 只刷新内容，事件 AddUniqueDynamic。当前已集成 Player atom 就在 NativeOnInitialized 建树（`InkPlayerMaterialSlotWidget.cpp:13-25`）。
- 限制：安装包没有对应 UMG 私有实现正文可供本轮读取；本项以公开生命周期契约及当前代码时序为依据，**不声称已看到黑屏/空白或重复 delegate 的运行故障**。

### PJ-02 [P1，已证实宿主类型不兼容] 独立 UUserWidget 尚不能直接注册为现有 feature 路由类

- 暂存 `.h:54,94,120,128`：ResourceDetail、Outcome、WorldResult、RaceResult 都属于 UUserWidget 分支，不属于 UInkFeaturePanel。
- 当前 `InkFeaturePanel.h:27` 的 ClassForRoute 返回 `TSubclassOf<UInkFeaturePanel>`；`InkFrontendExtensions.cpp:55-58` 用该类型创建 UInkFeaturePanel 后 Configure。直接把这些类 StaticClass 填到该路由映射不满足类型约束，也没有 PageId/SelectedEntry/feature stack 接口。
- `J/handoff.md` 要求后续 mapper/事件/结果路由，但没有提供此类型桥接。不能把“有 OnBackRequested”当成已经保留 feature 返回栈。
- **最小修正**：在已有 ResourceDetail 宿主或专门的 UInkFeaturePanel wrapper 中 CreateWidget 并嵌入这些子控件；wrapper 持有 UPROPERTY、映射准确 EntryId/结果源、绑定 travel 到新 Capture/Evaluate/Submit，并把 Back 接到原 feature stack。世界/竞采结果各自仍需真实 feature wrapper 路由。此项是必要接入缺口，非暂存 widgets 自身必然编译错误。

### PJ-03 [P2，已证实 setter 重置状态；实际影响取决于 mapper 调用方式] 同一条目刷新会清掉确认/请求中状态

- 暂存 `.cpp:100-110` 的 SetResourceDetail 每次均重置 bConfirmationOpen=false、bPending=false、TravelFeedback 空。
- 若 wrapper 像其它实时面板一样随 snapshot 更新调用该 setter，第一次点击后的确认可被刷新取消；请求尚未完成时也会把“处理中”显示提前清掉。宿主 Capture/Evaluate 的 Busy 门控仍应阻止真实重复 mutation，所以本项不能表述为已经绕过服务幂等。
- **最小修正**：只在 EntryId 改变/显式重开详情时重置交互状态；同条目的数据刷新保留 confirmation/pending/feedback。pending 解除只能来自实际入队失败/服务完成/页面取消路径；wrapper 在同一权威请求期间不可通过 setter 解除它。

## Activities

### PA-01 [P1，已证实名字查找失败] ESurface 没有限定命名空间

- `A/staged-source/InkActivitiesBatch2Panels.cpp:93,134,177,226,242,271` 使用 `ESurface::FeatureCard`，仅有 `using namespace InkActivitiesBatch2`。
- 实际枚举只在 `Frontend/InkPresentationStyle.h:8` 的 InkPresentationStyle 中定义；`InkFeaturePanel.h:54` 也使用全限定参数，没有把 ESurface 作为基类 typedef/member 暴露。
- **最小修正**：全部改 `InkPresentationStyle::ESurface::FeatureCard`，或在明确局部作用域 using 该 enum 类型。不能依赖其它 unity cpp 偶然注入的 using。未运行编译器，但当前独立源码中没有可解析的该名称。

### PA-02 [P1，已证实继承调用路径提前返回] 开服/贡献榜/限时详情不会随真实服务更新刷新

- 暂存 `.h:9-43` 三类均未 override RefreshLiveData；cpp:210,321 的调用继承基类。
- 当前 `InkFeaturePanel.cpp:309-330` 仅刷新 DailyActivityDetail+ACT_SIGN 和 Character*；LaunchEvent、LaunchRank、TimedActivityDetail、其它 daily detail 都直接 return。即使 frontend 在 `SetCharacterData` 中逐个调用它（`InkFrontendWidget.cpp:860-862`），这些新面板也不会更新。
- 影响：开服 StartLaunch 已通过 adapter 入队后，响应到了仍停在旧状态；task/rank/活动截止与服务恢复后的资料可能要切tab/重开/改视口才刷新。这个静态调用链已确认，具体帧序列未运行。
- **最小修正**：新增相应 RefreshLiveData override，以实际 Page active、version、enabled/connected/busy/status、活动选中id/服务日或实际展示字段组成变更键；变更时更新现有控件或 RebuildPanel，保持 SelectedTab/SelectedEntry 和适当确认态。不要只增加一次“请求后 RefreshLiveData”；异步返回必须有持续刷新路径，也不应每帧无条件重建。

### PA-03 [P2，已证实真实数组读成数值] 组织确认轮次永远显示待同步

- 暂存 `.cpp:165-167` 调用 NumberOrPending(Launch,"complete_rounds")，内部只 TryGetNumberField。
- 实际 `Tools/CultivationService/launch_event.py:61-64,121-122` 将 complete_rounds 维护为已确认 round id 数组；现有 `UI/InkWulingToolsPanel.cpp` 也按该数组数量显示。不是缺字段，也不是服务没有授权。
- **最小修正**：只在 TryGetArrayField 明确成功时显示数组 Num（真实确认轮次数）；缺失保持未知，明确空数组可显示0。不要要求服务另外补数字字段，也不把它作为 StartLaunch/参与资格门槛。

## 接入前最小处理次序及证据边界

1. 监督者转交 PP-01、PA-01 给对应 owner 修 staged 源码；本复核没有改任意 staged-source。
2. 先把 Journey wrapper/lifecycle、Rank 管理入口保留方案、Activities 实时刷新写成具体宿主接线，再由唯一执行者决定提升哪些文件。A 还需补缺失 handoff。
3. Source 释放后由唯一执行者统一构建。仅修复存在的 include/enum 不等于其它 staged 文件编译通过；后续原生/PIE 应聚焦上述变更和真实数据，不重复镜像实现的“假测试”。
4. 未发现本批又把 normalized Q/slots/sources 缺失作为提交前提的代码；Activities StartLaunch/SignIn 调用现有 InkJourneyAdapter，Journey travel 目前只是事件壳，仍必须由 wrapper 接权威 adapter；Rank/Player helper 主要复用既有动作。未发现即能声称绕过服务授权/幂等或伪造奖励的实证，不能因此豁免未来宿主接线检查。

未验证：UHT/UBT、保存 WBP 的当前实际树形、GC/Slate 重建、跨页返回、头像异步完成、触控/键盘焦点、非空排行/成员管理、真实服务日切换、结果receipt归属及全部运行画面。本报告没有读图、没有视觉结论、没有引擎/服务操作、没有修改首批或生产数据。仅产出本目录 review.md/readiness.json，不生成未应用 patch（多项修正依赖宿主设计选择，直接替换不安全）。
