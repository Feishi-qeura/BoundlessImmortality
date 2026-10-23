# Niagara UI corrections

Implement the user's four screenshot corrections in the existing UE frontend.

1. Verify the placed menu actor `BP_TopDownCharacter2` is presentation only, destroy it after successful start, and keep gameplay reentry independent of presentation transforms. Verify exact pawn class and lifetime in PIE.
2. Share honor/realm/icon artwork and IDs between projected sword labels, HUD and both leaderboards. Use actual CPU NiagaraUIRenderer particles, quality tiers, and hidden/offscreen/reduced motion cleanup.
3. Generate and import an ivory/jade mountain HUD backing, themed button states, three meter tracks/fills and a fully padded cultivation icon. Preserve unrelated content and saved widgets.
4. Center and outline live meter text, clip fill strictly inside the track, display verified streamer profile texture and a Niagara avatar frame. Propagate profile updates safely across sessions.
5. Keep compact tabs inside the full backing silhouette; unify full leaderboard backing, rows, controls and close button; restore explicit first-three ranks.
6. Build Editor and Game, run native and service regressions, render portrait/landscape and narrow HUD checks. Inspect pixels, changing particle positions/counts, avatar replacement, quality tiers, lifecycle and all four reported problem areas. State exact validation boundaries.

Evidence and immutable baseline: `Saved/Codex/NiagaraUiRevision20261022`.
