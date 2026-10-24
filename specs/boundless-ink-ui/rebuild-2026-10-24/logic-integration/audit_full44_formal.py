"""Persisted formal44coverage and strict geometry audit; no engine writes."""
from pathlib import Path
import json,csv,runpy,datetime,hashlib
H=Path(__file__).resolve().parent;D=H/'default-root-pie-14';R=json.loads((D/'report.json').read_text())
EXPECTED=['Main','HUD','Saves','Settings','Character','Inventory','Leaderboard','Wuling','SwordSpirit','GiftRules','CultivationMethods','RaceLobby','RaceLive','RaceSettlement','ResourceLease','EventSettlement','OfflineRewards','Shop','Market','TradeOrders','WorldCultivation','RegionDetail','ResourceContest','ResourceDetail','SignIn','MonthlySignIn','DailyFortune','Activities','DailyActivities','TimedActivities','DailyActivityDetail','TimedActivityDetail','LaunchEvent','LaunchRank','BreakthroughPrepare','Tribulation','ItemSources','GuildConstruction','EquipmentFabao','Extensions','WulingCraft','CharacterAppearance','ResourceRanking','ResourceRewards']
M=runpy.run_path(str(H/'geometry_metric_validation.py'),run_name='formal44_geometry_audit');coverage={p:{'portrait':[]} for p in EXPECTED};errors=[];nonfinite=[]
for png in R['captures']:
 p=Path(png).with_suffix('.json');s=json.loads(p.read_text());orientation='landscape' if s['width']>s['height'] else 'portrait'
 if s['page'] not in coverage:errors.append(dict(unexpected_page=s['page']));continue
 if orientation!='portrait' or s['width']*16!=s['height']*9:errors.append(dict(snapshot=str(p),wrong_target_aspect=[s['width'],s['height']]));continue
 coverage[s['page']][orientation].append(str(p.relative_to(H)))
 bad,failed=M['inspect'](s);nonfinite.append(dict(snapshot=str(p.relative_to(H)),retained_invalid_records=bad))
 if failed:errors.append(dict(snapshot=str(p),geometry_failures=failed))
 if s['runtime_class']!='/Game/newUI/Runtime/WBP_NewUIFrontend.WBP_NewUIFrontend_C':errors.append(dict(snapshot=str(p),wrong_runtime_class=s['runtime_class']))
for page,orientations in coverage.items():
 for orientation,proof in orientations.items():
  if not proof:errors.append(dict(missing_page=page,orientation=orientation))
negative=[c for c in R['checks'] if c['name'].startswith('deliberate nonfinite expected-painted metric rejected ')]
if len(negative)!=len(R['captures']) or not all(c['passed'] for c in negative):errors.append(dict(injection_rejection_mismatch=dict(checks=len(negative),captures=len(R['captures']))))
if not R.get('passed') or not R.get('finished'):errors.append(dict(formal_attempt_not_passed=True,current_step=R.get('current_step'),error=R.get('error')))
epoch=H/'source-epochs/source-epoch-build26.json'
if hashlib.sha256(epoch.read_bytes()).hexdigest()!=R['source_epoch_sha256']:errors.append(dict(source_epoch_mismatch=True))
rows=[dict(page=p,portrait_captures=len(v['portrait']),portrait_proof=';'.join(v['portrait'])) for p,v in coverage.items()]
with (H/'current44-portrait-coverage.csv').open('w',encoding='utf-8-sig',newline='') as out:
 w=csv.DictWriter(out,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
result=dict(checked_at=datetime.datetime.now().astimezone().isoformat(),source_build=26,source_epoch_sha256=R['source_epoch_sha256'],attempt=14,expected_pages=44,formal_captures=len(R['captures']),strict_geometry_rechecked=True,expected_bad_injections_rejected=len(negative),pre_layout_generations_retained=len(R.get('layout_generation_waits',[])),coverage=coverage,retained_nonfinite_records=nonfinite,errors=errors,passed=not errors,scope='All44actual default-root pages portrait only9:16, typed Back, real core/Journey/Character entries distinguished from programmatic37route probes. Metric audit never uses geometry_arranged to omit a bad expected-painted record. Visual approval remains pending.')
(H/'current44-formal-audit.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps(dict(passed=result['passed'],pages=44,captures=len(R['captures']),errors=errors),ensure_ascii=False))
