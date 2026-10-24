# 武灵 · overview and member management

## 页面定位

The `GuildVm` page has overview and members-management variants. It shows real organization data and preserves the full stable-user-id selection/removal path.

## 视觉约束

Use `wuling-v2` as a layout source only. Shared newUI style: warm rice paper, jade/ink-green identity, light old-gold focus, live Chinese text, 24-unit safe inset, and 88-unit targets. The semantic insignia is a separate transparent component; no page PNG is imported as runtime UI.

## 布局与组件

- Overview: Root-owned back/navigation; organization identity card with insignia, name/status; members/participating/helpers counts; public supply and reputation; host-supplied existing action buttons.
- Member management: service status; independently scrolling selectable user/name rows; previous/current/next server cursor; remove action only when host permission/readiness allows it; danger confirmation binds the same selected user id.
- Landscape reflows to a side rail/list/detail arrangement through Root; page does not scale down.

## 状态

Unknown/loading/empty/data/pending/offline/error/disabled stay distinct. Missing display fields remain unknown. Offline supplied members are readable but not selectable or mutable. Remove is never enabled from an unknown permission.

## 图像生成提示词

No full-page regeneration: reuse the existing Wuling candidate composition. The text-free insignia asset is separately versioned and measured. All counts, status, member labels, cursor controls, and confirmations are live UMG.

## 反向限制

No guessed role, online status, contribution, join date, supply, reputation, permission, or reward. Do not derive user id from display text. Do not submit mutations from the visual component.

## 验收记录

Staged widgets: `UInkWulingOverviewPage`, `UInkWulingMembersPage`. Actual route host, `GuildVm` mapping, service receipt/error feedback, and user visual acceptance remain pending.
