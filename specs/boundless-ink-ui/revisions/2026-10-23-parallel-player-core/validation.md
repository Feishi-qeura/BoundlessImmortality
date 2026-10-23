# 验证记录

- 用户指定范围复查：主菜单/HUD、角色属性、背包、突破准备；没有执行 UE 客户端修改。
- 获批保护基线：读取根 `contracts/asset-manifest.yaml` 的 `home-v2` 与 `home-components-trim-v2-package`，均 `approved: true` 并有批准证据路径。本分组未写上述文件或其资产。
- 页面状态交叉检查：读取根 `contracts/screen-contract.yaml`、根资源清单、portrait-refactor `README.md`、`hud-menu-report.md`、`hud-portrait-correction.md`、`screen-contract-proposed.yaml` 及对应主/角色/背包/突破 brief。确认主菜单、角色、背包根契约未批准视觉；portrait-refactor 提案把突破运行实现记录为批准，但根资产清单未登记突破页图。
- 外网来源：搜索并打开 TapTap《六界仙尊》官方入驻页，确认明确竖屏修仙品类；官方完美世界竖版 UI 文章搜索摘要可读、页面打开失败；诛仙手游官方界面介绍搜索结果可读。均仅文字研究，不下载图像。
- 生成图片人工检查：已用图像查看器打开本分组突破候选。画面内容可见且分区清晰；实际尺寸 `941×1672 Format24bppRgb`，SHA256 `025B4E5C8F89093F5A4AEEBF3D9F0836DA41D89925751985F6F95FB1DC0C3B86`。上一轮哈希校验失败原因为 YAML 值误录为65字符；修正为64字符 SHA-256 后与实测值一致，图片本身无哈希冲突。比例不是严格720×1280。
- 主菜单候选：已生图并人工查看，空白中央保留角色插入位，底部“开始游戏/继续游戏”和存档/设置/退出清楚；生成器仍添加了山水纸底，因此只供布局审阅，不能替换既有场景。实测 `941×1672 Format24bppRgb`，SHA256 `AA8924F782A39CD15CC86153242E77FAEA863A885B7570DC990644138BB8F400`；比例不是严格720×1280。
- YAML：用 GameUI 虚拟环境的 PyYAML 成功解析资产提案；逐项检查两图登记 `approved:false`。本次已把突破图清单哈希修正为实测的64字符值。
- 天劫候选：纯文字提示单页生成，未引用本地图片。人工查看941×1672 `Format24bppRgb` 图，SHA256 `67CA77B4F40973471CB88931AD4E2D305D3B10BE13FE177202B91E301767897F`；比例近似9:16但非严格720×1280。图内无概率/奖励数值；成功/失败并列框按互斥模板处理。
- 本次本目录文件结构/YAML验证待执行；UE构建、运行、PIE、Cook、设备验收均未执行，本分工不作这些通过声明。
