# Journey expansion implementation plan

Goal: deliver requested native Unreal widgets and runtime integrations using the approved ink UI system.
Architecture: independent feature widgets share the responsive shell; authority remains the only economy writer. Parent owns route/HUD/settings/map integration and serialized editor runs. Backups replace Git because this workspace is not a Git repository.
Spec: design.md. Existing approved baseline and direct implementation request authorize this continuation.

- [x] Read current contracts, routes, authority APIs; preserve source backup.
- [ ] Launch panel: new UI/InkLaunchEventPanel files; authoritative launch tasks, goals, expiry and start action. No shared-route edits by worker.
- [ ] Sect panel: new UI/InkWulingSectPanel files; true guild data, creation and routes; retain old world preparation panel. No shared-route edits by worker.
- [ ] Receipts: new UI/InkRewardReceiptPanel files and authority/subsystem receipt projection, tests for no-repeat and real grants; parent mounts overlay.
- [ ] Parent: new generated maps, map and destination widgets, authority destination projection, HUD and route wiring, responsive settings.
- [ ] Parent: Editor build, native and service tests, imports and rendered PIE with screenshots and action/navigation checks.
- [ ] Parent: inspect rendered results, correct failures, run GameUI validator and write delivery/limitations.

Review focus: narrow width/long labels, service disabled/disconnected, locked destinations, busy double clicks, replayed reward receipts, cancel returning to selected map, existing world preparation actions, no invented owner or inventory grants.
