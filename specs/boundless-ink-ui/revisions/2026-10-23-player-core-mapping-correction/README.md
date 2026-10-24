# 玩家核心候选映射修订

## 范围和状态

本目录将上一 revision 中两张内容互换的 v2 页面图复制到语义正确的新文件名，并将突破准备图复制为同组候选，方便一起审阅。它不修改原始 PNG、旧 revision、公共 manifest、共享契约、获批资源或 UE 代码。

三张图仍为设计候选，全部 `approved: false`。新文件名修正了页面与画面内容的对应关系，不代表新的视觉批准，也不代表运行时集成或验收。

## 页面映射

| 新文件 | 画面 | 来源文件 | 原始 SHA-256 |
|---|---|---|---|
| `main-menu-v3.png` | 主菜单 | `../2026-10-23-parallel-player-core/tribulation-live-v2.png` | `d980a8339ac1304a31b6f608e23a5549001281a87eeb140c51b4fa8679690f21` |
| `tribulation-live-v3.png` | 天劫进行中 | `../2026-10-23-parallel-player-core/main-menu-v2.png` | `0404447550aa3b0ea5b01288584e1b3839faaa92def0490ee9e056c18105e922` |
| `breakthrough-preparation-v3.png` | 突破准备空配置态 | `../2026-10-23-parallel-player-core/breakthrough-preparation-v2.png` | `82fd74b5b5d6f2cd60a048d03bdac13e33141d76fd1a7409d06a9013bd0f8e06` |

新文件为逐字节副本，不做像素编辑、裁切或重新生成。旧 manifest 中记录的 `source_file` 原样保留在本目录 `asset-manifest.yaml` 的 provenance 中；该字段记录原登记来源路径，不单独证明该外部文件与当前 PNG 字节一致。

## 突破准备占位项复核

画面能表达四个材料槽位（核心、护道、精华、奇珍）以及“拥有数量 / 所需数量”空值标签；这只覆盖类别和数量的占位表达。没有“查看来源”控件或来源状态，没有明确的准备度字段/状态，也没有可识别的预测结果字段。Q、品质区间、成长范围和天劫概率都只是空线/空框，其中天劫概率标明由服务端规则提供。

因此新 manifest 与验证记录将材料数量、来源、准备度、预测结果列为未配置 placeholder；不会把空框解释为服务数据，也不推断具体预测含义。需要 UI/服务契约补齐后再做下一版视觉稿。当前图中“等待配置”、空成本和灰态禁用 CTA 可保留为配置不可用状态表达。

## 审阅边界

- 主菜单候选中的山水纸底与抽象装饰仍是生成图内容，不能据此替换项目现有场景或人物。
- 天劫图的底部“天劫现场 / 规则说明 / 讨论交流 / 我的”导航装饰没有在当前页面路由契约中核准，不视为可交付控件。
- 未执行 UE 构建、PIE、Cook、设备或平台验收；本交付只核验文件映射及 PNG 文件属性。
- 后续合并公共登记时，应保留原始 v2 的 `content-mismatch-do-not-use` 历史记录，并单独登记本目录 v3 路径；仍不得标记为 approved。

## 文件

- `asset-manifest.yaml`：本修订的自包含候选登记和原始来源映射。
- `validation.md`：复制后实测尺寸、格式、哈希及视觉核验结果。
- `main-menu-v3.png`、`tribulation-live-v3.png`、`breakthrough-preparation-v3.png`：本修订候选图。
