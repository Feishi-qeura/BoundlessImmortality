# 武灵 v1.4 本机开发服务

仅用于已授权的封闭原型。没有正式身份登录、真实支付或生产发布能力。现有礼物映射、旧成长档、人工资产不由此服务迁移。

在项目根目录启动（使用本机可用的Python 3.12+）：

```powershell
& 'C:\Users\zxxuh\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' -m Tools.CultivationService
```

默认监听 `127.0.0.1:18770`，数据库位于 `Saved/WulingPrototype/v1.4/prototype.sqlite3`。可通过 `--port`、`--db` 指定独立实例；同一数据库由SQLite写事务串行提交。不要将其绑定公网或使用真实平台身份。

UE开发运行需同时加 `-WulingPrototype -GWLocalTest`。可选 `-WulingApp=... -WulingRoom=... -WulingUser=... -WulingUrl=http://127.0.0.1:18770`；不同主播实例用相同App、不同Room/User。新outbox在 `Saved/WulingPrototype/v1.4/`，不读取原v1.1队列。

现有扩展页→武灵历练保留创建、名册和世界入口；新增物品炼制、来源、突破天劫、装备功法、建设、签到开服、竞采、原型商店和寄售子页沿用原UI壳。功能是否完成以执行报告为准，入口存在不等于全部交互验收已通过。

测试：

```powershell
python -m unittest discover -s Tools/CultivationService/tests -v
```

测试数据库在 `Saved/Codex/GameplayPlan20261021/TestDatabases/` 的临时目录。HTTP测试只使用临时回环端口；受限沙箱可能需要批准本机回环访问。

协议为 `POST /v1/wuling/session`、`sync`、`state`、`command`。command携带唯一request_id、expected_version、profile、rule_version、command、args；相同请求提交后丢响应必须用原request_id重试。服务先查原收据再判断旧版本；不同payload复用ID会拒绝。客户端不能自报owner、roll、helpers、damage、score、online_seconds或occurred_at。

所有币物扣除/产出、事务收据都在同一SQLite事务中。命令预留使用服务内部命名空间，不能用客户端request_id消费市场托管或天劫预留；到期不延长来源期限。服务定时推进已开始事件，界面关闭不取消已经开始的天劫风险。

停止服务后可备份新数据库及WAL，再用原版本代码/新库恢复。旧档与旧礼物文件保持独立。当前规则hash不符时拒绝继续写入既有库；更改规则需新运行版本或单独的显式迁移，不能静默替换。
