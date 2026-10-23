# Feature-page implementation evidence

Date: 2026-10-23. Scope: the delegated feature-page family only. Status: implementation and static inspection complete; Unreal build, asset migration, PIE screenshots, route traversal, Cook and Windows package verification remain with the parent task. No runtime pass is claimed here.

## Files changed

- `Source/BoundlessImmortality/Frontend/InkFeaturePanels.cpp`
- `Source/BoundlessImmortality/Frontend/InkRewardsPanels.cpp`
- `Source/BoundlessImmortality/Frontend/InkFeatureGroupPanel.cpp`
- `Source/BoundlessImmortality/Frontend/InkDrawPanel.cpp`
- `Source/BoundlessImmortality/Frontend/InkSignInSample.cpp`

Parent-owned dependency: `UInkMonthlySignInPanel::CalendarMonthOffset` added by parent in `InkFeaturePanel.h`. This implementer did not modify that header, the shared feature shell, theme, HUD, preserved Character/Inventory/Leaderboard geometry, assets or gameplay code. `InkSignInPresentation.h` remains unchanged; its existing milestone interpolation is reused.

## Implemented behavior and layout

- Sign-in group: three 88-high tabs, with existing selected-route persistence. Weekly cards use a centered 4+3 arrangement, distinguish current/selected days and blue hover, expose each existing chest preview/claim, and add a confirmed weekly count plus isolated progress track. Landscape keeps the card grid on the left and selected reward details on the right. All weekly rewards still say placeholder and do not grant cultivation or items.
- Monthly sign-in: authoritative server month/date, seven-column calendar, current-day highlight and confirmed-record markers. Previous/next/current-month controls browse up to the preceding 12 months without issuing economic requests. Historical pages mark only dates present in the supplied signed_days list and explicitly state that unmarked dates are unconfirmed by the current records. The chest panel always describes the current server month. Calendar and cumulative milestones are separate columns at wide sizes. Milestone labels now read returned milestone values; the current business fallback remains 3/7/14/21, not the obsolete 7/14/21/28 design examples. Claims continue to submit the existing milestone value from the selected authoritative record.
- Daily verse: narrow vertical slip, right-to-left verse columns where the supplied two-part verse fits, full unmodified verse also available as readable text. Portrait places the explanation after the slip; landscape places it alongside. Server-provided fixed verse and no-numeric-reward semantics remain intact; preview interaction is retained when disconnected.
- Activity group: existing daily/timed tabs plus a service-enabled launch-activity entry. Launch opens the existing WulingActivity route instead of creating a hidden live panel or duplicating its requests. The visibility condition matches the pre-existing DailyActivities-to-WulingActivity link.
- Daily activities: actual service state and shared-cultivation controls use a portrait task flow or landscape summary/action split. Disconnected task previews become single-column cards in portrait and two columns in landscape. Original start/end action IDs are retained, with busy-state disabling and refresh when service state changes.
- Timed activities: real template disclosure, original WeekGuard/WeekIron actions and actual WulingRace entry. Disconnected previews become stacked portrait cards / landscape two-column cards. No invented calendar deadlines or rewards.
- Journey: prominent world-exploration art and primary entry, then grouped existing systems. Wide layout has the world entry on the left and a two-column system grid on the right. Wuling/my-spirit entries use existing enabled-service paths. Legacy medical expedition remains excluded when Wuling is enabled, as before.
- World: responsive region tabs wrap when five 88-wide targets cannot fit; fitted scene and live cultivation state reflow into separate landscape columns. Existing snapshot/progress reads and region gates remain unchanged. The entry action follows content without clamping into reward rows.
- Resource contest: live lease status/actual WulingRace path gets a focused summary and action region; disconnected resource list keeps filters/details and moves record actions into a separate side panel or subsequent portrait section. Existing preview ranking/reward routes remain available with explicit no-record/placeholder statements.
- Daily/timed/expedition/region/resource details: width-relative image/details columns replace the hard-coded landscape X=660. Each family receives an appropriate section title. Reward/availability/action blocks follow one another and no longer share a clamped bottom coordinate.
- Draw: wheel diameter uses available width rather than a potentially negative short-window body height. Action semantics, pre-determined local demo results, skip/close/resize persistence and no-currency/no-item disclosure are unchanged. Result columns adapt to available detail width.

All newly authored direct action controls in this scope use 88 height or larger. Calendar dates are non-interactive labels, not undersized touch targets. Embedded pages add no full-page paper; localized cards/slip/details use the shared card style.

## Static inspection performed

Evidence: `Saved/Codex/PortraitUI20261023/feature-pages-static-checks.json`.

Executed with `C:/Users/zxxuh/.codex/skills/gameui/.venv/Scripts/python.exe` from the project root. The inline inspection compared the five edited files against `Saved/Codex/PortraitUI20261023/Backup/Source/BoundlessImmortality/Frontend`.

- Five source delimiter checks passed after removing comments/string literals. This is a lightweight syntax screen, not a C++ compiler.
- Existing directly declared methods and direct action IDs were retained.
- Nine original business handlers/refresh functions compare equal ignoring whitespace: expedition action, world action/live refresh, resource action, sign-in chest action, daily sign-in action, draw action/tick, daily verse action.
- Eleven body widths checked: 444, 480, 540, 624, 720, 799, 800, 864, 1000, 1080, 1132. The arithmetic inspection found positive detail widths and at least 88 width for weekly/monthly chest targets, group tabs, region tabs and draw actions.
- Four month boundaries checked, including January crossing a year, 12-month history and supported calendar lower/upper year bounds.
- Four wide journey-grid widths checked.
- Source hashes captured after the final edits.

No new native tests were run and no Unreal process/build was launched by this implementer, avoiding interference with the parent's serialized build/asset workflow. Initial `python` launcher failed because the Windows Store alias was unavailable; rerunning with the existing GameUI virtual-environment executable succeeded.

## Integration and visual checks still required

1. Parent shared shell must supply scrollable standalone monthly/verse content and preserve each embedded group panel's scroll. This was requested and acknowledged.
2. Primary world/detail actions are authored after content with stable geometry. If fixed shell-footer actions are required for these routes, the parent can move `Ext_EnterRegion_*` / `Ext_Open_RegionDetail|*` and detail `Ext_Preview_Participate` into the footer. This scope cannot modify the shared shell. Sign-in already has the shell-owned fixed claim.
3. Actual Slate text measurement, material tint/readability, blank/long authoritative strings, calendar dot placement, chest hover/state visuals, scrollbar reachability and no-overlap need fresh PIE screenshots/clicks at all acceptance sizes.
4. Verify activity busy-state completion restores controls, opening launch and returning preserves activity tab, calendar browsing retains offset across refresh/resize, and chest detail/claim always refers to the current authoritative month.
5. Parent migrates resource paths after integration; no asset paths were rewritten here.

The service still intentionally lacks live daily-preview tasks, preview resource rankings/rewards and old expedition gameplay. Their UI remains explicit about unavailable data instead of inventing gameplay to satisfy visual mockups.
