# cleanup：剩余旧 Ink 资源审计交付

本轮是 `audit_only`，所有输出位于本目录。没有启动 Editor/UBT，没有改动 Content、游戏源码、共享配置或其他队文件，没有提交或推送。本轮独立工作已完成，后续实时 AssetRegistry 检查依赖监督者授予独占 lease。

## 已核实证据

- `baseline.json`：2026-10-24 09:53:32–09:53:38 +08:00 捕获原目录 583 个资产，合计 208,346,908 字节；逐文件尺寸、mtime_ns、SHA-256。捕获时 `ComponentRollout20261024` 尚无文件；后续审计永久排除该新子树。基线文件采用排他创建，禁止覆盖。
- `exact-six-current-check.json`：现场再次检查旧六路径均不存在，六 canonical 与原备份哈希相同，原 12 份备份均存在且匹配。历史成功证据原文保存在报告中供追溯；本轮没有重跑它们的清理。
- `static-text-evidence.json`：399 份 Source/Config/Content 文本、项目及工具文件的路径/行号、原行和文件哈希。兼容映射、编辑器/工具引用与运行时代码分开。
- `binary-string-leads.json`：1,498 个项目 `.uasset/.umap`，ASCII/UTF-16 字符串线索；不是包解析器，不提供 hard/soft 定性，更不能用未命中证明零用途。
- `dynamic-load-sites.json`：加载/拼接调用点。`known-dynamic-paths.json`：从现有真实数组展开的 8 个礼物和 7 个评级路径，关联当前重定向目标与源码哈希。只证明静态可构造路径，没有执行运行时加载。
- `asset-decisions.json` / `.csv`：583 个资产逐项决策，含静态、兼容、工具、二进制、动态路径证据与缺失的 live registry 列。所有 `zero_use_proven=false`、`deletion_authorized=false`。
- `offline-verification.json`：7 项离线检查通过；真实备份 smoke 的 manifest 位于报告所列的本目录 `backups/` 下。

| 决策 | 数量 | 解释 |
| --- | ---: | --- |
| RetainReferenced | 42 | 当前运行时代码/配置/数据有字面路径线索，保留 |
| RetainDynamicReferenced | 15 | 已核实有限数组构造的动态路径，保留 |
| RetainCompatibility | 525 | 仍为 C++ Resolve 或 PackageRedirects 的兼容目标，保留；映射本身不证明实际加载 |
| UnknownRetain | 1 | `/Game/UI/Ink/Materials/Leaderboard/M_InkLeaderboard`，23 个 MI 包出现该路径，保留等实时注册表定性 |

当前删除候选数 **0**。不能把 canonical typed folders、旧日期来源或缺少直接文本命中当成废弃证据。不能从历史 registry-audit.json 的零引用推断当前可删。

## 可直接使用的工具

PowerShell 当前目录为项目根；使用已现场定位的 Python：

```powershell
$py = 'C:/Users/zxxuh/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$team = 'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/cleanup'
& $py "$team/audit_legacy.py" audit
& $py "$team/verify_offline.py"
```

`baseline` 只可捕获一次，已经执行。后续重跑 `audit` 会刷新审计报告，不更改基线或资产。并发小队继续写源码时，扫描是一个有时间戳的快照；清理 lease 前必须重跑并比对文件哈希。

备份输入为 JSON 字符串数组，必须来自 baseline 中的精确 package 名；没有通配符，也没有删除模式：

```powershell
# 输入例子见 verification-backup-packages.json；它只是备份 smoke，不是删除请求。
& $py "$team/audit_legacy.py" backup --packages "$team/verification-backup-packages.json"
```

工具验证 baseline 哈希、复制 `.uasset` 及存在的 `.uexp/.ubulk/.uptnl`，同时复制 DefaultEngine.ini 与 InkUiAssetPaths.cpp，逐文件校验原件、拷贝及并发写入。输出为唯一时间目录和 backup-manifest.json。该 manifest 只证明复制成功，不授予任何删除权限。决策表中的 backup_verified 是该表生成时的状态；之后的 smoke 备份另见 offline-verification，不冒充一批删除候选的完整备份。

