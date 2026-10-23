# PIE screenshot visual review

**Latest disposition (post-lifecycle-fix packaged portrait P5, 05:09–05:10):** the final 540×960 package passes 173 checks / 43 captures, exit 0 and authority stopped. All three forced-GC cycles observe a real constructed monthly chest before close, then confirm that same chest is collected or non-tickable and HUD survives. The three requested P5 screenshots were opened; announcement clearance, monthly calendar and daily verse remain visually correct. Landscape L5 is running; previous L4 crashed and remains recorded as historical failure, so cross-orientation final acceptance is still pending. These results concern Windows with isolated local authority, not production platform/device approval.

Reviewed 2026-10-23. Screenshots were opened individually with `view_image`; findings below are based on pixels, not merely JSON pass flags. No source edits or UE control performed by the reviewer.

Evidence directory: `D:/demo/BoundlessImmortality/Saved/Codex/PortraitUI20261023/PIE/`.

## Visible findings

### P2 — Verse decoration overlaps the text-bearing slip

Evidence: `verse-portrait.png`, `verse-landscape.png`.

In portrait, the horizontal scenery and bamboo cross the vertical slip around Y=320–515; the words 清明 and 今日仙缘 sit directly over bamboo, scroll supports and landscape detail. Landscape repeats this overlap around Y=350–490. The outline of the vertical slip exists, but its paper does not separate the text from the illustration. The result is visibly cluttered and harder to read in both orientations. Give the slip a real opaque local paper above the scene (or move the scene outside its text rectangle), keeping text above that paper. Source location: `InkRewardsPanels.cpp`, `UInkDailyFortunePanel::BuildPanelContent`, Scene / FortuneVerseSlip / vertical labels.

### P2 — Activity-card primary buttons lose their visible backing

Evidence: `daily-portrait.png`, `timed-portrait.png`; portrait makes the defect fully visible.

“查看目标与条件” and “查看活动” render as pale filled, dark outlined text directly on the card paper. Their dark/gold button artwork is absent, although the tabs, standalone buttons and footer buttons in the same captures have the expected backing. This makes the main action look like decorative text and gives it markedly worse contrast. The source authors these as primary `Action(...)` controls, so the missing backing is not an intentional secondary-text design. Check the Canvas layer order: card Surface and Action currently share the default Z order, while labels have explicit Z=2. Assign explicit card-background / action / foreground layers, then recapture and click each card action.

### P2 — Embedded content is slightly clipped along its right edge

Evidence: `signin-portrait.png`, `monthly-portrait.png`, `daily-portrait.png`, `timed-portrait.png`, and their landscape counterparts.

The right end of the final tab or right-hand card is clipped at the scroll viewport, visibly cutting off its gold corner/border. For example, in portrait the last monthly tab is cut near X=660 although the designed content extends farther right; in landscape the right-hand timed card similarly loses its right border. Reserve the actual ScrollBox scrollbar/padding width when computing embedded BodyWidth, or move the scrollbar outside that content rectangle. Verify the complete right corner and target rectangle of the last tab/card at the smallest width.

## Coverage issue requiring a replacement capture

`signin-landscape.png` is **not a daily-sign-in capture**: it visibly selects “每月签到” and shows the monthly calendar. It duplicates the monthly state in `monthly-landscape.png`. The saved selected route persisted across orientation/page reopening. Explicitly click `Ext_GroupTab_SignIn`, assert the selected child, then take the landscape weekly 4+3 capture. Do not count this filename as weekly landscape evidence.

## Observations that are not established defects

- The body ends at about Y=1110 in portrait and Y=550 in landscape; text/card fragments at that edge are normal scroll clipping unless they cannot be reached. The current first-screen screenshots alone do not establish loss of content.
- The landscape activity screenshots show only the upper halves of the first two cards, with all entry actions below the fold. Landscape monthly shows one calendar row at the top position. These require bottom-of-scroll captures and actual click/reveal checks before acceptance; no assertion that scrolling is broken is made here.
- The portrait journey page has a large empty gap between the description and “修行百事”, left behind after its primary entry was moved to the shell footer. Landscape also spends substantial height on the hero before showing its description. This is a spacing/polish issue; the fixed “世界历练” action remains visible.
- “待同步”, placeholder rewards, unavailable activity schedules and blank calendar dates match the disconnected preview shown in these captures. They are not evidence of missing assets or failed live-service content. An authority-connected run remains separate coverage.

