# Native UI route coverage matrix — 2026-10-23

Scope: source-only audit of the 44 entries in `contracts/screen-contract.yaml`. This report follows the player-facing route to `CurrentPage` and its native C++/UMG class, then notes the source of displayed state. It does not inspect images, run Unreal, build, or claim PIE/runtime acceptance.

## Per-screen matrix

| # | Contract screen / route | Player entry → `CurrentPage` → native class | Displayed state / data source and boundary | Coverage |
|---:|---|---|---|---|
| 1 | main / `Main` | Application/menu start → `Main` → `UInkFrontendWidget` main page | Native widget tree; menu/game director state | Present |
| 2 | home / `HUD` | `StartGame` → `HUD` → `UInkFrontendWidget` HUD page | Native HUD/action bindings and current game state | Present |
| 3 | character / `Character` | HUD `OpenCharacter` → `CharacterProfile` → `UInkCharacterProfilePanel`; legacy `Character` page also remains in `UInkFrontendWidget` | Presented character attributes/cultivation snapshot; Wuling loadout when available. Contract explicitly preserves both route names. The HUD’s normal path is the profile panel, not the legacy page. | Partial: two existing character surfaces, one normal entry path |
| 4 | inventory / `Inventory` | HUD `OpenInventory` → `Inventory` → `UInkFrontendWidget` inventory surface | Existing inventory model and item subsystem; preserved page | Present |
| 5 | leaderboard / `Leaderboard` | HUD `OpenLeaderboard` → `Leaderboard` → `UInkFrontendWidget` leaderboard surface | Local/global entries supplied through leaderboard setters; global connection state is external and can be empty | Partial: native board is present; a connected global ranking feed is not guaranteed by this UI route |
| 6 | saves / `Saves` | Main menu or HUD `OpenSaves` → `Saves` → `UInkFrontendWidget` | Local `UInkSaveSubsystem` | Present |
| 7 | settings / `Settings` | Main menu or HUD `OpenSettings` → `Settings` → `UInkFrontendWidget` | `UGameUserSettings` and local UI options | Present |
| 8 | wuling / `Wuling` | Journey/feature CTA → `Wuling` → `UInkWulingSectPanel` | `UWulingPrototypeSubsystem` snapshot and commands | Partial: native authority-backed surface; service availability bounds actions/data |
| 9 | sword-spirit / `SwordSpirit` | HUD `OpenWuling` → Wuling `Open_WulingRights` → `WulingRights` → `UInkWulingRecordsPanel` | Wuling authority snapshot/rights records | Partial: shared rights page; no distinct SwordSpirit page id |
| 10 | gift-rules / `GiftRules` | HUD `OpenWuling` → `Wuling` (`UInkWulingSectPanel`) → `Open_GiftRules` → `GiftRules` → `UInkGiftRulesPanel` | No authoritative gift-effect rules API or player-facing `GiftEffectVm`; the page reads no GM/live-session data and sends no request | Reachable, honest unavailable state: “礼物效果规则暂未接入 / 等待权威配置” |
| 11 | methods / `CultivationMethods` | HUD `OpenWuling` → `Open_WulingEquipment` → `ToolView_methods` → `WulingEquipment` → `UInkWulingToolsPanel` | Wuling authority method/loadout data | Partial: state/context within shared tools page |
| 12 | race-lobby / `RaceLobby` | HUD `OpenExtensions` → resource map → `Open_ResourceContest` → `Open_WulingRace` → `WulingRace` → `UInkWulingToolsPanel` | Wuling race authority snapshot | Partial: shared route/state alias |
| 13 | race-live / `RaceLive` | Resource map → `Open_ResourceContest` → `Open_WulingRace` → `WulingRace` → `UInkWulingToolsPanel` | Wuling race authority snapshot; no separate live-page id | Partial: shared route/state alias |
| 14 | race-result / `RaceSettlement` | Resource map → `Open_ResourceContest` → `Open_WulingRace` → `WulingRace` → `UInkWulingToolsPanel` | Wuling race authority snapshot; no distinct settlement page id | Partial: shared route/state alias |
| 15 | lease / `ResourceLease` | Resource map → `Open_WulingRights`, or contest → `Open_WulingRace` → `WulingRace` → `UInkWulingToolsPanel` | Wuling race/lease authority snapshot | Partial: shared route/state alias |
| 16 | world-result / `EventSettlement` | HUD `OpenExtensions` → world map → `Open_WulingWorld` → `WulingWorld` → `UInkWulingPanel` | Wuling world authority snapshot | Partial: alias to shared Wuling world page |
| 17 | offline / `OfflineRewards` | HUD `OpenWuling` → Wuling `Open_WulingOffline` → `WulingOffline` → `UInkWulingRecordsPanel` | Wuling authority offline-reward records | Partial: alias to shared records page |
| 18 | shop / `Shop` | HUD `OpenWuling` → `WulingWorld` → `Open_WulingCraft` → `玩法目录` → shop item → `WulingShop` → `UInkWulingToolsPanel` | Wuling authority shop/catalog state | Partial: alias to shared tools page |
| 19 | market / `Market` | HUD `OpenWuling` → `WulingWorld` → `Open_WulingCraft` → `玩法目录` → market item → `WulingMarket` → `UInkWulingToolsPanel` | Wuling authority market state | Partial: alias to shared tools page |
| 20 | orders / `TradeOrders` | HUD `OpenWuling` → Wuling `Open_WulingOrders` → `WulingOrders` → `UInkWulingRecordsPanel` | Wuling authority order records | Partial: alias to shared records page |
| 21 | world / `WorldCultivation` | HUD `OpenExtensions` → `Extensions` → map “世界修炼” tab; Wuling `Open_WorldCultivation` | Journey/map entries and availability; route resolver shadows older `UInkWorldCultivationPanel` | Partial: actual player surface is the map, not the legacy cultivation panel |
| 22 | region / `RegionDetail` | World map node → `JourneyWorldDetail` → `UInkJourneyDestinationPanel` | Journey destination metadata/availability; closed nodes remain unavailable | Present as shared destination detail |
| 23 | resources / `ResourceContest` | HUD `OpenExtensions` → resource tab → `Open_ResourceContest` → `ResourceContest` → `UInkResourceContestPanel` | With authority enabled, current lease/race snapshot; fallback copy is explicitly preview text | Partial: center is reachable, but no full resource-period service is evidenced |
| 24 | resource-detail / `ResourceDetail` | Resource map entry → `JourneyResourceDetail` → `UInkJourneyDestinationPanel` | Journey destination metadata; no unsupported rewards are claimed | Present as shared destination detail |
| 25 | sign-in / `SignIn` | HUD `OpenSignIn` → `SignIn` → `UInkSignInGroupPanel` with embedded `UInkSignInPanel` | Wuling sign-in authority; incomplete eligibility/claim fields disable submit actions | Partial: grouped parent and embedded child |
| 26 | monthly-sign-in / `MonthlySignIn` | HUD `OpenSignIn` → monthly tab; parent `CurrentPage=SignIn`, embedded `UInkMonthlySignInPanel` | Monthly chest/claim state from sign-in authority; missing claim fields disable action | Partial: embedded group route |
| 27 | daily-verse / `DailyFortune` | HUD `OpenSignIn` → daily-verse tab; parent `CurrentPage=SignIn`, embedded `UInkDailyFortunePanel` | Existing daily-fortune view/authority state | Partial: embedded group route |
| 28 | activities / `Activities` | HUD `OpenActivities` → `Activities` → `UInkActivitiesGroupPanel` | Wuling activity snapshot/templates; empty templates display explicit empty state | Partial: shared group surface |
| 29 | daily-activities / `DailyActivities` | HUD `OpenActivities` → daily tab; parent `CurrentPage=Activities`, embedded daily page | Activity authority templates | Partial: embedded group route |
| 30 | timed-activities / `TimedActivities` | HUD `OpenActivities` → timed tab; parent `CurrentPage=Activities`, embedded timed page | Activity authority templates | Partial: embedded group route |
| 31 | daily-detail / `DailyActivityDetail` | HUD `OpenActivities` → daily card CTA, or empty-state “详情 · 等待活动配置” CTA → `DailyActivityDetail` → `UInkDailyActivityDetailPanel` | Authority activity template lookup; missing template shows explicit no-match/unavailable detail; empty-context route makes no request | Present with authority-dependent detail or honest empty state |
| 32 | timed-detail / `TimedActivityDetail` | HUD `OpenActivities` → timed card CTA, or empty-state “详情 · 等待活动配置” CTA → `TimedActivityDetail` → `UInkTimedActivityDetailPanel` | Authority limited-activity template lookup; missing template shows explicit no-match/unavailable detail; empty-context route makes no request | Present with authority-dependent detail or honest empty state |
| 33 | launch / `LaunchEvent` | Activity group `Open_LaunchEvent` → `LaunchEvent` → `UInkLaunchEventPanel` | Wuling launch-event authority snapshot and commands | Present; service-dependent actions are gated |
| 34 | launch-rank / `LaunchRank` | Launch honors tab CTA `Open_LaunchRank` → `LaunchRank` → `UInkLaunchEventPanel` in honors view | Same launch-event authority honor records/expiry | Partial: distinct route/page state reuses launch panel |
| 35 | breakthrough / `BreakthroughPrepare` | Wuling → `WulingWorld` → `Open_WulingCraft` → `玩法目录` → 突破与天劫 → `WulingBreakthrough` → `UInkWulingToolsPanel` | Wuling breakthrough authority and eligibility | Partial: preparation state shares tools route |
| 36 | tribulation / `Tribulation` | Same Wuling tools directory → 突破与天劫 → `WulingBreakthrough` → `UInkWulingToolsPanel` | Wuling tribulation/breakthrough authority; no independent page id | Partial: shared route/state alias |
| 37 | item-sources / `ItemSources` | Wuling → `WulingWorld` → `Open_WulingCraft` → `玩法目录` → 物品来源 → `WulingSources` → `UInkWulingToolsPanel` | Wuling source/catalog authority | Partial: alias to shared tools page |
| 38 | guild-build / `GuildConstruction` | HUD `OpenWuling` → Wuling `Open_WulingConstruction` → `WulingConstruction` → `UInkWulingToolsPanel` | Wuling construction authority | Partial: alias to shared tools page |
| 39 | equipment / `EquipmentFabao` | HUD `OpenCharacter` → profile “装备与法宝” CTA → `CharacterEquipment` → `UInkCharacterEquipmentPanel` | Wuling loadout supplies four equipment slots; fabao slot opens inventory treasure filter; separate `CharacterArtifacts` route remains | Partial; real profile entry; combined equipment/fabao flow is still split |
| 40 | journey / `Extensions` | HUD `OpenExtensions` → `Extensions` → `UInkJourneyMapPanel` | Journey/map metadata and current availability | Partial: this route is intentionally mapped to the map; the old `UInkFeatureHubPanel` table entry is shadowed |
| 41 | craft / `WulingCraft` | HUD `OpenWuling` → Wuling `Open_WulingWorld` → `Open_WulingCraft` → `WulingCraft` → `UInkWulingToolsPanel` | Wuling recipe/crafting authority | Partial: shared tools page |
| 42 | appearance / `CharacterAppearance` | HUD `OpenCharacter` → profile “外观试戴” CTA → `CharacterAppearance` → `UInkCharacterAppearancePanel` | Local frame/style preview state; no separate server appearance API evidenced here | Present as reachable local presentation page; preview only |
| 43 | resource-rank / `ResourceRanking` | Map/contest CTA → `ResourceRanking` → `UInkResourceRankingPanel` | Reads race snapshot only; panel explicitly states authority does not provide resource period, category scores, rank, or player position | Partial: real route and honest unavailable state; no fabricated ranking data |
| 44 | resource-rewards / `ResourceRewards` | Contest CTA → `ResourceRewards` → `UInkResourceRewardsPanel` | Placeholder rewards are labeled and explicitly not claimed as obtained; confirmed receipt is not present in this page | Partial: page exists, reward data is absent |

