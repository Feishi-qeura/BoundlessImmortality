# Handoff · panel-activities slice 01

## Delivered

- Three full-page native UMG compositions staged as source: Activities Hub, Activity Category Detail, Launch Activity / Tasks.
- Typed view models and action delegate in `staged-source/`; page prompts, three page concepts, three separate transparent semantic category icons, asset manifest, and adapter mapping notes are included.
- Category detail responds to available geometry: portrait stacks the selected detail and list in a scroll view; wide view uses a split list/detail layout.
- Launch tabs are functional local view state. Start confirmation and entry/rank/detail actions emit explicit integration intents. No client claim action is exposed.
- Page concepts are `approved:false`, RGB and opaque; semantic component candidates are `approved:false`, RGBA with actual partial alpha. All six are marked `runtime_texture:false` and `import_verified:false`.

## Concrete host interface

- Classes: `URebuildActivitiesHubWidget`, `URebuildActivityCategoryDetailWidget`, and `URebuildLaunchActivityWidget` in `staged-source/RebuildActivityWidgets.h`.
- Construction: there are no custom constructor arguments. Host calls `CreateWidget<WidgetClass>(OwningPlayer)`, binds `OnActionRequested`, then supplies `ApplyViewModel(const F...ViewModel&)`.
- VM refresh: every `ApplyViewModel` replaces presentation state and rebuilds the native page. `bCategoriesProvided`, `bActivitiesProvided`, `bTasksProvided`, and `bMilestonesProvided` distinguish unknown collections from known-empty ones. The host must not use empty arrays as an implicit zero/empty fact.
- Delegate payload: `FRebuildActivityActionRequest` carries contract action ID, `EntryId`, `Cycle`, `ExpectedServiceDay`, and `bConfirmed`. IDs use the shared lower-case semantic contract: `activity-cycle`, `open-activity-detail`, `sign-in`, `open-launch`, `open-launch-rank`, `start-launch`, `open-world`. Tab selection is local; the widget does not forward a fabricated event for it.
- Host adapter: source the VMs from `InkJourneyAdapter::Read` and the authorized launch snapshot; route navigation IDs through the shared root/route host; route `sign-in`/`start-launch` through existing `Capture/Evaluate/Submit` gates with a fresh context; revalidate permissions and service day at dispatch time. The shared root/back stack and current class/route bindings remain integration-owned.
- Asset paths: concepts are documented under `/Game/newUI/Activities/Concepts/`; icons under `/Game/newUI/Activities/Icons/`. These are intended paths only. No import or asset object exists yet.

## Not promoted or verified

- Nothing was changed under `Source/`, `Content/`, `Config/`, `Binaries/`, or `Intermediate/`.
- No production source promotion, asset import, `/Game/newUI` asset creation, `/Game/UI` migration, shared root/nav/theme integration, route registration, UBT build, Unreal Editor interaction, PIE, Cook, device, or package verification occurred.
- The staged C++ has static review only; compile-level correctness remains unverified.
- The launch concept shows empty progress rails. It is a composition reference, not evidence of live task state.

## Next work

1. `logic-integration`: review the typed models/actions, promote source, bind the live adapter/service snapshot, and mount these pages under the shared new root and route stack.
2. `cleanup`: own any authorized old `/Game/UI` to `/Game/newUI` asset migration and redirector work, after inventory and path-safe checks.
3. Continue visual authoring in separate slices: Launch Contribution Rank + compact HUD Activity Entry, then Offline/Prototype Shop, Consignment, and Orders. Keep every candidate unapproved until explicit visual acceptance.
4. Add semantic transparent component PNGs as separate assets in a later component batch; the current slice uses native UMG shapes and contains no component PNGs.
