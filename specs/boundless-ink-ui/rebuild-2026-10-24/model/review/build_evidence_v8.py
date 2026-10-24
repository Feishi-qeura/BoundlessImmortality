"""Update evidence tables from captured reports, without running UE or tests."""
from pathlib import Path
from datetime import datetime,timezone
import json,csv,hashlib,copy
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[4];BASE=HERE.parents[1];P=BASE.relative_to(ROOT).as_posix();SR='Source/BoundlessImmortality/Frontend/UIRebuild20261024/'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(n,x):(HERE/n).write_text(json.dumps(x,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def sha(data):return hashlib.sha256(data).hexdigest()
inputs=read(HERE/'selected-inputs-v8.json');idx={r['path']:r for r in inputs['sources']}
def frozen(rel):return HERE/idx[rel]['snapshot']
def source(rel):return frozen(rel).read_text(encoding='utf-8-sig')
# Carry existing receipt evidence, then replace the seven formerly missing rows.
assets=copy.deepcopy(read(HERE/'runtime-component-application-v7.json')['assets'])
receipt=read(frozen(P+'/logic-integration/remaining-component-import-result.json'))
new_by_hash={r['source_sha256'].lower():r for r in receipt['assets']}
fields={'wuling-sect-insignia-v3':(SR+'InkRebuildFeatureRank.cpp','FInkWulingOverviewData.Insignia -> Emblem brush'),
 'daily-activity-scroll-v1':(SR+'Activities/RebuildActivityWidgets.cpp','CategoryBadge cycle=daily -> UImage.SetBrushFromTexture'),
 'limited-activity-calendar-v1':(SR+'Activities/RebuildActivityWidgets.cpp','CategoryBadge cycle=limited -> UImage.SetBrushFromTexture'),
 'permanent-activity-lotus-v1':(SR+'Activities/RebuildActivityWidgets.cpp','CategoryBadge cycle=permanent -> UImage.SetBrushFromTexture'),
 'method-loadout-glyph-atlas-v1.png':(SR+'InkRebuildFeaturePlayer.cpp','FPageAtlases.MethodsGlyphs -> BuildMethodsPage'),
 'equipment-fabao-glyph-atlas-v1.png':(SR+'InkRebuildFeaturePlayer.cpp','FPageAtlases.EquipmentGlyphs -> BuildEquipmentPage'),
 'appearance-preview-ornament-atlas-v1.png':(SR+'InkRebuildFeaturePlayer.cpp','FPageAtlases.AppearanceOrnaments -> BuildAppearancePage')}
for a in assets:
    item=new_by_hash.get(a['source_sha256'].lower())
    if item:
        a.update(canonical_object_path=item['object'],classification='imported-source-referenced-runtime-pending',runtime_readback='pending: receipt/AssetRegistry readback is not a live UImage brush readback; PIE05 predates import')
        rel,field=fields[a['id']];a['production_composition_field']=field;a['production_reference_source']=rel;a['production_reference_sha256']=idx[rel]['sha256'];a['canonical_reference_present_in_source']=item['object'] in source(rel)
        disk=ROOT/('Content/'+item['object'].split('.')[0].removeprefix('/Game/')+'.uasset');a['uasset_exists']=disk.is_file();a['uasset_sha256']=sha(disk.read_bytes()) if disk.is_file() else ''
        a['import_receipt_uasset_hash_matches']=a['uasset_sha256']==item['uasset_sha256'];a['import_receipt_source_hash_matches']=sha((ROOT/a['source']).read_bytes())==item['source_sha256']
    elif a['id']=='shared-character-attribute-atlas':
        a.update(classification='existing-import-reused-in-source-runtime-pending',production_composition_field='FPageAtlases.CharacterAttributeGlyphs -> BuildAppearancePage',production_reference_source=SR+'InkRebuildFeaturePlayer.cpp',canonical_reference_present_in_source=a['canonical_object_path'] in source(SR+'InkRebuildFeaturePlayer.cpp'),runtime_readback='core Character historical binding passed; Feature Appearance reuse readback pending')
    else:
        # Recheck only six exact previously imported files, not the 596 directory tree.
        disk=ROOT/('Content/'+a['canonical_object_path'].split('.')[0].removeprefix('/Game/')+'.uasset');a['uasset_exists']=disk.is_file();a['current_uasset_hash_matches_v7']=disk.is_file() and sha(disk.read_bytes())==a['uasset_sha256'];a['runtime_readback']+='; historical attempt scope only, not build19/current-source proof'
assetdoc=dict(captured_utc=inputs['captured_utc'],unique_candidates=13,imported_candidates=13,missing_imports=0,newly_imported_source_referenced_runtime_pending=7,shared_existing_reference_runtime_pending=1,prior_runtime_resource_observations=6,approved=False,import_registry_receipt_passed=receipt.get('asset_registry_readback_passed'),directory596='reported by integration; no broad recount performed; 582+4+2+WBP+7=596 is not complete component runtime acceptance',assets=assets)
write('runtime-component-application-v8.json',assetdoc)
with (HERE/'runtime-component-application-v8.csv').open('w',encoding='utf-8-sig',newline='') as f:
    cols=['owner','id','production_composition_field','canonical_object_path','uasset_exists','classification','runtime_readback','production_reference_source','production_reference_sha256','source_sha256','uasset_sha256'];w=csv.DictWriter(f,fieldnames=cols,extrasaction='ignore');w.writeheader();w.writerows(assets)
pie=read(frozen(P+'/logic-integration/default-root-pie-05/report.json'));checks=pie['checks'];observed=sorted({c['actual'] for c in checks if c['name'].startswith('actual page ') and c['passed']})
runtime=dict(attempt='PIE05',finished=pie['finished'],passed=pie['passed'],checks=len(checks),captures=len(pie['captures']),source_epoch_sha256=pie['source_epoch_sha256'],started_at=pie['started_at'],finished_at=pie.get('finished_at'),current_step=pie['current_step'],failed_checks=[c for c in checks if not c['passed']],observed_routes=observed,observed_count=len(observed),source_epoch_predates_seven_component_import=True,strict_geometry_capture_checks_passed=sum(c['name'].startswith('finite metrics or independently proven culling ') and c['passed'] for c in checks),geometry_scope='only captured PIE05 subsets at its own epoch; not current build19, not all44',character_scroll_entry_passed_routes=[r for r in ['EquipmentFabao','CultivationMethods','BreakthroughPrepare','CharacterAppearance'] if any(c['name']=='actual page '+r and c['passed'] for c in checks)],failed04_preserved=True,new_component_brush_readbacks_pending=7,service_business_or_Busy_success=False)
write('runtime-evidence-v8.json',runtime)
findings=[dict(id='V7-ASSET-01-import',state='closed-static',detail='Seven exact PNG/uasset hashes match new import receipt; 13/13 unique candidates now imported; no missing import.'),dict(id='V7-ASSET-01-reference',state='closed-source-runtime-pending',detail='Rank Insignia, Activities CategoryBadge canonical textures, Player-c three atlases and existing shared CharacterAttribute now assigned/loaded; seven new live brush bindings and shared Feature Appearance reuse pending.'),dict(id='V7-CONFIRM-01',state='source-repaired-runtime-pending',source=SR+'InkRebuildFeaturePlayer.cpp',frozen_lines=[132,200,201,212,213,216],detail='Both request-confirmation and prepare/rework compare latest StageSubjectIdentity against displayed StageFingerprint; confirm captures fresh fingerprint; final submit also ConfirmedStageMatches. Native quote gate unchanged.'),dict(id='V7-CYCLE-01',state='source-repaired-runtime-pending',source=SR+'InkRebuildFeatureActivities.cpp',frozen_lines=[33],detail='DailyActivities=>daily, TimedActivities=>limited; filter Categories by explicit RouteCycle; consumer selects cycle and titles route; actual category binding/cycle readback pending.'),dict(id='ECONOMY-SELECTION',state='bounded-source-chain-qualified-runtime-pending',source=SR+'Activities/RebuildEconomyWidgets.cpp',frozen_lines=[172,188,199,200,375,388,418,438,659],detail='Shop/listing/order local selection broadcasts to host; host persists ID and clears semantic key without synchronous remount; refreshed VMs return same ID. Confirmed SKU/version/price and listing ID/version/price are rechecked and invalidated on disappearance/change/not-allowed. Actual refresh/service regression not executed.'),dict(id='V7-ENTRY-01',state='specific-character-entry-passed-PIE05-overall-still-failed',detail='Real scroll + click before capture for all four Character feature routes; failed04 retained. PIE05 later fails typed back OfflineRewards.'),dict(id='V8-ENTRY-01',state='observed-open',priority='P1',source=P+'/logic-integration/default-root-pie-05/report.json',detail='PIE05 479 checks/31 captures, finished=false passed=false, typed back OfflineRewards real Slate labeled 返回 returns None. Fix actual return path/scroll/layout according to live evidence; record a new attempt, preserve04/05.'),dict(id='GEOMETRY-SERVICE-VISUAL',state='scope-gates-open',detail='No all44/current-epoch strict geometry proof; Native Busy fixture does not prove real service Busy, ServiceDisabled display does not prove mutations; no user visual acceptance.')]
write('resolved-open-matrix-v8.json',dict(findings=findings,previous='resolved-open-matrix-v7.json'))
with (HERE/'resolved-open-matrix-v8.csv').open('w',encoding='utf-8-sig',newline='') as f:
    cols=['id','state','detail','source'];w=csv.DictWriter(f,fieldnames=cols,extrasaction='ignore');w.writeheader();w.writerows(findings)
table=copy.deepcopy(read(HERE/'stage-to-runtime-v7.json')['screens'])
for r in table:
    route=r['canonical_route'];r['default_route_observed']=route in observed;r['runtime_attempt']='PIE05 failed overall; earlier epoch scoped observations only';r['independent_geometry_capture_passed']=route in observed;r['geometry_acceptance']='captured-subset-at-PIE05-epoch-only';r['runtime_composition_asserted']=route in observed and r['production_host']=='UInkRebuildFeatureHost';r['blocking_evidence']='PIE05 typed OfflineRewards return None' if route=='OfflineRewards' else 'latest source/business/component readback pending' if route in observed else 'not reached in PIE05'
    if r['source'] in idx:r['source_sha256']=idx[r['source']]['sha256']
    r['source_hash_basis']='v8 bounded selected capture' if r['source'] in idx else 'v7 retained hash; not re-reviewed in v8'
    r['compile_proof']='build19 exit0 88 Source/212 ledger; Source hash/capture and attempt epoch remain separate'
    if route in ['DailyActivities','TimedActivities']:r['required_context']='RouteCycle='+('daily' if route=='DailyActivities' else 'limited')+'; authoritative Categories filtered by current producer'
write('stage-to-runtime-v8.json',dict(screen_count=44,feature_routes=37,screens=table))
with (HERE/'stage-to-runtime-v8.csv').open('w',encoding='utf-8-sig',newline='') as f:
    w=csv.DictWriter(f,fieldnames=list(table[0]));w.writeheader();w.writerows(table)
drift=[]
for r in inputs['sources']:
    current=sha((ROOT/r['path']).read_bytes())
    if current!=r['sha256']:drift.append(dict(path=r['path'],captured_sha256=r['sha256'],current_sha256=current))
write('source-drift-after-v8.json',dict(checked_utc=datetime.now(timezone.utc).isoformat(),changed=drift))
status=dict(team_id='logic-core',phase='bounded-v8-imports-and-source-repairs-runtime-pending',latest_report='bounded-v8-incremental-review.md',latest_readiness='readiness-v8.json',latest_matrix='resolved-open-matrix-v8.json',latest_screen_map='stage-to-runtime-v8.csv',latest_runtime_evidence='runtime-evidence-v8.json',latest_component_application='runtime-component-application-v8.md',production_source_mount_counts=dict(core=7,feature_source_wired_lazy=37),unique_component_candidates=13,imported_candidates=13,missing_imports=0,missing_source_references_in_selected_scope=0,new_component_brush_runtime_pending=7,shared_existing_feature_binding_pending=1,actual_default_route_observations=len(observed),latest_attempt_passed=False,all44_runtime_verified=False,real_service_Busy_flow_validated=False,service_business_acceptance=False,visual_approval=False,engine_started=False,build_run=False,tests_run=False,production_edits_by_this_worker=False,handoff_ready=True)
write('status.json',status);write('readiness-v8.json',dict(**status,captured_utc=inputs['captured_utc'],source_drift=drift,build19=read(frozen(P+'/logic-integration/build-19-result.json')),selected_inputs='selected-inputs-v8.json'))
print(json.dumps(dict(imports=13,new_seven_hashes_match=all(a.get('import_receipt_uasset_hash_matches') and a.get('import_receipt_source_hash_matches') and a.get('canonical_reference_present_in_source') for a in assets if a['id'] in fields),historical_six_unchanged=all(a.get('current_uasset_hash_matches_v7',True) for a in assets),routes=len(observed),drift=len(drift))))
