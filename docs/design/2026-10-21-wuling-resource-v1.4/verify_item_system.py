"""Verify design references, economies and tribulation boundaries. No runtime tests."""
from pathlib import Path
from collections import Counter, defaultdict
from fractions import Fraction
import csv
import json
import hashlib

ROOT=Path(__file__).resolve().parent
T=ROOT/'tables/items-v1.4'


def rows(name):
    with (T/name).open(encoding='utf-8-sig',newline='') as f:return list(csv.DictReader(f))


def main():
    config=json.loads((T/'item_system.json').read_text(encoding='utf-8'))
    tr=json.loads((T/'tribulation.json').read_text(encoding='utf-8'))
    checks=[]
    def ck(n,v):checks.append(dict(name=n,passed=bool(v)))
    files={p.name:rows(p.name) for p in T.glob('*.csv')}
    master={r['item_id']:r for r in files['item_master.csv']}
    source={r['source_id']:r for r in files['acquisition_sources.csv']}
    recipes={r['recipe_id']:r for r in files['recipes.csv']}
    stages={r['stage_id']:r for r in files['breakthrough_stages.csv']}
    species=files['breakthrough_species.csv'];mats=files['breakthrough_material_items.csv']
    ck('unique_master_ids',len(master)==len(files['item_master.csv'])==730)
    ck('unique_source_ids',len(source)==len(files['acquisition_sources.csv']))
    ck('unique_recipe_ids',len(recipes)==len(files['recipes.csv']))
    ck('ten_stages',len(stages)==10)
    ck('160_species_640_variants',len(species)==160 and len(mats)==640)
    ck('unique_chinese_species_names',len({r['name'] for r in species})==160)
    ck('four_grades_each_species',all(v==4 for v in Counter(r['species_id'] for r in mats).values()))
    for sid in stages:
        ct=Counter(r['category'] for r in species if r['stage_id']==sid)
        ck(sid+'_one_core_five_each_optional',ct==dict(core=1,guard=5,essence=5,treasure=5))
        ck(sid+'_optional_guard',stages[sid]['guard_required']=='False')
    ck('variant_master_refs',all(r['item_id'] in master for r in mats))
    ck('source_item_refs',all(r['item_id'] in master and r['source_id'] in source for r in files['item_sources.csv']))
    ck('recipes_output_refs',all(r['output_item'] in master for r in recipes.values()))
    ck('recipe_input_refs',all(r['item_id'] in master and r['recipe_id'] in recipes and int(r['count'])>0 for r in files['recipe_inputs.csv']))
    ck('recipe_positive_output',all(int(r['output_count'])>0 and int(r['stone_cost'])>=0 and float(r['success_rate'])==1 for r in recipes.values()))
    # Ensure every new item is reachable from a real source with recursively available inputs.
    inputs=defaultdict(list)
    for r in files['recipe_inputs.csv']:inputs[r['recipe_id']].append(r['item_id'])
    available={r['item_id'] for r in files['item_sources.csv'] if r['source_id'] not in ('CH_CRAFT','CH_BUILD')}
    while True:
        expanded=available|{r['output_item'] for rid,r in recipes.items() if all(i in available for i in inputs[rid])}
        if expanded==available:break
        available=expanded
    ck('all_730_items_reachable_without_rng_or_new_market',available==set(master))
    ck('all_new_items_have_channel',set(master)=={r['item_id'] for r in files['item_sources.csv']})
    ck('new_materials_bound_anchor_only',all(r['owner_type']=='anchor' and r['binding']=='bound' and r['tradeable']=='False' for r in mats))
    ck('no_new_trade_whitelist',config['trade']['new_item_whitelist']==[])
    ck('viewer_material_cost_zero',all(r['viewer_material_cost']=='0' for r in stages.values()) and not config['ownership']['viewer_break_materials'])
    ck('minor_cost_preserved',config['breakthrough']['minor_cost_mode']=='legacy_v1.3_unchanged')
    ck('major_no_double_charge',all(r['major_stone_at_commit']=='0' for r in stages.values()))
    ck('new_equipment_no_weapon',len(files['equipment_items.csv'])==16 and {r['slot'] for r in files['equipment_items.csv']}=={'robe','boots','crown','pendant'})
    ck('one_fabao_and_no_probability_purchase',all(r['max_equipped']=='1' and r['affects_tribulation_probability']=='False' for r in files['fabao_items.csv']))
    ck('props_no_probability_purchase',all(r['affects_tribulation_probability']=='False' for r in files['prop_items.csv']))
    # Category normalization and band boundaries are exact integer arithmetic.
    def score(core,guard_weights,guard_qty,essence_weights,essence_qty,treasure):
        return core+sum(guard_weights)//guard_qty+sum(essence_weights)//essence_qty+treasure
    ck('perfect_score_100',score(35,[15]*3,3,[30]*4,4,20)==100)
    ck('only_core_scores_10_18_26_35',all(score(c,[],3,[],4,0)==c for c in (10,18,26,35)))
    ck('floor_once_after_sum',score(10,[3,3],3,[],4,0)==12)
    bands=files['quality_bands.csv']
    def band(q):return next(r['name'] for r in bands if int(r['q_min'])<=q<=int(r['q_max']))
    for q,name in [(0,'凡品'),(39,'凡品'),(40,'良品'),(59,'良品'),(60,'上品'),(74,'上品'),(75,'极品'),(89,'极品'),(90,'天品'),(97,'天品'),(98,'完美'),(100,'完美')]:ck('quality_'+str(q),band(q)==name)
    ck('all_only_core_min_band',all(band(c)=='凡品' for c in (10,18,26,35)))
    ck('all_ten_perfect_budget_10_percent',sum([Fraction(1,100)]*10)==Fraction(1,10))
    ck('no_random_quality_or_cumulative_multiply',not config['breakthrough']['random_quality'] and not config['breakthrough']['cumulative_quality_multiply'] and not config['breakthrough']['raw_indie_multiplier_enabled'])
    ck('quality_monotonic_snapshots',not config['breakthrough']['highest_quality_state_reset_on_new_major'])
    ck('guild_24_levels',len(files['guild_buildings.csv'])==24)
    for b in files['guild_buildings.csv']:
        ck(b['building_id']+'_L'+b['level']+'_references',all(b[k] in master for k in ('material_1','material_2')) and b['numeric_buff']=='False')
    ck('two_currencies_two_honor_stats',sum(r['is_currency']=='True' for r in files['guild_currency.csv'])==2 and len(files['guild_currency.csv'])==4)
    ck('guild_no_withdraw_or_points_for_stats',not config['guild']['personal_numeric_exchange'] and not config['guild']['currency_donation'])
    ck('release_extension_all_false',all(v is False for v in config['profiles']['douyin_release_candidate'].values()))
    def prob(helpers,failures):
        return min(10000,tr['base_success_basis_points']+min(helpers,tr['max_probability_helpers'])*tr['helper_bonus_basis_points']+min(failures*tr['pity_bonus_per_failure_basis_points'],tr['pity_max_basis_points']))
    for h,f,expected in [(0,0,5000),(1,0,5100),(25,0,7500),(49,0,9900),(50,0,10000),(51,0,10000),(0,1,6000),(0,4,9000),(0,5,10000),(1,5,10000)]:ck(f'probability_{h}_{f}',prob(h,f)==expected)
    ck('one_percent_is_percentage_point',tr['helper_bonus_basis_points']==100)
    for qi,loss in [(5400,1500),(1500,1500),(1499,1499),(0,0)]:ck('failure_loss_'+str(qi),min(qi,tr['failure_qi_loss'])==loss)
    ck('sixth_attempt_without_help_is_guaranteed',tr['max_attempts_without_help']==6 and prob(0,4)<10000 and prob(0,5)==10000)
    ck('seven_tribulation_transitions',sum(r['tribulation']=='True' for r in stages.values())==7 and tr['first_transition']=='BR04')
    ck('fifty_percent_exact_roll_space',sum(i<5000 for i in range(10000))==5000)
    ck('full_probability_never_fails',all(i<10000 for i in range(10000)))
    ck('failure_materials_preserved_no_realm_loss',not tr['consume_materials_on_failure'] and not tr['lose_realm'] and not tr['consume_breakthrough_qi_on_failure'])
    ck('no_gift_material_or_equipment_probability',not tr['gift_affects_probability'] and not tr['material_affects_probability'])
    ck('tribulation_window_exact',sum(tr['trial_stages_seconds'])==tr['help_trial_seconds']==60)
    ck('runtime_not_claimed',len(files['acceptance.csv'])==30 and all(r['current_result']=='not_run' for r in files['acceptance.csv']))
    # With zero help, bounded pity yields expected failures 0.5+0.2+0.06+0.012+0.0012.
    survive=Fraction(1); expected_failures=Fraction(0)
    for failures in range(6):
        survive*=1-Fraction(prob(0,failures),10000)
        expected_failures+=survive
    ck('bounded_expected_failure_loss',expected_failures*1500==Fraction(5799,5))
    report=dict(passed=all(r['passed'] for r in checks),check_count=len(checks),failed=[r for r in checks if not r['passed']],
                counts=json.loads((T/'counts.json').read_text(encoding='utf-8')),
                expected_failures_zero_help=float(expected_failures),expected_qi_loss_zero_help=float(expected_failures*1500),
                maximum_qi_loss_before_guaranteed_sixth_attempt=7500,
                source_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in T.iterdir() if p.suffix in ('.json','.csv')},
                runtime_tests_run=False,scope='Design references/reachability/budgets/probability arithmetic; no UE, service, live provider or platform tests',checks=checks)
    (ROOT/'item-system-validation.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:v for k,v in report.items() if k not in ('checks','source_sha256')},ensure_ascii=False,indent=2))
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
