# Activity UI rebuild · slice 01

## Goal and scope

Replace the old activities screens with independent, data-driven UUserWidgets that integration can mount under the new UI root. This slice provides three complete page compositions: Activities Hub, Activity Category Detail, and Launch Activity / Tasks. It also defines the interface needed to add the launch rank and HUD entry in the next slice. Source is staged here only; the production source/config writer remains `logic-integration`.

## Calls made for this slice

- **Presentation:** native UMG controls and dynamic text, not a page bitmap or an old WidgetTree with a new paper overlay. The page concept is for composition review only.
- **Layout:** responsive portrait-first hierarchy at 720×1280 logical size: concise masthead, scrollable main content, a distinct persistent action/entry region where the page has a primary action. On wider viewports, the category detail becomes a list-plus-detail split.
- **Visual values:** temporarily use the project style contract's legible ink, jade, muted-gold, paper, and disabled colors and its title/body sizes. Its old page layouts and preservation rules do not carry forward. Logic-core's rebuild contracts will replace these provisional values when ready.
- **Model boundary:** pages receive a view model and emit a typed intent. They do not read a subsystem or make service authorization decisions. Integration maps intents to existing adapters and supplies a `content_key` built from the actual authority snapshot.
- **Unknowns:** missing counts, progress, service dates, reward descriptions, and offer details stay unavailable/pending. Only authority-provided values become text. No SKU, price, purchase path, reward, or new metric is invented.
- **Transitions:** hub category/action intents preserve the selected category and entry; category detail selects a real `activity_id`; launch start is a two-step UI confirmation intent, with the existing adapter remaining the final gate.

## Approaches considered

1. Paint each full screen and place a few buttons over it. Rejected: dynamic Chinese text, real task state, focus, scrolling, accessibility, and action state would be baked into an unmaintainable image.
2. Keep the current feature widgets and restyle their shell. Rejected: it cannot demonstrate the requested replacement page hierarchy and leaves the old composition visible.
3. Build independent native UUserWidgets from typed view models and delegates. Chosen: integration can mount each page through a thin feature host while the shared Root/Nav/Theme contract is still being authored; data ownership remains outside the pages.

## Page composition

### Activities Hub

- Header: clear `活动` title and server connection/service-day status only when supplied. Back/close and route stack are owned by the shared integration shell.
- Featured event slot: real launch status and route entry. No fabricated event countdown or reward artwork.
- Category rail: `每日`, `限时`, `常驻`; selected state is explicit, with counts only if the input marks them as provided.
- Activity feed: compact distinct rows with title, one-line description, state and one clear action. No repeated oversized poster cards.
- Empty/offline/loading states replace the feed in-place; one explanatory state, not duplicate notices.

### Activity Category Detail

- Header and category rail remain visible.
- Portrait: selected activity summary sits above a vertically scrollable activity list. Wide: list and selected detail appear side by side.
- Details show only supplied activity identity, description, cycle, eligibility, start/end, reward budget, progress, and service state.
- ACT_SIGN can emit a sign-in intent only when the model marks it allowed and the expected service day is present. Other activities emit no unsupported action.

### Launch Activity / Tasks

- A page header presents the live event state and server start/end fields; no invented countdown.
- Local tabs switch between overview, personal tasks, guild milestones, and honors summary. The full contribution ranking is a separate next-slice page.
- Task cards show server condition, actual progress/target, confirmed state, and numeric reward only when `task_numeric_rewards` is true and values are present. This page exposes no claim action.
- Offer slot remains visibly disabled with a reason while SKU, price/currency, entitlement, and purchase API are absent.
- Starting is explicit-confirmed and emits an intent; integration calls the existing `InkJourneyAdapter` gate. Enter-world remains a separate intent when the server reports an active event.

## Current inputs and sources

- Existing approved project color/type tokens: `../contracts/style-contract.yaml` (values only; old layouts not reused).
- Existing activity screen contracts and route aliases: `../contracts/screen-contract.yaml` (`Activities`, `DailyActivities`, `TimedActivities`, `DailyActivityDetail`, `TimedActivityDetail`, `LaunchEvent`).
- Current authority adapter and service fields: `Source/BoundlessImmortality/Frontend/UiRollout20261024/LogicJourney/InkJourneyAdapter.h/.cpp`, `WulingPrototypeSubsystem`, and `Tools/CultivationService/launch_event.py`.
- External product observations: the official [一念逍遥 activity article](https://xian.leiting.com/news/189.html) exposes a time-bounded event with multiple reward thresholds and an explicit current-progress area; the [梦幻新诛仙 official anniversary article](https://mhxzx.wanmei.com/m/news/gamenews/20260625/262881.shtml) separates event details, support ranking, and contribution ranking. These are hierarchy cues only; this project uses its own authorized fields and actions.
- Three new concepts were generated for this slice. They are opaque RGB composition references only, not runtime textures or new style authority.

## Next slice

Launch Contribution Rank and a compact HUD Activity Entry are next. After those: offline/prototype shop, consignment, and orders. Keep the page-screen map and status current across slices.
