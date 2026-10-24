# 新版样式全面接入与旧 UI 清理

用户于 2026-10-24 确认已看到 native-paper-v1，并授权旧 UI 清理、新样式全面接入及本地测试。本轮继续使用 UE 原生可交互控件承载动态文字与状态；既有候选稿作为设计交付，不将其烘焙数据直接铺为运行页面，也不把语义错配候选改成已批准资产。

## 实施范围

- [x] 读取活动/地图签到/玩家核心三条引用会话；排行榜武灵会话接口连续失败，改为核实该分组磁盘交付。
- [x] 共享 Button/Surface 默认入口统一映射到新版原生样式；移除旧按钮/菜单背景材质分支，保留正在使用的 HUD 表条、图标、头像、角色和品质资源。
- [x] 背包迁移已有保存 WBP，保留五列40格及操作；角色保留数据结构并更新皮肤。
- [x] 三类榜单和武灵总览/名册接入；移除旧榜单材质及硬编码示例，不增加不存在的周期字段。
- [x] 活动/开服/常驻/突破和工具页统一框架、层级与真实空态；地图/签到沿用上轮已接入布局并统一新默认皮肤。
- [x] 旧 UI 12 包备份与原基线哈希一致；569 条配置与 C++ 包路径映射一致。
- [x] 完整 Editor target 编译与本地 Frontend 自动化、清理模式解析测试。
- [x] 新同步 Asset Registry 扫描通过后，仅对 helper 内固定六个 ObjectRedirector 调用一次 FixupReferencers；验证六个目标消失、canonical 仍在且字节哈希不变。
- [x] 本批44条路由 PIE 组件截图及受限菜单入口检查；组件/规则自动化结果不冒充逐页玩家可达。
- [ ] 有数据业务状态、玩家完整入口链及设备/平台验收，需相应运行环境。

## 清理边界与工具修复

本轮资产删除仅限既有精确六个重定向包。其余旧贴图可能仍承载道具图标、头像或已批准 HUD 装饰，引用未证实为零的资源保留。实现侧移除旧样式的活动分支，不执行全目录删除或全项目 Fixup。

实际运行发现 commandlet 所在模块 PostEngineInit 导致类发现失败，改为 Default（模块本体为 FDefaultModuleImpl）。UE 还把 -run 选择器传给 Main，原先“完整argv必须只有一项”使合法调用也返回9；现保留唯一清理模式，另仅接受明列的引擎启动开关，并添加拒绝未知/重复/双模式的自动化测试。

证据统一写入 `Saved/Codex/FullUI20261024/`；只读预检前两次均未进入资产操作阶段。备份为 `precleanup-backup.json` 及对应目录。后续执行和测试结果在此追加，不使用历史 PIE/Cook/包验收代替。

## 本轮执行记录

- 清理成功：GUI helper 在空闲 Editor 中执行一次窄范围 Fixup；退出码 0，完整扫描确认六个旧包消失。六个正式资源 SHA-256 与备份一致，569 条配置/C++路径别名仍一致。证据 `cleanup-result.json`、`redirector-cleanup-gui.log`。
- helper 对原始 redirector 检查期间仅临时移除进程内六条匹配别名，退出恢复；不改配置。headless 的首次 Fixup 因缺 Slate 失败，当时12包哈希均未变；已加入 Slate 前置门禁，实际清理由 GUI 完成。
- 第一轮 Frontend 自动化：40 succeeded + 1 succeededWithWarnings，0 failed；额外清理模式解析测试 1 passed。警告是隔离临时 World 的销毁 context，不代表玩家存档或页面验收。
- 第一轮 720×1280 PIE 组件截图：44/44 captured，0 route failures。实际复看发现背包过小、榜单列头重叠/空行、签到空白/无日期日历、武灵空框与技术文案，正在修正后复跑。该报告的 player_entry_verified=false。
- 第一轮截图结束时工具同帧 RequestEndPlayMap 与 RequestExit，PIE 尚未清理即退出，引发 PCG ensure 和 Water 关闭异常。测试工具已改为等待 PIE 销毁后再正常退出；待新日志证实收尾。
- 截图修正批次：背包重建后清除方向布局缓存，使用720×1220竖屏设计、5列40格与可滚动100高槽位；榜单空态折叠填充行并修正窄屏列头；签到无有效日期/记录时显示单卡空态，移除嵌套滚动与伪日期；武灵空横幅折叠；活动/开服/突破/商店去掉技术文案、空目录假0、未经核实的费用推算与离线比例。存档和设置的选中按钮使用浅色文字。
- 最终修正批次 Editor target：20/20 actions，退出码0（`visual-fixes-build.log`）；既有布局测试中2处 double→float 编译警告保留。新DLL哈希见 `final-binaries.json`。
- 最终 Frontend 自动化复跑：40 succeeded + 1 succeededWithWarnings，0 failed/notRun/inProcess，进程退出码0（`frontend-final-tests/index.json`、`frontend-final-tests.log`）。清理模式解析未再变更，其独立1项测试此前已通过。
- GameUI `validate_project.py --strict` 再次通过，0 warning；此检查只证明文档/资产清单结构，不证明PIE画面质量。

## 最终归档

最终Editor增量7/7 actions成功（`delivery-build.log`），最新构建上FrontEnd 40通过+1带警告通过、0失败（`verified-frontend-tests/`）。组件截图44/44、88张PNG（`visual-suite-20261024T010041Z-A0CAA6DE/`）；真实Slate入口检查3通过、41受限未执行、0失败（`Saved/Codex/BoundlessInkPIE/20261024T005938Z-3DC85B0E/`）。两次GUI进程均正常退出、退出码0，未再触发关闭内存异常，且无遗留Editor/UBT实例。

入口检查中另修复了同步点击后才采集回调计数基线的工具误报，复跑确认存档和设置的点击及返回。最终源码/DLL指纹及计数见 `final-verification.json`、`final-binaries.json`；用户可读记录见 ../evidence/2026-10-24-full-ui-integration-result.md。受限路线不计通过，无数据组件预览不代表业务结算验收。
