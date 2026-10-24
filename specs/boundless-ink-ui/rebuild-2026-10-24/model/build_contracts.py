"""Author the newUI shared contracts from explicit authored rows and read-only live facts.
Writes only this rebuild's contracts/ and model/; never Source/Config/Content.
"""
from pathlib import Path
import csv
import hashlib
import json
import re
import yaml
from PIL import Image

MODEL = Path(__file__).resolve().parent
NEW = MODEL.parent
CONTRACTS = NEW / 'contracts'
ROOT = NEW.parents[2]
PROJECT = 'rebuild-2026-10-24'


def dump_yaml(path, obj):
    path.write_text(yaml.safe_dump(obj, allow_unicode=True, sort_keys=False), encoding='utf-8')


def dump_json(path, obj):
    path.write_text(json.dumps(obj, ensure_ascii=False, indent=2), encoding='utf-8')


base = dict(schema_version=1, project_id=PROJECT)
old = yaml.safe_load((ROOT / 'specs/boundless-ink-ui/contracts/screen-contract.yaml').read_text(encoding='utf-8'))
facts = {row['id']: row for row in old['screens']}

# New layouts are authored here. Old contracts supply only stable IDs/names/routes.
# id, owner, family, primary intent, action IDs, merge slice, authoritative VM(s)
rows = [
('main','player','scene-entry','看清开始与继续，人物场景成为新版主视觉',['start-game','continue-game','open-saves','open-settings','quit-game'],'player-a',['FrontendVm','SaveVm']),
('home','player','scene-hud','边缘HUD与真实修炼动作，中心保留战斗与人物',['toggle-cultivation','open-character','open-inventory','open-leaderboard','open-journey'],'player-a',['AnchorVm','WorldVm','LeaseVm']),
('character','player','character-sheet','新角色构图中的真实属性、装备和成长入口',['open-equipment','open-methods','open-breakthrough','open-appearance'],'player-b',['AnchorVm','EquipmentVm','MethodVm']),
('inventory','player','inventory-detail','真实物品筛选、稳定选择与动作详情',['inventory-select','inventory-use','inventory-equip','inventory-recycle'],'player-b',['InventoryVm']),
('leaderboard','rank','leaderboard','三榜完整构图，共享身份列与真实排序',['board-category','board-scope'],'rank-a',['LeaderboardVm']),
('saves','player','card-list','三份本地真实存档的元信息及确认动作',['save-slot','load-slot','delete-slot'],'player-c',['SaveVm']),
('settings','player','option-groups','可见当前设置、自定义状态与即时反馈',['set-quality','set-frame-limit','toggle-reduced-motion','apply-settings'],'player-c',['SettingsVm']),
('wuling','rank','organization','新组织hero、统计、成员管理和既有权限链',['create-guild','guild-member-select','remove-member','rename-guild','dissolve-guild'],'rank-b',['GuildVm']),
('sword-spirit','rank','character-sheet','观众个人剑灵资料与主播/组织成长分开',['open-gift-rules'],'rank-c',['SwordVm','GiftEffectVm']),
('gift-rules','rank','rules-sheet','只展示真实礼物效果说明，未知字段明确缺口',[],'rank-c',['GiftEffectVm']),
('methods','player','loadout-builder','主功法与辅助位的新构筑页',['learn-method','configure-methods'],'player-d',['MethodVm','InventoryVm']),
('race-lobby','journey','event-lobby','等待真实匹配，不把入队当开赛',['cancel-race-queue'],'journey-d',['RaceVm','GuildVm']),
('race-live','journey','live-event','阶段/分数/有效参与与实际行动入口',['race-action'],'journey-d',['RaceVm']),
('race-result','journey','settlement','真实竞采结果和匹配收据，不本地颁奖',['back'],'journey-e',['RaceVm','RewardVm']),
('lease','journey','lease-status','当前开采权、在线秒与有效状态',['back'],'journey-e',['LeaseVm']),
('world-result','journey','settlement','历练阶段结果及精确来源奖励',['back'],'journey-c',['WorldVm','RewardVm']),
('offline','activities','receipt-list','已入账离线记录与确认阅读',['ack-offline'],'activities-d',['RewardVm']),
('shop','activities','catalog-detail','真实原型SKU/费用/产出与购买确认',['query-shop','purchase'],'activities-e',['EconomyVm','InventoryVm']),
('market','activities','catalog-detail','真实寄售买卖条件及版本化订单动作',['query-market','create-listing','buy-listing'],'activities-e',['EconomyVm','InventoryVm']),
('orders','activities','card-list','自有订单状态、版本和撤单确认',['cancel-listing'],'activities-f',['EconomyVm']),
('world','journey','landscape-map','新版山水世界视口、真实节点与选中详情',['select-destination','world-travel'],'journey-a',['WorldVm']),
('region','journey','destination-detail','准确区域条件/机制与前往确认',['world-travel'],'journey-c',['WorldVm']),
('resources','journey','landscape-map','资源节点/归属与竞争规则分层',['select-destination','resource-travel'],'journey-a',['LeaseVm','RaceVm']),
('resource-detail','journey','destination-detail','同资源条目的真实详情、确认和请求态',['resource-travel'],'journey-c',['LeaseVm','RaceVm']),
('sign-in','journey','sign-in','只显示当前账户一个签到状态',['sign-in'],'journey-b',['ActivityVm']),
('monthly-sign-in','journey','calendar','七列月历、真实服务日期和里程碑详情',['month-navigate','inspect-chest','claim-monthly-chest'],'journey-b',['ActivityVm']),
('daily-verse','journey','fortune-sheet','服务提供的每日剑诀和签运，缺失不本地抽取',[],'journey-b',['ActivityVm']),
('activities','activities','activity-hub','日常/限定/常驻/开服的统一入口和单一状态',['activity-cycle','open-activity-detail','open-launch'],'activities-a',['ActivityVm','LaunchVm']),
('daily-activities','activities','activity-list','当前日常清单和精确详情入口',['open-activity-detail'],'activities-a',['ActivityVm']),
('timed-activities','activities','activity-list','当前限定清单和真实时限',['open-activity-detail'],'activities-a',['ActivityVm']),
('daily-detail','activities','activity-detail','所选日常模板与真实动作分离',['sign-in','start-supported-activity','end-supported-activity'],'activities-c',['ActivityVm']),
('timed-detail','activities','activity-detail','精确周期/模板和服务支持的参与条件',['start-supported-activity'],'activities-c',['ActivityVm']),
('launch','activities','launch-event','开服总览/任务/组织/荣誉，真实异步更新',['start-launch','open-launch-rank'],'activities-b',['LaunchVm']),
('launch-rank','activities','leaderboard','服务贡献分、并列名次和真实作用域',['back'],'activities-b',['LaunchVm']),
('breakthrough','player','breakthrough-builder','新材料配置与真实报价，展示缺失不阻断合法请求',['quote-breakthrough','prepare-breakthrough','rework-quality'],'player-f',['BreakthroughVm','InventoryVm']),
('tribulation','player','trial-event','真实天劫阶段与互斥结果/对账',['cancel-breakthrough','reconcile-breakthrough'],'player-f',['TribulationVm']),
('item-sources','player','source-detail','真实获取方式与已有定向选择动作',['query-sources','choose-focus'],'player-e',['ItemSourceVm']),
('guild-build','rank','catalog-detail','真实建筑等级与服务费用/贡献确认',['query-buildings','build-guild'],'rank-c',['GuildBuildVm']),
('equipment','player','loadout-builder','装备与法宝真持有/资格/槽位',['inventory-equip','inventory-unequip','activate-fabao'],'player-d',['EquipmentVm','InventoryVm']),
('journey','journey','journey-hub','新版仙途入口按世界/资源/组织明确导航',['open-world','open-resources','open-wuling'],'journey-a',['WorldVm','GuildVm']),
('craft','player','catalog-detail','配方输入和真实产出，缺配方不编造',['query-recipes','craft'],'player-e',['RecipeVm','InventoryVm']),
('appearance','player','appearance-preview','明确标记本地试戴，不冒充解锁/持久化权益',['preview-appearance'],'player-d',['AppearanceVm']),
('resource-rank','journey','leaderboard','资源贡献数据未有独立契约时展示缺口',[],'journey-f',['ResourceRankVm']),
('resource-rewards','journey','receipt-list','精确资源/事件来源对应已确认奖励',[],'journey-f',['LeaseVm','RewardVm']),
]
assert len(rows) == 44 and {r[0] for r in rows} == set(facts)

