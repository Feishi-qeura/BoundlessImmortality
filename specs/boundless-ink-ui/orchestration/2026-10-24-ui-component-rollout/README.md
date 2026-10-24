# UI 组件制作、功能接入与剩余旧资源清理
日期：2026-10-24，Asia/Hong_Kong。负责人聊天：01a16b97-94db-7070-acda-d35950bf8c41。

用户已授权：开新聊天、按需拆分续聊、使用 GameUI 将既有设计转为组件并落实 Unreal、功能实现和逻辑验证；视觉验收由用户负责。沿用现有规范，不重新要求用户批准本次派发计划。源图片与新组件未经用户视觉批准仍为 approved:false，可作为隔离的候选实现连续推进。

## 当前核实
- 已调用 read_thread 读取四个引用聊天；排行榜/武灵接口连续失败，无法获取正文，已核实其磁盘 verification.md、measured-assets-v2.json，不能说四个正文均读取成功。
- 现有候选不是已批准资源。player-core v2 主菜单/天劫命名互换，采用 revisions/2026-10-23-player-core-mapping-correction/ 中 v3 的语义映射。
- 活动采用 revisions/2026-10-23-activity-state-correction-3/ 的单一空态修订；缺失部分再读取 parallel-activities，不能复用同时加载/空态的旧页面。
- 地图签到采用 parallel-maps-signin/asset-manifest-v2-candidate.yaml，并自行检查画面语义。
- 排行武灵采用 parallel-rank-wuling v2 与 measured-assets-v2.json，校验真实文件。
- 当前已有 native-paper-v1 布局及原生功能，不从零重写已实现业务。
- Saved/Codex/FullUI20261024/cleanup-result.json 与 delivery-assets-check.json 记录精确六个旧 redirector 已清理；git status 对应六个删除。剩余 Ink 目录不等于全部废弃。
- dirty worktree 中有大量已有修改，任何小队不得 reset、clean、回滚他人修改或自动提交整棵目录。

## 分队与文件所有权
所有聊天使用本地项目同一 checkout。并发仅允许互斥目录；不要创建未包含当前未提交基线的 worktree。
面板小队模型 gpt-6-luna，推理 high；功能及清理小队 gpt-6.1-sol，推理 medium。create_thread 无 service tier 参数，快速1.5倍未设置也未确认，不修改全局设置。

| ID | 范围 | 独占新源码目录 | 独占候选资源目录 |
| --- | --- | --- | --- |
| panel-player | 主菜单/HUD/角色/背包/突破/天劫壳与组件 | Frontend/UiRollout20261024/Player | /Game/UI/Ink/ComponentRollout20261024/Player |
| panel-rank | 三类榜单/武灵相关壳与组件 | Frontend/UiRollout20261024/RankWuling | /Game/UI/Ink/ComponentRollout20261024/RankWuling |
| panel-journey | 世界/资源地图、签到/每日一签/月签 | Frontend/UiRollout20261024/Journey | /Game/UI/Ink/ComponentRollout20261024/Journey |
| panel-activities | 日常/限定/常驻/开服、贡献/任务/特惠入口/HUD角标 | Frontend/UiRollout20261024/Activities | /Game/UI/Ink/ComponentRollout20261024/Activities |
| logic-core | 玩家成长/背包/排行榜/武灵的真实数据和动作适配 | Frontend/UiRollout20261024/LogicCore | 不直接写资源 |
| logic-integration | 地图/签到/活动适配；唯一导入、公共路由、构建与验收执行者 | Frontend/UiRollout20261024/LogicJourney 与经登记的公共接入文件 | 统一导入上述四个目标目录 |
| cleanup | 剩余旧资源审计及证实安全的清理 | 不改游戏源码；工具放自己目录 | 默认只读，经 lease 才可改核实后的明确旧资产 |

各队文档、脚本、prompt、manifest、status、handoff 均只写本目录下各自 ID 子目录。根 README、registry.json、engine-lease.json 由监督者维护。完整源码目录前缀为 Source/BoundlessImmortality/。
已有公共文件仅 logic-integration 可编辑，编辑前记录路径与原哈希。logic-core 为公共业务旧文件提供精确 patch 建议，由 logic-integration 顺序应用。不得多队同时编辑 InkFeaturePanel.h 或 ClassForRoute。

