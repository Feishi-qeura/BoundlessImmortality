# Batch 2 coverage matrix

| Route | Data/action coverage | Visual artifact | Staged composition | Integration/runtime evidence |
|---|---|---|---|---|
| Saves | 3 local slots; `bExists`, `bValid`, `SavedAt`, `MapName`, `PlayedSeconds`, `ValidationError`; `LoadSlot_i`, `SaveSlot_i`, `DeleteSlot_i`; host confirmation/refresh | `assets/pages/saves-v1.png`, text-free concept, `approved:false` | `BuildSavesBody`; staged only | Not promoted, imported, compiled, PIE-verified, or route-verified |
| Settings | Existing quality 0/1/2, frame cap 30/60, reduced motion; apply/save/back in existing host | `assets/pages/settings-v1.png`, text-free concept, `approved:false` | `BuildSettingsBody`; staged only | Not promoted, imported, compiled, PIE-verified, or route-verified |

## Out of scope, still uncovered

Methods, equipment, appearance, crafting, item sources, breakthrough and tribulation remain explicit backlog. This batch does not claim coverage for them. Main, home, character and inventory remain unchanged and preserved.