# Every component has concrete data/event slots; any raster output is planned, not measured.
component_rows = [
('page-frame','container',[672,1232],'Shared','native','header/body/footer/overlay slots'),
('paper-card','container',[672,320],'Shared','native','heading/body/status/action slots'),
('section-heading','container',[672,56],'Shared','native','live heading and optional count'),
('primary-button','control',[624,88],'Shared','native','caption/reason/pending; OnActivate'),
('secondary-button','control',[304,88],'Shared','native','caption/reason; OnActivate'),
('danger-confirm','popup',[624,440],'Shared','native','exact action/subject/cost/current version; Confirm/Cancel'),
('back-button','control',[88,88],'Shared','native','Back to stack frame; do not reconstruct parent route'),
('tab-strip','control',[672,80],'Shared','native','stable choice keys and actual selected key'),
('bottom-navigation','control',[672,112],'Shared','native','five live action entries; selected route key'),
('state-panel','container',[624,256],'Shared','native','exclusive transport/domain state plus supported retry'),
('data-row','container',[624,104],'Shared','native','typed known/unknown value and unit; stable entity key'),
('progress-track','progress',[624,32],'Shared','native','optional known numerator/denominator; indeterminate if unknown'),
('receipt-row','container',[624,112],'Shared','native','source identity/item/count/confirmed fact, no grant callback'),
('main-scene-window','container',[624,552],'Player','native','transparent scene aperture, no baked scene character'),
('anchor-status','container',[320,112],'Player','native','real identity/health/qi/realm, optional fields'),
('character-attribute-group','container',[624,320],'Player','native','typed attribute rows with real unit/rating inputs'),
('inventory-slot','control',[104,120],'Player','native','EntryId/item/count/equipped/reserved; Select/Hover'),
('item-detail-sheet','container',[624,544],'Player','native','stable instance state and supported actions'),
('material-slot','control',[144,152],'Player','native','real item/count/local selection; RequestQuote'),
('trial-phase-strip','progress',[624,88],'Player','native','provider stage/deadline; never synthesize 3 phases'),
('rank-row','container',[624,120],'RankWuling','native','provider rank/participant/metric/title/avatar key'),
('rank-scope-strip','control',[624,88],'RankWuling','native','category and Local/Global keys; self rank optional'),
('guild-member-row','control',[624,112],'RankWuling','native','real user id; RecordRow selection; permission gate'),
('guild-identity-card','container',[624,248],'RankWuling','native','guild name/counts/supply/reputation; creation separate'),
('map-viewport','container',[672,720],'Journey','native','fit map art plus independent keyed nodes; pan/zoom'),
('map-node','control',[88,88],'Journey','native','real destination id/state/selected; open detail'),
('calendar-cell','control',[88,88],'Journey','native','service day/date/known state; inspect date'),
('sign-in-seal','badge',[144,144],'Journey','raster','state visual only; actual state text separate'),
('fortune-ornament','decoration',[256,320],'Journey','raster','empty text area; no generated fortune or reward'),
('settlement-card','container',[624,480],'Journey','native','one outcome, exact event source and known rows'),
('activity-entry','control',[624,176],'Activities','native','template id/cycle/name/optional time and progress'),
('launch-task-row','container',[624,152],'Activities','native','real condition/progress/target/confirmation/reward permission'),
('catalog-card','control',[300,256],'Activities','native','real sku/item/recipe identity and exact known costs'),
('order-row','control',[624,152],'Activities','native','listing id/version/status; supported confirm action'),
('paper-edge-panel','container',[672,896],'Shared','raster','text-free paper frame with unwarped end caps'),
('ink-header-ornament','decoration',[672,160],'Shared','raster','isolated landscape edge; no controls/text'),
]
components = []
for cid, category, size, owner, render, slots in component_rows:
    nine = cid == 'paper-edge-panel'
    components.append(dict(id=cid, name=cid, category=category, reusable=True,
        transparent_background=True, text_policy='live-umg-only' if render=='native' else 'no-text',
        target_size=size, dimensions_basis='design-target-not-measured',
        states=['default','hover','pressed','focused','selected','disabled','pending'] if category=='control' else (['unknown','loading','empty','data','pending','offline','error','disabled'] if cid=='state-panel' else ['default']),
        rendering=render, owner=owner, data_event_slots=slots,
        material='paper-ink-jade', atlas_group='shared' if owner=='Shared' else owner.lower(),
        slice=dict(type='9-slice' if nine else 'full', margins=[48,48,48,48] if nine else [0,0,0,0],
                   measured=False, usable_for_import=False, rule='Proposal only; measure actual PNG and preserve corner geometry.'),
        export_targets=['generic'], style_dependencies=['colors','typography','geometry','control_states'],
        prompt=f'contracts/prompts/components/{cid}.md' if render=='raster' else '', output='', approved=False,
        runtime_package_suggestion=f'/Game/newUI/{"Shared" if owner=="Shared" else "Components/"+owner}/T_NUI_'+''.join(x.title() for x in cid.split('-'))+'_Default_V1' if render=='raster' else None))
