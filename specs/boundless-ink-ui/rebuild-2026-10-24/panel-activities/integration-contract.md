# Activity rebuild integration contract

## Ownership and delivery

- This slice is staged for the sole production writer `logic-integration`; production C++/config and runtime asset import are not performed here.
- Proposed source destination: `Source/BoundlessImmortality/Frontend/UIRebuild20261024/Activities/RebuildActivityWidgets.{h,cpp}`.
- Runtime assets, if later approved and imported, belong below `/Game/newUI/Activities/`. The current PNGs are visual concepts only and must not be referenced by a runtime widget.
- The widgets are standalone page bodies. The shared root, route stack, back handling, navigation shell, and global theme remain the integration owner's responsibility.
- These `UUserWidget` classes have no custom constructor parameters. Create with `CreateWidget<URebuild...Widget>(OwningPlayer)`, bind `OnActionRequested`, then call `ApplyViewModel(...)`; keep the shared route host as parent. `URebuildActivityCategoryDetailWidget` measures its own geometry and selects portrait or split mode.

## Authority and action mapping

`ApplyViewModel` is the only presentation input. Populate it from a fresh authority-backed snapshot; set each `b...Provided` flag separately from its array so unknown collections stay distinct from authoritative empty collections. `ContentKey` should change whenever displayed state changes, but widgets do not use that key to suppress updates. The integration host owns widget lifetime and maps `OnActionRequested` to the exact lowercase action IDs below; it must re-check current permissions/state at dispatch time.

| Class | VM update API | Important host inputs |
| --- | --- | --- |
| `URebuildActivitiesHubWidget` | `ApplyViewModel(const FRebuildActivitiesHubViewModel&)` | service day/state, featured launch, categories-provided flag, categories/counts/rows |
| `URebuildActivityCategoryDetailWidget` | `ApplyViewModel(const FRebuildActivityCategoryDetailViewModel&)` | service day, selected cycle/entry, category-provided and per-category activity-provided flags, selected detail |
| `URebuildLaunchActivityWidget` | `ApplyViewModel(const FRebuildLaunchActivityViewModel&)` | service event dates/status, personal score availability, complete-rounds availability, tasks/milestones provided flags, start/world permissions, offer-disabled reason |

| Intent | Required payload | Integration boundary |
| --- | --- | --- |
| `activity-cycle` | `Cycle` in `daily`, `limited`, `permanent`, or `launch` | Keep route/parent state, update selected cycle and provide a new VM. |
| `open-activity-detail` | `EntryId`, `Cycle` | Resolve only a real activity ID from the current snapshot and use `SelectedEntry`. |
| `sign-in` | `ExpectedServiceDay` | Route through the existing sign-in adapter; reject if the server day changed or permission is absent. |
| `open-launch` | none | Route through the shared host to the existing `LaunchEvent` semantic route. |
| `open-launch-rank` | none | Route through the shared host to `LaunchRank` when available. |
| `start-launch` | `bConfirmed=true` on the second click | The widget requires a second click; integration must still call the existing start guard and server-backed operation. |
| `open-world` | none | Use the current service permission and existing world route. |
| local tab switch | widget-local `SelectedTab` state | No contract event is emitted; the widget changes its own tab state. |

Tasks and milestones are read-only in this page. The current contract has no task-claim intent. Numeric task rewards require `bTaskNumericRewardsAllowed` and an actual service value. Offer remains disabled until SKU, currency/price, entitlement, and purchase API are all integrated. Counts, progress fractions, dates, ranking fields, and rewards must not be synthesized to fill blank UI.

## Runtime asset candidates

The three page PNGs are full-screen composition references. The three RGBA PNGs are separate semantic category icons intended for `activity-entry` composition. The current C++ page source does not load them because they have not been approved or imported. Their intended object paths and measured file metadata are in `asset-manifest.yaml`.

## Static source notes

`staged-source/RebuildActivityWidgets.{h,cpp}` defines three independent native UUserWidgets with typed view-model structs and a typed dynamic multicast action delegate. It has received source-level review only. It has not been promoted into `Source/`, compiled, loaded in Unreal Editor, wired to the shared root/routes, or exercised in PIE/device builds.
