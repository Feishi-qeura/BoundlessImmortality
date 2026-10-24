# 礼物效果说明 · read-only catalog

## 页面定位

Provide a read-only explanation of synchronized `GiftEffectVm.rules` and actual `activeEffects`, when supplied.

## 视觉约束

Use the rules-sheet composition family and shared newUI paper/ink/jade typography, page frame, safe area, and state panel. All descriptions remain live UMG text.

## 布局与组件

- Root-owned back and service state.
- Scrollable rule cards with actual rule id/name/description.
- Separate active-effects section only when the VM provides those records.
- No action buttons or commerce claims.

## 状态

Unknown/loading/empty/data/offline/error remain distinct. An empty catalog is shown only when the host explicitly says the catalog was provided and empty. Missing rules keep an unavailable/unknown notice; actual active effects may still be shown separately.

## 图像生成提示词

No image generation required; this is live explanatory data. No benefit icons unless the source catalog provides a real semantic icon.

## 反向限制

No invented prices, durations, passive effects, guarantees, conversion ratios, or rewards. Never turn missing catalog into confirmed empty.

## 验收记录

Staged widget `UInkGiftRulesPage`; the shared contract says the current detailed effect catalog is unavailable, so the production page may honestly remain in its unknown state.
