# 首批 Unreal 接入与功能验证结果

2026-10-24，Asia/Hong_Kong。首批范围为 **5 张纹理、2 个保留式 WBP 迁移，以及对应源码/玩家入口/权威事务接入**。本批已完成，自己启动的 Editor 与服务均正常关闭。完整机器结果见 [engine-final-verification.json](engine-final-verification.json)。这不表示 44 页整体重制完成；用户视觉验收保持待定，候选仍为 `approved:false`。

| 证据 | 实际结果 |
| --- | --- |
| [build-11-result.json](build-11-result.json) | Editor Development 最终构建 exit0；[compiled-module-proof.json](compiled-module-proof.json) 固化模块与当前源码快照哈希 |
| [automation-03/index.json](automation-03/index.json) | 最终 build11：21 成功，0 警告/失败/未运行，进程 exit0 |
| [pie-07/report.json](pie-07/report.json) | build09：84 步/76 检查全通过，真实 Slate 玩家入口与 HTTP 权威路径 |
| [pie-04-boundaries/report.json](pie-04-boundaries/report.json) | build10：24 步/30 检查全通过，库存拒绝、响应丢失恢复、三个窗口尺寸的真实入口 |
| [pie-05-viewer/report.json](pie-05-viewer/report.json) | build11：6 步/11 检查全通过，真实 viewer 身份、FORBIDDEN、开服按钮禁用且无开启请求 |
| [pie-10-frame-final/report.json](pie-10-frame-final/report.json) | build11：18 步/19 检查全通过，最终纸框显示与原生按钮实际点击、横竖屏路由 |
| [fresh-ue-readback.json](fresh-ue-readback.json) | 新进程读取 5 个已保存 Texture2D 设置；2 个 WBP 编译及榜框硬引用通过，进程 exit0 |
| [unreal-batch-result.json](unreal-batch-result.json) | 5 张实际 UE 导出 PNG 与源图 alpha 逐像素相同；两 WBP 已保存，备份保留 |

## 实际业务覆盖

玩家从主菜单进入 HUD，经活动、签到/月签、排行榜、仙途节点详情、武灵、建设/工具菜单进入突破。实际领取签到和月签宝箱，创建武灵、开启活动、选择材料并取得原生报价；缺展示字段仍能发送正常准备请求。NOT_SAFE 拒绝、STALE_VERSION 后复用同 request_id 恢复、Pending/重复点击门控、正常突破结算、重炼、HTTP 离线/重连、跨日及跨月刷新均有实际证据。

材料不足通过 UE 子系统发送真实请求，服务返回 `INSUFFICIENT_FUNDS`，尝试未创建、核心材料保留。响应丢失测试是在服务**真实提交后**关闭该次响应，客户端复用同 ID 重试，得到同回执，签到累计只增加一次。观众通过真实服务签发 viewer 身份，UE 请求得到 `FORBIDDEN`，不能通过 UI 开启活动。

以上服务均为本目录独立 SQLite/隔离存档，明确使用 10 月 31 日晚的服务时钟与声明的测试材料/随机数夹具；没有修改正式规则、生产数据、用户存档或 Config。请求入队、服务成功回执和最终结算分别检查。

## 渲染修复与保留边界

实际 PIE 定位并修复了 NativeConstruct 太晚建树、月签内容裁切、日格标签重叠、HUD 倒计时遮挡入口、提交后按钮禁用滞后。29 条诊断修复记录覆盖 17 文件，每次均有准确备份、前后哈希和授权快照，见 [engine-source-repair-ledger.json](engine-source-repair-ledger.json)。[current-source-proof.json](current-source-proof.json) 验证原冻结清单不变、9 项有界授权差异，无意外漂移。原导入批次及 WBP 迁移前基线未被重写。

HUD 候选九宫格在紧凑尺寸下未通过实际 UE 绘制验证；[PIE09 对照](pie-09-countdown-diagnostic/report.json) 中仅临时改为 Image 就显示了同一纸框。最终使用已验证的 **整图绘制**，原 PNG/面板清单保持不变，差异记录于 [integration-rendering-override.json](integration-rendering-override.json)。这不构成九宫格已验收。

22 张实际截图附尺寸与 SHA256。初始 720×1280 请求得到的窗口截图实测为 **720×1282**；显式调整后的横屏/窄竖屏分别实测 **1280×720、680×1280**。这是 Windows PIE 证据，没有替代设备、Cook、打包或平台/生产验收。

首次迁移曾出现一次 GUID ensure；保存后新进程重新编译与引用回读通过，未在该回读复现。所有早期失败构建/PIE/诊断报告保留，没有用后续通过覆盖失败记录。

## 交接

[owned-process-cleanup.json](owned-process-cleanup.json)：本批 Editor/服务 PID 均为空，Editor end_play/quit_editor、服务 `/qa/shutdown` 正常退出，未使用强停。Git diff --check exit0，Config 状态为空；未 commit/push、清理旧资源或提升 batch2。

剩余页面/组件/权威字段缺口继续以 [screen-coverage.md](screen-coverage.md) 为准；局部纸框替换不是整页重制。已通过 [next-batch-request.json](next-batch-request.json) 请求监督复核后另授下一批提升/引擎租约，当前没有自行提升。
