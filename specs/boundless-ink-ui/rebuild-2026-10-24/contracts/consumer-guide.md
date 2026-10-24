# 四组与接入队消费顺序

1. 先读 `../quickstart.md`、`../architecture-and-interfaces.md`、`../asset-root-contract.md`；新 root、生产writer和newUI根以owner文件为准。旧审批/保留布局限制已被新授权覆盖。
2. 读取本目录 `style-contract.yaml` 和 `design-system.md`，使用 newui-r1 唯一令牌；不要各组另造字体/间距/按钮体系。
3. 在 `screen-contract.yaml` 取自己owner的行，结合 `screen-briefs/<id>.md` 组页。44屏稳定ID来自现有清单，但旧保留/旧布局/旧WBP默认路径没有继承。leaderboard一行包含全部三榜，wuling一行包含总览/成员管理；子变体不能省掉，也不多报整页数量。
4. 用 `component-contract.yaml` 的数据/事件slot和 `state-contract.yaml`，把真实值传进新组件。36项是组件规格，不是36份已完成控件；4个raster prompt只是准备，实际尺寸/alpha/hash仍为空，9slice margin未实测不可直接导入。32项文字/数值/普通控件优先原生实现，需视觉纹理时再登记对应资产，不重生整套页面。
5. 业务层读取 `../model/vm-contract.yaml`、`action-contract.yaml`、`screen-bindings.json`。VM名是声明式数据模型，不假装已生成25个C++类型。已有准确handler_ref可复用，新宿主presentation hook函数名由integration发布后对齐，不臆造API。
6. 每片2~3页：交独立组件树、实际字段与动作slot、源/目标路径hash、layout marker、透明组件信息和未完成项。PageKey和兼容route分开；WorldResult/RaceResult/Lease必须有真实独立宿主，不沿用旧误别名把它们全送回通用live页面。
7. `coverage-matrix.csv` 是r1契约基线，当前44行都只到contract_ready，不能把它当实时运行完成表。integration的实际验收账本以 `runtime-evidence.schema.json` 的形状增量记录，带真实class/WBP/树/资源/截图/功能证据；不要重跑生成器覆盖已积累的验收证据。

任何新运行资源仅 `/Game/newUI/Runtime`、`/Game/newUI/Shared`、`/Game/newUI/Components/<team>` 等owner允许分组。需要保留的历史数据图标/字体按cleanup映射迁移，但移了目录不等于新版页面。候选页图保持provenance，不作为运行整页纹理。删除旧目录/Registry归零由cleanup和integration串行证明，契约不执行该删除。

优先带入 `../model/regression-acceptance.md` 中10项既有问题；新体系不限于修这10项。尤其保存时间一次转换、保留设置实际值、名册管理选择链、早期建树、同主体pending不清零、活动持续刷新、complete_rounds数组，以及普通UButton真实公开API。

验证记录：`../model/validation-result.json` 是契约引用/结构、令牌对比度与隔离GameUI规范快照结果；不是引擎测试。`../model/gameui-validation/` 是带哈希可复核的验证副本，不是第二设计事实来源，不应让四组引用该副本。
