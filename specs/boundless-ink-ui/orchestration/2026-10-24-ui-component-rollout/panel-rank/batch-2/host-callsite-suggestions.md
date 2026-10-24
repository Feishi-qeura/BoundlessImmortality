# Narrow host call-site suggestions (not applied)

These notes are staged for logic-integration after the frozen source and UBT batch are released. They are not patches to current public files.

## Leaderboard

Target: `Frontend/InkLeaderboardWidget.cpp` and `Frontend/InkFrontendWidget.cpp`.

1. Before any `Clear(Page)` or call to the old `RebuildLeaderboardSurfaces`, capture the existing `RankAnchorScroll` and its persistent row subtree from the active category tree. The current row widget instances, names, children, and avatar request associations must survive; do not reconstruct or copy them.
2. Build the shell around that exact scroll once. Keep the canonical names consumed by `UpdateLeaderboardAvatars` and the host refresh path, especially `RankAnchorScroll` and the original row names.
3. `FRankChrome` is a plain C++ struct, not a reflected type; it cannot be declared as `UPROPERTY(Transient) FRankChrome`. Simplest safe option: do not store it. Query the named widgets from the host `WidgetTree` in the existing refresh functions. The attached tree owns these child widgets. If the host needs cached pointers, add separate reflected `UPROPERTY(Transient) TObjectPtr<...>` fields for each widget type and null/reset them before destructive rebuild, then assign the newly constructed widgets.
4. Preserve `RefreshLeaderboardRows`, `UpdateLeaderboardAvatars`, and `PresentedLeaderboardRows` as data and row owners. Keep row visibility/state updates on those same widget instances.
5. The current host's `UpdateLeaderboardAdaptiveLayout` positions canonical names including `RankAnchorFrame`, `RankAnchorHeading`, `RankAnchorTitleText`, `RankClose`, `RankLocal`, `RankGlobal`, `RankAnchor`, `RankCultivation`, `RankAnchorColumns`, `RankAnchorColumnRank`, `RankAnchorColumnName`, `RankAnchorColumnTitle`, `RankAnchorColumnMetric`, `RankAnchorScroll`, `RankAnchorEmptyCard`, `RankAnchorEmptyState`, and `RankAnchorCount`. Either make the composite use these canonical names and call `UpdateLeaderboardAdaptiveLayout` after assembly and on viewport changes, or add an explicit adaptive-layout update for every new shell widget. Calling the current fixed 720-coordinate factory alone does not satisfy portrait/landscape adaptation.
6. Until the service supplies self position, pass empty `FText`; do not infer it from a row or profile.

## Wuling overview

Target: `Frontend/UI/InkWulingSectPanel.cpp` and its existing UPROPERTY text references in `InkWulingSectPanel.h`.

1. Build `ExistingActionWidgets` from actual widgets already created for the overview. Do not create an action to fill a layout slot.
2. Call `BuildWulingOverviewComposite` once after those widgets exist, passing the current service snapshot/status flags. Update its dynamic text references from the existing live-data refresh path.
3. Preserve current `CreateGuild` handling and its confirmation/error path. Do not move or replace actions that belong to another page.

## Wuling roster and management

Target: `Frontend/UI/InkWulingRecordsPanel.cpp`.

1. Keep the existing `Rows` JSON array, requested cursor, `SelectedMember`, service request, and handlers. While building the existing `RecordRow_i` loop, capture each returned clickable widget in `ExistingMemberRowWidgets`; keep its original stable action id and selected/enabled state. For example, retain the pointer returned by `Action(List, *FString::Printf(TEXT("RecordRow_%d"), I), Label, ...)` rather than recreating a text-only row.
2. Capture the exact widgets returned for `RecordPrevious`, `RecordNext`, and `RecordRemove`. Pass the member row array, previous/next array, and remove widget to the updated `BuildWulingRosterComposite` signature. The factory reparents those instances only; it does not create, rename, or bind replacement buttons.
3. Keep `HandlePanelAction("RecordRow_...")` as the selection owner: it still reads the indexed item from `Rows`, assigns `SelectedMember = Rows[I]["user"]`, and refreshes the remove-enabled state. Keep `RecordRemove` handler submitting that selected user id through the existing service command and clearing selection as it currently does. Do not derive a user id from widget text.
4. Only expose rows for the matching server `member_cursor` and current ready state, consistent with the existing page gate. The factory caps the layout to the existing eight-item page; it does not make claims about the meaning of roles or permissions.
5. Move `RecordRemove` into the composite's management area while preserving its precomputed enabled state, click delegate, and stable identity. Existing authorization and error behavior remain in the host and service.
6. The parent page must update these same widgets after `RequestMemberPage` / `RefreshLiveData`; do not call the factory on every refresh. Rebuild only when the host already rebuilds its page, recapturing the actual widgets each time.

Compile/UHT, route migration, responsive layout, import, PIE, and nonempty authority data remain for logic-integration after the engine lease; this staged handoff does not prove them.
