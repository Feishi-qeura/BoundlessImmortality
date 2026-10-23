"""Validate design data only; never opens game assets, saves, network or secrets."""
from __future__ import annotations

import csv
import hashlib
import json
from collections import defaultdict
from datetime import datetime, timezone
from decimal import Decimal, ROUND_CEILING
from pathlib import Path

ROOT = Path(__file__).resolve().parent
TABLES = ROOT / "tables"


def read_csv(name):
    with (TABLES / name).open(encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle))


def main():
    config = json.loads((TABLES / "balance.json").read_text(encoding="utf-8"))
    realms = read_csv("realm_targets.csv")
    items = read_csv("items.csv")
    rewards = read_csv("world_rewards.csv")
    nodes = read_csv("nodes.csv")
    shop = read_csv("shop.csv")
    market = read_csv("market.csv")
    methods = read_csv("methods.csv")
    activities = read_csv("activities.csv")
    encounters = read_csv("encounters.csv")
    checks = []

    def check(name, condition, detail=None):
        checks.append({"name": name, "passed": bool(condition), "detail": detail})

    for name, rows, key in [
        ("realms", realms, "realm_index"), ("items", items, "item_id"),
        ("nodes", nodes, "node_id"), ("shop", shop, "sku"),
        ("market", market, "item_id"), ("methods", methods, "method_id"),
        ("activities", activities, "activity_id"),
        ("encounters", encounters, "preset_id"),
    ]:
        check(f"unique_{name}", len(rows) == len({r[key] for r in rows}))

    item_map = {r["item_id"]: r for r in items}
    targets = [int(r["cumulative_standard_minutes"]) for r in realms]
    check("34_realms", len(realms) == 34)
    check("continuous_zero_based_indices", [int(r["realm_index"]) for r in realms] == list(range(34)))
    check("strictly_increasing_targets", targets[0] == 0 and all(b > a for a, b in zip(targets, targets[1:])))
    derived = []
    for i, row in enumerate(realms):
        viewer_cost = (targets[i + 1] - targets[i]) * config["standard_minute_qi"] if i + 1 < len(realms) else 0
        anchor_cost = int((Decimal(viewer_cost) * Decimal(str(config["anchor_threshold_ratio"]))).to_integral_value(rounding=ROUND_CEILING))
        materials = int(row["anchor_material_next"])
        derived.append({
            "realm_index": i, "name": row["name"],
            "cumulative_qi": targets[i] * config["standard_minute_qi"],
            "viewer_qi_to_next": viewer_cost, "anchor_qi_to_next": anchor_cost,
            "anchor_material_to_next": materials,
            "anchor_stone_to_next": materials * config["anchor_stone_per_break_material"],
            "support_slots": int(row["support_slots"]),
        })
        check(f"realm_{i}_slots", 2 <= int(row["support_slots"]) <= 6)
        check(f"realm_{i}_nonnegative_costs", min(viewer_cost, anchor_cost, materials) >= 0)
    check("viewer_costs_telescope", sum(r["viewer_qi_to_next"] for r in derived) == 4760 * 60)
    check("anchor_costs_telescope", sum(r["anchor_qi_to_next"] for r in derived) == 4760 * 30)
    check("max_realm_no_next_cost", all(derived[-1][k] == 0 for k in ("viewer_qi_to_next", "anchor_qi_to_next", "anchor_material_to_next")))

    reward_totals = defaultdict(int)
    check("unique_stage_rewards", len(rewards) == len({(r["stage"], r["item_id"]) for r in rewards}))
    for row in rewards:
        check(f"reward_{row['stage']}_{row['item_id']}", row["item_id"] in item_map and int(row["count"]) > 0 and int(row["stage"]) in (1, 2, 3))
        reward_totals[row["item_id"]] += int(row["count"])
    expected = {"CUR_STONE": 80, "MAT_BREAK": 12, "MAT_HERB_B": 5, "MAT_HERB_T": 1, "MAT_IRON_B": 3, "MAT_IRON_T": 1, "MAT_PAGE": 2}
    check("world_reward_budget", dict(reward_totals) == expected, dict(reward_totals))

    node_examples = []
    for row in nodes:
        qbonus = Decimal(row["qi_additive"])
        check(f"node_{row['node_id']}_qi_cap", Decimal(0) <= qbonus <= Decimal(str(config["node_qi_bonus_cap"])))
        check(f"node_{row['node_id']}_lease", int(row["lease_max_seconds"]) == 1800)
        item_id = row["item_id"]
        if item_id:
            check(f"node_{row['node_id']}_item", item_id in item_map and item_map[item_id]["tradeable"] == "false")
        interval = int(row["interval_seconds"])
        max_count = 1800 // interval * int(row["item_count"])
        for elapsed in (0, 59, 599, 600, 1200, 1799, 1800, 3600):
            count = min(max(elapsed, 0), 1800) // interval * int(row["item_count"])
            check(f"node_{row['node_id']}_elapsed_{elapsed}", 0 <= count <= max_count)
        node_examples.append({"node_id": row["node_id"], "max_item_count_30m": max_count, "base_extra_qi_30m": float(qbonus * 1800)})
    check("one_active_lease", config["active_lease_limit"] == 1)
    check("race_duration", config["race_prepare_seconds"] + config["race_running_seconds"] + config["race_settle_seconds"] == 600)
    check("race_score_1000", sum(config[k] for k in ("race_score_common", "race_score_anchor", "race_score_audience")) == 1000)
    check("race_real_hosts", 2 <= config["race_min_hosts"] <= config["race_max_hosts"] <= 4)
    for representative_count in (1, 2, 10):
        for responses in range(2 * representative_count + 1):
            points = 200 * responses // (2 * representative_count)
            check(f"audience_score_{representative_count}_{responses}", 0 <= points <= 200)

    def rank_leases(scores):
        qualifying = sorted({s for s in scores if s >= config["race_qualifying_score"]}, reverse=True)
        return [config["lease_rank_minutes"][min(qualifying.index(s), 2)] if s in qualifying else 0 for s in scores]

    check("tie_all_equal", rank_leases([1000, 1000, 1000, 1000]) == [30, 30, 30, 30])
    check("tie_dense_groups", rank_leases([900, 900, 800, 650]) == [30, 30, 20, 10])
    check("below_threshold_no_lease", rank_leases([1000, 599]) == [30, 0])

    for row in shop:
        sku = row["sku"]
        check(f"shop_{sku}_output", row["output_item"] in item_map and int(row["output_count"]) > 0)
        check(f"shop_{sku}_nonnegative", int(row["stone_cost"]) >= 0 and int(row["daily_limit"]) > 0)
        for n in (1, 2):
            ref, amount = row[f"extra_item_{n}"], int(row[f"extra_count_{n}"])
            check(f"shop_{sku}_extra_{n}", (ref in item_map and amount > 0) if ref else amount == 0)
        check(f"shop_{sku}_owner", item_map[row["output_item"]]["owner_type"] == row["owner_type"])
    sku_map = {r["sku"]: r for r in shop}
    baseline_30m = {k: v * 3 for k, v in expected.items()}
    planned_spend = derived[0]["anchor_stone_to_next"] + sum(int(sku_map[s]["stone_cost"]) for s in ("SKU_HEAL", "SKU_SHIELD", "SKU_STUDY"))
    check("30m_stone_example", baseline_30m["CUR_STONE"] - planned_spend == 40)
    check("30m_material_example", baseline_30m["MAT_BREAK"] >= derived[0]["anchor_material_to_next"] and baseline_30m["MAT_HERB_B"] >= 2 and baseline_30m["MAT_IRON_B"] >= 2 and baseline_30m["MAT_PAGE"] >= 2)

    white = {r["item_id"] for r in market}
    check("trade_only_two_raw_materials", white == {"MAT_HERB_T", "MAT_IRON_T"})
    check("trade_template_matches_whitelist", {r["item_id"] for r in items if r["tradeable"] == "true"} == white)
    for row in market:
        item = item_map[row["item_id"]]
        check(f"trade_{row['item_id']}_origin", item["paid_origin"] == "false" and item["binding"] == "unbound")
        check(f"trade_{row['item_id']}_price", 0 < int(row["min_unit_price"]) <= int(row["max_unit_price"]) and int(row["npc_recycle_unit_price"]) <= int(row["min_unit_price"]))
    for source, target in config["material_binding_conversions"].items():
        check(f"binding_conversion_{source}", source in white and target in item_map and item_map[target]["binding"] == "bound" and item_map[target]["tradeable"] == "false")
    tax = int((Decimal(80) * Decimal(str(config["market_tax_rate"]))).to_integral_value(rounding=ROUND_CEILING))
    seller_after = 10 - config["market_listing_fee"] + 80 - tax
    buyer_after = 100 - 80
    check("market_example_balances", (tax, seller_after, buyer_after) == (4, 85, 20))
    check("market_currency_conservation", 110 - seller_after - buyer_after == tax + config["market_listing_fee"])

    release = config["profiles"]["douyin_release_candidate"]
    for feature, enabled in release.items():
        check(f"release_{feature}_closed", enabled is False)
    check("prototype_not_production", config["profiles"]["prototype_full"]["production_allowed"] is False)
    check("legacy_gift_preserved", config["gift_profile_default"] == "legacy_preserve")
    check("seven_method_routes", len(methods) == 7)
    check("three_initial_methods", sum(r["initial_implementation"] == "true" for r in methods) == 3)
    combat_examples = []
    attack = config["baseline_sword_attack"]
    for row in encounters:
        effective_output = (int(row["reference_viewers"]) + 1) * attack * attack / (attack + int(row["target_defense"])) * config["modeled_effective_hit_rate"] / config["baseline_sword_interval_seconds"]
        boss_seconds = int(row["boss_health"]) / effective_output
        stage1_seconds = int(row["stage1_waves"]) * int(row["stage1_targets_per_wave"]) * int(row["normal_health"]) / effective_output
        obstacle_seconds = int(row["obstacle_count"]) * int(row["obstacle_health"]) / effective_output
        check(f"encounter_{row['preset_id']}_boss_90sec", abs(boss_seconds - 90) < 1e-8)
        check(f"encounter_{row['preset_id']}_stages_fit", stage1_seconds <= 180 and obstacle_seconds <= 180 and boss_seconds <= 120)
        combat_examples.append({"preset_id": row["preset_id"], "modeled_effective_output": effective_output, "boss_seconds": boss_seconds, "stage1_damage_seconds": stage1_seconds, "obstacle_damage_seconds": obstacle_seconds})
    for row in activities:
        check(f"activity_{row['activity_id']}_dates", row["enabled"] == "false" or row["cycle"] != "limited" or bool(row["start_at"] and row["end_at"]))

    # Scenario arithmetic: no production transfer, no claim about retention or conversion.
    scenarios = []
    for name, minutes, node_minutes, offline, personal_multiplier in [
        ("free_30m_offline10", 30, 0, True, 1),
        ("free_30m_with_spring", 30, 30, True, 1),
        ("free_90m_offline10", 90, 0, True, 1),
        ("30m_without_offline", 30, 0, False, 1),
        ("30m_candidate_potion_entire_session", 30, 0, True, 3.25),
    ]:
        # Potion scenario assumes enough paid units for 30m; no price/revenue inference.
        online = Decimal(minutes * 60) * Decimal(str(personal_multiplier))
        online += Decimal(node_minutes * 60) * Decimal("0.20") * Decimal(str(personal_multiplier))
        event_count = min(minutes // 10, config["event_extra_daily_limit"])
        extra = event_count * config["event_extra_qi"] + config["sign_qi"]
        offline_qi = Decimal(str(config["offline_rate"])) * config["offline_cap_seconds"] if offline else Decimal(0)
        daily = online + extra + offline_qi
        steady_28 = daily * 28
        fresh_28 = steady_28 - offline_qi
        reached = max(i for i, r in enumerate(derived) if r["cumulative_qi"] <= fresh_28)
        scenarios.append({"scenario": name, "steady_daily_qi": float(daily), "steady_daily_standard_minutes": float(daily / 60), "steady_28day_qi": float(steady_28), "fresh_account_28day_qi": float(fresh_28), "fresh_account_realm": realms[reached]["name"], "note": "新账号第1天无离线收益；每天完整参与和签到；药水场景仅是假设全时覆盖"})
    check("baseline_steady_daily_qi", scenarios[0]["steady_daily_qi"] == 5160)
    check("spring_steady_daily_qi", scenarios[1]["steady_daily_qi"] == 5520)
    check("active_steady_daily_qi", scenarios[2]["steady_daily_qi"] == 8940)
    check("full_free_bonus_gift_cap", abs((1 + 0.3 + 0.15 + 0.1) * 3.25 - 5.0375) < 1e-9)

    source_hashes = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(TABLES.iterdir()) if p.suffix in (".csv", ".json") and p.name != "realm_costs.csv"}
    with (TABLES / "realm_costs.csv").open("w", encoding="utf-8-sig", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(derived[0]))
        writer.writeheader()
        writer.writerows(derived)
    report = {
        "design_version": config["design_version"],
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "passed": all(c["passed"] for c in checks),
        "check_count": len(checks),
        "failed": [c for c in checks if not c["passed"]],
        "verification_scope": "策划配置引用、边界和算术；未测试游戏实现、网络并发、平台、设备、支付或商业效果",
        "source_hashes": source_hashes,
        "world_reward_totals": dict(reward_totals),
        "baseline_30m_reward_totals": baseline_30m,
        "early_budget_stone_remaining": baseline_30m["CUR_STONE"] - planned_spend,
        "market_example": {"buyer_after": buyer_after, "seller_after": seller_after, "tax": tax, "listing_fee": config["market_listing_fee"]},
        "node_examples": node_examples,
        "combat_examples": combat_examples,
        "scenarios": scenarios,
        "checks": checks,
    }
    (ROOT / "validation-report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"passed": report["passed"], "checks": len(checks), "failed": report["failed"], "scenarios": scenarios}, ensure_ascii=False, indent=2))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
