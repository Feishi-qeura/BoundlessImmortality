# RenameAssets false — 2026-10-24 13:08:49 modal cancellation

> 历史失败记录，已非当前阻塞。build06/同PID47604的实际582Rename及窄范围warningack已成功；后续四纹理ImportData保存恢复见 `partial-importdata-review.md`。不能用本历史Cancel记录覆盖当前部分迁移现场或推断零磁盘变化。

根因已有直接日志证据：**源码/配置/文本引用警告返回 Cancel，随后 RenameAssets 返回 false**。不是当前已证实的磁盘ACL或路径碰撞失败。

## 证据与边界

- `logic-integration/migration-editor-04.log:468`：该GUI进程带 `-unattended -nop4 -NoAutoSave -NoLiveCoding`。
- 同日志2374–2389：05:08:49.043 UTC（13:08:49 +08:00），`Message dialog closed, result: Cancel`；消息为“源代码、配置INI和文本文件可能需要查找/替换”，列11个名称，末尾“否则烘焙版本可能缺失资产。继续重命名？”
- 同日志2390–2397：13:08:58.508 +08:00，Python报告 RenameAssets false。源码文本提示取消发生在它之前。
- `migration/engine-preflight.json`：PID9420，13:08:45.610 +08:00，通过588物理源/582真实canonical/6既有redirector；注册表10566项，589包的备份覆盖已通过。
- `engine-legacy-pin-result.json`：同PID成功pin6；`engine-migration-journal.json`只到before_rename（exact_count582）；`asset-write-journal.json`13:08:58.505的原文件delta0、新canonical0。
- 日志352–355、1690：source control禁用。没有本次rename的checkout/只读/碰撞错误。启动时的aqProf/Vtune/SDK/插件图标/测试日志发生在此警告之前，不能当本次rename根因。

磁盘delta0 **不单独证明** 所有内存对象仍保持旧身份。当前没有每对象old/new identity的独立内存快照，不能反推出582对象内存绝对未改名。本队没有保存、重试或关闭Editor。

本次只读复核还观察到controller结果已由integration推进到sequence3/quit/normal_shutdown_requested（13:11:05）；controller的quit分支在任意dirty Content/Map存在时会拒绝。此后本机Get-Process -Id9420无返回。这是integration自己已请求的正常关闭状态，不是本队建议关掉现场，也不能伪装成关闭前逐对象identity报告。下一次执行前仍需独占执行者记录新鲜磁盘/hash/源对象身份及dirty情况。

## 本机 UE5.8 参数/实现边界

已安装文件直接核对：

- `Engine/Source/Developer/AssetTools/Public/IAssetTools.h:89–152`：FAssetRenameData的Asset/NewPackagePath/NewName及bOnlyFixSoftReferences/bAlsoRenameLocalizedVariants。当前582条填的是actual asset+new parent path+same name，符合真实rename构造。不能把OnlyFixSoftReferences设true来冒充迁移。
- 同文件473：RenameAssets只有const TArray参数，返回bool；477的RenameAssetsWithDialog额外bool是bAutoCheckout，**不是跳过源码警告的开关**。
- `Engine/Source/Developer/AssetTools/Private/AssetRenameManager.h:68–78`：FixReferencesAndRename及FindCDOReferences声明，后者明确收集CDO hard/soft references；121–124明确PerformAssetRename在选择继续后执行。这与日志的源码/配置警告取消吻合。
- `Engine/Source/Runtime/Core/Public/Misc/MessageDialog.h:43–48`：带DefaultValue的Open在Unattended时记录并返回DefaultValue。结合命令行与实际Cancel，确认当前无交互警告被取消；不把它变成用户尚未授权的流程门禁。
- `Engine/Source/Runtime/Core/Public/Misc/CoreDelegates.h:271`：ModalMessageDialog为四参数delegate，签名`EAppReturnType::Type(EAppMsgCategory,EAppMsgType::Type,const FText&,const FText&)`。
- `Engine/Source/Runtime/Core/Public/CoreGlobals.h:181`：GIsRunningUnattendedScript；App.h:490–495提供IsUnattended。仅从启动参数移除-unattended仍需考虑Python调用作用域的script-unattended状态。

**安装目录没有AssetRenameManager.cpp或MessageDialog.cpp，只提供相应private/public headers。** 本分析不伪称已读到该版本cpp的完整分支/默认值常量。根因由日志原文确证；无交互策略由实际头文件接口支持，需integration编译并验证。

## 最小受控建议

保留582+6精确方案、全部备份和引用保存校验。不要更换成OS移动、globalFixup、直接Rename UObject、soft-only操作或bAutoCheckout猜测修复。

下一次由唯一integration执行者采用**仅包住这一次582条RenameAssets调用**的native scoped handler：正常GUI不带-unattended；在该native作用域内保存/临时清除GIsRunningUnattendedScript，确认FApp::IsUnattended为false，然后保存并替换ModalMessageDialog delegate。只在消息结构及11个已知名称全部精确匹配（均来自批准的582表）且消息类型是OkCancel时返回Ok；所有未知消息Cancel并记录。使用scope-exit恢复delegate和script flag。若实际消息type不同，先记录type，不扩大自动响应。

这是对已授权完整路径迁移及Source/Config替换风险的**窄范围技术确认**，不需要用户再次审批。不是所有弹窗自动接受。若仍有-unattended或delegate未被触发，不应称修复生效。

不能直接重跑现有main('migrate')：before_rename intent journal会正确拒绝重跑。先由integration用只读新鲜dirty/old-new内存identity、588源hash/582目标不存在验证，无部分迁移才归档本次失败intent/controller/log/delta（保留不删除），再由独立revision执行受控重试。若发现内存改名或dirty/磁盘delta，保留现场并按真实状态处理，不清空journal强行重试。

本队只在新目录 `revisions/v3-known-warning-cancel/` 暂存诊断/建议，不修改正在加载的migrate_ui_ue.py或生产helper。实际构建/执行及最终零旧目录/registry证据仍归integration。