## 工作顺序
1. 四个面板队并行核实候选、按 GameUI 拆解语义组件、修复含字底板、生成必要的缺失透明资源；创建可复用 C++ UUserWidget/合适 UInkFeaturePanel 子类，实际 WidgetTree 与动态文字/回调槽可分离。只做确有必要的新类，不重建全部业务。
2. 每队提交 component-contract、独立 PNG/状态/9-slice/像素和 hash、Unreal 导入脚本、组件到控件表、实际类与路由接入说明。先准备导入包，不启动 Editor/UBT；交给唯一执行者做真实导入，脚本不算已导入。
3. 两个功能队先核实权威子系统、数据结构、当前行为与可测规则，分别提供可消费的契约/适配器，再对齐完成的壳。没有壳时先完成独立工作，不盲等、不假称整体完成。
4. logic-integration 收到至少一批完整 handoff 后，监督者授予 engine lease；确认其他面板源码不在写入后再构建，串行导入并回读 AssetRegistry/Texture2D 参数，接入实际运行路由、已保存 WBP 的迁移路径。所有导入资源需明确引用；不可只落 PNG 或生成整页贴图蒙住原页。
5. 接真实业务：loading/empty/error/offline/data、权限、时间/日期、重复点击与请求中门控、背包/排名/签到/突破等动作。保留原有权威规则；附件中建议与代码有分歧时登记，不擅自改玩法/数值/付费。
6. 逻辑与路由验证按变更范围执行。构建、资产导入、自动化、PIE玩家入口分别记录，旧报告不证明本批通过。截图可准备给用户；小队不替用户签署视觉通过。
7. 清理队在新增资源出现前先保存旧资产范围基线，持续只读审计。清理写操作必须等导入/功能依赖明确，并由监督者协调独占 lease；备份、完整注册表与硬/软/配置/代码引用可追溯后仅删除明确无用途的具体资产。Unknown 保留。

## 全队交付约定
status.json 至少含 team_id、phase、updated_at（带时区）、completed、remaining、blockers、owned_files、handoff_ready、engine_required。handoff.md 必须简短，含实际文件、接口、验证命令和结果、未完成项、下一步。
分开标记：source_ready、assets_ready、import_verified、route_wired、logic_verified、visual_pending_user。实际未导入时 import_verified=false；approved:false 不因构建通过自动改变。
GameUI 使用 C:/Users/zxxuh/.codex/skills/gameui/SKILL.md；阅读组件、切图、资产管线阶段。原生 exporter 不支持 Unreal，不把 generic JSON 冒充 uasset。按需要真正编写执行引擎导入与集成。严格校验在各自隔离且满足技能 schema 的项目副本执行，不覆盖根公共合同；共同合并由接入队负责。
候选输入是本次用户授权的待加工设计；禁止用其他项目本地旧图充当新增风格参考。新增外部研究仅官方/商店可核查来源，不复制整页。禁止用伪造价格、奖励、排名、日期和进度填空。
无需每个页面重新生概念图；保留可用设计，仅补有必要的缺页/组件概念，以降低额度。组件文字由 Unreal TextBlock 实时绘制，含字纹理必须经图像编辑修复，不能挖洞当成无字底板。
不可停止用户 Editor 或其他聊天进程，不可自动改模型/提档、不做自动提交/推送。

## 监督与上下文续接
监督者使用 wait_threads 的 compact cursor 每批最多8个目标；不读巨大历史/图片，不循环高频轮询。状态不变时不通知。
检测依据：工具明确报告上下文/流式错误、任务失败，或两次间隔检查没有工具/磁盘进展且排除正常生图/编译/等待依赖；不能仅凭耗时断言 token 卡死。
各队每个可交接里程碑更新 status/handoff；上下文接近上限先写短交接并结束当前回合等监督续接。失败可开新小范围聊天，沿用队的模型/推理。给旧聊天发送停止写入并交接指令，确认 idle/停止后才把相同文件所有权交给新聊天；无法停止时新聊天只能只读诊断或用全新输出目录，不能并发抢写。
用户已明确授权监督者给本批小队、后续替代聊天下发任务和跟进。消息仅包含本任务工作，不对外发送。
旧 heartbeat 已存在 boundlessimmortality-ui；更新原项到本监督聊天及新 registry，保留历史目标与30分钟频率，避免双重调度。全部交付达标后关闭监督；视觉批准仍待用户，不擅自勾选。

