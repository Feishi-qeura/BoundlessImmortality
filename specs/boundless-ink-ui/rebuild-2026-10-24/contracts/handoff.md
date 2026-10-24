# logic-core · newUI统一契约交接

本次按新 quickstart 全面重做外观，旧Main/HUD/Character/Inventory保留限制已取消。只写新根 contracts/ 和 model/；未编辑生产Source/Config/Content，没有执行引擎或资源删除。13张指定新候选已实际查看并实测尺寸/模式/hash；不是旧项目图研究，也未将候选升为批准。

## 四组现在可消费

- `style-contract.yaml`：newui-r1 唯一颜色/字体字级/间距/88触控/720×1280响应/按钮和状态令牌。所有新运行包仅 /Game/newUI；已对齐owner的Runtime/Shared/Components分组。
- `design-system.md` + `consumer-guide.md`：页框/导航/场景保护/整页重排/合并片和消费顺序。新默认root以integration的 `architecture-and-interfaces.md` 为准：UInkRebuildFrontendWidget，/Game/newUI/Runtime/WBP_NewUIFrontend。
- `component-contract.yaml`：36项可复用控件数据/事件/尺寸/状态规格，32原生优先、4raster prompt准备。PNG尺寸/alpha/hash与9slice必须实际生产后测量；当前设计margin均不可作为已验证值。
- `screen-contract.yaml`、`coverage-matrix.csv`、`screen-briefs/`：44屏逐项列当前真实入口/解析route、独立新PageKey、VM、动作、设计来源、合并片与真实阶段。三榜/成员/开服子变体已列；局部组件不得冒充整页。全部当前只记contract_ready，新实际class/WBP和运行证据留空。
- `source-registry.json`：13张候选的当前实测文件/尺寸/模式/hash和已查看状态；地图/签到/三榜/武灵/活动修订/player v3的语义按正确源图登记。
- `state-contract.yaml` + `runtime-evidence.schema.json`：unknown/empty/0/false、pending/offline、合法业务和展示缺口边界；每页真实可见新UI的证据槽。
- `../model/vm-contract.yaml`：25个声明式VM，准确来源与缺口；不是已写入生产的C++模型类型。
- `../model/action-contract.yaml`：76个UI意图到现有handler/outbox的映射；保留权限、版本、忙碌、库存、确认和服务日期。viewer助力不变成anchor按钮，领取/到账不靠本地推算。
- `../model/regression-acceptance.md`：旧review10项带入新体系，并补真实非空数据/错误主体/重放/日期/结果来源/新root的聚焦验收建议。未运行镜像“假测试”。

## 验证

使用 GameUI隔离Python，执行 `model/build_contracts.py`、`model/build_models.py`、`model/validate_contracts.py`。详细计数见 `model/validation-result.json`：44屏、36组件、25VM、76动作；30项契约引用/结构/哈希/对比度检查通过。真实GameUI `validate_project.py --strict` 在本队owned的契约/规划快照上通过，0错误/0警告，日志 `model/gameui-validation.log`。

该快照仅用于校验，不是第二设计事实来源；未对共享根未属本队的GDD/PRD等文件作越权补写。JSON Schema文件只做解析/必要包络检查，没有安装或声称运行通用meta-validator。纸面文字/正文与主按钮令牌对比度实算>=4.5；实际栅格拼装对比度/字形仍待视觉验证。

## 诚实边界与下一步

4个新raster仍pending-generation；actual dimensions/alpha/hash为空。所有新页源组件/路由/导入/玩家入口/业务验证仍分别待执行，视觉pending_user，approved:false。架构定义的具体presentation hook签名未公布，四组用独立组合+数据事件slot先推进，由integration适配，不能臆造新宿主API或把当前契约写成已兼容。

Owner在每片2~3页真正接入后，把实际root/route/class/新树版本/canonical资源/截图与业务证据写进自己的运行账本，并按runtime-evidence schema分辨 partial_component / runtime_visible / business_gap；不要重新运行契约生成器覆盖运行证据。cleanup继续独立完成完整备份、明确迁移和旧目录/Registry归零，两项结果不能互相替代。

特别修正：现有服务报价缺q_normalized/readiness/slots/sources/成长预测只保留展示unknown，不再据此禁止原生合法请求；current stage/echo/kind、权限/连接/忙碌、服务最终库存与安全校验仍保留。GiftRules完整效果目录、个人剑灵完整模型、资源贡献独立榜、部分fortune/外观权益仍明确business gap，不为画面补数或宣称功能通过。