## Images actually inspected

| Family | Portrait | Landscape | Visual conclusion |
|---|---|---|---|
| Weekly sign-in | `signin-portrait.png` | `signin-landscape.png` | Portrait 4+3 visible and readable; landscape file is monthly, so weekly coverage missing |
| Monthly sign-in | `monthly-portrait.png` | `monthly-landscape.png` | Calendar/chest layout reflows; right-edge clipping; lower content needs scrolling evidence |
| Daily verse | `verse-portrait.png` | `verse-landscape.png` | Confirmed scene/slip/text overlap |
| Daily activities | `daily-portrait.png` | `daily-landscape.png` | Primary backing absent; lower/landscape actions require scroll coverage |
| Timed activities | `timed-portrait.png` | `timed-landscape.png` | Primary backing absent and right card edge clipped |
| Journey | `Extensions-portrait.png` | `Extensions-landscape.png` | Main footer action visible; lower system cards require scroll coverage |
| World | `WorldCultivation-portrait.png` | `WorldCultivation-landscape.png` | Title, live state, progress and fixed action readable; landscape rewards below fold |
| Region detail | `RegionDetail-portrait.png` | `RegionDetail-landscape.png` | No unintended overlap found; fixed action and Back visible |
| Resource detail | `ResourceDetail-portrait.png` | `ResourceDetail-landscape.png` | No unintended overlap found; fixed action and Back visible |
| Daily detail | `DailyActivityDetail-portrait.png` | `DailyActivityDetail-landscape.png` | No unintended overlap found; fixed action and Back visible |
| Timed detail | `TimedActivityDetail-portrait.png` | `TimedActivityDetail-landscape.png` | No unintended overlap found; preview-pool entry needs landscape scrolling evidence |
| Expedition detail | — | `ExpeditionDetail-landscape.png` | No unintended overlap found; fixed action and Back visible |
| Resource contest | `ResourceContest-portrait.png` | `ResourceContest-landscape.png` | List/records reflow visible; lower reward action needs scroll coverage |

Total: 25 individual PNGs opened. No page was wholly blank. Major detail images appear fitted without obvious stretching. This review covers the displayed first-screen states at 720×1280 and 1280×720; it does not establish the remaining four acceptance sizes, live authority data, bottom-of-scroll reachability, repeated-open stability or final packaged rendering.

## Required follow-up

Fix/recheck the three visible P2 issues; replace the mislabeled weekly landscape capture; capture and click the lower controls on sign-in/activity/journey/detail pages. Review the new pixels before marking visual QA complete.

## Second pass — 2026-10-23, files modified after 03:32

The reviewer checked filesystem modification times, then opened 28 replacement/additional images with `view_image`. This avoids accidentally accepting the earlier files before they were overwritten.

### Confirmed improvements

- `verse-portrait.png` (03:33:12) and `verse-landscape.png` (03:34:26): the vertical slip now has an opaque local paper. Both columns of characters remain legible over it; bamboo and scenery are visibly behind the slip instead of crossing the text.
- `daily-portrait.png` / `timed-portrait.png` (03:33:16 / 03:33:18): dark/gold primary button backing is now clearly visible. Bottom portrait captures show the last buttons completely.
- Weekly/monthly and activity portrait tabs fit inside their right content edge. Timed activity card borders now have a complete right side. The inset produces two visible scrollbars for nested groups; this is visually busy but not itself a demonstrated functional failure.
- `signin-landscape.png` (03:34:24) now actually selects 每日签到, with the weekly heading and four day labels visible. This corrects the earlier wrong-state capture. Most reward-card content remains below the initial landscape viewport; a scrolled weekly image is still needed to visually assess all seven cards in landscape.
- `DailyActivities-bottom-portrait.png` (03:34:01), `TimedActivities-bottom-portrait.png` (03:34:06), `ResourceContest-bottom-portrait.png` (03:34:16): last actions are fully visible, including 丹道研习, 云山问道 and 争夺奖励. The corresponding report entries verify the correct destination pages for these three cases.
- `ResourceContest-bottom-landscape.png` (03:35:29): 争夺奖励 is fully visible; no overlap with Back.
- The 360×640 `SignIn-small`, `Activities-small`, `Extensions-small` and 720×1560 `SignIn-ultratall`, `Activities-ultratall`, `Extensions-ultratall` images were inspected. No extra horizontal overflow or footer overlap was observed. Text at 360×640 follows the existing DPI scale and is physically small; no independent new font-size regression is claimed. These SignIn captures select the monthly child, not the weekly child.
- `draw-fixed.png` (03:36:21): single/ten draw, skip and Back now occupy a separate bottom strip in the 360×640 capture, resolving the original fixed-action concern for this displayed state.

