# Windows 完整玩法本机包

最新竖屏 UI 版本见 [Windows 竖屏交付报告](../../docs/2026-10-23-portrait-ui-windows-release.md)；默认540×960竖屏窗口，可自行调整窗口，显式分辨率参数优先。此前玩法交付保留在 [本机玩法报告](../../docs/2026-10-23-windows-local-v14-release.md)。

按照已确认的 v1.4 策划继续使用现有权威服务。随包 Python 3.12、规则、启动器和用户存档使完整玩法脱离编辑器、系统 Python 和开发机目录运行。采用 Development 构建，Shipping 的原型身份防护保留。

在工程根目录使用实际 Python 和引擎路径运行：

```powershell
python -m Tools.WindowsRelease.package --engine F:/UNREAL/UE_5.8/UE_5.8 --output Saved/Packages/WindowsFullV14
```

脚本拒绝非空输出目录。先构建 Editor，再执行上面的 Cook；只指定实际使用的天空神殿地图，UI 字符串加载目录依 DefaultGame.ini 全量入包。角色展示用 OverView 样例地图有历史外部 Actor 损坏，不作为玩法地图打包，源文件保留。BuildCookRun 日志在输出根目录，独立可分发内容在其 `Windows/`。不自动上传、签名或部署。

```powershell
python -m unittest discover -s Tools/WindowsRelease/tests -v
python -m Tools.WindowsRelease.package --output Saved/Packages/WindowsFullV14 --verify-only
& 'Saved/Packages/WindowsFullV14/Windows/Runtime/python.exe' -I 'Saved/Packages/WindowsFullV14/Windows/LocalService/launcher.py' --profile package-smoke --smoke-check
python -m Tools.WindowsRelease.verify_package --package Saved/Packages/WindowsFullV14/Windows --evidence Saved/Codex/ReleaseReadiness20261022/PackagedPortrait --profile acceptance-portrait
```

玩家双击 `Start-Wuling.cmd`；见包内说明。启动器仅监听回环，自动绑定空闲端口；确认服务规则哈希后才启动游戏。存档在 `%LOCALAPPDATA%/BoundlessImmortality/LocalV14/<profile>`。同档重复启动被拒绝；子进程的 stdin 生命周期管道保证启动器退出也会通知服务停止；服务崩溃时游戏保留待确认事务并展示断线状态。

缺失美术允许使用既有通用资产占位。前三个区域共用现有场景，机制分别为护阵、第一阶段免费采药路线解封、第二阶段免费破障开启弱点。区域参数在 `Tools/CultivationService/runtime_balance.json` 的 `region_mechanics` 中，弱点默认8秒，随规则哈希冻结。直接伤害与持续效果都遵守目标锁定；来源专注、阶段奖励和竞采标准值保持。新本机档不自动迁移旧冻结规则档。

后五个独立区域依策划分期保持未开放；本机包不替代生产身份、付费履约和平台审批。

`verify_package` 从与安装目录无关的工作目录运行包内 Python 和游戏，使用显式隔离 profile。只有 `-WulingPackageAcceptance -WulingPrototype -GWLocalTest` 同时存在才启动 Development 包体验收；真实 Slate 点击、服务回执及截图在用户目录的 `PackageAcceptance/` 留证，随后复制回工作区。该流程不赠送物品或修改策划概率。素材生成父材质的 RGB/Alpha 连线修复脚本为 `repair_realm_material.py`；原纹理和34个境界裁切实例保持。
