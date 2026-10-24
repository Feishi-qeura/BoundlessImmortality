# Diagnostic support V3 — known source/config warning cancelled

This is an independent revision. **Do not replace or mutate the currently loaded migration/migrate_ui_ue.py/controller while integration executes.** Root cause and exact evidence are in ../../rename-failure-analysis.md. The V2 582+6 map, full snapshot, pin and referencer validations remain unchanged.

`inspect_memory_readonly.py` is a separate find-only diagnostic for integration's owned Editor. It does NOT load assets, save, rename, delete, clear journals or close an Editor. It records loaded old/new object identities, new generated-class identities, dirty content/maps, all588 original source hashes and real target disk presence. Unloaded objects are reported as unknown, not silently treated as unchanged. Running this is integration-only. If the previous PID has already normally exited, this cannot reconstruct its memory history.

`ScopedKnownRenameWarning.h/.cpp` is an **uncompiled native suggestion**, not a complete replacement migration runner and not a Python-exposed function. Integration can incorporate it into a native command in its own reviewed Source epoch, passing the same existing exact582 rename rows and retaining ALL V2 preflight/pin/backup/journal/save/fixup checks.

It changes only the warning acknowledgement during ONE synchronous `AssetTools.RenameAssets(Exact582)` call:

- Rejects a process still launched with `-unattended`; that mode can choose a default before a delegate is invoked. This is no instruction to close or kill a current session. Preserve facts/memory and coordinate safe execution first.
- Requires exactly582 actual old-root nonredirector assets, completed discovery, unique new-root destinations and approved observed names.
- Saves/restores GIsRunningUnattendedScript through TGuardValue and saves/restores the previous FCoreDelegates::ModalMessageDialog delegate through ON_SCOPE_EXIT. Refuses if FApp::IsUnattended remains true.
- Matches only the captured Chinese source/config search-replace warning, exact header/trailer and the exact SET of11 approved names; it ignores order/blank-line/CRLF variation only. Names must all be in the current canonical rename batch.
- Accepts only message type OkCancel and returns Ok. The log did not record the message type, so this is a deliberately narrow implementation hypothesis, not an assertion that its runtime type is already proven. Any different type/text/additional name is logged and returns Cancel. Never blindly expand the handler or accept all modals.
- Result includes all modal type/category/title/body evidence, known-warning acknowledgement count, unknown rejection count and actual RenameAssets result. Integration must persist it before any subsequent stage; an acknowledged warning alone is not rename success.

The installed engine has AssetRenameManager.h/IAssetTools.h/MessageDialog.h/CoreDelegates.h but no corresponding cpp sources. Their signatures/semantics and the concrete cancellation log support this suggestion; it still needs an integration build and actual scoped delegate/runtime verification. Switching to RenameAssetsWithDialog's bAutoCheckout or soft-only flags is not the same fix.

Before any retry, the failed before_rename intent must be preserved alongside controller/log/delta evidence. Do not clear it to bypass the guard. Only after integration establishes clean memory identities/dirty state and unchanged original snapshot/no targets may it archive that failed attempt and begin a separately recorded retry. If any partial in-memory or disk mutation exists, keep that state and resolve it explicitly.

Current diagnosis does not claim migration success or authorize unsafe session shutdown. Actual absence of Content/UI and registry old root zero still await real engine execution and owner audit.