dump_yaml(CONTRACTS/'component-contract.yaml',dict(**base,source_screen_id='main',source_screen_status='authorized-trial-pending-user',
    authority='../quickstart.md',components=components))

family_components = {
 'scene-entry':['main-scene-window','primary-button','secondary-button'],
 'scene-hud':['anchor-status','bottom-navigation','progress-track'],
 'character-sheet':['character-attribute-group','tab-strip','data-row'],
 'inventory-detail':['tab-strip','inventory-slot','item-detail-sheet'],
 'card-list':['data-row','secondary-button'], 'option-groups':['tab-strip','data-row'],
 'organization':['guild-identity-card','guild-member-row','danger-confirm'],
 'rules-sheet':['data-row'], 'loadout-builder':['inventory-slot','item-detail-sheet','data-row'],
 'event-lobby':['state-panel','data-row'], 'live-event':['trial-phase-strip','data-row'],
 'settlement':['settlement-card','receipt-row'], 'lease-status':['data-row','progress-track'],
 'receipt-list':['receipt-row'], 'catalog-detail':['catalog-card','item-detail-sheet','danger-confirm'],
 'landscape-map':['map-viewport','map-node'], 'destination-detail':['data-row','danger-confirm'],
 'sign-in':['sign-in-seal','receipt-row','primary-button'], 'calendar':['calendar-cell','tab-strip','receipt-row'],
 'fortune-sheet':['fortune-ornament','data-row'], 'activity-hub':['activity-entry','tab-strip','state-panel'],
 'activity-list':['activity-entry','state-panel'], 'activity-detail':['data-row','state-panel'],
 'launch-event':['launch-task-row','tab-strip','data-row'],
 'breakthrough-builder':['material-slot','data-row','primary-button'],
 'trial-event':['trial-phase-strip','data-row','settlement-card'],
 'journey-hub':['activity-entry','map-node'], 'source-detail':['data-row','activity-entry'],
 'appearance-preview':['character-attribute-group','tab-strip'],
 'leaderboard':['rank-row','rank-scope-strip','state-panel'],
}

