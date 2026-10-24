# 完整 /Game/UI → /Game/newUI 迁移工具与回退证据

用户2026-10-24的新授权覆盖旧audit_only和精确六个范围门禁。目录移除已获授权，无需再次问批准。目录迁移与全新GameUI表现是两个必须分别验证的结果；本目录处理前者，不能用换目录声称新版页面完成。

**唯一 Editor/UBT/Content 执行者是 logic-integration**；cleanup负责方案、完整备份、精确工具与结果复核，不另开引擎。直接使用新根 engine-lease.json，**不要求cleanup单独lease或另加审批**。其他小队不能继续往 /Game/UI 导入。执行前由integration核对用户会话已正常保存/关闭，不强杀进程，不在有未保存内容或PIE的Editor里迁移。

## 已落盘的映射/备份

- `migration-map.json`：588个明确包，214623195字节；严格把 `/Game/UI/<suffix>` 迁移为 `/Game/newUI/<suffix>`，保留子目录/对象名。每包源路径/目标路径/旧字节/SHA256/类型hint都有记录。
- 无目标碰撞，目的根捕获时不存在。策略是任何磁盘、AssetRegistry或Windows case-fold目标碰撞都中止，禁止覆盖、去重或自动重定向到别人的资源。新面板资源需避开这588个目标。
- `class_hint`仅文件名约定，**不是本轮已查到的AssetRegistry类型**。`engine-preflight.json`必须补齐全部588包的真实类型、incoming/outgoing和referencer备份覆盖后才允许重命名。
- `backup-result.json`：1759个文件、1538854456字节，快照位于 `Saved/Codex/UIRebuild20261024/Backups/20261024T123117935118/`。完整项目Content、项目插件Content及Source/Config磁盘快照均已逐文件复制并核对SHA256；旧588包完全覆盖。未保存的Editor内存不在备份里，所以执行工具拒绝dirty/PIE。
- `reference-replacement-plan.json`：1857行代码/配置/工具路径证据；区分569个历史alias与588个本次迁移canonical。归档文件在Content外，不会重新生成 /Game/UI 包。

## 生产路径替换，由 integration 顺序实施

运行Source/配置默认加载路径应改为newUI；字符串拼接的目录前缀也要改。业务/图标后缀/对象名不变。历史alias保留old-name键，仅把destination直接指向newUI目标，避免old→old链。

新增588个迁移前canonical路径作为兼容old-name键，可以在精确表中登记；**不要在RenameAssets加载旧源包前启用这些canonical CoreRedirects**，否则`GetAsset`可能被别名引向不存在的新包。建议在迁移执行进程启动配置中只含不与物理源包同名的历史alias，迁移成功后再落盘canonical兼容alias并重启回读。C++ resolver同时保留历史key和迁移前canonical key，所有value统一newUI。不要使用MatchSubstring wildcard替换。

`InkUiAssetPaths.cpp`中的old keys及判断旧路径的compatibility分支可以保留；普通运行代码不能继续以旧路径作为默认加载，也不能用静默旧样式fallback掩盖新入口失败。Config `OldName`允许历史字面路径，`NewName`和默认asset配置必须newUI。

## 一次性执行流程

