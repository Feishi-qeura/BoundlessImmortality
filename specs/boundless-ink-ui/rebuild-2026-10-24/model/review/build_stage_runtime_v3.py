"""Create a 44-screen evidence map. Class presence is not runtime proof."""
from pathlib import Path
import csv
import json
from collections import Counter

HERE = Path(__file__).resolve().parent
BASE = HERE.parents[1]
manifest = json.loads((HERE / 'source-delta-v3.json').read_text(encoding='utf-8'))
sources = {r['path']: r for r in manifest['sources']}
bindings = json.loads((BASE / 'model/screen-bindings.json').read_text(encoding='utf-8'))['screens']
historical = {r['screen_id']: r for r in json.loads((HERE / 'screen-coverage-v2.json').read_text(encoding='utf-8'))['screens']}
prefix = 'specs/boundless-ink-ui/rebuild-2026-10-24/'
mapping = {}

def add(ids, file, symbol, vm, seam, context='', kind='UUserWidget class', blockers='host mapping/build/runtime proof pending'):
    for id in ids.split():
        mapping[id] = dict(file=prefix+file, symbol=symbol, vm=vm, seam=seam, context=context, kind=kind, blockers=blockers)

p='panel-player/staged-source/Frontend/UIRebuild20261024/Player/'
for id, file, symbol, vm in [
 ('main','InkPlayerVisibleEntryWidgets','BuildMainMenuSurface','CanContinue + save status'),
 ('home','InkPlayerVisibleEntryWidgets','BuildHudSurface','UInkAnchorStatusWidget + live status'),
 ('saves','InkPlayerMenuPageWidgets','BuildSavesBody','FInkSaveSlotInfo array + CanSave'),
 ('settings','InkPlayerMenuPageWidgets','BuildSettingsBody','quality + frame cap + reduced motion'),
 ('character','InkPlayerCharacterInventoryWidgets','BuildCharacterPage','FInkCharacterAttribute/FInkDisciplineEntry arrays + provider state'),
 ('inventory','InkPlayerCharacterInventoryWidgets','BuildInventoryPage','UInkInventoryModel + stable SelectedEntryId + filter/page/quantity/provider'),
]:
    add(id,p+file+'.h',symbol,vm,'UInkFrontendActionButton.Action -> BindFrontend',kind='pure native composer',blockers='runtime proof pending'+('; N03/N06' if id=='inventory' else ''))
pc='panel-player/player-c/staged-source/Frontend/UIRebuild20261024/Player/InkPlayerMethodsEquipmentAppearanceWidgets.h'
for id, symbol, vm, blockers in [('methods','BuildMethodsPage','FMethodsVm','PC3-01/PC3-02'),('equipment','BuildEquipmentPage','FEquipmentVm','PC3-01/PC3-03'),('appearance','BuildAppearancePage','FAppearanceVm','PC3-01')]:
    add(id,pc,symbol,vm,'FActionRequested(action ID, FActionPayload); ui.* local intents',kind='pure native composer',blockers=blockers+'; no production host observed')
r='panel-rank/review-v3/staged-source/'
for id, file, symbol, vm, seam, context in [
 ('leaderboard','InkRankWulingPages','UInkRankBoardPage','FInkNewRankBoardData','OnCategoryRequested/OnScopeRequested/OnRetryRequested/OnAvatarRequested','Damage/CultivationSpeed/Anchor variants'),
 ('wuling','InkRankWulingPages','UInkWulingOverviewPage + UInkWulingMembersPage','FInkWulingOverviewData + FInkWulingMembersData','OnActionRequested/OnMemberSelected/OnPageRequested/OnRemoveRequested','overview + stable user-id member subpage'),
 ('sword-spirit','InkRankWulingExtendedPages','UInkSwordSpiritPage','FInkSwordSpiritPageData','OnOpenGiftRulesRequested; provider setter',''),
 ('gift-rules','InkRankWulingExtendedPages','UInkGiftRulesPage','FInkGiftEffectPageData','read-only provider setter; route/back host-owned',''),
 ('guild-build','InkRankWulingExtendedPages','UInkGuildBuildPage','FInkGuildBuildPageData','OnBuildRequested with captured building/version/target',''),
]:
    add(id,r+file+'.h',symbol,vm,seam,context)