### New P2 — Timed activity illustrations hidden by the layer-order change

Evidence: `timed-portrait.png` (03:33:18), `timed-landscape.png` (03:34:32), `TimedActivities-bottom-portrait.png` (03:34:06). Both 仙缘寻宝 and 云山问道 have blank upper card regions where the prior run visibly showed their scene illustrations. The new global Scene Z=-1 places those images behind their opaque card Surface at Z=0. Restore card background < scene < buttons/icons/text layering, and put FortuneVerseSlip explicitly above its own scene. The parent acknowledged this and plans another layer revision; no later corrected capture has yet been reviewed here.

### Test evidence still incomplete, not classified as product unreachability

`Extensions-bottom-portrait.png` and `Extensions-bottom-landscape.png` remain at the top, as do `DailyActivities-bottom-landscape.png` and `TimedActivities-bottom-landscape.png`. Their matching reveal/click/target checks failed. Parent identified the helper filtering out width/height=0 widgets before invoking reveal; offscreen items can legitimately have no current geometry. Those files therefore do not establish that a user cannot reach the controls. Fix the helper, scroll to the end where necessary, repeat real Slate clicks, assert the correct destination, and recapture. Retain this failed run as evidence instead of reporting it passed.

### Second-pass image list

`verse-portrait`, `daily-portrait`, `timed-portrait`, `signin-portrait`, `monthly-portrait`, `Extensions-portrait`; `signin-landscape`, `verse-landscape`, `daily-landscape`, `timed-landscape`; the four `DailyActivities` / `TimedActivities` / `Extensions` / `ResourceContest` `bottom-portrait` captures; the same four `bottom-landscape` captures; `SignIn-compact`, `Activities-compact`, `Extensions-compact`; `SignIn-small`, `Activities-small`, `Extensions-small`; `SignIn-ultratall`, `Activities-ultratall`, `Extensions-ultratall`; `draw-fixed`.

No new complete blank-page failure or fixed-footer overlap was found. Remaining release-blocking visual evidence: corrected timed card art, successful formerly failed lower-action tests, and final screenshots from the corrected build.

## Fourth-pass preview review (03:46–03:48)

The third run used `-WulingPrototype` and showed the service-mode offline branch. Its missing preview-only action names do not establish a product fault; parent archived it as `PIE-ServiceModeOffline`. This review uses the fourth run without that argument and verified file modification times after 03:46.

The reviewer opened all 11 requested new images with `view_image`: `timed-portrait` (03:46:33), `timed-landscape` (03:47:48), `verse-portrait` (03:46:27), plus DailyActivities / TimedActivities / Extensions / ResourceContest `-bottom-portrait` (03:47:17–31) and `-bottom-landscape` (03:48:31–45).

- **Timed illustrations restored:** the golden celestial scene and night gate are visible above their respective titles in both orientations. Surface 0 / Scene 1 / Action 2 now preserves both illustration and button backing. The second-pass regression is closed visually.
- **Verse separation fixed:** the opaque central slip separates both vertical text columns from bamboo/scenery. No decoration crosses the text-bearing region. The lower “查看剑诀” button is partly below the initial scrolling viewport, which is not itself evidence of inaccessible content.
- **All eight lower targets visible:** portrait shows the final 丹道研习 and 云山问道 buttons, 角色名片 card, and 争夺奖励 button in full; their landscape equivalents are also inside the scroll viewport. Footer Back and Extensions 世界历练 stay separated from the body. Clipping at the top of a scrolled body is expected viewport behavior, not a new text-layout defect.
- **Navigation remains unresolved in portrait:** `report.json` finished with `passed=false`. All four landscape target-page checks reach DailyActivityDetail / TimedActivityDetail / CharacterProfile / ResourceRewards. All four portrait checks remain at Activities / Activities / Extensions / ResourceContest. `ClickAction` returns true for every one, but its return value is the handled result of mouse-down only; it does not assert OnClicked or route completion. Therefore portrait bottom navigation cannot yet be accepted.
- **Inner offset regression has runtime evidence:** the appearance lower-style apply check reports retained offset 577. This supports the previously closed source fix for that tested path; it does not independently exercise every Wuling preset/route.

