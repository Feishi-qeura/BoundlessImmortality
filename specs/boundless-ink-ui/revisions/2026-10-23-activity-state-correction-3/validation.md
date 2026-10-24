# 活动状态修订 v3 验证记录

## 范围与来源

- 仅复用已批准的活动 UI style-contract、对应 activities/launch screen-contract 与现有活动组 v2 文字 QA；未查看任何项目原有 UI 截图。
- 已查看活动组 v2 的 `activity-center-v2.png`、`launch-event-v2.png` 两张候选及其文字 QA。确认旧活动中心同一画布含空态和 loading skeleton 两种互斥状态；旧开服图用里程碑奖励标题和多个空 pedestal/node 视觉暗示具体奖励档位。
- 新输出仅写于本 revision 子目录；未改共享清单、共享 contract、旧候选或 UE 源码。

## 视觉检查

- `activities-empty-v3.png`：单屏、单一“日常”选中状态；四个类别签标签正确；中央只出现“暂无活动”和“当前分类暂无可参与活动”。没有 loading skeleton、占位行、活动卡、奖励、进度、倒计时或领取动作。宣纸/墨绿/青玉/少量旧金方向符合共享契约。
- `launch-milestone-pending-v3.png`：时间、任务、贡献均明确待同步；里程碑仅用普通信息卡写“服务端数据待接入”及阈值/进度/奖励待权威数据返回说明。没有里程碑节点/数字、奖励图示、进度条、完成/锁定标记、领奖/购买 CTA。中文可读，信息区块分离清楚，整体风格与契约一致。

## 文件度量

由 GameUI 隔离 Python 环境中的 Pillow 实测尺寸与模式；SHA-256 由 PowerShell `Get-FileHash -Algorithm SHA256` 计算。两图实际比例均为 941/1672 = 0.562799，接近目标 9:16；RGB、不含透明通道。

| ID | 文件 | 尺寸 | 模式 | SHA-256 | approved |
|---|---|---:|---|---|---|
| activities-empty-v3 | `assets/pages/activities-empty-v3.png` | 941×1672 | RGB | `0896129F9B0C91F04FDF25495B630A9640303132713D17781FB63E5DD0E73338` | false |
| launch-milestone-pending-v3 | `assets/pages/launch-milestone-pending-v3.png` | 941×1672 | RGB | `1E7BD5B9DEC1AE860F03A300760499FD0CB8A47ECFCC2ADC4FE7C3BF27FFE538` | false |

## 限制

- 这是未批准的候选视觉稿；AI 图中文字虽经人工查看可读，但正式 UMG 应使用运行时字体重新排版。
- 生成图的安全区/点按尺寸是视觉参考，未在 UE 中测量；未做响应式变体、UE 导入、编译、PIE 或设备验收。
- 不代表业务 API 已提供数据；凡缺字段均以待同步/待接入文案表达。

- 独立 manifest 一致性脚本：2/2 资产路径、提示词路径、approved=false、SHA-256、尺寸与模式均匹配。
- GameUI project validator：python scripts/validate_project.py D:\demo\BoundlessImmortality\specs\boundless-ink-ui → OK，0 warning(s)。
