# HUD navigation rail concept prompt

Create one isolated horizontal mobile-game HUD navigation rail on a transparent background, 16:3 aspect ratio. Show exactly six separate equal touch-control silhouettes in a single row, joined by a very restrained low-opacity warm-paper tray with dark ink-green details and thin antique-gold edges. Each control must have a generous 88-unit minimum touch silhouette. The component should feel light, compact, and readable over a game scene. Do not draw any background, character, scenery, game screen, letters, words, numerals, logos, icons, badges, or status values. Leave open negative space around the rail and avoid a full-page overlay. The controls will be individual live UMG buttons with runtime text and real existing actions; this image is for visual review only and will not be used as an interactive surface.

## Mapping

The six controls follow the host's current action order: `OpenCharacter`, `OpenInventory`, `OpenLeaderboard`, `OpenSaves`, `OpenSettings`, `ReturnToMainMenu`. Their runtime text is the actual existing Chinese labels and their actions remain dispatched by `UInkFrontendActionButton`.
