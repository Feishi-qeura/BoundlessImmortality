# Rank quality correction slice v2

This isolated replacement targets `panel-rank/staged-source`; it leaves those files and the original handoff unchanged because integration may already consume them. Review `replacement.diff`, then select the replacements through the sole integration writer.

The patch adds `UInkRankWulingPageBase` and `UInkRankWulingCallbackButton`, and replaces the four staged page source files. It addresses C01–C04 and L02–L03 across all six Rank/Wuling pages. View models and host action ownership remain intact. Member removal confirmation binds the selected stable `user_id`; build confirmation binds `building_id`, target level and snapshot version.

Text uses shared title/body font injection through `SetSharedThemeFonts`. Controls have 88×88 minimum hit boxes. Width-constrained `UWrapBox` strips and card grids reflow at portrait and landscape widths; the host continues to own SafeZone and scale.

No image was generated. Static evidence and remaining integration checks are in `handoff.md`.
