# 无极仙道 · 水墨UI设计项目
当前阶段：**首页组件已生成，待审阅**。首页v2、规范和屏幕地图已批准，无需重复确认。

## 本轮成果
- [首页组件总览](component-review.html)：18个独立透明无字素材，可切换棋盘/白/墨底与状态示意。
- [组件总览截图](evidence/component-review-checker.png)：已在浏览器实际渲染；另有白底/墨底检查图。
- [组件拆解说明](components.md)、[开发交接](component-handoff.md)、[组件CSV表](evidence/home-components.csv)。
- [组件契约](contracts/component-contract.yaml)、[九控件七态规则](contracts/home-state-recipes.yaml)。
- [透明PNG审阅包](packages/home-components-v1-review.zip)：当前18张素材、提示词与契约；组件仍待用户批准。
- [已批准首页v2](assets/pages/home-v2.png)、[视觉规范](contracts/style-contract.yaml)、[44屏地图](screen-map.md)。
- [图像检查](evidence/component-image-review.json)、[严格校验](evidence/validate-components-strict.txt)、[本轮保留核对](evidence/component-preservation.json)。

## 范围与状态
保留运行主页面、角色属性、背包、场景与相机。18个当前PNG都是真实RGBA透明素材，姓名/数值/按钮标签不烘焙；另有2张停用修订稿留档。9控件的63条状态通过共享图和参数表达，不冒充63张新状态PNG。
图片提示词均由内置image_gen执行，原始PNG未裁切、改色或缩图；HTML仅以测量的Alpha内容范围作无损预览。所有素材本身尚未获用户批准。
本轮保护文件670/670哈希一致。设计包--strict校验通过，0错误/0警告；这不证明UE、真机或平台运行。未生成Atlas或UMG/uasset，也未将审阅包标成最终批准资产包。

## 下一步文本
`使用 $gameui，读取本项目；批准首页组件当前版本，继续组件切图与9-slice审核。`
