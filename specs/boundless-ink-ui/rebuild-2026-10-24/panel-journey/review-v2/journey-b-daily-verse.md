# Journey B extension: separate daily verse

Owner: panel-journey; contract revision: `newui-r1`; candidate `approved:false`.

`daily-verse` / existing `DailyFortune` gets a distinct `UInkJourneyDailyVersePageWidget`, separate from `sign-in` and `monthly-sign-in`. The host may display only `snapshot.activities.verse` from `ActivityVm`. Missing verse stays unknown. Fortune classification and reward fields may be absent; no local fortune draw, favorable/unfavorable value, or reward is derived. This page is read-only with Back through the existing parent stack.

Together the contract's `journey-b` merge slice contains three pages: `sign-in`, `monthly-sign-in`, `daily-verse`. The daily sign-in page's verse card is a distinct field presentation; it is not a fortune surface and does not replace the separate read-only route.

No page route, service mapping, compile, import or runtime evidence is claimed by this staged revision.
