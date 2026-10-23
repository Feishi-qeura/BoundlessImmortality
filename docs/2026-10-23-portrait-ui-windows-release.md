# 无极仙道 · 竖屏 UI 重构与 Windows 分发包

用户已批准全量实现及 Windows ZIP；本轮已完成原生集成、资源迁移与运行验证。

- 保留角色属性、背包、排行榜的布局、控件与业务绑定；更新资源路径，减少共享外壳背景叠层。
- 重设计主菜单、HUD、设置、存档、签到/剑诀/月历、日常/限时/开服活动、仙途和各子页。正文可滚动，主要操作固定；宽屏在可读宽度下分栏。
- 原有 570 项 UI 资源全部保留，按 Texture/Material/MaterialInstance/Niagara/Widget/Font/Data 与页面族归档；内容删除 0。旧路径兼容映射保留在配置和 `InkUiAssets`。
- 背景外壳单层纸底；嵌入页不再铺整页背景。修复卡片/按钮/景物绘制层级、滚动条宽度、重建滚动位置和抽取页固定操作。
- 按最后一次用户指正重排主 HUD：紧凑状态带、右侧签到/活动/仙途纵排、88×88 排行入口及104高底部墨色操作栏。细血量/修为条已修复画刷着色，公告条自动避开右侧入口并省略过长文字；最新包实际画面见 `PackagePortrait/02-hud.png`、`PackagePortrait/hud-announcement-fixture.png`（后者为明确注入的公告布局验证，无服务奖励或状态修改）。

## 分发

ZIP：`D:\demo\BoundlessImmortality\Saved\Packages\BoundlessImmortality_Windows_PortraitUI_20261023.zip`。

解压后运行 `Start-Wuling.cmd`，默认打开 540×960 竖屏窗口，可拖动边缘改变尺寸。随包基础 Python 自动启动本机玩法服务并随游戏退出。无需安装 Unreal Editor 或 Python。此包沿用 Windows Development / prototype_full 本地完整玩法流程；不是生产平台上传包。

首次运行若 Windows 弹出公共/专用网络访问授权，可选择“取消”；本次验收在取消该授权后正常连接本机回环服务。第一次被系统弹窗挡住的验收另存 `PackagePortrait-FirstFirewall`，不计入最终通过结果。

## 本轮验证

- Editor C++ 构建成功；原生自动化 95 通过、1 带警告通过、0 失败。
- 本地服务 88 项、启动器 7 项测试通过。
- Windows PIE：415 项检查、93 张截图；720×1280、1280×720、540×960、960×1280、720×1560、360×640，另有安全边距、重复打开和下方操作点击。
- 最终 HUD 另完成 31 项检查、6 张截图及血量/修为条像素着色验证。
- 修复签到宝箱和称号 Niagara 控件释放后仍 Tick 的生命周期缺口，并为插件组件销毁后的入口加保护；新增真实组件销毁自动化，最终包横竖屏各重复三轮签到打开、关闭和强制垃圾回收验证。
- 实际 Windows 安装包竖屏 173 项、横屏 173 项检查通过，涵盖本机服务、世界历练、签到三页签、活动入口和子页。
- Cook/Stage/Archive 成功；ZIP 解压文件 SHA256 检查通过。证据目录：`Saved/Codex/PortraitUI20261023/`。

## 调整与边界

主题资产：`/Game/UI/Ink/Data/Shared/DA_InkUITheme_v1`。可调 PaperOpacity、BackdropOpacity、DecorationOpacity、HeaderHeight、FooterHeight；原有字体和配色配置仍在。

没有新增虚构玩法收益。签到占位、尚未开放的活动、抽取演示、外观试戴等继续按当前服务/功能开关如实显示。未做 Android/iOS 真机或生产平台审核。

既有场景问题：Windows 打包版石台及部分植被材质偏灰褐，日志含场景材质缺少 InstancedStaticMeshes 使用标记的告警；本次重构前 `ReleaseReadiness20261022/FinalPackagedPortrait/02-hud.png` 与旧包日志已存在同样现象。本轮未修改这些场景材质，UI 贴图与交互验收不代表此既有场景问题已修复。

迁移前 887 个文件备份及哈希在 `Saved/Codex/PortraitUI20261023/Backup` 和 `backup-manifest.json`。历史审阅稿另存 `review-README.md`；概念图不是运行截图，实际画面见 PIE/PackagePortrait/PackageLandscape。
