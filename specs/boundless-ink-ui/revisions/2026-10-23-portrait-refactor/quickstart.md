# 复现与状态

Python：C:/Users/zxxuh/.codex/skills/gameui/.venv/Scripts/python.exe，3.12.14；已验证PyYAML和Pillow。

1. UE5.8命令行执行本目录audit_assets_ue.py，读取/Game/UI并写registry-audit.json。命令参数为 `-run=pythonscript -script=<本目录脚本绝对路径> -unattended -nop4 -nosplash -nullrhi`。实际日志registry-audit.log保留。
2. 用上述Python执行本目录build_review.py生成逐页布局、资源迁移建议、哈希和review-validation.json。脚本仅写本修订目录，不移动资产。当前源码存在38个ClassForRoute显式路由，旧地图覆盖其中32个，补充6个。
3. 图片由内置image_gen生成，最终提示分别保存在home-v3-prompt.md和home-v4-prompt.md；真实尺寸941×1672，RGB，无透明背景。v3因生成环境偏离只保留探索记录；v4为待审稿，未拆组件。
4. 执行 `C:/Users/zxxuh/.codex/skills/gameui/scripts/validate_project.py D:/demo/BoundlessImmortality/specs/boundless-ink-ui`，结果见gameui-validation.txt。此校验覆盖旧正式设计契约；新修订使用review-validation.json另外核对，不把旧契约通过当新设计批准。

本修订沿用已批准的gdd/prd/interaction中的玩法与数据约束。保留范围和布局变更写在本目录draft契约，尚未提升为正式契约；旧文件批准记录不篡改。未生成最终资产包，故不声称strict最终资产交付。

未完成：背景实际复现与修复、资源迁移、其他页面实际视觉、组件切图、UE集成、新构建/PIE/Cook/设备测试。GameUI的基准页批准依赖尚未满足；请审阅README链接的v4与逐页布局后继续。
