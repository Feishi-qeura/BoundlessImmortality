# /Game/UI → /Game/newUI 最终独立审计

2026-10-24 14:13:17 +08:00，cleanup只读复核完成。**本次资源目录迁移完成：Content/UI物理目录不存在，实际引擎报告旧根注册表为0，582个真实canonical完整保留。** 新UI整体表现、完整default PIE/44屏及用户视觉验收另行记录，不影响该目录迁移结论。

机器报告：`revisions/v5-final-independent-audit/final-owner-audit.json`；静态路径证据：同目录 `canonical-source-config-audit.json`；可复核脚本：`final_owner_audit.py`。脚本本轮已运行成功，未启动Editor/UBT、未写Content/生产源码、未删除资产、未重复1759/612整档哈希。

## 完成证据

- 完整七报告（preflight、pin6、实际nativeRename、可追溯journal、fixup、实际write-journal、final-verification）相互核对：同PID47604、同一精确map SHA `2fcf7fcb432a6066a8ecfa880e376129a3bddd9326abed53f86fd2f2168cbdac`，源588=canonical582+既有redirector6。
- 582条原生Rename实际成功，窄范围源码/配置提醒确认1次、未知弹窗确认0次；不存在重新执行582Rename来掩盖保存失败的证据。恢复continuation明确记录native_rename_not_repeated=true、dirty_packages=[]。
- `engine-migration-journal.json`于13:53:09推进renamed_complete，保留原失败before_rename/failure/write-journal在 `logic-integration/migration-attempts/05-recovered-save-failure/`。状态推进前有真实四纹理和八副本协调、582保存类核对、dirty0，而非仅修改字段绕过helper。
- 精确原生Fixup最终只处理29个剩余UObjectRedirector；逐项old_object均属于原map，destination均为正确new canonical，incoming_before_fixup全为空。29是当时实际剩余redirector数量，不能误要求固定588个仍存在。源588完整处置、582唯一canonical和6既有redirector覆盖仍成立。
- `engine-final-verification.json`于13:54:46 passed=true：old_registry_package_count=0、old_directory_exists=false、582类/文件/hash正确、6legacy removed、6伪projected new路径不存在、unexpected_modified_packages=[]。
- 本轮现场重新读文件：Content/UI仍不存在。**582个迁移目标逐个当前SHA与引擎最终报告一致**；该报告的stable class均等于preflight真实类且非ObjectRedirector，因此以当前字节同一性验证现有582仍是引擎已核实的canonical。
- 当前Content/newUI有587个uasset：582迁移目标加4个新Player组件纹理及 `/Game/newUI/Runtime/WBP_NewUIFrontend`。这5个是后续新增资产，不应误报迁移多出5个或把迁移范围扩为587。
- 原owner migrate_ui_ue.py已恢复 `cbbe4812a141893ecc24b6636653c161cc96ed65e3b4315dde4250dc665e2155`，restore_required=false。最终验证执行副本只把Python未暴露的remove_path换为newUI路径强制rescan；本队逐字核对这是唯一替换，原成员/类/hash/旧文件/写范围条件未被删改。
- migration Editor日志记录正常退出，控制器seq24经dirty检查请求正常退出，本次PID47604查询无结果。本队没有关闭/强杀任何进程。

## 八副本及像素/字体保留

四Texture的既有独立报告继续有效：4组旧/新/协调后PNG字节、RGBA及alpha一致，11项设置和尺寸对等。该证据没有被扩张成全部材质/GPU/Cook验收。

本轮独立重核 `eight-source-copies-proof.json` 与其协调结果：8组精确old→new均属于canonical582映射，均非把原existing6 redirector当资产合并。2个Texture（HUD countdown、XianxiaMeters）旧/新/协调后PNG字节和解码RGBA一致；ThemeData、3个FontFace及2个Font的T3D在仅将旧canonical根归一后结构完全相同，协调后new T3D与协调前new原文一致。

3个FontFace的源字体文件hash/字节数与证明一致，完整原字体payload仍能在**归档旧package**和**当前new package**中找到；旧物理路径已移除，不能再要求读取当前旧文件来证明保留。这里只针对这3个有具体payload证明的字体，不泛化到未知资产。

四纹理与随后八副本协调后的数据已经落实为正确新目标，剩余两个空包容器的问题也由后续native reload/精确协调完成。13:43/13:45仍dirty或set_dirty_flag=false均是保留的历史失败，不再是当前状态；不能再据旧记录声称47604必须一直保留或旧目录尚未清。

## 引用保存和回退覆盖

最终write-journal实际改写/移除589个原包：全部落在preflight精确白名单589内，before SHA与已核验原档案manifest一致，备份文件存在；当前原路径状态逐项符合after SHA/不存在记录。582新目标实际hash与最终报告一致。唯一旧根之外的必要原包是已备份的Sky_Temple引用地图，未发现意外作用域外改写。

原1759档案manifest SHA `625eef06abf776f245406e0991bd8b563bfc6b5c51a206fa8bbb84826a0b0980` 与部分现场612档案manifest SHA `5d918ea9d0e27d645a381890ee42c869371c5d20420c614416b5fa291c531ba2` 保持一致。复用此前逐文件校验链，本轮没有重哈希整档。选取的四纹理8份档案及字体payload证据独立复核已分别留报告。

回退范围必须区分：原snapshot/journal可追溯588旧包、589被写原包与582迁移目标；**后续4个新组件纹理、新Runtime WBP及后续Source/Config epoch不属于原recovery工具的一键整体回退范围**。若将来需要回退，先对当前额外5资产及新源码引用另存checkpoint并协调引用一致性，不能直接运行旧计划删除582目标而留下新WBP悬空。本轮没有回退。

## Source/Config 窄静态审计与未声明证据

本轮重新扫描生产Source的cpp/h及Config ini：1151个C++旧名兼容项和1151个CoreRedirect OldName均与resolved mapping精确一致、目标指向582真实newUI canonical。普通源码/配置无白名单外旧根引用，newUI实际字面路径对应文件存在；必要旧词仅保留在显式兼容key/guard、列明的历史精确迁移helper及测试fixture中，逐行分类在机器报告内。所有读取文件hash留档，无检测到读取中改写。

单独的“全部mounted包每一类依赖均无任何旧root边”的新报告未提供，本队没有补造或声称已运行它。当前保存引用结论基于同PID实际Rename/dirty0 continuation、精确29旧redirector incoming0、589实际写包白名单/档案覆盖及当前目标hash。额外完整依赖图负向读回可作为后续独占Editor验证项，**不把这个未追加的证据项或未完成UI视觉验收写成‘Content/UI仍未删除’**。

目录迁移状态：migration_complete=true。完整新版UI/default PIE/响应式/44屏与视觉批准由integration及用户继续验收，本审计不替其签署。