source_specs = [
('player-main-v3','2026-10-23-player-core-mapping-correction/main-menu-v3.png','main',True),
('player-breakthrough-v3','2026-10-23-player-core-mapping-correction/breakthrough-preparation-v3.png','breakthrough',True),
('player-tribulation-v3','2026-10-23-player-core-mapping-correction/tribulation-live-v3.png','tribulation',True),
('activities-empty-v3','2026-10-23-activity-state-correction-3/assets/pages/activities-empty-v3.png','activities',True),
('launch-pending-v3','2026-10-23-activity-state-correction-3/assets/pages/launch-milestone-pending-v3.png','launch',True),
('world-map-v3','2026-10-23-parallel-maps-signin/assets/world-map-v3-candidate.png','world',True),
('resource-map-v2','2026-10-23-parallel-maps-signin/assets/resources-map-v2-candidate.png','resources',True),
('sign-in-v2','2026-10-23-parallel-maps-signin/assets/daily-sign-in-v2-candidate.png','sign-in',True),
('monthly-sign-in-v2','2026-10-23-parallel-maps-signin/assets/monthly-sign-in-v2-candidate.png','monthly-sign-in',True),
('rank-damage-v2','2026-10-23-parallel-rank-wuling/assets/pages/rw-leaderboard-damage-candidate-v2.png','leaderboard',True),
('rank-cultivation-v2','2026-10-23-parallel-rank-wuling/assets/pages/rw-leaderboard-cultivation-candidate-v2.png','leaderboard',True),
('rank-spirit-v2','2026-10-23-parallel-rank-wuling/assets/pages/rw-leaderboard-spirit-candidate-v2.png','leaderboard',True),
('wuling-v2','2026-10-23-parallel-rank-wuling/assets/pages/rw-wuling-home-candidate-v2.png','wuling',True),
]
sources=[]
for sid,relative,screen,viewed in source_specs:
    path=ROOT/'specs/boundless-ink-ui/revisions'/relative
    with Image.open(path) as im:
        alpha=im.getchannel('A').getextrema() if 'A' in im.getbands() else None
        sources.append(dict(id=sid,screen_id=screen,path=str(path),dimensions=list(im.size),mode=im.mode,
            alpha_extrema=alpha,sha256=hashlib.sha256(path.read_bytes()).hexdigest(),approved=False,
            metadata_measured=True,visually_inspected_in_this_run=viewed,
            use='authorized candidate input; never runtime whole-page texture',
            old_restrictions_in_manifest='superseded by rebuild quickstart; no old layout-preservation requirement inherited'))
