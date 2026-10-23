# 本轮复现

继承正式已批准 gdd/prd/interaction 与水墨样式。用户本轮授权新 Widget、地图及真实 Unreal 接入；本轮变更契约为 design.md、screen-contract.yaml。设计修订不覆盖旧已批准底图。地图生成用内置 image_gen，提示词在 world-map-prompt.md、resource-map-prompt.md，PNG 在 assets/。图像不含文字，全部地名、参数与状态由 UMG 排版。

Python: `C:/Users/zxxuh/.codex/skills/gameui/.venv/Scripts/python.exe`，3.12，含 PyYAML/Pillow。UE: `F:/UNREAL/UE_5.8/UE_5.8`。

1. 使用引擎绝对 Build.bat 构建 BoundlessImmortalityEditor Win64 Development。
2. UnrealEditor-Cmd 执行本目录 import_assets_ue.py（-ExecutePythonScript=绝对路径 -nullrhi -unattended -nop4 -nosplash）。仅新增两张地图和五个新 WBP，不改旧关卡资源。报告 asset-import-report.json。
3. 原生自动化：Automation RunTests Boundless，-TestExit="Automation Test Queue Empty"。报告 automation/。HUD 公告修正复测为 automation-layout/。
4. 外部 Python 运行本目录 serve_qa.py；启动渲染编辑器执行 verify_pie.py，参数 -WulingPrototype -GWLocalTest -WulingUrl=http://127.0.0.1:18778 -WulingApp=journey-qa -WulingRoom=journey-room -WulingUser=journey-anchor。测试数据库每次独立生成；仅将月签次数=3作为明确测试输入，真实领取仍走服务事务。运行证据在 Saved/Codex/JourneyExpansion20261023/PIE/。
5. 外部 Python `-m unittest discover -s Tools/CultivationService/tests -q`；SQLite 临时库需要正常Windows文件权限。
6. 外部 Python `C:/Users/zxxuh/.codex/skills/gameui/scripts/validate_project.py D:/demo/BoundlessImmortality/specs/boundless-ink-ui --strict`。

编辑入口：五个 WBP 在 asset-import-report.json 中。WBP_JourneyMap 默认值 `Ink UI / Atlas` 的 WorldMapTexture、ResourceMapTexture 可调。资源点数量在 Tools/CultivationService/runtime_balance.json 的 map_resource_counts（每类0至8），数量决定地图可进入条目；该配置参与规则哈希，已冻结旧本机档需要使用原规则或新建隔离档。每个同类条目进入现有对应竞采类型，不冒充新的独立战斗地图。世界前三域进入现有场景/真实区域机制，后五域不可进入。

当前完成与实际验证结果见 README.md；文档校验、编译、自动化和 Windows PIE 各自限定证据边界。未打包新版可分发文件、未做手机设备或平台提审。