## 完成判断
用户能进入新组件拼装的实际界面；资源已导入且运行有引用；新增壳绑定实际数据与动作；逻辑测试通过或明确环境阻塞；所有旧资源处置有证据；视觉待审项目可单独交用户。仅创建聊天、生成脚本或刷新共享样式不等于项目完成。

## 用户保存关闭后的执行批次
用户已明确确认保存关闭编辑器，并要求继续构建、导入、逻辑验证及剩余页面完善，视觉验收交用户。现场已确认 Editor/UBT/UAT/dotnet/LiveCoding/MSBuild/ShaderCompileWorker 均为0，七个工作聊天均idle后，监督者授 logic-integration 有界引擎批次；具体范围/到期时间/资源名单以 engine-lease.json 为准。不要继续沿用历史PID占用结论。

首批引擎期间，只有 logic-integration 可改当前 Source、Binaries、Intermediate、Content，且限本批及真实编译/测试诊断所需的最小修复；保留原冻结快照，所有修复写精确前后hash例外台账。其它队继续完善剩余页面时，代码只放自己 batch-2/staged-source/ 下（不在真实Source内），纹理/prompt/契约/导入建议也只写本队 batch-2/。禁止更改首批已消费的交付文件和清单。总接入结束当前编译/引擎进程后，再由监督者安排下一批stage提升，避免UBT扫描到一半新增类。

44屏后续责任：Player负责main/home/character/inventory/saves/settings/methods/equipment/appearance/craft/item-sources/breakthrough/tribulation；RankWuling负责leaderboard/wuling/sword-spirit/gift-rules/guild-build；Journey负责journey/world/region/world-result/resources/resource-detail/race-lobby/race-live/race-result/lease/resource-rank/resource-rewards/sign-in/monthly-sign-in/daily-verse；Activities负责activities/daily-activities/timed-activities/daily-detail/timed-detail/launch/launch-rank/offline/shop/market/orders。每队先完成监督本轮指定的2到3个可独立合并页面/共享组件，不一次灌入整组历史。main/home/character/inventory仍须遵守PRD保留边界；有功能适配或部分组件不等于新整页重制完成。每日一签的吉凶与当前每日剑诀存在真实业务差异，不擅自把剑诀改称吉凶。

首批导入/功能稳定后，cleanup仍需独占只读AssetRegistry批次；清理资产写操作没有随本次引擎授权授予。没有零用途证据就保留，禁止为清理数量删除资产。自动化监督继续按30分钟频率协调后续批次，不必每一批重新询问用户批准。

## 用户反馈后的当前优先级
最新实施范围见visible-replacement-correction.md：首批功能验证已完成不等于全套外观替换。用户仍看到旧样式，优先修正batch2的真实接入问题并更新主菜单/HUD可见新版组件；保留业务、人物场景及旧资产备份，不沿用旧PRD把入口外观无限期留旧。角色/背包后续片保留原业务并交付新版本布局，视觉仍由用户验收。当前用户新开Editor36400只允许只读核实，禁止结束；旧首批引擎授权释放，新阶段先做可审阅源码，构建/导入另按现场占用授唯一执行批次。

## 当前最高优先级：全新体系重构
用户已明确解除旧项目规范对UI重构的限制，要求按GameUI实现全新UI体系。以 ../../rebuild-2026-10-24/quickstart.md（实际项目路径 specs/boundless-ink-ui/rebuild-2026-10-24/quickstart.md）和本轮后续派发为准。此前保留四页外观、局部换皮、必须沿旧布局的约束失效；生产源码仍由logic-integration唯一维护，四组新设计/组件暂存至新重构根。用户业务、数据和备份保留；新版必须实际默认进入玩家运行入口。旧lease/旧PRD不得被当成重复审批理由。

