# Candidate UI Visual Review Evidence — 2026-10-23

## Scope and status

This is a visual-only review of 15 newly generated candidate images. The review used the supplied candidate images only; no local legacy screenshots were opened. No images, manifests, or approval records were changed. Every image remains a **candidate** with `approved:false`; this review does not approve assets or establish runtime behavior.

## Main menu, tribulation, and breakthrough candidates

### `specs/boundless-ink-ui/revisions/2026-10-23-parallel-player-core/main-menu-v2.png`

**Issue:** The image depicts tribulation rather than the main menu. The candidate file/content mapping is wrong and must not be used for the main-menu screen.

### `specs/boundless-ink-ui/revisions/2026-10-23-parallel-player-core/tribulation-live-v2.png`

**Issue:** The image depicts the main menu rather than active tribulation. The candidate file/content mapping is wrong and must not be used for the live-tribulation screen.

### `specs/boundless-ink-ui/revisions/2026-10-23-parallel-player-core/breakthrough-preparation-v2.png`

**Passed:** The overall preparation hierarchy is recognizable and the slot structure is serviceable as a layout direction.

**Issues:** Several placeholder labels are vague and do not establish the authoritative material category, quantity, source, readiness, or predicted outcome. These values must come from the service contract and native text, not from illustrative copy.

## World map and sign-in candidates

### `specs/boundless-ink-ui/revisions/2026-10-23-parallel-maps-signin/assets/world-map-v3-candidate.png`

**Issues:** Visible `{{...}}` template fields require replacement with runtime text, and the map points are static illustrative placements rather than connected world-map data. The candidate does not prove map navigation or region selection. A clear global label should state that nodes/routes are visual examples only; the current detailed routes, lock, and purple portal can imply confirmed world content or progression that the authority has not supplied.

### `specs/boundless-ink-ui/revisions/2026-10-23-parallel-maps-signin/assets/resources-map-v2-candidate.png`

**Issues:** Visible `{{...}}` template fields require runtime text. The screen lacks a region/list switch for accessing resources beyond the currently shown map points. Specific resource pins and the large crystal illustration can imply actual spawn locations despite the empty-data labels; label pins as type examples and crystal artwork as decoration, or render the pins as hollow/dashed examples.

### `specs/boundless-ink-ui/revisions/2026-10-23-parallel-maps-signin/assets/daily-sign-in-v2-candidate.png`

**Issues:** The fortune-result explanation and `{{reward_preview}}`, `{{connection_status}}`, and `{{pending_notice}}` are too small at narrow portrait sizes. Increase placeholder text and reclaim decorative space. A visual candidate alone does not verify the daily fortune service result; status bar values and the shown safe inset are illustrative only.

### `specs/boundless-ink-ui/revisions/2026-10-23-parallel-maps-signin/assets/monthly-sign-in-v2-candidate.png`

**Issues:** The 31-day grid has no weekday headers, so it can be mistaken for an actual month layout. Label it as a date-number example, mark the state legend as illustrative, and bind the real month length and weekday offset before runtime use.

## Activities and launch event candidates

### `specs/boundless-ink-ui/revisions/2026-10-23-parallel-activities/assets/activity-center-v2.png`

**Passed:** The portrait tabs for 日常、限定、常驻、开服 have balanced widths and clear selected-state treatment. Parchment, ink landscape, and dark-green/gold accents fit the water-ink direction.

**Issues:** The single PNG contains two vertically stacked activity-center states: an upper “无活动” empty state and a lower skeleton/loading list. They contradict each other if treated as one screen asset; separate the states into independent candidates. “无活动” alone also does not distinguish an authoritative empty result from data not yet loaded.

### `specs/boundless-ink-ui/revisions/2026-10-23-parallel-activities/assets/launch-event-v2.png`

**Passed:** The portrait hierarchy is clear from title and countdown to daily tasks, co-cultivation contribution, and milestone rewards. Missing values are explicitly marked “时间待同步” or “数据待同步”; no fabricated countdown, reward quantity, or purchase flow is shown. The water-ink landscape and paper sections are coherent.

**Issues:** The long page is information-dense; secondary captions may become small on a phone. No clear join, claim, or return action is visible in the candidate, so the actual touch path is not established by this image. Keep the missing-data labels distinct from any claim that the event is active or confirmed.

### `specs/boundless-ink-ui/revisions/2026-10-23-parallel-activities/assets/launch-rank-v2.png`

**Passed:** The portrait leaderboard frame and empty-state illustration are clear and stylistically consistent. No fake ranking rows or reward amounts are displayed. “周期数据待同步” signals that the data is unavailable.

**Issues:** The adjacent “未上榜” sounds like a confirmed result and conflicts with “贡献数据待同步.” While data is missing, use a single pending state such as “榜单数据待同步”; show “未上榜” only after a successful load confirms no rank. The bottom “青云初启” control does not communicate its action, and no return or rank-switch control is evident.

