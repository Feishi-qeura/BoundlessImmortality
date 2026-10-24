# Partial migration owner review — 2026-10-24 13:47 +08:00

> 历史部分状态，已由13:54:46实际引擎完成和14:13:17独立审计取代。当前migration_complete=true，Content/UI已不存在、旧registry0、582目标完整；47604已正常退出。见 final-independent-audit.md，勿用下述历史dirty/缺报告状态冒充当前现场。

**现场已越过旧Cancel问题。582条实际RenameAssets成功；四个旧真实Texture的精确引用替换成功且像素未变。最终引用保存、剩余旧包处置和旧根0仍未完成。** cleanup未开Editor/UBT、未写Content/生产源码，也未修改integration正在livebridge的migrate_ui_ue.py。

## 实际证据和边界

- build06与同PID47604的 `engine-rename-result.json`：passed/native_rename_succeeded=true，exact582，known_warning_ack_count=1、rejected_unknown_dialog_count=0；源码/配置提醒已受控确认，不再是当前根因。
- `engine-preflight.json`：588原物理包=582canonical+6既有redirector，589被覆盖包含唯一外部Sky_Temple地图。`engine-legacy-pin-result.json`：同PID pin6成功。
- `engine-migration-journal.json`仍是before_rename，原因是执行在后续dirty保存处失败，不能据此说Rename未发生，更不能重跑582 Rename。应结合实际native报告和恢复步骤继续。
- `four-texture-reconciliation-result.json`：13:39:43，四个实际旧Texture2D已Consolidate到精确newUI canonical，逐项after-export SHA与此前old/new导出相同。报告留下两个旧dirty包。
- cleanup于13:41:51只读观察：582个map内新文件存在、29个旧文件仍存在。这是有时间戳的物理观察，不替代引擎后续状态。
- cleanup独立报告 `revisions/v4-partial-importdata-review/four-texture-independent-audit.json`：四组共12个PNG的编码字节SHA、解码RGBA SHA和alpha SHA均逐组三者相同；11项实际Texture设置及尺寸前证据相等，ImportData身份共享且outer为new canonical。只重核相关8个包档案和612条manifest的SHA，没有重哈希原1759/部分612整档。

## 四对实际资产与redirector区别

四个source_old在执行前均由 `find_object(...follow_redirectors=False)` 加完整path/class核实为旧路径的真实Texture2D，区别于normal old-name lookup可能返回的新canonical。它们不是可以直接传给redirector删除API的对象。

四个new target均是map中已实际迁移后的真实canonical；old/new共享的InterchangeAssetImportData outer为new target，所以保存old package时引用了另一个包内的private import object。该证据解释保存失败，不能推断像素损坏。保留目标必须始终是new target，不能把旧Texture重新搬回或把其私有ImportData随意reparent来让旧包可保存。

已审阅reconcile_four_migrated_textures_ue.py：在第一项更改前检查全部4对的真实类/完整路径/不同对象/尺寸/11项typed设置/ImportData身份与outer、old/new PNG等价、相关partial源文件及备份hash；然后逐项调用原生 `EditorAssetSubsystem.consolidate_assets(new,[old])`，保存操作journal，再导出new验证像素hash。

本机 `EditorAssetLibrary.h:97–110` 明确Consolidate会替换已加载对象中的引用，删除指定旧source并留下target redirector，成功时保存被修改对象。这次是明确四个真实旧副本的引用协调，不是按名字通用去重，也没有对old redirector调用delete_asset。返回true支持这四项操作成功，**不能扩张为所有589引用者均最终保存、所有old包均已修复或新UI整体已验收**。

| 保留new canonical纹理 | 像素尺寸 | Alpha范围 | 三份PNG字节/RGBA/alpha |
| --- | --- | --- | --- |
| T_RankWulingPaperPanel_Default_v1 | 1448×1086 | 0–255 | 一致 |
| T_RankSwordRows | 887×1774 | 0–255 | 一致 |
| T_CohesionIcons | 1254×1254 | 0–255 | 一致 |
| T_PolishOrnaments | 1254×1254 | 0–255 | 一致 |

该像素结论针对现场导出源像素与已记录11项设置，不声称保存后新会话读回、GPU显示、Cook或设备通过。

## 剩余现场：不能省略的区分

