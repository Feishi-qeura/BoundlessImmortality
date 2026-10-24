# migration handoff — 唯一执行者 logic-integration

新授权已消费，本队不等待自己的引擎 lease，也不启动 Editor/UBT。integration 是唯一 Content/Editor/构建执行者，本队负责精确工具/备份和回读审计。

## 已可消费

- `migration-map.json`：**588** 个精确 `/Game/UI/... → /Game/newUI/...` 包映射，保留 `Ink/` 后缀及对象名；旧总字节 **214623195**。目的地不存在、Windows case-fold 一对一无碰撞。任何后来目标碰撞必须中止，禁止覆盖/合并去重。
- `backup-result.json`：**1759** 个文件 **1538854456** 字节完整备份并逐文件hash核对（旧UI+全部项目Content/项目插件Content+Source/Config磁盘快照）。manifest：`Saved/Codex/UIRebuild20261024/Backups/20261024T123117935118/backup-manifest.json`，SHA256 `625eef06abf776f245406e0991bd8b563bfc6b5c51a206fa8bbb84826a0b0980`。
- `reference-replacement-plan.json`：现有INI alias和Source/Config/Tools行级证据，canonical迁移与历史old-name键分开。production只由integration改。
- `migrate_ui_ue.py`：已暂存 preflight/rename/save/fixup/verify流程；**等待integration构建及实际API验证，不可把脚本当迁移成功**。
- `staged-source/NewUiMigrationExactFixup.cpp/.h`：已齐，拷贝到 `Source/BoundlessImmortalityEditor/Private/`，现Editor模块依赖已含AssetTools/AssetRegistry/UnrealEd/Json/Slate，不新增UCLASS/UHT。注册控制台命令 `BoundlessNewUI.FixupExactMigrationRedirectors`。raw LoadObject使用LOAD_NoRedirects，验证精确旧对象和newUI目标；仅map旧路径数组Fixup，回读0旧包及588新canonical。
- `preparation-verification.json`：2026-10-24 12:37:20 +08:00 六项离线检查通过，1759份备份全部重新hash，无失败；C++尚未编译、Unreal API尚未执行。
- `recover_snapshot.py`：真实delta journal驱动的精确原路径回退；恢复前再归档当前字节，拒绝后来hash变化，拒绝引擎/LiveCoding仍运行。Source/Config不自动覆盖。
- `audit_migration_results.py`：执行后本队独立核对目录/registry报告/588新canonical类与hash/所有改写原包备份/运行Source和Config旧路径残留；当前已运行pending模式，明确缺五份真实engine结果，没有冒称完成。

## 立即执行次序/防冲突

1. integration 现在可消费映射/备份/helper，拷贝staged helper后统一构建。脚本直接消费新根现有 `engine-lease.json`（granted/active、sole_executor=logic-integration）；**不要求本队单独lease或新增审批**，不在 /Game/UI 新增资源。
2. 在映射源包被UE RenameAssets之前，**不要新增588个 canonical旧路径→newUI的CoreRedirect**，否则正常GetAsset加载可能先被别名导向不存在的newUI。本轮历史569个old-name键可保留，但destination要统一新canonical；运行代码改newUI由integration时序协调。
3. 单次明确588包 `AssetTools.rename_assets`，不是OS移动；源类型必须由engine preflight补齐，不把class_hint当现场类型。preflight输出每包类型/referencers/dependencies和备份覆盖清单，任何未备份外部referencer中止。建议先完成迁移+fixup再导入其他newUI新组件，避免中途新增未备份且指向旧路径的新referencer。
4. 精确迁移后调用新的C++ helper，仅使用map中已迁移旧路径对象数组FixupReferencers。不能用六redirector旧helper，不能global Fixup，不能delete_asset。
5. 最终新canonical存在且非redirector、旧root registry=0、旧目录零文件，才以逐个rmdir删除空目录。若存在任何残留文件，停下，不能整树删除。

详尽执行/回退说明已在本目录README.md；status=tooling_ready_for_sole_executor。脚本执行：在唯一闲置GUI Editor的Python文件入口加载本文件，用模块 `main('preflight')` 或 `main('migrate')`；命令行文件入口显式加 `-NewUIMigrationPhase=preflight|migrate|verify`。Fixup需Slate，使用正常GUI Editor而非nullrhi commandlet。先preflight确认类型/覆盖，再migrate。执行后本队消费 engine-preflight.json、engine-migration-journal.json、engine-fixup-result.json、asset-write-journal.json、engine-final-verification.json 做独立审计。当前 **migration_executed=false / old_directory_removed=false**，没有完成移除声明。