### `specs/boundless-ink-ui/revisions/2026-10-23-parallel-activities/assets/hud-launch-countdown-v2.png`

**Passed:** The transparent horizontal plaque, ink-green mountain silhouette, and gold border suit a HUD overlay. It contains no invented countdown value.

**Issues:** “开服倒计时 ---” does not explain whether the value is loading, unavailable, or failed; use an explicit pending label or a clearly formatted time placeholder. Only a very wide banner is shown, so its screen coverage and legibility in a compact HUD placement remain unproven.

## Leaderboard and Wuling candidates

### `specs/boundless-ink-ui/revisions/2026-10-23-parallel-rank-wuling/assets/pages/rw-leaderboard-damage-candidate-v2.png`

**Passed:** Portrait title, tabs, table headers, and empty-state message are legible and ordered clearly. The screen does not invent ranking entries.

**Issues:** “伤害” has no unit or aggregation window, and the image does not identify a ranking period. “我的排名：未上榜 | 无数据” conflates a confirmed non-ranking result with missing data. Add the metric unit/window and use a pending rank state until data has loaded.

### `specs/boundless-ink-ui/revisions/2026-10-23-parallel-rank-wuling/assets/pages/rw-leaderboard-cultivation-candidate-v2.png`

**Passed:** The selected 修炼速度 tab, headers, and empty state are easy to scan in portrait layout; the water-ink treatment is consistent.

**Issues:** The metric lacks a unit (for example, 修为/秒) and ranking period. The bottom “未上榜 / 无数据” repeats the pending-versus-confirmed ambiguity. Provide the unit and period, and distinguish loading/missing data from a confirmed unranked state.

### `specs/boundless-ink-ui/revisions/2026-10-23-parallel-rank-wuling/assets/pages/rw-leaderboard-spirit-candidate-v2.png`

**Passed:** Portrait row spacing is serviceable and the dark-ink/gold ranking frame matches the broader style.

**Issues:** The visible top-three rank medals, avatar silhouettes, and row marks look like populated leaderboard rows, while the footer says “未上榜 / 无数据.” This is a material placeholder contradiction. For a pending/empty snapshot, remove the ranked rows and show an empty state; once data exists, label the actual subject field, metric, unit, and period. “榜单对象字段 / 榜单指标字段” is too generic to establish what is ranked.

### `specs/boundless-ink-ui/revisions/2026-10-23-parallel-rank-wuling/assets/pages/rw-wuling-home-candidate-v2.png`

**Passed:** The mountain gate, banners, and sect framing present 武灵 as an organization rather than a pet. 总览、成员、贡献、管理 tabs are readable and have generous portrait touch areas. The locked management entry and 权限字段控制 note communicate restricted access. Water-ink styling is consistent.

**Issues:** “组织资料加载中” has no visible failure, timeout, or retry state, so a stalled load has no next step. The image does not show actual organization/member data; it remains a loading-state candidate rather than evidence of populated content.

## Review boundary

These findings describe only the visual contents of the 15 named candidates. No interaction, responsive runtime, asset integration, service snapshot, build, or platform acceptance was tested. `approved:false` remains unchanged for all candidates.

## Independent follow-up review

A second read-only pass reviewed the player-core, activity, and map/sign-in groups independently. It used only newly generated candidates and their prompts/contracts; it did not inspect legacy project UI images or change files/approval state.

- **Player core:** The main-menu/tribulation image swap is independently confirmed. The breakthrough-preparation composition is usable as a concept, but text/runtime bindings must explicitly map Q to `q_normalized`, quality to `quality_band`, growth to `growth_min/max`, and tribulation probability to `tribulation_chance`; illustrative values are not defaults. Replace plus signs on material slots with neutral empty-slot marks or a clear “待配置” label. The two swapped images remain quarantined and require replacement candidates.
- **Activities:** The activity-center image combines mutually exclusive empty and loading states and must be split into separate variants. The launch countdown is semantically clear and has alpha 0–255 transparency, but it must bind to a real countdown field before runtime use. Five unlabelled milestone stones on the launch-event page can imply configured rewards/progress; replace them with an explicit pending-data block or marked skeletons. The launch leaderboard has no fake ranking rows, but labels must stay pending until the service has loaded. All four files are 941×1672 while their manifest target is 720×1280; do not describe them as exported at the target size.
- **Sign-in and maps:** Daily sign-in structure and disabled claim state are clear, but small placeholder strings need enlargement. The monthly grid must be marked as an example until weekday offset and month length are generated; mark the state legend as illustrative. Resource-map pins and world-map routes/portal/lock need global “visual example only” labels so they cannot be mistaken for authority data or unlocked content.
- The second pass did not approve any candidate. Candidate status remains `approved:false`; only the two known swapped files also carry `content-mismatch-do-not-use`.