## Structured player navigation for route harnesses

`player_entry_actions` lists exact action ids in click order, separated by semicolons. `DYNAMIC:<context>` means the following action needs a real route-specific id from authority/map content. `player_entry_safety` describes opening the route only; it does not authorize clicking page operations. Wuling tools routes are `side_effect_or_unknown` because active panels can automatically request a catalog snapshot when opened. `final_CurrentPage` and `final_SelectedRoute_or_state` record canonical shared routes; aliases are not separate runtime states.

| # | player_entry_actions | player_entry_safety | final_CurrentPage | final_SelectedRoute_or_state |
|---:|---|---|---|---|
| 1 | `PIE_STARTUP` | navigation_only | `Main` | — |
| 2 | `StartGame` | side_effect_or_unknown | `HUD` | Starts gameplay/possession, initializes gameplay systems, and may publish gameplay data before showing HUD |
| 3 | `OpenCharacter` | navigation_only | `CharacterProfile` | — |
| 4 | `OpenInventory` | navigation_only | `Inventory` | — |
| 5 | `OpenLeaderboard` | navigation_only | `Leaderboard` | — |
| 6 | `OpenSaves` | navigation_only | `Saves` | — |
| 7 | `OpenSettings` | navigation_only | `Settings` | — |
| 8 | `OpenWuling` | navigation_only | `Wuling` | — |
| 9 | `OpenWuling; Open_WulingRights` | navigation_only | `WulingRights` | SwordSpirit alias; shared rights page |
| 10 | `OpenWuling; Open_GiftRules` | navigation_only | `GiftRules` | Explicit unavailable page |
| 11 | `OpenWuling; Open_WulingEquipment; ToolView_methods` | side_effect_or_unknown | `WulingEquipment` | `SelectedEntry=methods`; CultivationMethods alias |
| 12 | `OpenExtensions; AtlasResource; Open_ResourceContest; Open_WulingRace` | navigation_only | `WulingRace` | RaceLobby alias; shared race snapshot |
| 13 | `OpenExtensions; AtlasResource; Open_ResourceContest; Open_WulingRace` | navigation_only | `WulingRace` | RaceLive alias; no distinct live state |
| 14 | `OpenExtensions; AtlasResource; Open_ResourceContest; Open_WulingRace` | navigation_only | `WulingRace` | RaceSettlement alias; no distinct settlement state |
| 15 | `OpenExtensions; AtlasResource; Open_ResourceContest; Open_WulingRace` | navigation_only | `WulingRace` | ResourceLease alias; shared race page, lease state remains authority data |
| 16 | `OpenExtensions; Open_WulingWorld` | navigation_only | `WulingWorld` | EventSettlement alias; shared world page |
| 17 | `OpenWuling; Open_WulingOffline` | navigation_only | `WulingOffline` | OfflineRewards alias; shared records page |
| 18 | `OpenWuling; Open_WulingWorld; Open_WulingCraft; ToolNavigation; ToolRoute_7` | side_effect_or_unknown | `WulingShop` | Shop alias; shared tools page |
| 19 | `OpenWuling; Open_WulingWorld; Open_WulingCraft; ToolNavigation; ToolRoute_8` | side_effect_or_unknown | `WulingMarket` | Market alias; shared tools page |
| 20 | `OpenWuling; Open_WulingOrders` | navigation_only | `WulingOrders` | TradeOrders alias; shared records page |
| 21 | `OpenExtensions; AtlasWorld` | navigation_only | `Extensions` | WorldCultivation alias; map tab `AtlasWorld` |
| 22 | `OpenExtensions; AtlasWorld; DYNAMIC:world-map-entry-id; AtlasNode_<id> or AtlasEntry_<id>` | navigation_only | `JourneyWorldDetail` | RegionDetail alias; entry id required |
| 23 | `OpenExtensions; AtlasResource; Open_ResourceContest` | navigation_only | `ResourceContest` | — |
| 24 | `OpenExtensions; AtlasResource; DYNAMIC:resource-map-entry-id; AtlasNode_<id> or AtlasEntry_<id>` | navigation_only | `JourneyResourceDetail` | ResourceDetail alias; entry id required |
| 25 | `OpenSignIn; GroupTab_SignIn` | navigation_only | `SignIn` | `SelectedRoute=SignIn` |
| 26 | `OpenSignIn; GroupTab_MonthlySignIn` | navigation_only | `SignIn` | `SelectedRoute=MonthlySignIn` |
| 27 | `OpenSignIn; GroupTab_DailyFortune` | navigation_only | `SignIn` | `SelectedRoute=DailyFortune` |
| 28 | `OpenActivities; GroupTab_DailyActivities` | navigation_only | `Activities` | Group parent; default selected child `DailyActivities` |
| 29 | `OpenActivities; GroupTab_DailyActivities` | navigation_only | `Activities` | `SelectedRoute=DailyActivities` |
| 30 | `OpenActivities; GroupTab_TimedActivities` | navigation_only | `Activities` | `SelectedRoute=TimedActivities` |
| 31 | `OpenActivities; GroupTab_DailyActivities; DYNAMIC:activity_id; Open_DailyActivityDetail\|<id>` | navigation_only | `DailyActivityDetail` | Matching daily template required; empty-list fallback `Open_DailyActivityDetail` shows no-match state |
| 32 | `OpenActivities; GroupTab_TimedActivities; DYNAMIC:activity_id; Open_TimedActivityDetail\|<id>` | navigation_only | `TimedActivityDetail` | Matching limited template required; empty-list fallback `Open_TimedActivityDetail` shows no-match state |
| 33 | `OpenLaunchEvent` | navigation_only | `LaunchEvent` | — |
| 34 | `OpenLaunchEvent; LaunchTab_honors; Open_LaunchRank` | navigation_only | `LaunchRank` | Honors view; shared launch panel |
| 35 | `OpenWuling; Open_WulingWorld; Open_WulingCraft; ToolNavigation; ToolRoute_2` | side_effect_or_unknown | `WulingBreakthrough` | BreakthroughPrepare alias; authority-derived preparation state |
| 36 | `OpenWuling; Open_WulingWorld; Open_WulingCraft; ToolNavigation; ToolRoute_2` | side_effect_or_unknown | `WulingBreakthrough` | Tribulation alias; no distinct tribulation CurrentPage |
| 37 | `OpenWuling; Open_WulingWorld; Open_WulingCraft; ToolNavigation; ToolRoute_1` | side_effect_or_unknown | `WulingSources` | ItemSources alias; shared tools page |
| 38 | `OpenWuling; Open_WulingConstruction` | side_effect_or_unknown | `WulingConstruction` | GuildConstruction alias; shared tools page |
| 39 | `OpenCharacter; Open_CharacterEquipment` | navigation_only | `CharacterEquipment` | EquipmentFabao alias |
| 40 | `OpenExtensions` | navigation_only | `Extensions` | Journey map page; shadowed FeatureHub legacy route |
| 41 | `OpenWuling; Open_WulingWorld; Open_WulingCraft` | side_effect_or_unknown | `WulingCraft` | — |
| 42 | `OpenCharacter; Open_CharacterAppearance` | navigation_only | `CharacterAppearance` | Local presentation preview only |
| 43 | `OpenExtensions; AtlasResource; Open_ResourceRanking` | navigation_only | `ResourceRanking` | Explicit unavailable ranking state |
| 44 | `OpenExtensions; AtlasResource; Open_ResourceContest; Open_ResourceRewards` | navigation_only | `ResourceRewards` | Labeled preview; no confirmed receipt |

