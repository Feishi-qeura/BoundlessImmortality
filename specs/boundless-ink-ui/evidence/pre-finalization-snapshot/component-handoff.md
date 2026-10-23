# 首页组件交接
状态：18个组件已批准。原v2计划图10/10、v3必要图4/4已检查；v3仅narrow纸纹中心压成竖带未通过。v4已生成排行纸底narrow Fit候选，当前只剩1张必要图待查；未启用候选，未完成本阶段验收。

## 文件
18个当前透明无字PNG在assets/components；组件表见evidence/home-components.csv；逐组件提示词在prompts/components；版本与实际尺寸在contracts/asset-manifest.yaml。修为填充和头像采用v2，旧v1保留并标stale，不供新装配。
图像已逐张看过无文字、无截屏矩形背景；Alpha数值检查见component-image-review.json。所有当前图RGBA、四角透明，头像框和齿轮中心透明；头像v2已补留边。纸底内部完整、不通过抠字留下孔洞。

## 装配步骤
1. 保留当前真实3D场景、主角、相机和运行HUD。只将本包作为获准后接入的独立UI素材，禁止覆盖保留角色/背包资产。
2. 以原始941×1672页面为对照，source_bbox提供大致区域；换算逻辑720×1280时约乘0.765，不把生图像素直接当UE坐标。源参考范围含装饰，按钮热点另按SafeZone与88×88逻辑单位核对。
3. 原图content_uv保留供溯源，实际裁切PNG在assets/sprites/home-components-trim-v1/items。3个9-slice候选为轨道、顶部底板、排行纸底；其余15项保持full/Fit。参数来自裁切后原生像素，顺序left/right/top/bottom；轨道与排行纸底锁定高度、只拉伸X，顶部底板允许X/Y。详见sprite-manifest与home-9slice-review；当前参数仍待完整视觉审核和用户验收。
4. 头像先圆形mask再叠透明框；血量/修为先共享轨道，填充在轨道内mask按真实fraction裁切，数字单独TextBlock。头像未知沿用现有占位，不将修士肖像冒充真人。
5. 顶部tile在下、独立图标在上、动态标签单行；底部云带和阅读渐变不接收点击。预览标签只在预览输入启用。
6. 九控件七态见home-state-recipes.yaml，63条各自登记；共享基础纹理+参数，不是63张已生图。pending不可模拟成功，返回保持可用；修炼/收功由实际快照决定。绑定ActionId前核查当前源码。

## UE导入建议（未执行）
新建独立目录导入，保留RGBA；检查UI压缩、sRGB与线性采样、直通Alpha/材质混合是否匹配。半透明云雾与发光用实际UI材质验证，不将背景黑色烘进PNG。低端尺寸、Mip与打包策略留待设备采样；本包保留工具原始尺寸，没有脚本缩图。
只交通用素材与契约，不声称有UMG、uasset、已接线按钮、Atlas、设备或直播验收。home-components-v1-review.zip是历史组件审阅包，内部状态保持生成时快照；home-components-trim-v1-png.zip包含18张切图与生成时契约。两包均保留原字节，当前审核进度以磁盘evidence/home-9slice-visual-review.json与delivery-status.json为准。

## 下一步
每次回复只查看一张必要审核图，下一张为evidence/slice-correction-v4/home-rank-paper-narrow-fit-v4.png。修订方案：顶部底板整图Fit，排行纸底左右固定端区820/400px、锁高仅X轴伸缩。v4窄尺寸预览通过并解决问题后，启用新版本元数据、生成新ZIP并完成交付；原PNG及旧ZIP保留。不重复要求批准18个组件，本阶段不导入UE，不生成Atlas或扩展其他页面。

修订候选与数值证据：contracts/home-slice-correction-v3.yaml、contracts/sprite-contract-candidate-v2.yaml、evidence/home-slice-correction-checks.json。整图Fit允许透明余量，未来文字及热点应跟随Fit图形范围；尚未执行任何UE布局改动。

当前采用v4候选策略：排行纸底窄于原生1575/399宽高比时整图Fit；更宽时沿用v3锁高X轴九宫格。引擎集成时必须实现该条件分支并按Fit实际图形定位文字/热点，禁止只抄Box margin。v3窄图已明确拒绝，源PNG未改。
