"""Extend the stage inventory with all newly read slices; keep prior snapshots."""
from pathlib import Path
import csv
import json

HERE=Path(__file__).resolve().parent
manifest=json.loads((HERE/'source-delta-v4.json').read_text(encoding='utf-8'))
source={r['path']:r for r in manifest['sources']}
previous=json.loads((HERE/'stage-to-runtime-v3.json').read_text(encoding='utf-8'))
rows=previous['screens']
prefix='specs/boundless-ink-ui/rebuild-2026-10-24/'
updates={}
def add(id,file,symbol,vm,seam,kind,dependency):
    updates[id]=dict(file=prefix+file,symbol=symbol,vm=vm,seam=seam,kind=kind,dependency=dependency)
a='panel-activities-02/staged-source/RebuildActivitySecondaryWidgets.h'
add('launch-rank',a,'URebuildLaunchRankWidget','FRebuildLaunchRankViewModel','OnActionRequested(back); service ordering and known-field flags','UUserWidget class','V4-API-01; production host/build/runtime pending')
add('offline',a,'URebuildOfflineRewardsWidget','FRebuildOfflineRewardsViewModel','OnActionRequested(ack-offline); no invented grant arguments','UUserWidget class','V4-API-01/V4-ACT-01; production host pending')
e='panel-economy-03/staged-source/RebuildEconomyWidgets.h'
for id,symbol,vm,seam in [
 ('shop','URebuildShopWidget','FRebuildShopViewModel','query-shop(offset, OwnedOnly); purchase(SKU, version, price, confirmed)'),
 ('market','URebuildConsignmentWidget','FRebuildConsignmentViewModel','query-market; create-listing(item ID, count, original draft text, price); buy-listing(listing ID/version)'),
 ('orders','URebuildOrdersWidget','FRebuildOrdersViewModel','cancel-listing(exact listing ID/version, confirmed)'),
]:
    add(id,e,symbol,vm,'OnActionRequested: '+seam,'UUserWidget class','V4-API-01/V4-ECO-01; fresh host/native checks pending')
d='panel-player/player-d/staged-source/Frontend/UIRebuild20261024/Player/InkPlayerCraftSourcesWidgets.h'
add('craft',d,'BuildCraftPage','InkPlayerCraftSourcesWidgets::FRecipeVm + FInventoryVm','FActionRequested: query-recipes; craft(confirmed RecipeId); ui.* query/selection/confirmation','pure native composer','V4-PD-01 documented namespace; production host pending')
add('item-sources',d,'BuildItemSourcesPage','InkPlayerCraftSourcesWidgets::FItemSourceVm','FActionRequested: query-sources; choose-focus(exact source/item ID, method gate)','pure native composer','V4-PD-01 documented namespace; production host pending')
p='panel-player/player-e/staged-source/Frontend/UIRebuild20261024/Player/InkPlayerBreakthroughTribulationWidgets.h'
add('breakthrough',p,'BuildBreakthroughPage','InkPlayerBreakthroughTribulationWidgets::FBreakthroughVm + FInventoryVm','FActionRequested: quote-breakthrough; prepare-breakthrough/rework-quality(selection, known-count flags, revision, mode)','pure native composer','V4-PE-01 stage confirmation subject; production host pending')
add('tribulation',p,'BuildTribulationPage','InkPlayerBreakthroughTribulationWidgets::FTribulationVm','FActionRequested: cancel-breakthrough(confirmed actual attempt); reconcile-breakthrough(actual attempt/phase)','pure native composer','fresh host attempt/phase/Busy and production mounting pending')

for row in rows:
    id=row['screen_id'];u=updates.get(id)
    row['embedded_stage_components']=''
    row['stage_evidence_level']='source declaration and bounded logic/API review; not compiled or mounted'
    if u:
        text=(HERE/source[u['file']]['snapshot']).read_text(encoding='utf-8-sig')
        assert u['symbol'] in text, (id,u['symbol'])
        row.update(actual_stage_class=u['symbol'] if u['kind']=='UUserWidget class' else '',
            actual_stage_builder=u['symbol'] if u['kind']=='pure native composer' else '',
            stage_kind=u['kind'],stage_header=u['file'],stage_header_sha256=source[u['file']]['sha256'],
            stage_vm_input=u['vm'],actual_stage_action_seam=u['seam'],stage_review='new slice bounded source review v4',
            open_dependencies=u['dependency'])
    if 'panel-journey/review-v3/' in row['stage_header']:
        path=row['stage_header'].replace('/review-v3/','/review-v4/')
        assert path in source
        row['stage_header']=path;row['stage_header_sha256']=source[path]['sha256']
        row['stage_review']='inherited page set; only ready Journey v4 repair delta reviewed'
    if id=='monthly-sign-in': row['open_dependencies']='J3-01 source repaired in review-v4; three-parameter host comparison/import/build/runtime pending'
    if id=='methods': row['open_dependencies']='PC3-01 satisfied at current hash; PC3-02 minimal ready diff not applied; production host pending'
    if id=='equipment': row['open_dependencies']='PC3-01 satisfied at current hash; PC3-03 minimal ready diff not applied; production host pending'
    if id=='appearance':
        row['open_dependencies']='typed relay present; production host and preview-only mapping pending'
        row['stage_vm_input']='FAppearanceVm + FCharacterAttributePreview'
    if id=='inventory': row['open_dependencies']='N03/N06 source corrections retained; I3-01 pending-content projection still open; runtime pending'
    if id=='character': row['open_dependencies']='I3-01 pending-content projection; runtime pending'
    if id=='launch':
        row['embedded_stage_components']='URebuildLaunchSpecialOfferWidget (route-less): '+prefix+a+'; FRebuildLaunchOfferViewModel; V4-API-01/V4-ACT-02'
    if row['production_evidence'] in source:
        row['production_evidence_sha256']=source[row['production_evidence']]['sha256']
    row['production_state_basis']='inherited v3 mount checkpoint; relevant production hashes unchanged at v4 capture'
    row['runtime_proof']='';row['player_entry_proof']='';row['runtime_verified']=False;row['player_entry_verified']=False
assert len(rows)==44 and len({r['screen_id'] for r in rows})==44
assert all(r['stage_header'] and (r['actual_stage_class'] or r['actual_stage_builder']) for r in rows)
counts=dict(screens=44,stage_symbol_present=44,no_ready_stage=0,
            production_mount=previous['counts']['production_mount'],runtime_verified=0,player_entry_verified=0,
            route_less_offer_is_extra_screen=False)
out=dict(captured_utc=manifest['captured_utc'],scope='source inventory; not 44-page integration/runtime acceptance',
    historical_tables_preserved=['screen-coverage-v2.json','stage-to-runtime-v3.json'],counts=counts,screens=rows)
(HERE/'stage-to-runtime-v4.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
with (HERE/'stage-to-runtime-v4.csv').open('w',newline='',encoding='utf-8-sig') as f:
    writer=csv.DictWriter(f,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
print(json.dumps(counts))
