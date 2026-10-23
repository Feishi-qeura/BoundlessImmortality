# Windows 本机完整玩法验收包 v1.4

交付日期：2026-10-23。目标是用户确认的 Windows 本机运行与验收，配置为 Win64 Development / prototype_full。已生成实际分发 ZIP，使用随包 Python 和权威服务启动。正式抖音身份、支付、生产服务和平台审批仍是独立后续范围。

## 交付与启动

- 分发 ZIP：[BoundlessImmortality-Windows-Local-v1.4.zip](../Saved/Packages/WindowsFullV14_Final_20261023/BoundlessImmortality-Windows-Local-v1.4.zip)，1,170,675,194 字节，约1.17 GB。
- 已解压版本：[Start-Wuling.cmd](../Saved/Packages/WindowsFullV14_Final_20261023/Windows/Start-Wuling.cmd)。分发时交付整个 Windows 文件夹，或先完整解压 ZIP 后运行该文件。
- 使用说明：[请先阅读.txt](../Saved/Packages/WindowsFullV14_Final_20261023/Windows/请先阅读.txt)。系统缺少运行库时安装包内 `Engine/Extras/Redist/en-us/vc_redist.x64.exe`。
- SHA256：`092dda76b47e7562e956f0b6f16defe8353dc5599987501f8aec66aca30971d8`。ZIP 同目录附校验文件，Windows 文件夹内含942个文件的 `SHA256.json`。
- 用户存档：`%LOCALAPPDATA%/BoundlessImmortality/LocalV14/default`。关闭游戏后服务退出，数据库、身份和待确认事务保留。直接运行游戏 EXE 会使用旧默认模式，完整本机玩法请用启动脚本。

## 本轮补齐

| 范围 | 最终行为 |
|---|---|
| 独立启动 | 随包 Python 3.12、SQLite、标准库和许可证；随机回环端口、健康与规则哈希确认、同档互斥、持久身份、失败日志、所属子进程清理 |
| 安装与存档 | 安装目录不存玩家数据；支持独立工作目录、中文空格解压路径；解析 Windows 用户目录虚拟化后进行原子写入 |
| 前三地区域 | 青云山护阵；落霞谷第一阶段免费采药路线解封；玄铁岭第二阶段免费破障开启8秒弱点。主播按钮及观众口令均可触发，持续伤害遵守目标锁定，完整奖励预算保持80灵石、12破境石 |
| UI接入 | 世界修炼选区进入实际玩法、未开放地区拒绝进入、按钮状态按动作ID刷新、权益显示实际境界；本机业务导航移除旧医馆历练预览 |
| Cook修复 | 仅 Cook 实际天空神殿地图及已有字符串加载目录，保留存在历史损坏的 OverView 样例源地图；修复自动生成境界父材质的RGB/Alpha输入，34个裁切实例和原图保持 |
| 既有玩法随包 | 沿用组织成长、730项物品、556配方、制作重炼、天劫助力、装备功法、建设、开服活动、竞采租约、原型商店寄售、订单离线和审计实现，不建立第二套组织或经济系统 |

三地区域当前共用已有场景、通用目标和图集作为美术占位；后五个地区及高阶章节按策划分期未开放。七日签到沿用当前已有的“不发经济奖励”占位规则，没有借本轮素材占位授权编造奖励。

## 本轮实际验证

证据根目录：[ReleaseReadiness20261022](../Saved/Codex/ReleaseReadiness20261022/)。汇总：[verification-summary.json](../Saved/Codex/ReleaseReadiness20261022/verification-summary.json)。

| 检查 | 结果及边界 |
|---|---|
| 服务自动化 | 88项通过；包含三地区域免费机制、关闭窗口、完整结算预算、事务去重与经济边界 |
| 启动器集成 | 6项通过；真实HTTP、重启持久化、重复启动、规则错误拒绝及清理；玩家CMD/pythonw烟测通过 |
| Game / Editor | 最终 Development 编译成功；本机完整模式的Shipping禁用保护保留 |
| BuildCookRun | Win64 Development成功；Cook 0错误、0警告。末次纯C++修复另行编译并同步游戏EXE，最终报告确认包内与当前源码编译产物一致 |
| 项目原生自动化 | 92成功、1成功带警告、0失败；该警告是测试临时世界无上下文的清理诊断。加载阶段引擎自身另有诊断，未将整份日志声称为零Error |
| 竖屏实际包 | 720×1280，77检查、24截图，报告通过；真实Slate开始、选区、采药按钮和权威回执，15个功能页遍历，游戏及自建服务退出 |
| 横屏实际解压包 | 1280×720，77检查、24截图，报告通过；从ZIP实际解压到中文空格路径运行相同流程，游戏及自建服务退出 |
| 分发完整性 | ZIP CRC通过；解压942文件SHA256通过；安装目录无存档及运行缓存；包内服务源码、启动器与当前源文件一致 |
| 资产保护 | 1497个观察到的Content文件中1496未变，仅上述生成父材质修复；无缺失，地图、WBP、相机、武器资产未覆盖 |

竖屏：[verification.json](../Saved/Codex/ReleaseReadiness20261022/PortraitFinal/verification.json)、[采药回执截图](../Saved/Codex/ReleaseReadiness20261022/PortraitFinal/04-luoxia-gathering.png)。横屏：[verification.json](../Saved/Codex/ReleaseReadiness20261022/LandscapeFinal/verification.json)、[采药回执截图](../Saved/Codex/ReleaseReadiness20261022/LandscapeFinal/04-luoxia-gathering.png)。

早期屏外按钮验收失败及Cook失败记录保留，没有改成通过。代码复核发现的按钮名匹配及滚动顺序问题已修正，并由后续实际包回归覆盖。备份和逐文件差异见证据目录 `Backup/`、`Diffs/`、`changes.json`。

## 验证范围

本轮已验证当前Windows机器上的本机完整玩法包。服务测试的三地区完整结算使用受控时钟，打包渲染流程实际等待落霞谷第一阶段并操作采药机制，玄铁岭验证进入和中断；不能把它描述为三地区每一阶段都已在渲染包完整打通。

原策划284条UI状态×分辨率矩阵、物品与开服专项原场景未全部逐条执行，仍保留原验收状态。本包没有在全新系统镜像、Android/iOS设备、真实生产身份或正式支付回调上验收。既有完整玩法实现及此前实飞/双主播证据另见[上一轮实施交付](2026-10-22-wuling-v14-implementation.md)。

规则哈希为 `74c5225f635b0d2804ea5e3c04849355551c397ae05a3e3b39336e596c2b1340`。早期冻结规则数据库不自动迁移；规则不匹配时拒绝写入并保留原档，不应修改数据库哈希或删除事务文件来绕过。

后续重新构建及测试命令见 [Tools/WindowsRelease/README.md](../Tools/WindowsRelease/README.md)。
