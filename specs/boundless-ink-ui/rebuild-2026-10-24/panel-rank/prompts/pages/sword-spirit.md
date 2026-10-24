# 我的剑灵 · exact participant profile

## 页面定位

Show the current viewer's own sword-spirit profile only when `SwordVm` has an exact participant identity and supplied fields. This is a personal page, separate from guild or anchor growth.

## 视觉约束

Use the character-sheet composition family under shared newUI colors, fonts, safe area, and touch rules. Keep live identity/attributes as UMG text and actual portrait as a separate optional texture.

## 布局与组件

- Root-owned back and state panel.
- Exact participant portrait/name/realm/honor only when provided.
- Attribute rows only for supplied values.
- A route action opens the read-only Gift Rules page.

## 状态

Unknown profile data stays unknown. If no exact participant id or profile record exists, show the honest unavailable message with no invented statistics. Offline data, if supplied, is visibly stale and read-only.

## 图像生成提示词

No page art generation: the participant portrait comes only from the authoritative profile if present. Do not invent a character or reuse the guild banner as a personal portrait.

## 反向限制

No guild/anchor data substitution, no fabricated attributes/honor/realm, no unsupported equipment or growth action.

## 验收记录

Staged widget `UInkSwordSpiritPage`; current shared contract notes that a complete personal profile is not verified. Runtime may remain unknown until that source exists.