j='panel-journey/review-v3/staged-source/Frontend/UIRebuild20261024/Journey/InkJourneyNewWidgets.h'
for id,symbol,vm,seam in [
 ('world','UInkJourneyWorldMapPageWidget','SetAuthorityData(FText, FInkJourneyNewMapEntry array, exact selected ID)','OnEntrySelected/OnTravelRequested(exact ID)'),
 ('resources','UInkJourneyResourceMapPageWidget','SetAuthorityData(FText, FInkJourneyNewMapEntry array, exact selected ID)','OnEntrySelected/OnTravelRequested(exact ID)'),
 ('region','UInkJourneyRegionPageWidget','SetAuthorityData(exact EntryId, name, state, rows, CanTravel)','OnTravelRequested(exact EntryId)'),
 ('resource-detail','UInkJourneyResourceDetailPageWidget','SetPageData(exact SourceId, rows, CanTravel)','OnTravelRequested(exact SourceId)'),
 ('world-result','UInkJourneyWorldResultPageWidget','SetPageData(exact SourceId, rows, CanTravel)','OnTravelRequested(exact SourceId); receipt projection host-owned'),
 ('race-result','UInkJourneyRaceResultPageWidget','SetPageData(exact SourceId, rows, CanTravel)','OnTravelRequested(exact SourceId); receipt projection host-owned'),
 ('race-lobby','UInkJourneyRaceLobbyPageWidget','SetAuthorityData(QueueSubjectId, state, rows, CanCancel)','OnCancelRequested; host captures exact queue subject'),
 ('race-live','UInkJourneyRaceLivePageWidget','SetAuthorityData(exact RaceId, phase, rows, actions)','OnRaceActionRequested(exact RaceId, kind)'),
 ('lease','UInkJourneyLeasePageWidget','read-only SetAuthorityData(collectionProvided, exact SourceId, rows)','OnBackRequested only; do not invent claims'),
 ('sign-in','UInkJourneyDailySignInPageWidget','SetPresentation(ServerDay, claim state, status, count/verse known flags)','OnClaimRequested(ExpectedDay)/OnMonthlySignInRequested'),
 ('monthly-sign-in','UInkJourneyMonthlySignInPageWidget','SetPresentation(ExpectedDay, month, days, count, milestones)','OnMilestoneClaimRequested(captured day, month, milestone)'),
 ('daily-verse','UInkJourneyDailyVersePageWidget','read-only SetAuthorityData(collectionProvided, exact SourceId, rows)','OnBackRequested only'),
 ('journey','UInkJourneyHubPageWidget','SetAuthorityState(world, guild)','OnNavigationRequested(action ID)'),
 ('resource-rank','UInkJourneyResourceRankPageWidget','read-only SetAuthorityData(collectionProvided, exact SourceId, rows)','OnBackRequested; ranking provider gap retained'),
 ('resource-rewards','UInkJourneyResourceRewardsPageWidget','read-only SetAuthorityData(collectionProvided, exact SourceId, rows)','OnBackRequested; no invented claim'),
]:
    add(id,j,symbol,vm,seam,blockers=('J3-01; ' if id=='monthly-sign-in' else '')+'production host/build/runtime pending')
a='panel-activities/review-v3/staged-source/RebuildActivityWidgets.h'
for ids,symbol,vm,context in [
 ('activities','URebuildActivitiesHubWidget','FRebuildActivitiesHubViewModel','hub'),
 ('daily-activities','URebuildActivitiesHubWidget','FRebuildActivitiesHubViewModel','cycle=daily; distinct route identity host-owned'),
 ('timed-activities','URebuildActivitiesHubWidget','FRebuildActivitiesHubViewModel','limited/timed cycle mapping must use real provider; distinct route identity host-owned'),
 ('daily-detail','URebuildActivityCategoryDetailWidget','FRebuildActivityCategoryDetailViewModel','cycle=daily + exact activity ID'),
 ('timed-detail','URebuildActivityCategoryDetailWidget','FRebuildActivityCategoryDetailViewModel','limited/timed provider cycle + exact activity ID'),
 ('launch','URebuildLaunchActivityWidget','FRebuildLaunchActivityViewModel','captured absent-event StartSubject'),
]:
    add(ids,a,symbol,vm,'OnActionRequested(FRebuildActivityActionRequest)',context)

rows=[]
for b in bindings:
    id=b['screen_id']; stage=mapping.get(id); old=historical[id]
    path=stage['file'] if stage else ''
    if stage:
        assert path in sources, path
        text=(HERE/sources[path]['snapshot']).read_text(encoding='utf-8-sig')
        for symbol in stage['symbol'].split(' + '):
            assert symbol in text, (id,symbol,path)
    rows.append(dict(screen_id=id, owner=b['owner'], entry=b['existing_route']['entry'],
        resolved_route=b['existing_route']['resolved'], canonical_vm=';'.join(b['vm']),
        canonical_actions=';'.join(b['actions']),
        actual_stage_class=stage['symbol'] if stage and stage['kind']=='UUserWidget class' else '',
        actual_stage_builder=stage['symbol'] if stage and stage['kind']=='pure native composer' else '',
        stage_kind=stage['kind'] if stage else 'no ready new source observed',
        stage_header=path, stage_header_sha256=sources[path]['sha256'] if stage else '',
        stage_vm_input=stage['vm'] if stage else '', actual_stage_action_seam=stage['seam'] if stage else '',
        required_context=stage['context'] if stage else '',
        stage_review='new slice API/action review' if id in ('methods','equipment','appearance') else 'review-v3 delta only' if stage and ('review-v3' in path) else 'previous source review' if stage else 'not available',
        production_mount_state=old['state'], production_evidence=old['evidence_path'],
        production_evidence_sha256=sources[old['evidence_path']]['sha256'],
        historical_v2_evidence_sha256=old['evidence_sha256'],
        production_state_basis='mount counts retained after bounded five-file Source delta review; v2 coverage preserved separately',
        production_class='UInkRebuildFrontendWidget' if old['state']=='new-composition-connected-in-source' else '',
        compile_proof='', saved_wbp_proof='', runtime_proof='', player_entry_proof='',
        runtime_verified=False, player_entry_verified=False,
        open_dependencies=stage['blockers'] if stage else 'await owner slice and ready handoff; no inferred implementation'))
assert len(rows)==44 and len({r['screen_id'] for r in rows})==44
out=dict(captured_utc=manifest['captured_utc'], scope='stage declarations and existing source mount checkpoint; not runtime acceptance',
    parent_coverage='screen-coverage-v2.json',
    counts=dict(screens=44, stage_symbol_present=sum(bool(r['stage_header']) for r in rows), no_ready_stage=sum(not bool(r['stage_header']) for r in rows),
                production_mount=dict(Counter(r['production_mount_state'] for r in rows)), runtime_verified=0, player_entry_verified=0), screens=rows)
(HERE/'stage-to-runtime-v3.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
with (HERE/'stage-to-runtime-v3.csv').open('w',newline='',encoding='utf-8-sig') as f:
    writer=csv.DictWriter(f,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
print(json.dumps(out['counts']))