## Source changes and static evidence

- `InkFeaturePanel.cpp` maps `GiftRules` to `UInkGiftRulesPanel`; `InkWulingSectPanel.cpp` adds the player CTA from the HUD’s Wuling route. `UI/InkGiftRulesPanel.*` shows only the explicit unavailable message and has no authority/GM/live-session reads or request calls.
- `InkFrontendExtensions.cpp`: normalize contract aliases to existing native routes, preserve context for shared routes, and resolve `EquipmentFabao` to the existing character equipment page.
- `InkCharacterPanels.cpp`: the character profile now exposes `Open_CharacterEquipment` and `Open_CharacterAppearance`; both lead to existing native pages through the player route resolver.
- `InkRewardsPanels.cpp`: when daily or limited templates are empty, a detail CTA opens the existing detail panel in its explicit no-match state; the empty-context route makes no request.
- `InkFeaturePanel.cpp`: keep the journey map route precedence explicit and map `LaunchRank` to `UInkLaunchEventPanel`.
- `InkJourneyMapPanel.cpp` / `InkFeaturePanels.cpp`: expose the resource map → contest center → resource ranking path, retaining the existing `Open_WulingRace` CTA.
- `InkLaunchEventPanel.cpp`: add the launch honors route state and `Open_LaunchRank` entry from the honors section.
- `CharacterFate`, `Draw`, and `Expedition` are not among the 44 top-level ids in the current screen contract. Their legacy classes/routes are therefore out of this matrix and are not counted as accepted contract screens.
- This pass found two pure-UI reachability gaps inside the 44: the profile had no CTA to the existing equipment or appearance panels, and empty daily/limited template lists had no CTA to the detail panels’ unavailable state. All four routes now have explicit player navigation; empty-context detail routes only render the no-match state and send no request.
- Remaining limits are data/API coverage, not missing page classes for these routes: gift rules have no player authority read model; resource period rankings/reward receipts are not supplied; race lobby/live/settlement/lease share one authority-backed panel rather than separate CurrentPage states; journey aliases open shared map/detail classes; character appearance is presentation-only. Candidate art remains unapproved until the user grants visual approval; this audit did not inspect candidate images.
- `screen-contract.yaml`: includes the current `Extensions`, `WorldCultivation`, and `ResourceMap` aliases. The equipment screen already lists `EquipmentFabao` and `CharacterEquipment` as aliases.

Static checks run on 2026-10-23:

1. PowerShell parsed exactly 44 top-level screen entries from the contract.
2. Targeted source inspection confirmed the entry/button handlers, route aliases, shared page classes, and new character-profile/activity-empty-state CTAs; both Markdown route tables contain 44 numbered rows with consistent columns.
3. Scoped `git diff --check` passed for the changed C++ panels. Git emitted only existing working-tree CRLF conversion warnings.

These checks establish source-level route wiring only. They do not establish compilation, Unreal widget creation at runtime, PIE behavior, rendering/layout, package acceptance, or production-service availability. No image was opened and no Unreal/PIE/build process was started for this audit.
