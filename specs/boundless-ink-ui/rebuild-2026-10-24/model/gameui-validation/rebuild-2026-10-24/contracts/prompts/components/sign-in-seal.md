# sign-in-seal · default · v1

来源与用途：newUI统一宣纸/墨绿/细旧金语言；源候选见source-registry.json。授权连续试作，approved:false；本文件仅prompt准备，无生成/导入证明。

尺寸与状态：目标144×144，单组件default状态；实际像素/alpha/hash生成后读回。所有动态标题、按钮文字、日期、数量、奖励和签运由UMG绘制。

继承：style-contract.yaml colors/visual/geometry/effects。背景透明，主体纸面可不透明，外缘保留透明；左上柔光，哑光墨绿/宣纸、细旧金，低对比纸纹，完整轮廓，可排版中心无干扰。

图像生成提示词：单独的游戏UI组件素材，sign-in-seal，state visual only; actual state text separate，isolated transparent RGBA background, blank text-free writing area, warm xuan paper, restrained jade-green ink, fine aged-gold edge, soft top-left light, full uncropped silhouette, high-resolution compositing asset. Do not produce a full page, no labels, no words, no numbers, no watermark, no baked character or fake state, no white matte.

切片：full；margins=[0, 0, 0, 0]只是设计提案，实测前禁止作为已可用9-slice。复杂纹饰/印章不得拉伸。

验收：状态单独登记；实测尺寸/透明边界/hash/文字为空；拉伸预览后确定margin；真实Texture2D/引用回读另验。
