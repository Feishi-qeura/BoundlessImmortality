"""Add selected real runtime observations without promoting failed attempts."""
from pathlib import Path
import csv
import json

HERE=Path(__file__).resolve().parent
inputs=json.loads((HERE/'selected-inputs-v5.json').read_text(encoding='utf-8'))
source={r['path']:r for r in inputs['sources']}
late=json.loads((HERE/'late-source-v5.json').read_text(encoding='utf-8'))
source.update({r['path']:r for r in late['sources']})
evidence=json.loads((HERE/'runtime-evidence-v5.json').read_text(encoding='utf-8'))
old=json.loads((HERE/'stage-to-runtime-v4.json').read_text(encoding='utf-8'))
rows=old['screens'];prefix='specs/boundless-ink-ui/rebuild-2026-10-24/'
latest=evidence['default_pie_attempts'][1]
observed={r['page']:r for r in latest['page_observations'] if r['all_five_assertions_recorded_passed']}
pages=dict(main='Main',home='HUD',saves='Saves',settings='Settings',character='Character',inventory='Inventory')
texture_for=dict(main='T_MainActionOrnament_v1',home='T_HudRouteGlyphAtlas_v1',character='T_CharacterAttributeGlyphAtlas_v1',inventory='T_InventoryCategoryGlyphAtlas_v1')
for row in rows:
    id=row['screen_id'];header=row['stage_header']
    replace=(header.endswith('/RebuildActivityWidgets.h') and '/panel-activities/review-v3/' in header) or '/panel-activities-02/staged-source/' in header or '/panel-economy-03/staged-source/' in header
    if replace:
        name=Path(header).name;new=prefix+'panel-activities/review-v5/staged-source/'+name
        assert new in source
        row['stage_header']=new;row['stage_header_sha256']=source[new]['sha256']
        row['stage_review']='ready paired review-v5 replacement delta inspected; no stage compile/runtime proof'
        row['open_dependencies']='v4 finding group repaired in v5 source; consume paired request/base/host files and run actual native authority/runtime verification'
    row['stage_pending_interface_overlay']=''
    if id in ('craft','item-sources'):
        row['stage_pending_interface_overlay']=prefix+'panel-player/review-v5/contracts/player-d-adapter-namespace-overlay.yaml'
        row['open_dependencies']='V4-PD-01 corrected by ready namespace overlay; original contract frozen; host/runtime pending'
    if id=='breakthrough':
        row['stage_pending_interface_overlay']=prefix+'panel-player/review-v5/diff/player-e-breakthrough-confirmation.diff'
        row['open_dependencies']='V4-PE-01 ready StageSubjectRevision patch/host tuple contract; apply-check passed, unapplied and unintegrated'
    if id=='launch':
        row['embedded_stage_components']='URebuildLaunchSpecialOfferWidget (route-less), review-v5 paired replacement; purchase carries captured OfferId/SKU/version/price/confirmation'
    if row['production_evidence'] in source:
        row['production_evidence_sha256']=source[row['production_evidence']]['sha256']
    row['production_state_basis']='bounded late Source wiring review; six prior default routes observed in failed PIE02; new Rank host unverified; 37 other new hosts unobserved'
    row['production_class_source_present']=row['production_mount_state']=='new-composition-connected-in-source'
    row['production_class_source_header']=''
    row['production_class_source_header_sha256']=''
    if id in ('leaderboard','wuling','guild-build','sword-spirit','gift-rules'):
        row['production_class_source_present']=True
        name='InkRankWulingPages.h' if id in ('leaderboard','wuling') else 'InkRankWulingExtendedPages.h'
        row['production_class_source_header']='Source/BoundlessImmortality/Frontend/UIRebuild20261024/Rank/'+name
        row['production_class_source_header_sha256']=source[row['production_class_source_header']]['sha256']
    if id=='leaderboard':
        path='Source/BoundlessImmortality/Frontend/UIRebuild20261024/InkRebuildLeaderboard.cpp'
        row['production_mount_state']='new-composition-connected-in-source'
        row['production_evidence']=path;row['production_evidence_sha256']=source[path]['sha256']
        row['production_class']='UInkRebuildFrontendWidget hosting UInkRankBoardPage'
        row['open_dependencies']='late lazy mount Source only; compile/runtime board variants and scope assertions unverified; V5-AVATAR-01 backoff latch open'
    row['default_route_observed']=id in pages and pages[id] in observed
    row['actual_default_runtime_class']=observed.get(pages.get(id),{}).get('runtime_class','')
    row['runtime_assertion_evidence']=latest['evidence'] if row['default_route_observed'] else ''
    row['runtime_assertion_evidence_sha256']=latest['sha256'] if row['default_route_observed'] else ''
    row['runtime_observation_epoch_sha256']=latest['source_epoch_sha256'] if row['default_route_observed'] else ''
    row['runtime_observation_attempt_passed']=False
    row['current_shell_source_matches_observed_runtime_epoch']=False
    row['current_body_patch_matches_runtime_epoch']=id not in ('character','inventory') if row['default_route_observed'] else ''
    row['pending_current_body_source_sha256']='62b4dc029c8359e10f11948bd4946290d188fa0e1e18b9ade6c42c1473e55a70' if id in ('character','inventory') else ''
    row['observed_body_epoch_sha256']='aef71a153d255186b701813e099dac991c808b0e3066686df1edbe3078c983c4' if id in ('character','inventory') else ''
    row['compile_proof']='owner build07/08/09 results for integrated shell; later Character/Inventory body correction unpaired' if row['default_route_observed'] else ''
    row['saved_wbp_proof']='runtime-evidence-v5.json#saved_default_wbp (shared default shell; not a separate per-page WBP)' if row['default_route_observed'] else ''
    row['runtime_proof']='five actual page/root/version/WBP assertions observed; overall attempt failed; not full acceptance' if row['default_route_observed'] else ''
    row['imported_texture_disk_match']=any(texture_for.get(id,'<none>') in r['object'] and r['disk_matches_report'] for r in evidence['imported_textures']) if id in texture_for else ''
    row['native_regression_execution_proof']=''
    if id=='inventory':
        row['open_dependencies']='I3 source mapping repaired; N03/Busy execution records absent in selected artifacts; 40 visible nonfinite landscape metrics; later body-scroll repair unpaired'
    if id=='character':row['open_dependencies']='I3 source mapping repaired; live Busy transport scenario unverified; later body-scroll repair not paired with reviewed runtime epoch'
    if id=='home':row['open_dependencies']='default route/resource observed; explicit old HUD/GM suppression and final layout acceptance still absent'
    row['runtime_verified']=False;row['player_entry_verified']=False;row['player_entry_proof']=''
counts=dict(screens=44,stage_symbol_present=44,no_ready_stage=0,
    production_mount={'new-composition-connected-in-source':7,'new-root-placeholder':0,'no-new-production-component-observed':37},
    observed_prior_runtime_mount_checkpoint=old['counts']['production_mount'],saved_default_shell=True,actual_default_route_observations=6,
    complete_runtime_acceptance=0,player_entry_verified=0)
out=dict(captured_utc=inputs['captured_utc'],scope='separate staged declarations, Source mounting, saved assets, observed runtime assertions and failed acceptance',
    inherited_stage_inventory='stage-to-runtime-v4.json',late_source='late-source-v5.json',unchanged_stage_files_not_rehashed=True,runtime_evidence='runtime-evidence-v5.json',counts=counts,screens=rows)
(HERE/'stage-to-runtime-v5.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
with (HERE/'stage-to-runtime-v5.csv').open('w',newline='',encoding='utf-8-sig') as f:
    writer=csv.DictWriter(f,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
print(json.dumps(counts))
