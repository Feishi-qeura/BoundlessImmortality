# 新体系业务/接入验收建议（未执行）

这里定义真实行为验收输入和观察点，不用复制实现的假测试代替引擎或服务验证。旧batch-2 review仅作修正输入，目标仍是全新44屏。生产改动归integration；本目录没有应用任何Source patch。

| 旧问题 | 新体系必须满足 | 最小有意义的验证 |
|---|---|---|
| PP-01 include搜索路径 | 新目录头使用模块相对路径，准确include真实公开API | promotion后由唯一执行者实际编译该translation unit，不能用文件存在替代 |
| PP-02 存档双时区 | UTC SavedAt只转换一次；服务日资格不由显示时区决定 | 隔离存档的固定UTC时间在UTC与香港显示下各核对，不写用户存档 |
| PP-03 设置自定义值丢失 | 即使quality/custom或45/120/不限帧，也显示实际有效值 | 导入实际设置状态到隔离实例，检查新可见标签，不查已脱树旧Label |
| PR-01 管理选择链 | 新成员行保留user id/Rows/SelectedMember/Remove权限与反馈 | 非空真实成员页选择目标，确认准确user_id；重复点击不重复入队 |
| PJ-01 树构造时序 | NativeOnInitialized或RebuildWidget前建新树；Construct只刷新 | 第一次CreateWidget->显示即可见；移出重挂后一次回调，根/事件仍匹配 |
| PJ-02 route类型 | 独立组合由真实page宿主包装，实际新root/返回栈持有 | ResourceDetail/WorldResult/RaceResult/Lease进入不同新PageKey，Back恢复原父页选择 |
| PJ-03 确认/请求态刷新 | 同条目快照刷新不清确认/请求id；换实体才有明确取消 | 确认期间/请求pending期间各来一次真实状态更新；仍绑定同一主体 |
| PA-01 enum名字 | 使用真实namespace/type；普通UButton不调用不存在的SetIsFocusable | UE5.8当前头声明核对+真实编译；不依赖unity偶然using |
| PA-02 异步刷新 | 所有新活动页有active/版本/状态驱动刷新；不每帧重建 | 开服操作入队、返回、离线/恢复、榜单更新均在不切页时正确变化 |
| PA-03 complete_rounds类型 | 已提供数组才以Num显示确认轮次；缺失unknown | 真实空数组=0，含2个真实round id=2，缺失=unknown；不是资格开关 |

其它必验业务：

1. 同一个缺字段值分别输入 unavailable/null、known/0、known/false 和明确空集合；画面与动作不得混同。合法0不可显示为“未同步”。
2. 正常与重炼使用真实协议Quote/Prepare/Rework路径：展示缺q_normalized/readiness/slots/sources/成长区间仍可发送原生合法校验请求；匹配错误selection、错误stage/kind、明确权限拒绝/active attempt仍阻止。服务继续验证库存、安全和活动；未把Quote成功当授权。
3. 真实背包至少含未绑定/预留/过期/装备/不同稳定id；排序/过滤/刷新后目标仍是同一EntryId，不按名字或可见index扣除。
4. 同一request_id重放只观察原receipt；库存/奖励/阶段不二次改变。新页面只复用现有outbox，不能每次刷新生成新请求。
5. 日签到确认后跨服务日或月签跨月，原日期动作应重新校验而非默改到新日期。香港时间只负责展示。
6. 三榜每类别至少一组非空提供者数据；真实名次含缺口/并列时不重排造名次，Local/Global断线互不借数据。头像异步返回后仍落到原participant，不因重挂而写错行。
7. WorldResult/RaceResult只匹配本round/race/actor的source receipts；另一个事件的收据不得显示为本次奖励。页面不drain根的收据队列，也不发伪ClaimSettlement。
8. 天劫Prepare/Trial/Reconcile/成功/失败/取消互斥；取消边界等于trial_start时拒绝，未知助力身份不显示可助力按钮，重炼不复用历史天劫概率。
9. 经济动作显示真实SKU/全部费用/数量/订单版本后确认；真实价格或SKU不存在只禁该购买，不虚构币种、购买资格、限购和奖励。
10. 程序入口类/保存WBP parent/newui-r1标记/活动树/字体纹理路径均指向newUI；缺新WBP时仍是同一新体系原生root，不静默回旧UI。Content/UI与Registry旧包归零的迁移证据由cleanup另交，不用新截图代替。

每页在540×960、720×1280、1280×720检查真实入口、长文字、滚动、Back、键盘焦点与触控热区。服务测试、native、PIE、截图、Cook/平台证据各自标记；本文件没有任何passed/runtime_verified结论。