dump_json(CONTRACTS/'source-registry.json',dict(**base,sources=sources,viewed_count=sum(s['visually_inspected_in_this_run'] for s in sources)))
explicit_refs={'main':['player-main-v3'],'leaderboard':['rank-damage-v2','rank-cultivation-v2','rank-spirit-v2'],
 'wuling':['wuling-v2'],'world':['world-map-v3'],'resources':['resource-map-v2'],'sign-in':['sign-in-v2'],
 'monthly-sign-in':['monthly-sign-in-v2'],'activities':['activities-empty-v3'],'launch':['launch-pending-v3'],
 'breakthrough':['player-breakthrough-v3'],'tribulation':['player-tribulation-v3']}
family_refs={'player':['player-main-v3','player-breakthrough-v3'],'rank':['rank-damage-v2','wuling-v2'],
             'journey':['world-map-v3','sign-in-v2'],'activities':['activities-empty-v3']}

resolved={'SwordSpirit':'WulingRights','CultivationMethods':'WulingEquipment','RaceLobby':'WulingRace','RaceLive':'WulingRace',
 'RaceSettlement':'WulingRace','ResourceLease':'WulingRace','EventSettlement':'WulingWorld','OfflineRewards':'WulingOffline',
 'Shop':'WulingShop','Market':'WulingMarket','TradeOrders':'WulingOrders','BreakthroughPrepare':'WulingBreakthrough',
 'Tribulation':'WulingBreakthrough','ItemSources':'WulingSources','GuildConstruction':'WulingConstruction','EquipmentFabao':'CharacterEquipment',
 'MonthlySignIn':'SignIn','DailyFortune':'SignIn'}
contexts={'methods':'methods','monthly-sign-in':'MonthlySignIn','daily-verse':'DailyFortune'}
classfacts={'main':'UInkFrontendWidget','home':'UInkFrontendWidget','character':'UInkFrontendWidget; OpenCharacter enters UInkCharacterProfilePanel',
 'inventory':'UInkFrontendWidget','leaderboard':'UInkFrontendWidget','saves':'UInkFrontendWidget','settings':'UInkFrontendWidget',
 'wuling':'UInkWulingSectPanel','gift-rules':'UInkGiftRulesPanel','world':'UInkJourneyMapPanel','region':'UInkJourneyDestinationPanel',
 'resources':'UInkResourceContestPanel (ResourceMap uses UInkJourneyMapPanel)','resource-detail':'UInkJourneyDestinationPanel',
 'journey':'UInkJourneyMapPanel','appearance':'UInkCharacterAppearancePanel','equipment':'UInkCharacterEquipmentPanel',
 'sign-in':'UInkSignInGroupPanel','monthly-sign-in':'UInkSignInGroupPanel child UInkMonthlySignInPanel','daily-verse':'UInkSignInGroupPanel child UInkDailyFortunePanel',
 'activities':'UInkIntegratedActivitiesPanel','daily-activities':'UInkIntegratedDailyPanel','timed-activities':'UInkIntegratedLimitedPanel',
 'daily-detail':'UInkDailyActivityDetailPanel','timed-detail':'UInkTimedActivityDetailPanel','launch':'UInkLaunchEventPanel','launch-rank':'UInkLaunchEventPanel',
 'resource-rank':'UInkResourceRankingPanel','resource-rewards':'UInkResourceRewardsPanel'}
