# 首页组件切图复现
输入为18个已批准独立RGBA素材，非带文字的UI截图。
运行authoring/build_home_slices.py：以Alpha>1/255测量边界，保留框内RGBA字节，丢弃框外最多1/255的透明噪点，四周增加8px全透明留边；不改色、不缩图、不覆盖原文件。
独立素材再以已知坐标无损汇集为真实source-sheet-v1.png供溯源；按显式bbox输出，避免发丝/镂空/多团云的连通域被误拆。该汇集图不是优化Atlas。
九宫格参数按实际切图像素测量，测试native/wide/narrow/tall四种尺寸；预览允许确定性9-slice缩放，原始批准图和正式切图不重采样。
