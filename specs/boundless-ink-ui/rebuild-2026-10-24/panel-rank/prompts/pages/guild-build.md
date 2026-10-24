# 武灵建设 · authority catalog

## 页面定位

Browse the real `GuildBuildVm` building catalog and request an authority-backed build for an exact building and target level.

## 视觉约束

Follow shared newUI tokens and the catalog-detail composition family: quiet paper surface, ink-green hierarchy, restrained jade/gold focus, live values, 24-unit safe inset, and 88-unit actions. Source art is only a composition cue; no full-page bitmap.

## 布局与组件

- Root-owned back and provider state.
- Filter strip for all/buildings-owned using the exact service query.
- Scrollable building cards show id-bound name, current level if supplied, target level if supplied, authoritative costs if supplied, service state, and host gate/reason.
- Pagination uses explicit offsets from the VM.
- Danger confirmation shows exact building, target level, actual costs, and current snapshot version before emitting the build callback.

## 状态

Unknown/loading/empty/data/pending/offline/error/disabled remain distinct. Offline may show stale catalog read-only. Build is disabled without a known target, host permission/readiness, or snapshot version.

## 图像生成提示词

No new full-page bitmap. Compose live native UMG catalog cards from real `GuildBuildVm`; do not add decorative building art unless a real catalog image field exists.

## 反向限制

No invented building, level, cost, contribution, cap, completion, or eligibility. Never submit without host revalidation and the existing outbox/idempotency/version path.

## 验收记录

Staged widget `UInkGuildBuildPage`; import/route/compile/real catalog and action result verification pending integration.
