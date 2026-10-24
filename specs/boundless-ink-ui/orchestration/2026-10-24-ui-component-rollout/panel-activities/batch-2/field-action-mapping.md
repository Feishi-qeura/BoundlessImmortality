# Batch-2 field and action mapping

## Launch activity page

| UI element | Authority source | Handling |
|---|---|---|
| Period | `launch.start`, `launch.end` (Unix seconds) | Render as Hong Kong time from server fields; pending when absent |
| Personal contribution | `launch.personal.score`, `complete_count` | Render numeric values only when present |
| Task label/progress/state | `launch.tasks[].condition_type`, `progress`, `target`, `confirmed` | Known condition IDs get readable labels; missing values remain pending |
| Task numeric reward | `launch.permissions.task_numeric_rewards`, task `reward_item`, `reward_count` | Show only when permission is true and both values exist; state that settlement is automatic |
| Guild milestone | `launch.guild_milestones[].complete_rounds`, `launch.complete_rounds[]` | Render the scalar server target; count current rounds only after successful array retrieval (an explicit empty array is 0); a missing array stays pending |
| Offer panel | No launch offer/SKU/price/purchase contract found in current service snapshot/API | Explanatory disabled panel; no buy action |
| Start event | `InkJourneyAdapter::Submit(StartLaunch)` | First click arms confirmation; second click submits after adapter rechecks profile, current page, connection/busy state, event absence, anchor/guild identity, and server window |
| Enter active event | `launch.status`, `launch.end`, root `server_utc` | Navigate to existing `WulingWorld` route only when status is Active and authority end is later than server time |
| Rank navigation | Existing `Open_LaunchRank` action convention | Open `LaunchRank` route |

## Launch rank page

| UI element | Authority source | Handling |
|---|---|---|
| Scope | `launch.start`, `launch.end` | Same server-time formatter as launch page |
| Personal summary | `launch.personal.score`, `complete_count` | Pending when absent; does not infer personal rank |
| Rows | `launch.leaderboard[].rank`, `name`, `score` | Render returned rank values verbatim, so server tie ranks remain intact |
| Eligibility/rewards | No client-derived values | Not displayed; service remains the authority |

## Activity detail shell

| UI element/action | Authority source | Handling |
|---|---|---|
| Selected activity | `SelectedEntry` plus `activities.templates[]` | Exact `activity_id` and exact cycle match required |
| Name/description/eligibility/times/budget | Matching template fields `name`, `description`, `profile_requirement`, `start_at`, `end_at`, `reward_budget` | Display supplied text only; absent values are pending |
| Progress | Matching template `progress`, `target` | Display only if server supplies numbers |
| Service states | `InkJourneyAdapter::Read` | Distinct disabled/offline/loading/pending/ready-missing copy; no guessed expiry |
| ACT_SIGN button | `View.Signed`, captured `ServiceDay` | Show done if signed, show button only when signed is explicitly false; Submit revalidates the captured service day and current route |
| Other participation | No verified action contract | Pending copy, no action |