gaps={
 'gift-rules':['Current GiftRules is unavailable copy; no synced detailed effect catalog contract verified.'],
 'sword-spirit':['Current WulingRights is rights/records, not a complete personal sword model; do not invent a profile.'],
 'resource-rank':['Current resource contribution page has no distinct real ranked result contract; keep unavailable.'],
 'resource-rewards':['Reward rows need exact event/node source; no inferred entitlement from inventory delta.'],
 'daily-verse':['Verse is supplied by activities; fortune classification/reward fields may be absent. No local draw.'],
 'appearance':['Existing Frame_/Bar_ actions are local preview; no verified server unlock/equip/persist contract.'],
 'breakthrough':['q_normalized/readiness/slots/sources/growth_min/max not returned by current quote; display unknown without blocking valid native submit.'],
 'tribulation':['Anchor UI does not expose an authorized Assist command; viewer assistance is received through existing verified live-comment actor path.'],
 'race-result':['Existing alias resolves to generic WulingRace; new distinct outcome host is required.'],
 'world-result':['Existing EventSettlement alias resolves to WulingWorld; new distinct outcome host is required.'],
 'lease':['Existing ResourceLease alias resolves to WulingRace; new lease page must be an actual distinct view.'],
 'shop':['Only real prototype catalog; no platform payment API or invented SKU.'],
 'market':['Prices and limits need real catalog/order rules and user input; do not inherit hardcoded legacy cyclic UI prices.'],
}
screens=[]
for sid,owner,family,purpose,actions,batch,vms in rows:
    prior=facts[sid];route=prior.get('route');existing=resolved.get(route,route)
    refs=explicit_refs.get(sid,family_refs[owner])
    variants=[]
    if sid=='leaderboard': variants=['Damage','CultivationSpeed','Anchor']
    if sid=='wuling': variants=['overview','members-management','construction-entry']
    if sid=='activities': variants=['daily','limited','permanent','launch-entry']
    if sid=='launch': variants=['overview','tasks','guild','honors','offers-unavailable']
    if sid=='tribulation': variants=['Prepare','Trial','Reconcile','Succeeded','Failed','Cancelled']
    components_for=['page-frame','state-panel','back-button']+family_components[family]
    components_for=list(dict.fromkeys(components_for))
    item=dict(id=sid,name=prior['name'],category=family,priority='must-have' if sid in ['main','home','character','inventory','leaderboard','world','sign-in','activities','breakthrough','tribulation'] else 'genre-specific',
        status='planned',contract_ready=True,approved=False,purpose=purpose,owner='panel-'+owner,
        new_page_id=sid,new_root='UInkRebuildFrontendWidget',new_root_package='/Game/newUI/Runtime/WBP_NewUIFrontend',
        architecture='../architecture-and-interfaces.md',
        existing_route=dict(entry=route,resolved=existing,context=contexts.get(sid,''),
            current_host=classfacts.get(sid,'UInkWulingRecordsPanel' if existing in ['WulingRights','WulingOffline','WulingOrders'] else 'UInkWulingToolsPanel'),
            evidence=['Source/BoundlessImmortality/Frontend/InkFrontendExtensions.cpp','Source/BoundlessImmortality/Frontend/InkFeaturePanel.cpp'],
            limitation='Source route resolution only; saved WBP/runtime branch not newly verified.'),
        new_runtime_binding=dict(route_key=route,distinct_page_id_required=True,actual_class=None,actual_wbp=None,
            layout_version='newui-r1-'+sid,package_suggestion='/Game/newUI/Runtime/Pages/'+owner.title()+'/WBP_NUI_'+''.join(x.title() for x in sid.split('-')),
            owner='logic-integration',stage='contract-ready-not-wired'),
        entry_points=[prior.get('entry_points',[prior.get('entry','home')])[0]] if prior.get('entry_points') else [('application-start' if sid=='main' else next((r['entry'] for r in json.loads((ROOT/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/screen-coverage.json').read_text(encoding='utf-8'))['screens'] if r['screen_id']==sid),'home'))],
        primary_actions=actions[:3],all_actions=actions,dependencies=[] if sid=='main' else ['main'],
        layout=dict(family=family,top=['page-header','provider-state'],center=family_components[family],
                    bottom=['primary-action-or-navigation'],floating=['confirmation-only-when-required']),
        responsive=dict(portrait='720x1280; middle region scrolls; fixed header and footer; 88 hit targets',landscape='1280x720 reflow; list/detail or map/right detail; no full-page portrait scaling'),
        components=components_for,states=['unknown','loading','empty','data','pending','offline','error','disabled'],variants=variants,
        edge_cases=['Unknown is not zero or empty','Retain stable entity selection and request identity','Back restores parent draft/filter/scroll state','Missing display fields do not become business permission gates'],
        data_needs=vms,visual_source=dict(mode='direct-candidate-layout-input' if sid in explicit_refs else 'new-layout-derived-from-shared-language',source_ids=refs,
            exact_page_candidate_exists=sid in explicit_refs,whole_page_import_allowed=False),
        gaps=gaps.get(sid,[]),delivery=dict(merge_slice=batch,stage='contract_ready',design_source_ready=True,
            component_source_ready=False,page_composed=False,import_verified=False,route_wired=False,logic_verified=False,player_entry_verified=False,visual_pending_user=True),
        output=dict(brief='contracts/screen-briefs/'+sid+'.md',prompt='',image=''))
    screens.append(item)
    brief=CONTRACTS/'screen-briefs'/f'{sid}.md';brief.parent.mkdir(exist_ok=True)
    brief.write_text(f'''# {prior['name']} · {sid} / newui-r1

Owner: panel-{owner}; 合并片: {batch}; 状态: contract_ready / approved:false / visual_pending_user。

目标：{purpose}。新 PageKey={sid}；兼容入口={route}，目前源码解析到 {existing}。当前类只是业务事实，不要求复用旧可见树。新实际class/WBP尚未核实；依architecture由integration创建真实新页宿主，保留新root与页面标记。

构图族：{family}。竖屏采用 style-contract 的720×1280页框/字级/24安全边距/88触控目标；横屏重新排列表/详情，不缩放整页。必备组件：{', '.join(components_for)}。

视觉来源：{', '.join(refs)}（source-registry.json）；{'有对应候选作为布局输入' if sid in explicit_refs else '无对应整页候选：按统一语言新组页，不能把族参考登记为本页已出图'}。禁止全页贴图，动态文案与操作分层。

数据模型：{', '.join(vms)}。操作：{', '.join(actions) if actions else '只读；无已核实新动作'}，精确映射见 model/action-contract.yaml。Unknown/loading/empty/data/pending/offline/error/disabled分别演示；仅展示缺字段不阻断其它合法动作。

特定变体：{', '.join(variants) if variants else '单页；按服务实际状态呈现'}。缺口：{'；'.join(gaps.get(sid,[])) or '未确认的新字段继续unknown；不得为版面补数'}。

执行交付：1) 独立新页面/组件树；2) 真实VM字段和事件slot；3) 绑定到已有业务处理和新版宿主；4) 仅newUI运行纹理/字体与新布局版本；5) portrait/landscape实际入口；6) 数据/动作与截图各自记证据。局部slot、纸框、PNG、静态编译均不能置page_composed/player_entry_verified=true。
''',encoding='utf-8')
dump_yaml(CONTRACTS/'screen-contract.yaml',dict(**base,status='draft',approved=False,screen_count=44,
    policy='All 44 pages are new visual compositions; legacy preservation flags are superseded.',screens=screens))

with (CONTRACTS/'coverage-matrix.csv').open('w',encoding='utf-8-sig',newline='') as f:
    writer=csv.writer(f);writer.writerow(['screen_id','name','owner','existing_entry_route','existing_resolved_route','new_page_id','new_root','planned_layout_marker','vm','actions','visual_source','exact_page_candidate','merge_slice','stage','actual_new_class','actual_new_wbp','page_composed','import_verified','route_wired','logic_verified','player_entry_verified','visual_pending_user','business_gaps'])
    for s in screens:
        writer.writerow([s['id'],s['name'],s['owner'],s['existing_route']['entry'],s['existing_route']['resolved'],s['new_page_id'],s['new_root'],s['new_runtime_binding']['layout_version'],';'.join(s['data_needs']),';'.join(s['all_actions']),';'.join(s['visual_source']['source_ids']),s['visual_source']['exact_page_candidate_exists'],s['delivery']['merge_slice'],'contract_ready','','',False,False,False,False,False,True,';'.join(s['gaps'])])

assets=[]
for c in components:
    if c['rendering']=='raster':
        prompt=CONTRACTS/'prompts/components'/f'{c["id"]}.md';prompt.parent.mkdir(parents=True,exist_ok=True)
        prompt.write_text(f'''# {c['id']} · default · v1

来源与用途：newUI统一宣纸/墨绿/细旧金语言；源候选见source-registry.json。授权连续试作，approved:false；本文件仅prompt准备，无生成/导入证明。

尺寸与状态：目标{c['target_size'][0]}×{c['target_size'][1]}，单组件default状态；实际像素/alpha/hash生成后读回。所有动态标题、按钮文字、日期、数量、奖励和签运由UMG绘制。

继承：style-contract.yaml colors/visual/geometry/effects。背景透明，主体纸面可不透明，外缘保留透明；左上柔光，哑光墨绿/宣纸、细旧金，低对比纸纹，完整轮廓，可排版中心无干扰。

图像生成提示词：单独的游戏UI组件素材，{c['id']}，{c['data_event_slots']}，isolated transparent RGBA background, blank text-free writing area, warm xuan paper, restrained jade-green ink, fine aged-gold edge, soft top-left light, full uncropped silhouette, high-resolution compositing asset. Do not produce a full page, no labels, no words, no numbers, no watermark, no baked character or fake state, no white matte.

切片：{c['slice']['type']}；margins={c['slice']['margins']}只是设计提案，实测前禁止作为已可用9-slice。复杂纹饰/印章不得拉伸。

验收：状态单独登记；实测尺寸/透明边界/hash/文字为空；拉伸预览后确定margin；真实Texture2D/引用回读另验。
''',encoding='utf-8')
        assets.append(dict(id=c['id']+'-default-v1',kind='component',component_id=c['id'],screen_id='main' if c['owner']=='Shared' else ('sign-in' if c['owner']=='Journey' else 'main'),
            state='default',path='',prompt_path=c['prompt'],dimensions=c['target_size'],dimensions_basis='target-not-measured',status='pending-generation',approved=False,
            transparent_background=True,sha256=None,actual_dimensions=None,alpha_extrema=None,
            runtime_package=c['runtime_package_suggestion'],slice=c['slice']))
dump_yaml(CONTRACTS/'asset-manifest.yaml',dict(**base,assets=assets,reference_registry='source-registry.json',
    note='Only future new-runtime assets listed here; input candidates are measured separately, not mislabeled as newUI imports.'))
dump_json(MODEL/'screen-bindings.json',dict(**base,screens=[dict(screen_id=s['id'],page_id=s['new_page_id'],owner=s['owner'],existing_route=s['existing_route'],
    vm=s['data_needs'],actions=s['all_actions'],gaps=s['gaps'],new_runtime_binding=s['new_runtime_binding']) for s in screens]))

print(json.dumps(dict(screens=len(screens),components=len(components),reference_images_measured=len(sources),reference_images_visually_inspected=sum(s['visually_inspected_in_this_run'] for s in sources),pending_raster_assets=len(assets))))
