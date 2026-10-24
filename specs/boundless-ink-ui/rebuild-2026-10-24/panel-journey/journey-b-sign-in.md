# Journey B: daily and monthly sign-in

Owner: panel-journey. Contract revision: `newui-r1`. Merge slice: `journey-b`. Candidate only; `approved:false`, visual review pending.
### Daily sign-in

Page is composed from service day, explicit sign-in status, optional month count, a separate read-only 每日剑诀 verse card, and a real sign-in request callback. It does not show or calculate fortune. Unknown verse stays “每日剑诀暂未提供”; it does not change claim eligibility.

The action carries the exact displayed service-day key (`ExpectedDay`) through `OnClaimRequested`. Integration uses `InkJourneyAdapter::Capture/Evaluate/Submit(SignIn)` and only reports completion from the authoritative result/receipt. A UI pending label is not success.

### Monthly sign-in

Month header, 88×88 calendar hit cells and milestone cards. The content uses a width-aware wrap container: it flows as one column at portrait width and places the calendar and milestone panel side by side in landscape. The current service date/month/count are explicit inputs. Calendar cells show day numbers only; because the current provider does not supply per-day history in the agreed VM, this candidate does not claim historical days were signed, missed, or future. Milestones are selectable only when the provider returns known records; claim remains disabled when eligibility is unknown.

A confirmed claim callback includes exact `ExpectedDay` and selected milestone. Integration recaptures context, re-evaluates `MonthlyChest` with confirmation and these exact arguments, submits once, then resolves pending from the actual service result. The UI does not grant rewards or infer receipt success.

## Shared composition rules

- Portrait logical baseline 720×1280, 24-unit content insets, shortest-side scaling owned once by root. Landscape is independently reflowed to 1280×720, never portrait-scaled.
- Live text remains UMG text and follows shared typography, colors, spacing, and 88-unit touch minimums. Final contrast/raster fitting still needs assembled visual review.
- New fonts resolve under `/Game/newUI/Shared/Fonts`; no runtime reference to `/Game/UI`.
- The daily verse remains a separate read-only field; no fortune is shown or derived.

## Readiness boundary

The staged classes are not compiled, imported, routed, or runtime-verified. A root-owned presenter must own each child widget with UPROPERTY and connect route/back stack, VM mapping, focus, adapter actions, and pending completion. The integration owner remains the only production Source/Config/Content writer.


