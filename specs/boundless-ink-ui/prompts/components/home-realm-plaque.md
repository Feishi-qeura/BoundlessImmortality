# 境界空白牌
## 来源与用途
来源：已批准assets/pages/home-v2.png；组件ID：home-realm-plaque；类别：panel；页面图中的参考范围[22, 178, 160, 47]，仅定位，不做硬裁切交付。
## 尺寸和状态
目标素材尺度[384, 128]，实际输出尺寸与Alpha另行实测。图像状态default。交互态通过独立state-recipes记录，共享基础图，保持轮廓。
## 必须继承的契约字段
style-contract.colors/visual.materials/visual.lighting；透明；无文字；完整轮廓；source=home-v2。
## 图像生成提示词
完整最终文本：home-realm-plaque-default-v1.txt。使用内置image_gen，逐组件单独调用。
## 反向限制
无文字数字/伪字/标签/截屏背景/合图；不截断、不抠字留洞；不擅自改变材质。
## 验收记录
已生成；下述是当前版本实测结果，原图不覆写。

本轮实测：2170×725 RGBA，四角透明，Alpha内容框[101, 136, 2068, 590]；当前版本home-realm-plaque-default-v1。已逐图查看无文字和截图背景。状态generated，等待用户审核组件本身；首页批准记录见approval-home-v2.json。