### Read-only diagnosis of the portrait click discrepancy

`InkFrontendEditorLibrary.cpp:154-166` locates the button at its cached absolute center, verifies its presence in the hit path, then synchronously dispatches Down and Up; it returns only Down's handled value. `InkFrontendWidget.cpp:106-117` binds OnClicked to TriggerAction; `InkFeaturePanel.cpp:228-242` parses Open_ routes without an orientation branch. No deterministic portrait-only binding defect was found in these paths, and all four same action identities succeed in landscape.

The portrait 丹道 target snapshot is x=64, y=991, width=560, height=88. With the recorded viewport origin approximately (923,115), its absolute center is approximately (1267,1150), inside the 2560×1440 desktop. The screenshot confirms it is unobstructed. This excludes the simple explanation that its center lies below the physical display. It does not exclude cursor clipping, capture, hover or focus changes during synthetic input. Recommended diagnostic evidence is actual Slate cursor position after SetCursorPos, Down and Up results, button pressed/hovered state, and whether HandleClicked executes; separating Down and Up across ticks can distinguish synchronous injection behavior. Do not change production action semantics merely to make this helper pass.

No new visual P2 was identified in these 11 images. This limited recheck does not approve the latest HUD redesign, protected-page pixel preservation, packaged service behavior, or a subsequent build. Those retain their own evidence requirements.

## Follow-up input diagnostics (03:53–03:56)

Independently read both `Saved/Codex/PortraitUI20261023/BottomDiagnostic/report.json` and `BottomDiagnosticHistory/report.json` after they finished. Both report `finished=true`, `passed=true`. The direct run uses the same portrait dimensions and lower targets. The history run repeats the full portrait prelude before the four lower checks. Each reports OnActionTriggered for all four expected actions, pending confirmation `None`, and immediate `after` pages DailyActivityDetail / TimedActivityDetail / CharacterProfile / ResourceRewards. Parent states no production input code was changed between these diagnostic runs and the fourth run.

This establishes that the four portrait routes can be reached through the actual Slate click path, including after the full portrait navigation history. It does not identify the cause of the original four failures; do not rewrite that run as passed or claim a proven input fix. No production binding change is justified by the current evidence. A later complete run covering the new HUD remains required for final acceptance. These diagnostic conclusions come from the completed event/target reports; the visual conclusions above come from the separately opened fourth-pass screenshots.

## Final portrait HUD visual closure (04:05)

Opened all five final images from `Saved/Codex/PortraitUI20261023/HUD-final/` with `view_image`, after checking their modification times: `hud-portrait.png` 04:05:15, `hud-rank-expanded-portrait.png` 04:05:17, `hud-landscape.png` 04:05:22, `hud-rank-expanded-landscape.png` 04:05:24, and `hud-default-540x960.png` 04:05:29. These are the rebuilt painter-tint result, separate from the earlier white-meter captures.

