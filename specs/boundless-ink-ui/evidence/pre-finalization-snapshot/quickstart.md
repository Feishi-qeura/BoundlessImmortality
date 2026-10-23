# 使用与真实状态
项目：D:\demo\BoundlessImmortality\specs\boundless-ink-ui。
续作日期：2026-10-22（Asia/Hong_Kong）。仅处理独立切图与九宫格v2审核；保留现有运行UI，本阶段不导入UE。

## 批准与当前进度
首页home-v2、规范和屏幕地图已批准，见evidence/approval-home-v2.json。
18个组件也已批准，见evidence/approval-home-components.json；头像与修为采用v2，旧v1保留stale。
组件批准不等于派生切图/九宫格参数获批，sprite-contract保持generated、review.complete=false、user_approved=false。

18张裁切RGBA PNG、4096×7130溯源合图、3张九宫格分割线图与12张测试图均已生成。溯源合图不是优化Atlas。
原v2计划图10/10已检查：轨道3图通过，顶部底板wide/narrow/tall与排行纸底wide/narrow共5张测试图不通过；两项原v2问题均有修订候选；顶部底板Fit已通过作者视觉检查，排行纸底仍待核查，见evidence/home-9slice-visual-review.json。
原v2计划图10/10及v3必要图4/4均已检查；v3顶部底板Fit、排行纸底分割线和wide通过，但narrow纸纹中心压成竖带未通过。v4仅新增1张排行纸底narrow Fit预览，尚待查看。
9控件×7态共63条渲染规则共享基础图层，不是63张PNG，所有文字独立渲染。

## 素材与参数
切图：assets/sprites/home-components-trim-v1/items。
清单：assets/sprites/home-components-trim-v1/sprite-manifest.yaml与contracts/sprite-contract.yaml。
以Alpha>1/255测量边界，框内RGBA逐字节保留，框外最多1/255的透明噪点被丢弃，四周增加8px透明边；没有改色或缩图。
18项总像素面积减少约39.94%，不是文件体积或运行显存实测结果。

3个九宫格候选：home-bar-track、home-top-tile、home-rank-paper；其余15项保持full/Fit。
Margin顺序left/right/top/bottom，单位为裁切后原生像素。轨道与排行纸底先按目标高度等比缩放边框，再锁定高度，只拉伸X；顶部底板允许X/Y。
所有参数仍处于审核中，未做UE Box brush验证。

## 复现与验证
Python 3.12.14，PyYAML 6.0.3，Pillow 12.3.0。
续作实测：18/18保留RGBA与合图区域一致，源图哈希一致；12/12测试图与确定性重建一致；48/48角区、16/16完整端区检查通过。
ZIP完整性通过，21项含18张PNG，各PNG哈希与清单一致；保护基线670/670文件一致，批准首页哈希一致。
严格设计包校验0错误/0警告。数值校验不代表视觉审核、UE运行或设备验收。

```powershell
$env:PYTHONUTF8='1'
& 'C:\Users\zxxuh\.codex\skills\gameui\.venv\Scripts\python.exe' 'D:\demo\BoundlessImmortality\specs\boundless-ink-ui\authoring\verify_home_slices.py'
& 'C:\Users\zxxuh\.codex\skills\gameui\.venv\Scripts\python.exe' 'C:\Users\zxxuh\.codex\skills\gameui\scripts\validate_project.py' 'D:\demo\BoundlessImmortality\specs\boundless-ink-ui' --strict
```

verify_home_slices.py只读现有图与ZIP，更新evidence/home-trim-recheck.json，不生成或修改素材。
build_home_slices.py是生成时记录，会重写契约与审核状态；勿用作复核命令。
build_project.py、finalize_review.py、prepare_components.py、finalize_components.py、verify_component_delivery.py是历史阶段脚本，部分会回退批准状态或覆写旧包，续作不要重跑。
当前进度见evidence/delivery-status.json；旧component-delivery-status.json和旧验证文本为历史阶段快照。

## 使用现有ZIP
packages/home-components-trim-v1-png.zip可解压得到items下18张PNG、sprite-manifest.yaml、home-9slice-review.yaml和README.txt。
SHA256：22c5e48cfa6a51fe565289a013619a79bd59b8d196427db12c110e4b09544f0d。
该ZIP保留生成时元数据；磁盘续作审核字段可能更新，像素和切图参数逐项核对一致。不要把ZIP内的待审快照作为最新进度。
packages/home-components-v1-review.zip为更早的组件审阅包，内部待批准状态是历史快照。两包均不覆盖、不改写。

## 剩余工作
每次只查看一张必要审核图。当前只剩排行纸底narrow Fit v4一张，检查通过后才启用修订元数据、生成新ZIP并整理交付；保留原PNG与旧ZIP。
下一张：evidence/slice-correction-v4/home-rank-paper-narrow-fit-v4.png。
未做Atlas、引擎JSON导出、UE导入、构建、PIE、Cook、设备或平台验证。

`使用 $gameui，接续specs/boundless-ink-ui，读取evidence/home-slice-correction-visual-review.json，每次只查看一张必要审核图，从排行纸底narrow Fit v4继续，完成修订审核和当前阶段交付；复用素材，保留运行UI，不导入UE。`

## 修订候选（未启用）
contracts/home-slice-correction-v3.yaml记录修订，contracts/sprite-contract-candidate-v2.yaml及assets/sprites/home-components-trim-v2/sprite-manifest.yaml为候选元数据；当前contracts/sprite-contract.yaml仍引用旧v1，不能将旧参数当作已通过视觉验收。
顶部底板改为整图full/Fit，保持宽高比、居中留透明余量；未来文字和热点需按实际Fit图形范围布局，本阶段不修改运行UI。
排行纸底候选Margin为[820,400,115,125]，锁定399px原生高度、只拉伸X；最小原生宽1221px，测试窄宽1260px仍有40px伸缩中心。
候选共2项9-slice、16项full/Fit，原18张PNG逐字节复用。7张版本化预览位于evidence/slice-correction-v3，其中4张需视觉审核；另3张由原像素完全一致和透明居中检查覆盖。
authoring/build_home_slice_corrections.py是一次性候选生成记录，使用Pillow确定性排布/缩放预览，无新增生图。脚本发现候选已存在即停止，避免回退审核；当前不应重跑。
新ZIP尚未生成，候选不属于已批准交付包。数值证据见evidence/home-slice-correction-checks.json；v3必要审核见evidence/home-slice-correction-visual-review.json。

## v4窄尺寸策略
v3排行纸底narrow的端区保形，但纸纹中心被压至原宽11.27%，出现竖向纹带，不能交付该预览策略。
contracts/home-slice-correction-v4.yaml替代v3方案；候选manifest/contract已更新policy_revision=v4，v3候选快照保存在evidence/candidate-v3-snapshot。
排行纸底按容器宽高比选择：小于1575/399时整图Fit居中；达到该比例时，先按目标高度统一缩放边框，再锁高X轴九宫格。按399px原生高度折算，九宫格策略下限1575px；技术切片极限1221px不作为允许的视觉下限。
新窄预览1260×399，实际图形1260×319，上下各40px透明余量。该策略需要显式宽高比分支，单个Box margin不足以表达；引擎未实现，本阶段不导入UE。
authoring/build_home_rank_narrow_fallback.py为一次性复现记录；当前候选已存在，不重跑。数值证据见evidence/home-slice-correction-v4-checks.json，必要审核见evidence/home-slice-correction-v4-visual-review.json。
