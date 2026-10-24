# Rank/Wuling newUI handoff · slices A and B

## Ownership and promotion boundary

All files in this handoff are under `specs/boundless-ink-ui/rebuild-2026-10-24/panel-rank/`. Production target for all six native page classes is `Source/BoundlessImmortality/Frontend/UIRebuild20261024/RankWuling/`. Only logic-integration writes that production source/config and creates the actual route host/WBP. Only `/Game/newUI` asset paths appear here. No Source, Config, or Content file was changed; no Editor, UBT, import, PIE, or user process action was taken.

The shared contracts consumed are `contracts/style-contract.yaml`, `contracts/screen-contract.yaml`, `contracts/component-contract.yaml`, `model/vm-contract.yaml`, `model/action-contract.yaml`, and `model/screen-bindings.json`. Owner pages are standalone UUserWidgets; the integration Root supplies SafeCanvas/PageSwitcher, safe area, back/receipt handling, theme/font objects, orientation layout, and route mapping. The page widgets do not copy old visible trees.

## Slice A: three leaderboard variants, Wuling overview, Wuling members

**Staged files:** `staged-source/InkRankWulingPages.h/.cpp`.

### `UInkRankBoardPage`

- **Construct:** `CreateWidget<UInkRankBoardPage>(OwningPlayer)`; parent it in the Root `Leaderboard` page slot. The component constructs its own live UMG tree in `NativeOnInitialized`.
- **Update:** `SetBoardData(const FInkNewRankBoardData&)` from `LeaderboardVm`. Map provider `FInkLeaderboardEntry` fields without sorting: rank, participant id, display name, actual title/realm/metric, resolved avatar if present. Set metric/title from the existing theme mapping. `FBoardView` has no self position; keep `SelfPosition`/`SelfMetric` empty. Map unknown/loading/empty/data/pending/offline/error/disabled distinctly. Offline may show the last supplied rows in read-only form.
- **Actions:** `OnCategoryRequested(Category)` -> existing `board-category` / `TriggerAction(RankDamage|RankCultivation|RankAnchor)`; `OnScopeRequested(Scope)` -> existing `board-scope` / `TriggerAction(RankLocal|RankGlobal)`. State/result returns through a later `LeaderboardVm` update, not a fabricated local success. `OnRetryRequested` is only bound where the host already has a supported refresh action.
- **Visual structure:** live title and status, three category tabs, Local/Global scope tabs, rank/identity/metric header, persistent scroll region of provider rows, and self-position footer. Row minimum 120 units; identity/avatar are separate from metric. Selected tabs use ink-green plus light text.

### `UInkWulingOverviewPage`

- **Construct:** `CreateWidget<UInkWulingOverviewPage>(OwningPlayer)` under the Root `Wuling` page host.
- **Update:** `SetOverviewData(const FInkWulingOverviewData&)` mapped from `GuildVm`: guild name/status; counts.members, participating_members, helpers; public supply and reputation. Missing values remain em dash/unknown. Assign the transparent insignia texture only after import readback.
- **Actions:** `ExistingActions` contains only actual host actions with ActionId, label, enabled state, and disabled reason. `OnActionRequested(ActionId)` returns to the existing feature host, which opens its name-entry or danger-confirm flow for create/rename/dissolve; the widget does not synthesize parameters or submit mutations. Result comes back in the updated GuildVm/service state.
- **Visual structure:** identity card with insignia, live sect identity/status, three current count slots, supply/reputation summary, and host-supplied action section.

### `UInkWulingMembersPage`

- **Construct:** `CreateWidget<UInkWulingMembersPage>(OwningPlayer)` under the `Wuling` host's members-management variant.
- **Update:** `SetMembersData(const FInkWulingMembersData&)` from `GuildVm`: user id/name rows, current cursor, explicit previous/next cursors, selected user id, remove readiness/reason, service state, and `bRemovePending`.
- **Actions:** `OnMemberSelected(UserId)` maps to `guild-member-select` and sets the existing selected member from the stable user id. `OnPageRequested(Cursor)` maps to existing `RequestMemberPage(Cursor)`. `OnRemoveRequested(UserId)` fires only after a local danger confirmation bound to the same selected user id and current readiness. The host must route this into its existing `RecordRemove`/`SubmitPrototypeCommand(RemoveMember)` path, preserving current request identity, expected version, known-role/connected/not-busy/fresh-subject gates, and server authorization. It is not a direct client mutation. `bRemovePending` disables repeat submit; the host clears it on the actual result.
- **Visual structure:** selectable name rows (identity only), 112-unit minimum, isolated scroll region, real cursor navigation, selection-dependent danger action, cancel/confirm controls. Offline may show supplied stale names read-only.

