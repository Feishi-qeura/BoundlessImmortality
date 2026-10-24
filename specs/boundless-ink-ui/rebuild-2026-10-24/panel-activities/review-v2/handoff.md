# Activity page source review v2 handoff

## Input boundary

The review report's `readiness.json` captured `panel-activities/staged-source` at SHA256 `2abd8e58977f6c3b57add1a162c17e111f6b4f0f90eeb538a9c0462e10f6548c` (header) and `501ab09b0de412c2799c42394a63dce16a91b150c3456814904ed5098d3bfe63` (source). Those hashes did not match the source currently in the workspace when v2 was made. V2 therefore starts from a new copy of the current staged pair; the frozen `panel-activities/staged-source` was not edited. Findings below were reconciled against the current copy and the current shared review report, not silently applied to the older hash.

## Exact class and host interface

- `URebuildActivitiesHubWidget`, `URebuildActivityCategoryDetailWidget`, and `URebuildLaunchActivityWidget` remain the page classes.
- `URebuildActivityPageWidget` owns the public `OnActionRequested` dynamic multicast delegate. `URebuildActivityIntentButton` stores an `FRebuildActivityActionRequest`, binds its native `UButton::OnClicked` to a `UFUNCTION`, then broadcasts a typed intent. The page forwards through a reflected `UFUNCTION`; no native lambda is attached to `OnClicked`, no borrowed delegate reference is captured, and repeated binds use `AddUniqueDynamic`.
- The three page classes take no custom constructor arguments. Integration creates them with `CreateWidget<WidgetClass>(OwningPlayer)`, binds the inherited delegate once, then calls `ApplyViewModel(const F...ViewModel&)`.
- Every `ApplyViewModel` updates the existing page. A stable `UVerticalBox` and frame are created during `NativeOnInitialized`; refresh clears and reconstructs only that root's children. Scroll offset is captured/restored around content refresh. Orientation changes in the category page keep the same root.
- Launch start captures `EventId` and service day on the first confirmation click. Same event/day refresh preserves the confirmation; a changed event/day clears it. The second click emits `Action=start-launch`, actual `EventId`, captured `ExpectedServiceDay`, and `bConfirmed=true`. UI also checks current connected/busy/start state; the host must freshly re-capture and revalidate the same event/day, Busy, identity, and server permission before dispatch.
- All text uses the shared new-root font helper `InkRebuildUI::Font`; button content tint stays white while the child card supplies its surface; buttons use the shared button style and an 88 logical-pixel minimum height.
- Added the explicit `Error` page state so it remains distinct from unknown, offline, disabled, pending, and empty states.

## Review finding disposition

| Finding | V2 source disposition | Remaining evidence |
| --- | --- | --- |
| C01 dynamic `OnClicked` | Fixed with `URebuildActivityIntentButton::HandleClicked` + `AddUniqueDynamic`; rows retain their request in a reflected button property. | Compile and Slate interaction still need integration validation. |
| C07 progress background | Uses `FProgressBarStyle::SetBackgroundImage` and `SetWidgetStyle`; no `SetBackgroundColor` call. | Compile/render check still pending. |
| C08 const delegate | Removed the borrowed delegate parameter from the row/button ownership path; request is owned by each button UObject. | Compile check still pending. |
| V01 transparent content tint | Button content multiplier is white; card fill remains on child `UBorder`. | Screenshot/render evidence still pending. |
| L01 root replacement | Initial stable root is built from `NativeOnInitialized`; view-model/orientation refresh replaces children, not `WidgetTree->RootWidget`, and restores scroll offset. | First open, repeated refresh, scroll, focus, and confirmation require a Slate/PIE check. Source structure alone is not runtime proof. |

## Promotion and state

- Source destination remains `Source/BoundlessImmortality/Frontend/UIRebuild20261024/Activities/RebuildActivityWidgets.{h,cpp}`; production writer is `logic-integration`.
- Runtime art, when approved/imported, stays under `/Game/newUI/Activities/`.
- No `Source/`, `Content/`, config, generated project, engine, UBT, PIE, Cook, device, or package operation was performed by this patch.
- This v2 source has static review only. It is not compile-verified, imported, mounted, or runtime-verified. Preserve the original slice unchanged; integration should consume this exact v2 path and report any follow-up patch in a new revision.

Current staged v2 SHA256: header `B306EAB1FE044BF807F3D5AA19CF967D9828E7D6D5A6495D8D464369CB58B5ED`; source `DA9D45F6FE6D7895C86C42B29C588A52193B96C39EA278E92251C8C488D6D92A`.