- **720×1280 portrait:** compact identity is at the upper left; three separate dark shortcut targets form the right rail, with a separate ranking entry below. The central standing character remains clear. All five bottom icons and labels fit within the compact dark dock without overlapping adjacent targets or the preview caption. This is a visible structural change from the rejected earlier HUD.
- **Meter color correction:** full 100/100 health is a red filled thin meter. Cultivation 1/100 has a dark track and only a tiny cyan fill at the left, matching the visible value. The white-track regression is absent. Identity labels, values, avatar/frame and realm plaque remain separated in both orientations and at 540×960.
- **1280×720 landscape:** shortcuts use the top-right horizontal arrangement, ranking entry sits underneath, and the narrower dock is centered at the bottom. No inspected HUD target crosses the viewport edge or covers another fixed HUD target.
- **Expanded ranking in both orientations:** shortcut rail and collapsed entry disappear; existing paper ranking panel shows its header actions and eight visible rows. Identity and dock remain visible, with no new geometry change in the protected expanded ranking contents identified. Real HUDRankView / HUDRankClose dispatch assertions pass. The subsequent default portrait image shows the shortcuts/rank entry restored after closing.
- **540×960 default:** the same structural layout remains legible with all dock/rail targets inside the screen. This final focused run did not recapture 360×640, so no final-tint 360×640 claim is made. Moving world weapon/nameplate content can pass behind fixed UI; this is not a fixed-layout overlap or a new source change in the reviewed HUD.

Independently read the completed latest `PIE/report.json`: finished=true, passed=true, 415 checks, 93 captures, zero failed checks. The final full run includes action dispatch and destination assertions for all four lower targets in portrait and all four in landscape; this now supplies a successful complete-run result after the earlier intermittent failure. Independently read `HUD-final/report.json`: finished=true and passed=true, including real open/close ranking actions, hidden shortcut rail while expanded, vertical portrait rail and compact dock checks. The reviewer did not launch UE or rerun builds. No new actionable visual defect was found in the five focused final images; packaged service and device verification remain separate.

## Pre-capture-fix packaged image inspection

Opened the requested four images under `PackagePortrait-BeforeCaptureFix` with `view_image`, plus `final-hud.png` to investigate a visible world-material difference. These are historical package pixels, not the final capture-fixed P3/L3 run.

- `page-15-SignIn.png` (04:23:24) correctly shows the real daily verse branch: dated 2026-10-23, 心静如水 / 剑意自明, service-confirmed explanatory text, and disabled no-numeric-reward footer. Slip text remains separated from the landscape; tabs, body and footer are readable.
- `page-18-Activities.png` (04:23:36) correctly shows the real timed-activity service branch with 本期历练, 云台护阵 / 玄铁采集 start actions, and 灵脉竞采. It is intentionally different from offline preview illustrations. No clipped primary text or footer collision was identified.
- `page-19-Extensions.png` (04:23:40) shows the service journey hub: mountain illustration, 山海寻道 description, service entries and fixed 世界历练 / 返回 footer. Lower cards continue through the scroll region; the final visible resource card is cropped by that viewport, not lost data.
- `02-hud.png` (04:20:50) visibly retains the new compact identity/rail/dock and correctly colored meters with live values 1000/1000 and 2/60. No fixed HUD target overlaps another target.

### Open packaging visual discrepancy: world materials

In both `02-hud.png` (04:20:50) and `final-hud.png` (04:24:36), the main rock pillar and most foliage have a fairly uniform brown/gray appearance and lack the stone/green-leaf texture detail present in the final PIE HUD images. Sky, right-edge bamboo and UI textures still have color/detail. Persistence for approximately 3 minutes 46 seconds does not support the explanation that only the initial screenshot preceded normal texture streaming. Root cause and whether this predates the UI work are not established; no scene/material edits were made by the reviewer. Parent was alerted to verify the final P3/L3 images/logs while preserving the user's scene. The final historical HUD also shows a real service mechanic banner (“护阵 · 5秒后”) crossing beneath the right shortcut region, though the banner's text itself remains readable.

Parent independently confirmed the historical P2/L2 verification reports as 156 checks / 42 captures / process exit 0 / services stopped. The reviewer could read the PNGs but encountered Access denied on historical `report.json` and selected service logs, so that numeric result is attributed to parent, not independently reverified here. The two known transition captures (`signin-verse-tab`, `activities-timed-tab`) remain invalid visual evidence until replaced by the capture-fixed run. No final packaged acceptance is implied by this historical image review.

## Scene preservation attribution and capture-fix closure