1. 把 `staged-source/NewUiMigrationExactFixup.h/.cpp` 复制到Editor模块Private，由integration统一构建。已有Build.cs含必要依赖，不新增UCLASS。控制台命令为 `BoundlessNewUI.FixupExactMigrationRedirectors`。
2. 在integration独占的正常GUI Editor中执行 `migrate_ui_ue.py` 的 `main('preflight')`。脚本读取当前granted/active新根lease、验证有效期、项目根、无PIE/dirty；完整同步registry，检查旧root精确588包/目的地无碰撞、档案hash、源hash、真实类和所有referencers的档案覆盖。未覆盖外部/Engine引用者直接中止。
3. 读preflight后执行 `main('migrate')`。它再跑新鲜preflight，强引用加载588个精确源对象，以单次588条`AssetTools.rename_assets`真正改package。保存明确dirty且在预查备份白名单内的包，记录before-rename/renamed_complete持久journal。失败保存asset delta，保留现场，不盲重跑。
4. Python在同一进程调用C++命令。Helper验证同进程完整rename/preflight、当前lease和所有备份覆盖，临时停用的也只是map旧canonical的精确process alias，退出时恢复；`LOAD_NoRedirects`取得每个真实旧redirector，核对对象完整路径及直接newUI目标，只把这批对象数组传给`FixupReferencers(...DeleteFixedUpRedirectors)`。已无redirector的映射源须连物理旧package也不存在，否则失败。没有global Fixup、AdvancedCopy、旧路径delete_asset。
5. 完整registry回读须旧root包数0、588新canonical均存在且类型/名称与preflight相同。Python须观察旧目录零文件，才逐层`rmdir()`移除空目录；有任何残留文件就拒绝，绝不先删目录或递归删文件。输出 engine-final-verification.json 及资产delta journal。
6. integration刷新Source/Config的canonical路径、构建并实际新入口验证。cleanup运行 `audit_migration_results.py` 独立检查报告、实际目录、新包hash/类型证据、改写referencer档案及生产路径残留。启动/route/UI视觉需integration另外提供证据，不用迁移报告替代。

在GUI Editor的Python文件入口可按下面方式导入执行（路径固定，不改项目脚本搜索配置）：

```python
import importlib.util
p = r'D:/demo/BoundlessImmortality/specs/boundless-ink-ui/rebuild-2026-10-24/migration/migrate_ui_ue.py'
s = importlib.util.spec_from_file_location('ui_to_newui_migration', p)
m = importlib.util.module_from_spec(s)
s.loader.exec_module(m)
m.main('preflight')  # inspect engine-preflight.json
# Same reviewed exclusive engine session, after the staged helper is compiled:
# m.main('migrate')
```

命令行文件入口用`-ExecutePythonScript=<绝对路径> -NewUIMigrationPhase=preflight|migrate|verify`，应为正常GUI Editor且Slate已初始化；不承诺nullrhi commandlet可运行Fixup。不要自动退出/停止用户Editor。`verify`可供后续独占会话回读，原进程的complete journal和helper报告必须仍在。

## 失败恢复点

- Rename前：snapshot与map已完整hash核验，可中止且不动Content。
- Rename中/之后：engine-migration-journal.json、engine-failure.json、asset-write-journal.json记录确切改写旧文件、新目标、before/after hash。所有可能被写的原项目Content都已有备份。不要盲重跑RenameAssets；先保留/正常关闭自己引擎并评估delta。
- `recover_snapshot.py plan`仅生成方案。它从真实写journal提取需要恢复的原路径、原始包字节和确切新目标，检查当前hash仍等于journal，不能覆盖后续新工作。
- 回退execute必须integration当前有效迁移lease，且tasklist无UnrealEditor/LiveCoding；提供刚审过的recovery-plan SHA256。工具先把当前迁移后字节再备份到Saved内，再将档案原始字节恢复**原路径**，只移除已核对hash且归档的本次新文件。没有整目录递归删除。此处是恢复原始合法package，不能用于把OS移动伪装成正常迁移。
- Source/Config自动回退被刻意排除，由integration对照同一snapshot单独复核差异，避免覆盖后来接入代码。恢复后需新鲜registry/路径/启动验证，不能直接称业务恢复。

```powershell
$py = 'C:/Users/zxxuh/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$team = 'specs/boundless-ink-ui/rebuild-2026-10-24/migration'
& $py "$team/verify_preparation.py"
# After actual integration engine execution:
& $py "$team/audit_migration_results.py"
# Only if a real partial migration needs operator-reviewed rollback:
# & $py "$team/recover_snapshot.py" plan
# & $py "$team/recover_snapshot.py" execute --confirm-plan-sha256 '<实际生成并复核的plan hash>'
```

snapshot只捕获一次，不覆盖现有migration-map.json；若源成员/hash在引擎运行前变化，必须保存现有证据并生成明确新revision，不静默把588快照当最新。备份存在与脚本就绪不等于真实迁移已完成；以engine结果和独立owner audit为准。
