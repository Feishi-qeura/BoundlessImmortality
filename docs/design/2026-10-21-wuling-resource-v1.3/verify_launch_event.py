"""Check launch-event design data and examples only. No runtime game verification."""
from pathlib import Path
import csv
import json
from decimal import Decimal, ROUND_HALF_UP

ROOT = Path(__file__).resolve().parent
T = ROOT / "tables"


def rows(name):
    with (T / name).open(encoding="utf-8-sig", newline="") as h:
        return list(csv.DictReader(h))


def main():
    c = json.loads((T / "launch_event.json").read_text(encoding="utf-8"))
    balance = json.loads((T / "balance.json").read_text(encoding="utf-8"))
    tasks = rows("launch_tasks.csv")
    tiers = rows("launch_guild_milestones.csv")
    items = {r["item_id"]: r for r in rows("items.csv")}
    checks = []

    def check(name, ok):
        checks.append({"name": name, "passed": bool(ok)})

    check("task_ids_unique", len(tasks) == len({r["task_id"] for r in tasks}) == 7)
    check("task_budget_780", sum(int(r["reward_count"]) for r in tasks) == c["task_qi_total"] == 780)
    check("tasks_have_no_paid_condition", all(r["paid_condition"] == "false" for r in tasks))
    check("all_task_rewards_for_viewer", all(r["reward_owner"] == "viewer" and r["reward_item"] == "STAT_QI" for r in tasks))
    check("title_requires_all_tasks", set(c["title"]["requires_tasks"]) == {r["task_id"] for r in tasks})
    check("title_no_payment_no_rank_no_stats", not any(c["title"][k] for k in ("requires_gift", "requires_rank", "has_numeric_bonus", "tradeable")))
    check("title_viewer_only", c["title"]["owner_type"] == "viewer" and items[c["title"]["item_id"]]["owner_type"] == "viewer")
    check("title_free_template", items[c["title"]["item_id"]]["paid_origin"] == "false")
    check("title_duration_30days", c["title"]["max_duration_seconds"] == 30 * 86400)
    check("guild_thresholds", [int(r["complete_rounds"]) for r in tiers] == [1, 3, 6, 12])
    check("guild_stone_budget", sum(int(r["anchor_stone"]) for r in tiers) == c["guild_reward_budget"]["CUR_STONE"] == 200)
    check("guild_material_budget", sum(int(r["anchor_break_material"]) for r in tiers) == c["guild_reward_budget"]["MAT_BREAK"] == 60)
    check("guild_no_paid_threshold", all(r["paid_condition"] == "false" for r in tiers))
    check("buff_only_last_tier", [r["tier_id"] for r in tiers if r["grant_final_buff"] == "true"] == ["G12"])
    b = c["final_guild_buff"]
    check("buff_event_qi_cap", b["cultivation_additive"] <= balance["event_qi_bonus_cap"])
    check("buff_no_offline_race_or_claim_reset", not any(b[k] for k in ("offline_effect", "race_scoring_effect", "claim_restarts_timer")))
    check("buff_duration", b["duration_seconds"] == 1200)
    check("window_seven_days", c["duration_seconds"] == 7 * c["activity_day_seconds"] == 604800)
    preset = c["world_preset"]
    check("launch_round_total_600", preset["prepare_seconds"] + sum(preset["stage_seconds"]) + preset["settlement_seconds"] == c["complete_round_seconds"])
    check("launch_three_stages", len(preset["stage_seconds"]) == 3)
    check("free_mechanism_opportunities", preset["guard_opportunities"] >= 2 and preset["break_opportunities"] >= 2 and preset["free_route_vote"])
    pending = rows("launch-acceptance.csv")
    check("thirty_runtime_cases_not_claimed", len(pending) == 30 and all(r["current_result"] == "not_run" for r in pending))
    check("unique_launch_acceptance_ids", len(pending) == len({r["case_id"] for r in pending}))
    s = c["score"]
    check("round_start_bucket_declared", s["day_bucket_basis"] == "round_start_utc")
    score_max = sum(s["stage_points"]) + s["correct_response_points"] * s["correct_responses_per_round_cap"] + s["route_vote_points"] + s["free_participation_points"]
    check("per_round_twenty", score_max == s["max_per_round"] == 20)
    check("daily_120", score_max * s["scored_rounds_per_activity_day"] == s["max_per_day"] == 120)
    check("full_window_840", s["max_per_day"] * 7 == s["max_seven_days"] == 840)
    check("no_paid_or_damage_score", all(s[k] == 0 for k in ("gift_value_weight", "gift_count_weight", "damage_weight", "realm_weight")))
    check("points_not_currency", s["exchangeable"] is False)
    for complete, pts, expected in ((1, 40, False), (2, 29, False), (2, 30, True), (6, 120, True)):
        qualifies = complete >= s["rank_min_complete_rounds"] and pts >= s["rank_min_points"]
        check(f"qualification_{complete}_{pts}", qualifies is expected)
    for row in tasks:
        kind = row["condition_type"]
        available_in_six_rounds = {
            "join_and_free_response": 1,
            "complete_world_rounds": 6,
            "correct_guard_response": 6,
            "correct_break_response": 6,
            "route_vote_in_distinct_rounds": 6,
        }[kind]
        check("six_round_title_" + row["task_id"], available_in_six_rounds >= int(row["target"]))
        check("task_item_" + row["task_id"], row["reward_item"] in items)
    for profile, flags in c["profiles"].items():
        check(profile + "_rank_numbers_closed", flags["rank_numeric_rewards"] is False)
        check(profile + "_no_real_payment", flags["real_payment_allowed"] is False)
    check("rank_candidates_disabled", c["numeric_rank_candidates"]["enabled"] is False)
    check("release_launch_closed", all(v is False for v in c["profiles"]["douyin_release_candidate"].values()))

    # Distinct activity days: a relative seven-day event cannot get eight midnight resets.
    day_indices = {i // c["activity_day_seconds"] for i in range(0, c["duration_seconds"], 3600)}
    check("exactly_seven_activity_day_buckets", day_indices == set(range(7)))
    for start, expected in ((0, True), (604200, True), (604201, False), (-1, False)):
        qualifies = start >= 0 and start + c["complete_round_seconds"] <= c["duration_seconds"]
        check("round_time_boundary_" + str(start), qualifies is expected)

    # Comparison table is generated disabled; never opened by the existence of data.
    comparison = []
    for rank in range(1, 31):
        if rank <= 5:
            amount = c["numeric_rank_candidates"]["first_five"][rank - 1]
        else:
            value = Decimal(500) - Decimal(rank - 6) * Decimal(350) / Decimal(24)
            amount = int(value.quantize(Decimal(1), rounding=ROUND_HALF_UP))
        comparison.append({"rank": rank, "candidate_qi": amount, "enabled": "false", "purpose": "disabled_comparison_only"})
    with (T / "launch_rank_candidates.csv").open("w", encoding="utf-8-sig", newline="") as h:
        writer = csv.DictWriter(h, fieldnames=list(comparison[0]))
        writer.writeheader()
        writer.writerows(comparison)
    amounts = [r["candidate_qi"] for r in comparison]
    check("candidate_first_five", amounts[:5] == [4800, 3480, 2520, 1800, 1260])
    check("candidate_tail_endpoints", amounts[5] == 500 and amounts[-1] == 150)
    check("candidate_monotonic", all(a >= b for a, b in zip(amounts, amounts[1:])))
    check("candidate_round_half_up", comparison[11]["candidate_qi"] == 413)
    values = [100, 100, 90, 80, 80]
    competition_ranks = [1 + sum(other > value for other in values) for value in values]
    check("tie_competition_ranks", competition_ranks == [1, 1, 3, 4, 4])

    # Economic examples, no gift, no offline. Sign-in is assumed once.
    first_hour_base = 3600 + 6 * balance["event_extra_qi"] + balance["sign_qi"]
    first_hour_with_launch = first_hour_base + c["task_qi_total"]
    check("first_hour_example", (first_hour_base, first_hour_with_launch) == (4260, 5040))
    qi_benefit = b["duration_seconds"] * b["cultivation_additive"] * balance["base_qi_per_second"]
    check("buff_base_qi_per_member", qi_benefit == 120)
    report = {
        "passed": all(x["passed"] for x in checks),
        "check_count": len(checks),
        "failed": [x for x in checks if not x["passed"]],
        "task_qi_total": c["task_qi_total"],
        "guild_reward_budget": c["guild_reward_budget"],
        "first_hour_no_gift_no_offline_qi": first_hour_with_launch,
        "first_hour_assumptions": "完整6轮、全部任务、签到一次、新规则档、无礼物无离线",
        "buff_base_extra_qi_per_member": qi_benefit,
        "rank_numeric_enabled": False,
        "disabled_rank_reward_total_no_ties_30_people": sum(amounts),
        "runtime_tests_run": False,
        "scope": "开服策划配置、算术及边界示例，未验证游戏实现、平台或商业效果",
        "checks": checks,
    }
    (ROOT / "launch-planning-validation.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({k: v for k, v in report.items() if k != "checks"}, ensure_ascii=False, indent=2))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
