# Windows 完整玩法打包实施计划

> **For agentic workers:** Use superpowers:executing-plans inline. 不委派实现；用逐文件备份和 SHA256 记录替代 Git 提交。

**Goal:** 交付可在没有 UE 编辑器及系统 Python 的 Windows 机器上启动的 v1.4 完整本机玩法包，并以实际 Cook、打包和运行证据标明可发布边界。

**Architecture:** 保留现有 UE、Python/SQLite 权威、数据表和人工资源。用随包 Python 标准库运行服务及启动器；服务绑定随机回环端口，启动器确认规则版本后启动游戏，游戏退出后只关闭自己启动的服务。完整本机包使用 Development 构建，Shipping 继续禁止模拟身份。生产抖音适配不由本机启动器替代。

**Tech Stack:** UE 5.8 C++/UMG，Python 3.12 标准库，Windows Win64，BuildCookRun。

**Spec:** docs/design/2026-10-21-wuling-resource-v1.4/README.md、01–16及 tables；用户 2026-10-22 授权继续实现剩余功能，允许缺失素材使用占位符，并选择 Windows 完整玩法本机运行及验收。

## Global Constraints

- 保护现有地图、镜头、WBP、Niagara、已配置礼物及旧成长档。新增代码不覆盖资产。
- 素材缺失允许占位；规则、身份、经济事务及完成状态不可伪造。
- 不将 prototype_full 改为可公开连接真实身份或支付；不解除 Shipping 防护。
- 用户数据位于 Windows 用户本地应用数据目录，不能写只读安装目录；包可移动，启动工作目录不限。
- 拒绝覆盖非空交付目录、运行中编辑器及包内 QA 夹具；规则哈希不匹配停止并保留数据库。
- 不自动部署、上传、发布平台或获取凭证。实际包、原生测试、渲染、正式平台证据分别记录。

## Review Focus

1. 安装路径含空格/中文且当前目录无关时，正确找到运行库、规则和游戏。
2. 服务启动失败、退出或规则不一致时不能进入无权威的完整玩法；清理只影响本次子进程。
3. 重复启动或端口占用不撤销另一个游戏的同身份会话；同一存档目录只能有一个启动器。
4. 打包不能漏掉字符串加载资源、Python DLL、SQLite 扩展及权威规则；不能依赖开发机绝对路径。
5. 经济命令重试与旧档保护继续成立；退出保留数据库及 outbox，重启能读原档。

## Tasks

### Task 1: 服务就绪与生命周期

Files: Tools/CultivationService/http_server.py、__main__.py；Tools/WindowsRelease/launcher.py；Tools/WindowsRelease/tests/test_launcher.py；Tools/CultivationService/tests/test_http.py。

Interfaces: health 返回 service/profile/rule_version/rules_hash；服务 --port 0 --ready-file 启动并原子写实际端口；launcher 用规则清单校验健康状态，UE 参数继续使用既有 WulingUrl/App/Room/User。

- [x] 增加真实 HTTP 健康检查、动态端口及错误/退出测试，先运行确认缺口。
- [x] 实现服务就绪文件、连接超时、包启动器与用户目录锁、日志及退出清理。
- [x] 跑服务全量与启动器集成测试，验证重启持久化及重复启动拒绝。

### Task 2: 完整包构建和完整性检查

Files: Tools/WindowsRelease/package.py、README.md；必要的精确 Cook 配置；包内 manifest/Start-Wuling.cmd。

Interfaces: package.py --engine --python --output；只接收空交付目录；生成独立 Python 运行库、规则和服务副本、SHA256 清单。

- [x] 构建 Development 游戏与 Editor，修复真实编译阻塞。
- [x] 执行 BuildCookRun（Win64 Development，默认地图和字符串加载的玩法/UI目录），保留日志。
- [x] 携带 Python 可再分发运行库和许可证、服务和规则；排除 QA、数据库、密钥及开发缓存。
- [x] 从独立工作目录使用包内运行库执行启动器烟测，核对清单、状态及进程退出。

### Task 3: 运行验收与实际缺口修复

Files: 实际发现的最小功能源码；Saved/Codex/ReleaseReadiness20261022 下证据。

- [x] 运行项目原生自动化；针对失败或真实操作缺口先写回归测试，再修复。
- [x] 实际运行打包游戏确认主菜单、进入游戏、服务连接和可操作功能；截图保留。
- [x] 汇总策划剩余条目、已实现/已验证/未验证状态与素材占位，不将占位当业务实现。
- [x] 最终核对改动和受保护资产，交付包路径、启动方式及仍需发布环境验证的限制。

## Ledger

- 2026-10-22：原目录无 Git。备份在 Saved/Codex/ReleaseReadiness20261022/Backup，哈希 baseline-hashes.json。
- 基线服务80项，沙箱阻止HTTP与硬链接两项；已安排相同命令在授权环境复核，未修改测试掩盖限制。
- Ruling: 用户已明确开始实现并选择 Windows 本机完整版，直接执行本计划；不再要求额外策划审批。
- Ruling: Development 是支持完整本机模拟的交付配置，Shipping 原型禁用是既有保护规则，不能把删除保护当成功发布。
- 2026-10-23：服务88项、启动器6项通过；最终代码原生92成功+1警告、0失败。警告为测试临时世界无上下文的清理诊断，未隐藏引擎加载诊断。
- 2026-10-23：前三地区域已实现差异机制；服务回归涵盖免费采药路线、8秒弱点、持续伤害锁定和同额完整结算。原奖励预算及后五区分期保持。
- 2026-10-23：首次全图 Cook 暴露 OverView 样例地图历史外部 Actor 问题；改为实际默认地图精确 Cook，源地图保留。仅修复一份自动生成的境界父材质 RGB/Alpha 输入，34个裁切实例及原图不改。最终 Cook 0错误、0警告，BuildCookRun成功。
- 2026-10-23：Windows 用户目录虚拟化导致跨卷原子重命名失败，改用解析后实际用户路径；Python标准库 SSL 依赖相邻 PEM 文件，使用随包 Lib 文件夹并保留原信任策略。
- 2026-10-23：竖屏原失败保留；零尺寸屏外按钮未滚动及按无后缀控件名刷新状态已修正，后续竖屏真实点击77检查通过、24截图，服务退出正常。最终 C++ 游戏编译产物同步入包，其 SHA256 和源服务副本最终交叉核验。
- 2026-10-23：分发 ZIP 已通过 CRC及实际解压942文件哈希校验，137个开发运行缓存已移除；玩家 Start-Wuling.cmd 的 pythonw 服务烟测通过。
- 2026-10-23：最终横竖屏各77检查、24截图通过；横屏来自ZIP真实解压至中文空格路径。942文件校验及源代码副本一致，1496/1497 Content不变、唯一生成父材质修复、无缺失。最终交付报告 docs/2026-10-23-windows-local-v14-release.md；未把原284条UI矩阵或全部专项状态改为通过。
