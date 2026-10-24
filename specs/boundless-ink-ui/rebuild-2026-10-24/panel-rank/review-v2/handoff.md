# panel-rank review-v2 handoff

**State:** isolated replacement staged; `approved:false`; original `panel-rank/handoff.md` and `staged-source` are frozen. The integration owner remains the only production writer.

## Replacement files

Add `staged-source/InkRankWulingPageBase.h/.cpp`; replace `InkRankWulingPages.h/.cpp` and `InkRankWulingExtendedPages.h/.cpp` from this directory. Expected integration target: `Source/BoundlessImmortality/Frontend/UIRebuild20261024/RankWuling/`. Review `replacement.diff` and `replacement-contract.yaml` before consumption.

## Review fixes

C01 uses a `UButton` subclass whose `UFUNCTION` click handler is bound with `AddUniqueDynamic`; the action closure is retained on the button UObject. C02 applies padding to `UButtonSlot`. C03 constructs `FSlateChildSize` with one rule argument and then assigns `Value`. C04 injects the shared MaShanZheng/LXGWWenKai `UFont` assets through `SetSharedThemeFonts`; CoreStyle's composite font is a temporary pre-injection fallback.

L02 keeps one `RootWidget` per page and refreshes only its persistent `Content` child. L03 requests, caches and maps images by `ParticipantId`, so delayed textures follow the participant across rank changes, refresh, and page return. `SetParticipantAvatar(id, nullptr)` clears the in-flight marker for retry. No rank or array index is used as identity.

All six pages retain their VMs, host-owned actions, and confirmation rules. Member removal is still gated by the current selected `user_id`; construction binds the current building id, target level and snapshot version. Buttons have 88×88 minimum hit boxes. WrapBox controls and 440-unit card grids form one portrait column and multiple landscape columns at 720×1280 / 1280×720 logical canvases. The host supplies SafeZone and scale.

## Static evidence and limits

The review report and current original source were checked against installed headers under `F:/unreal/UE_5.8/UE_5.8/Engine/Source`. Static source checks found no `OnClicked.AddLambda`, `UButton::SetPadding`, two-argument `FSlateChildSize`, or invalid `FSlateFontInfo(NAME_None,size,FName("Regular"))`; they also checked the callback binding, 88×88 constraints, participant-keyed avatar map, stable content root, responsive wrap groups, confirmation key, and brace balance. The review's recorded SHA-256 for the original rank `.cpp` differs from the current checkout, so findings were rechecked against the current stage and installed API declarations before creating this replacement.

No engine, UBT, PIE, runtime screenshot, or compile check was started. Shared font object paths still need integration resolution under `/Game/newUI/Shared/Fonts`; UMG visual behavior and delayed avatar completion remain runtime checks. No page or component image was generated.

## Next owner

Integration should consume or reject this isolated replacement, resolve/load both canonical shared fonts and inject them into all six pages, connect participant-keyed avatar requests to the existing provider, then compile and verify visible refresh, delayed avatar completion after refresh/page return, portrait/landscape layout, and exact-subject confirmations. There is no separate Rank/Wuling slice remaining; this task is now waiting on integration.
