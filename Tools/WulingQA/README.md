# 武灵 v1.4 运行与验证

本轮沿用现有 UMG、水墨资源、地图和剑灵轨迹，新增功能页面和权威数据接入，跳过独立 UI 视觉重构。交付范围为 Windows 本机 `prototype_full`；正式身份、生产服务、平台审批、移动设备与后续地图不在当前运行证据内。

## 启动

在项目根目录启动本机服务：

```powershell
& 'C:\Users\zxxuh\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' -m Tools.CultivationService
```

默认 `127.0.0.1:18770`，数据库 `Saved/WulingPrototype/v1.4/prototype.sqlite3`。另开编辑器或游戏时传入：

```text
-WulingPrototype -GWLocalTest -WulingApp=local-v14 -WulingRoom=room-a -WulingUser=anchor-a -WulingUrl=http://127.0.0.1:18770
```

两主播共用相同 App、服务与规则，分别使用不同 Room/User；建议各自指定不同 `-saveddirsuffix`。相同观众 User 切房会撤销旧会话，不能保留两个收益身份。新模式需要服务在线；首次启动不会赠送 QA 夹具的修为、物品或公库资金。模拟观众及礼物沿用原 GM 测试工具，不能将模拟事件当正式平台回调。

入口：游戏内“扩展”→“武灵历练”。“玩法功能”包含炼制、来源、突破天劫、装备功法、建设、活动、竞采、商店和寄售；“组织与记录”包含成员/改名/解散、订单、离线记录和剑灵权益。现有角色、背包、签到/月历与资源页面接权威状态。观众通过原模拟弹幕入口发送“加入”“护阵”“破障”“共修”“助力”“确认离线收益”“展示青云踏飞雁”等业务口令；具体阶段与操作可用性由服务判定。

新库、命令 outbox 和旧成长档/礼物权益分开，不自动迁移。退出页面不撤销已提交的经济事务或天劫；断线后以同一 request_id 对账。

## 调整配置

- 原始策划源：`docs/design/2026-10-21-wuling-resource-v1.4/`，未改动；规则由 `RuleBundle` 校验导入，730 项物品、556 项配方、34 境界。
- 实际飞行采样后的目标血量系数：`Tools/CultivationService/runtime_balance.json` 的 `flight_health_factor=0.15`。只应用到运行时世界目标模板副本，不修改源 encounters.csv、奖励或飞行曲线。原单人 Boss 360 血在当前实际命中频率下超过阶段预算；校准后 54 血，实飞第三阶段约 100.6 秒完成。
- 已配置 `BP_LiveWeaponSwarm`、武器 DataAsset、相机及 WBP 默认值保持。天劫预警使用 participant/source/generation 租约；道具表现克隆角色已有 InkCultivation Niagara 资源到独立组件，过期/断线销毁，不改源组件。
- 规则文件和运行系数进入 manifest SHA256。改规则后旧库会拒绝 hash 不匹配写入；使用新的开发数据库，或单独实现显式迁移，不能直接篡改库中的 hash。

## 验证命令

```powershell
python -m unittest discover -s Tools/CultivationService/tests -v
python -m Tools.WulingQA.verify_two_process
python -m Tools.WulingQA.verify_two_ue
python -m Tools.WulingQA.check_preservation
python -m Tools.WulingQA.build_acceptance_report
```

使用上文 Python 绝对路径替代 `python` 即可。测试只访问本机回环；受限沙箱可能需要允许回环、进程或硬链接测试。两 UE 进程脚本采用明确的服务测试时钟及 NullRHI，不能作为渲染或设备证据。

渲染功能检查：先启动 `python -m Tools.WulingQA.serve_rendered_qa --port 18771 --short-visuals`，按同样 App/Room/User 启动 UE，并传 `-ExecCmds="py D:/demo/BoundlessImmortality/Tools/WulingQA/verify_final_interactions.py" -Render -unattended`。夹具使用隔离 SQLite、预置物品和修为、固定天劫 roll；`--short-visuals` 仅将 QA PROP03 时长设为 8 秒且进入 QA manifest，默认服务仍使用策划时长。完整页面脚本为 `verify_gameplay_pie.py`。

真实飞剑验证使用 `serve_world_qa.py` 与 `verify_world_flight.py`；脚本观察游戏产生的真实碰撞/命中命令，完成阶段后才跳过剩余等待，不调用 SubmitStrike 伪造命中。证据在 `Saved/Codex/GameplayPlan20261021/WorldFlight/`。

最终原生报告 `NativeDeliveryFinal/index.json`；服务日志 `service-suite.log`；渲染报告 `Rendered/pie-report.json`、`FinalInteractions/report.json`；双 UE `TwoUE/report.json`；双 HTTP `TwoProcess/report.json`。均位于 `Saved/Codex/GameplayPlan20261021/`。确认报告结果、检查过程和退出日志，不能只依据进程退出码。

## 本地经营审计

```powershell
python -m Tools.CultivationService.metrics --db Saved/WulingPrototype/v1.4/prototype.sqlite3 --start 1800000000 --end 1800086400 --out Saved/WulingPrototype/v1.4/audit.json
```

时间参数为 UTC Unix 秒，区间左闭右开。导出使用只读数据库连接，拒绝覆盖数据库及 WAL/SHM/同文件别名，不导出会话 token 或玩家身份。包含规则/窗口、已确认命令、实际世界响应参与率、完整轮完成率、回访、币物产消和付费来源分组；真实收入/转化为 `null`。物品审计与每日到访从安装此版 schema 后开始，不倒推或伪造旧档流水。

## 验收边界与回退

`acceptance-report.json` 逐行保留物品 30、开服 30、UI 功能状态 284 项。19 条原始专项用例有直接完整断言；其他条目有相关模块测试但完整原场景未独立验证，保持 `not_run`。284 条要求指定状态及全部分辨率，不能用部分 720/1080 竖屏和 1280 横屏截图统称通过。纯视觉规范另列 `excluded_by_user`，没有把 UI 功能排除。

停止本机服务和编辑器后备份新数据库、WAL 与新 outbox；移除 `-WulingPrototype -GWLocalTest` 可回到原模式。源码回退按 `changes.json`、`SourceDiffs/` 和 `preservation-final.json` 逐文件核对；当前目录无 Git，禁止用整套旧备份覆盖同时进行的已授权 UI 修改。原基线只保护列出的 272 个文件，不能当作整个 Content 的备份。