13:36的旧对象快照里，除了4个Texture，还能找到实际旧ThemeData、3个FontFace、2个Font；同时有实际ObjectRedirector。四纹理协调之后需重新分类，不能把整29个旧文件都当redirector，不能把注册表查询empty当旧物理文件无数据。

真实旧Font/FontFace/Theme对象不能只因同名同类或包hash不同就删除；包重命名会自然改变包字节。若需要协调它们，应核实真实new canonical、归档覆盖、FontFace原字形数据/加载设置、Font composite/typeface/fallback结构、Theme参数与纹理字体引用；引用值需按map归一到new canonical后比较。不要把这四纹理的像素证明复用成其他类型的证明。

剩余两个旧dirty包是 T_XianxiaMeters(HUD) 与 T_HudLaunchCountdownFrame_v1(Activities)。13:43的 `remaining-two-save-result.json`记录完整ObjectIterator未发现这两个包的对象exports，SavePackages返回true但dirty列表仍有它们。不能以SavePackages=true直接推出dirty0或完成保存。

最新额外观察：integration的seq14 `retire_two_empty_source_packages_ue.py`在 `EditorAssetSubsystem.set_dirty_flag(package,False)`返回false处断言失败；`empty-source-package-retirement-result.json`为passed=false、operations=[]，并未到unload/最终registry报告。本机 `EditorAssetSubsystem.h:227–234`注释是“for an asset”的API，不能把对UPackage返回false解释成成功retire。不能单凭空operations推断flag绝对无副作用；下一步需现场再读dirty/完整对象/磁盘状态。此事实不等于本队建议关闭或清标记；不得以隐藏dirty状态替代真实数据保存/可追溯空容器处置。

可选只读辅助 `revisions/v4-partial-importdata-review/classify_remaining_old_ue.py` 供唯一integration在同PID调用：精确raw old与normal lookup分别记录，stable类名、物理包/registry/引用者、两个dirty包完整UObject内容区分；已兼容ObjectIterator返回native class wrapper的TypeError。不load/save/rename/delete、不清flag、不关闭进程。

## 最终独立审计契约

待真实恢复完成，必须审核完整七份机器证据：原五份 preflight / migration-journal / fixup-result / asset-write-journal / final-verification，加 legacy-pin-result 与实际native engine-rename-result。`controller phase=verify,state=passed`现在可能只是livebridge的诊断/四项协调结果，不能代替这七份。

1. 保留failed intent及部分现场备份，记录当前582真实rename成功、4项修复、剩余包/dirty处理与实际保存。只有保存边界确实完成，才可由唯一执行者以可追溯continuation记录推进renamed_complete；不能仅凭nativeRename成功修改状态字段蒙过helper。
2. 全部新582 canonical的真实类/对象名、保存后当前hash符合map/engine读回；六个原existing redirector只能指向这582内的真实目标，六个naive projected new packages须不存在。
3. 实际old对象必须先分清真实副本与raw redirector，最后只向精确map旧路径UObjectRedirector数组Fixup，禁止old-path delete_asset/globalFixup。剩余文件没有证明前不能OS整树删除。
4. dirty0、原有项目引用者保存均有实证；asset-write-journal列出实际改写原包与生成目标，分别对原1759档案与partial612档案覆盖追溯。复用已核验档案manifest/SHA，按变化/疑点抽查，不重复哈希不变整档。
5. 当前/full mounted AssetRegistry旧root包0、582类名稳定，已保存依赖图无仍指向旧root的hard/soft/searchable/registered management边；物理Content/UI必须不存在。只有零文件、零旧包后才逐个rmdir空目录。
6. Source/Config普通默认canonical全部newUI，old-name仅显式兼容表；新默认WBP/native root及真实业务入口由integration另外验，不用目录迁移代替新UI表现/视觉批准。

已提供 `audit_engine_reports_v4.py`：当前只读执行结果识别实际582rename=true，明确缺 engine-fixup-result.json / engine-final-verification.json、旧目录仍在，**不宣告完成**。完成后它检查完整七报告与实际当前582输出、兼容路径残留、档案覆盖；不重算整1759/612档案。

可选唯一integration只读 `verify_saved_reference_graph_ue.py` 在真实保存/精确fixup/旧空目录移除后回读完整mounted依赖图、582稳定类型和dirty列表，输出本revision下post-save-reference-readback.json，供离线审计消费。它不代替缺失保存步骤，也不由cleanup自行启动引擎。仍须保持当前47604现场，无rerename、无另开UE/UBT、无未经现场确认的关进程。