The previously open brown/gray world-material discrepancy is now confirmed to predate this UI revision. The reviewer actually opened `Saved/Codex/ReleaseReadiness20261022/FinalPackagedPortrait/02-hud.png`: it shows the old wide identity/top-shortcut HUD together with the same gray/brown rock pillar and foliage. Independently read the older package log `Saved/Packages/WindowsFullV14_Final_20261023/Windows/BoundlessImmortality/Saved/Logs/BoundlessImmortality.log` around line 1087: `MI_Cliff_Rock_Atlas_01` and other temple material instances lack the `InstancedStaticMeshes` usage flag, with explicit warning that the default material will be used in game. This supports an existing packaged scene-material issue, not a new UI asset regression. Parent is preserving the scene as requested and documenting the existing limitation; no scene edit was made by the reviewer.

The reviewer also opened both corrected portrait tab captures in `PackagePortrait-BeforeAnnouncement`: `signin-verse-tab.png` now clearly renders the daily verse tab and `activities-timed-tab.png` clearly renders the actual timed service activity tab. Neither is a transition to the following page. This visually closes the capture scheduling defect for those two portrait images. Parent reports P3/L3 each passed 156 checks / 42 captures; the reviewer has not rerun the packages. The new announcement/rail clearance fix is separate and awaits P4/L4 fixture images.

## Final packaged portrait P4 closure

Actually opened the final `PackagePortrait/hud-announcement-fixture.png` (04:46:30), `02-hud.png` (04:42:31), `signin-verse-tab.png` (04:46:12), and `activities-timed-tab.png` (04:46:18). All are new final-package captures at the launcher's default 540×960.

The announcement fixture renders a readable single line below the identity, with its dark rectangle ending near x=438 and the shortcut rail beginning near x=456: a visible approximately 18-pixel gap. It neither intersects the rail nor covers identity, character or bottom dock. The previously observed portrait banner collision is closed in actual packaged pixels. This particular fixture text fits without ellipsis; the visual evidence therefore verifies clearance and readable fitting, while overflow ellipsis remains supported by source review rather than a visibly truncated long-text example.

The opening HUD has the compact identity, right vertical shortcuts and bottom dock with live service values. Verse and timed-activity tab captures contain their intended stable pages, with readable text and separate fixed footer actions; neither shows the subsequent page or a transition. No new actionable visual issue found in these four images. Existing gray/brown world materials remain consistent with the previously confirmed baseline issue.

Independently parsed `PackagePortrait/verification.json`: finished=true, passed=true, 158 checks, 43 screenshots, game_exit_code=0, resolution=[540,960], used_launcher_portrait_default=true, profile=PortraitUI20261023FinalP4, authority_stopped=true. This is actual Windows package/local-authority verification. Final landscape L4 is still running and is not inferred passed from portrait.

## Final landscape L4 crash supersedes pending acceptance

Parent reported L4 crash after leaving sign-in, with `UNiagaraUIComponent::RequestDeactivateSystem` reached from `UInkSignInChestWidget::Tick`. P4 portrait visual closure remains valid for its captured pixels, but the final package is not accepted across orientations until this lifecycle blocker is corrected and the GC/reopen regression plus final package runs pass. Static lifecycle diagnosis and proposed regression are recorded in `code-review.md`; an untracked raw pointer is not claimed as the proven root cause because the plugin already used UPROPERTY references and cleared them on Slate release.

## Post-lifecycle-fix portrait P5 verification

Opened new final-package images: `PackagePortrait/hud-announcement-fixture.png` (05:10:20), `signin-month-tab.png` (05:09:46), and `signin-verse-tab.png` (05:09:50). Announcement still ends before the rail with a visible gap. Monthly calendar dates, month navigation, active-tab state and fixed claim/Back actions remain readable; cumulative reward content continues below the scroll viewport. Verse slip and explanatory text remain separate from background art, with the correct disabled no-numeric-reward footer. No new actionable visual defect in these three images.

Independently read `PackagePortrait/verification.json`: profile PortraitUI20261023FinalP5, finished=true, passed=true, 173 checks, 43 screenshots, exit code 0, default launcher resolution 540×960, authority_stopped=true. Each of three GC cycles reports: monthly page opened, a constructed/tickable monthly chest observed before close, released chests do not tick after GC, the same observed chest was collected or stopped, and HUD survived. This closes the previously noted empty-iterator evidence gap for this run. It demonstrates successful portrait release/forced-GC/reopen execution after the lifetime fix, not a proof that every possible Niagara teardown sequence is covered. L5 landscape remains pending.
