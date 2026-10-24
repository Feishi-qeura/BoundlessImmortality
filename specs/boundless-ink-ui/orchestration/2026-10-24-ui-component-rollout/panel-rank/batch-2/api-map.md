# API and promotion map

## Rank composite

- Factory: `InkRankWulingCompositeViews::BuildLeaderboardComposite` in the staged `.h/.cpp`.
- Inputs: real `InkCore::FBoardView`, current action owner, and the actual persistent `UScrollBox` captured from the live tree before any old `Clear(Page)` call.
- Output: dynamic title/provider/self-position and scroll references. The factory reparents the same scroll and subtree.
- Existing row names, children, and avatar update ownership remain intact. Keep `RefreshLeaderboardRows`, `UpdateLeaderboardAvatars`, and `PresentedLeaderboardRows`.
- `FRankChrome` is an ordinary C++ struct, not `USTRUCT`; it cannot be held as a UPROPERTY. Keep it as a temporary return value, then resolve named controls from the attached `WidgetTree` during refresh. If integration chooses cached references, use separate correctly typed `UPROPERTY(Transient) TObjectPtr<...>` members and clear them before rebuilding.
- Adaptive integration: use the host's canonical control names consumed by `UpdateLeaderboardAdaptiveLayout`; call that method after the composite is assembled and on its existing viewport/orientation update path. If retaining any new control names, extend that adaptive method to explicitly position each new shell widget. The fixed 720-coordinate factory alone is not adaptive acceptance.
- `FBoardView` has no self-rank, period, tie, reward, or claim contract. These remain absent unless the authority adapter adds them.

## Wuling overview and roster

- Factories: `BuildWulingOverviewComposite` and the revised `BuildWulingRosterComposite`.
- Overview inputs use existing snapshot paths `guild.name`, `guild.supply`, `guild.reputation`, and `counts.{members,participating_members,helpers}`. Existing page actions are passed through unchanged.
- Roster inputs include the current snapshot/cursor plus host-created `ExistingMemberRowWidgets`, `RecordPrevious`/`RecordNext`, and the exact `RecordRemove` widget. The factory moves these same widget instances; it does not render replacement name-only rows or bind new handlers.
- `UInkWulingRecordsPanel` remains owner of `Rows`, `SelectedMember`, row click selection, remove enable state, `RequestMemberPage`, `HandlePanelAction`, and service permission/error handling. Keep the indexed user id in the host data; never recover it from the visible label.
- Only display the captured rows when the service snapshot matches requested cursor and existing readiness gates. The composition limit is the existing eight-row page.

## Promotion and sequencing

1. Promote the two staged source files only after integration source freeze/engine lease release.
2. In leaderboard rebuild, capture the existing scroll and subtree before any destructive clear, create the composite once, then apply existing adaptive layout. Refresh the same widgets without rebuilding the shell.
3. In Wuling roster construction, capture the actual row/action widgets from the original host loops and pass them into the revised signature. Keep all host state and handlers in place.
4. Apply Wuling overview to its existing host-owned live refresh path.
5. Integration compiles and verifies responsive layout, row selection/removal, pagination, avatar completion, and route behavior. Nothing in this handoff marks import, compile, runtime, or visual review complete.