## 独占引擎阶段的消费契约

`registry_audit_ue.py` 已做语法和拒绝未授权 lease 的检查；**没有在 Unreal 中执行，不声明 API 运行通过**。字段及调用根据本机 UE5.8 的 `Engine/Source/Runtime/AssetRegistry/Public/AssetRegistry/IAssetRegistry.h` 核对：Options 146+，GetAssetsByPackageName 298，GetAllAssets 471，K2 dependencies/referencers 565/611，ScanPathsSynchronous 793，SearchAllAssets 809，IsLoadingAssets 1025。

监督者需在确认唯一引擎执行者空闲、其他资产写入暂停后，把共享 engine-lease.json 切到下列含义；只有监督者写该文件，本队不会自行获取 lease：

```json
{
  "owner_team": "cleanup",
  "state": "granted",
  "cleanup_state": "registry_audit",
  "expires_at": "<监督者实际设置的带时区 ISO 时间>",
  "scope": "Read-only full mounted AssetRegistry scan; no package mutations"
}
```

这是脚本消费字段说明，不是已发放 lease。当前 `logic-integration / not_granted / audit_only` 会在导入 Unreal 前拒绝。lease 过期也拒绝。脚本运行期间持续复核 lease。

唯一执行者可在其既有获准 Editor 的 Python 文件执行入口运行该脚本；若需独立进程，先协调现有 Editor，不能自行并发开第二个引擎。以下命令仅供获准后使用，本轮未执行：

```powershell
$editor = 'F:/UNREAL/UE_5.8/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$project = 'D:/demo/BoundlessImmortality/BoundlessImmortality.uproject'
$auditScript = 'D:/demo/BoundlessImmortality/specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/cleanup/registry_audit_ue.py'
& $editor $project '-run=pythonscript' "-script=$auditScript" '-unattended' '-nosplash' '-nullrhi' '-nop4' '-stdout' '-FullStdOutLogOutput'
```

脚本 search_all_assets + mounted roots 强制同步扫描（含 /Game，忽略扫描 denylist），回读所有 on-disk 资产及 baseline 的完整覆盖。每个 canonical 与兼容旧名分别记录 hard/soft package、searchable name、hard/soft management 的 incoming 和 canonical outgoing。扫描范围包含新增组件及外部插件的引用者，不局限旧 Ink 内部。输出唯一 `registry-live-*.json`，记录基线和静态证据哈希、资产/包总数、扫描根、缺失包、资产及源码漂移。它不 load_asset、不 save、不 fixup、不 delete。

该脚本的管理引用只是当前 AssetManager 注册状态快照，`management_registration_verified=false`；独占执行者仍需确认真实管理数据/主资产配置已注册。未保存 Editor 对象、运行时外部数据、拼接路径与业务入口无法靠磁盘注册表穷尽，仍需功能队/接入队确认。脚本即使给出零 referencers，也不会设置 zero_use_proven 或 deletion_authorized。

## 将来允许清理的最低条件

1. 当前批组件导入、路由、实际业务依赖已稳定；完整 mounted registry 对硬/软/管理/搜索名称引用检查有本批证据；原 asset 与配置/源码快照无未复核漂移。
2. 逐项核实 C++ Resolve、PackageRedirects、PrimaryAsset/cook 配置、动态路径和业务用途；任何 Unknown、兼容义务、加载线索或仍在用资源继续保留。零注册表引用不是充分条件。
3. 监督者确认一个明确 package 白名单，候选全部有可回读并匹配的当前备份。对依赖成组的资源需证明所有外部引用者及整组去向，不能只看单个 incoming 计数。
4. 监督者另发清理 lease；本轮 registry lease 本身不允许删。执行实现需另行审阅，必须精确到白名单包，禁止整目录删除、全局 Fixup、对 old redirector path 调用 EditorAssetLibrary.delete_asset。
5. 精确删除后保存逐资产结果、剩余 canonical 哈希、引用图、配置兼容关系及 Git diff；异常停下保留现场，不重做六 redirector。

本轮交付工具故意没有自动删除入口。没有获得零用途证据时，继续保留全部 583 个资产是审计结果。
