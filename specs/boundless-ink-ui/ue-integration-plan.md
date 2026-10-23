# 首页 v2 / v4 UE 接入实施计划

批准依据：evidence/approval-ue-integration-v2-v4.json。本次直接连续执行，沿用已批准设计，不新增设计门禁。

入口：现有 InkMenuDirector → InkFrontendWidget → InkHudLayout、InkAnchorStatusWidget、HUDRankView。保留主菜单、三个状态条、角色、背包、完整排行榜、业务和场景。

- [x] 核对契约、批准、原图、裁切清单；未找到项目或父目录 AGENTS.md。
- [x] Saved/Codex/HomeV2Integration20261022/Backup：源代码、配置、UI资产、设计包及工具逐文件备份并验证哈希。
- [x] 在现有入口保存接入前截图；PIE验收先确认旧素材不满足新资源路径要求。
- [x] InkHomeArt：由当前清单生成资产表；全图 Fit、锁高横向切片、排行宽窄分支；原生几何测试覆盖1260/1575/2205×399、零尺寸、归一化边距。
- [x] 导入18张原始裁切PNG到 /Game/UI/Ink/HomeV2；只改首页装配。头像插图用于角色入口，未知主播保持原占位；预览纸底仅本地预览开关出现。
- [x] Windows Editor编译、资源加载、竖横PIE及真实Slate点击、既有角色/背包/签到/排行/菜单/关闭回归，保存截图和JSON。
- [x] 仅在接入通过后：全项目硬软引用、源码动态路径、配置Cook/AssetManager、设计证据依赖审计；确定废弃资源先复制到项目外归档并验证哈希和ZIP可恢复，再移除工作目录副本。uasset只用编辑器工具；不明用途保留。
- [x] 清理后复查引用、加载、受影响PIE；写清单、恢复说明和阶段状态。Cook、设备、生产平台未执行则保持not-run。

验证关注：排行Fit透明余量不参与文字/热点；顶部卡片不可压扁；轨道端帽按实际高度缩放；动态图标/头像保持比例；真实数值不被预览替代；旧v1目录含当前引用，禁止按版本名整目录删除。

进度与续接：Saved/Codex/HomeV2Integration20261022/progress.json。该目录baseline.json不可覆盖。

实际结果：Editor构建及资源加载通过；接入PIE 78项、清理后PIE 98项；原生回归93通过+1带警告通过，0失败；最终交付检查100项通过；设计strict 0警告。归档8文件、更新17引用，恢复演练通过；UE资产删除0，551项保留待处理。完整证据见ue-integration-handoff.md。