## Slice B: Guild Build, Sword Spirit, Gift Rules

**Staged files:** `staged-source/InkRankWulingExtendedPages.h/.cpp`.

### `UInkGuildBuildPage`

- **Construct:** `CreateWidget<UInkGuildBuildPage>(OwningPlayer)` for page key `guild-build`, route compatibility `GuildConstruction`.
- **Update:** `SetBuildPageData(const FInkGuildBuildPageData&)` from `GuildBuildVm`: actual catalog rows (building id/name, current/target levels only when present, authoritative costs only when present), offset cursors, `SnapshotVersion`, and host action gates.
- **Actions:** `OnQueryRequested(Offset, OwnedOnly)` -> existing `query-buildings` operation `RequestCatalog(kind=buildings)`. `OnBuildRequested(BuildingId, TargetLevel, SnapshotVersion)` fires only after confirm; host rechecks current selection/version/role/supply/contribution/caps and routes to existing `SubmitPrototypeCommand(Build)` with exact `building_id`/`target_level`. Existing subsystem owns request id/version and returns pending/error via later VM update. No local costs or success state.
- **Visual structure:** filter strip, scrollable native catalog cards, current/target level, authoritative cost line, state/pager, and danger confirmation naming exact building and target. Snapshot version change cancels the open confirmation.

### `UInkSwordSpiritPage`

- **Construct:** `CreateWidget<UInkSwordSpiritPage>(OwningPlayer)` for page key `sword-spirit`, compatibility route `SwordSpirit`.
- **Update:** `SetSwordSpiritData(const FInkSwordSpiritPageData&)` from exact participant `SwordVm`: participant id, name, realm, attributes, honor, portrait only when supplied. With no exact participant id/profile, it shows an honest unknown/unavailable state and no placeholder statistics. It never substitutes guild or anchor values.
- **Action:** `OnOpenGiftRulesRequested()` navigates through the host to `open-gift-rules`. This view adds no mutation.

### `UInkGiftRulesPage`

- **Construct:** `CreateWidget<UInkGiftRulesPage>(OwningPlayer)` for page key `gift-rules`, compatibility route `GiftRules`.
- **Update:** `SetGiftEffectData(const FInkGiftEffectPageData&)` from `GiftEffectVm`: actual `rules` and `activeEffects`, plus explicit `bRulesProvided` so an absent catalog cannot be mistaken for an empty catalog.
- **Actions:** read-only; none. Missing catalog remains “详细礼物效果目录尚未同步”; only an explicitly provided empty array displays an empty directory. No prices, durations, grants, or passive descriptions are invented.

## Visual asset

- Semantic component candidate: `assets/components/wuling-sect-insignia-v3.png` (1254×1254 RGBA, alpha extrema 0–255, 1,207,656 bytes, SHA-256 `d42fbe4f9a002fc8927c4cc47471a236c43a7c1eec1f94d1810c28be2a942ca8`).
- Proposed package: `/Game/newUI/Components/RankWuling/T_NUI_Wuling_SectInsignia_Normal_V1`; intended 160×160 logical display, `contain`. Prompt and all superseded iterations are traced in the manifest.
- v1/v2 rank-header attempts had visible alpha fringe/speckle and were discarded; no rank-header raster is referenced. Leaderboard title, controls, rows, states, and Wuling summary are constructed with native UMG/live text.
- Current shared theme names fonts `/Game/newUI/Shared/Fonts`; no font asset was imported in this slice. Integration must bind the new shared font/style tokens before visual acceptance; UMG fallback typography here is a staging gap.

## Verification and remaining work

- Project-local candidate images were visually inspected; the insignia PNG's dimensions, mode, alpha extrema, and hash were measured and recorded.
- `approved:false`; visual review remains with the user.
- UHT/UBT source compilation, native class registration, font binding, actual `WBP_NUI_Leaderboard` and `WBP_NUI_Wuling` creation, texture import/readback, portrait/landscape Root layout, route wiring, real service data, action receipt replay, PIE, and real player-entry screenshots are not verified.
- Shared runtime package architecture proposes `/Game/newUI/Runtime/Pages/Rank/WBP_NUI_Leaderboard`, `/WBP_NUI_Wuling`, `/WBP_NUI_GuildBuild`, `/WBP_NUI_SwordSpirit`, and `/WBP_NUI_GiftRules`; actual WBP class/path remains null until integration creates and records it.
- Continue after this six-page set with any remaining RankWuling contract gaps; no unprovided SwordVm or GiftEffectVm fields were filled.
