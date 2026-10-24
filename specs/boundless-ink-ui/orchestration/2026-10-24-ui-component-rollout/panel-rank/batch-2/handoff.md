# panel-rank batch-2 handoff

## Result

Batch-2 source is staged in this directory and ready for promotion after the active integration compile releases `Source/`. It builds a shared leaderboard composite for all three categories, plus separate Wuling overview and member-list composites. It retains the current data owners and existing row/action widgets.

No new raster was generated. The existing v2 page concepts and the transparent batch-1 panel are present and measured; this slice adds dynamic UMG assembly around real fields. `asset-inventory.yaml` records each reused file and hash. The optional paper texture argument has an existing native `FeatureCard` fallback, so compilation does not depend on importing the candidate texture.

## Staged source

- `staged-source/InkRankWulingCompositeViews.h`
- `staged-source/InkRankWulingCompositeViews.cpp`
- Promote to `Source/BoundlessImmortality/Frontend/UiRollout20261024/RankWuling/` only after the current engine batch exits and coordinator confirms Source is writable.

## Pages and real seams

- **Damage / cultivation speed / anchor boards:** `BuildLeaderboardComposite` reads `InkCore::FBoardView`, uses existing `RankDamage`, `RankCultivation`, `RankAnchor`, `RankLocal`, and `RankGlobal` actions, and draws title/metric through `InkLeaderboardTheme`. It reparents the host's existing row `UScrollBox`; current keyed rows, `InkCore::ReadBoard`, room/global subscriptions, and asynchronous avatar updates remain the data and row owners.
- **Wuling overview:** `BuildWulingOverviewComposite` reads actual snapshot paths `guild.name`, `guild.supply`, `guild.reputation`, and `counts.{members,participating_members,helpers}`. Caller-supplied existing action widgets retain `UInkFeaturePanel` `Ext_` routing; `UInkWulingSectPanel` continues to own `CreateGuild` and refresh.
- **Wuling members:** revised `BuildWulingRosterComposite` receives and reparents the actual `RecordRow_i`, `RecordPrevious`, `RecordNext`, and `RecordRemove` widgets. The host keeps `Rows`, `SelectedMember`, row selection, remove enabled state, `RequestMemberPage`, and the existing service command/error path. This preserves the required selection-to-removal chain; the host capture callsite is still only a staged integration instruction.
- **Backlog:** Wuling construction, My Sword Spirit, and Gift Rules. Contribution route/period remains unconfirmed and is not fabricated.

See [page-coverage.md](page-coverage.md), [api-map.md](api-map.md), [host-callsite-suggestions.md](host-callsite-suggestions.md), [review-resolution.md](review-resolution.md), [component-contract.yaml](component-contract.yaml), and [asset-inventory.yaml](asset-inventory.yaml) for the full crosswalk.

## State and evidence

- Source available: `true` (staged only).
- Import verified: `false`.
- Compile verified: `false`.
- Runtime / route verified: `false`.
- Visual review: pending user; all candidate state remains `approved:false`.
- Existing code was read only. No `Source`, `Content`, `Binaries`, or `Intermediate` files were changed in this batch.
- No Editor, import, UBT, PIE, or device action was run.

## Promotion and verification suggestions

After the engine owner releases the current batch, promote the two staged files in the order listed in `api-map.md`, then make narrow host call-site patches. Capture the rank scroll before destructive clear; use canonical adaptive-layout names or extend `UpdateLeaderboardAdaptiveLayout`, and invoke it after assembly and on the existing orientation/resize path. Keep existing persistent leaderboard rows and avatar requests intact. For Wuling, capture and pass the original row, paging, and remove widgets; keep the selection and service handlers in the host.

Then the integration owner can compile the named source set, import/read back only if choosing the optional parchment texture, run the existing route/data checks with real provider responses, and capture the final UI for user review. Validate missing/unavailable/offline/loading/empty/error states, category/scope action selection, self-position omission, Wuling member cursor mismatch, empty roster, disabled/disconnected/busy service, and repeat pagination while pending. This handoff itself proves none of those runtime outcomes.
