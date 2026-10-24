"""Generate data/action contracts only. Runtime business functions are referenced, not duplicated."""
from pathlib import Path
import hashlib
import json
import yaml

MODEL=Path(__file__).resolve().parent
NEW=MODEL.parent
ROOT=NEW.parents[2]
base=dict(schema_version=1,project_id='rebuild-2026-10-24')


def yaml_file(name,data):
    (MODEL/name).write_text(yaml.safe_dump(data,allow_unicode=True,sort_keys=False),encoding='utf-8')


# VM IDs describe fields/provenance, not new reflected C++ classes.
vms=[
('FrontendVm','UInkFrontendWidget + AInkMenuDirector',['CurrentPage','Director gameplay/menu state','UInkSaveSubsystem::FindLatestSlot'],['currentRoute','canContinue','continueReason','inputContext'],'Do not fabricate a profile/login/server flow.'),
('SaveVm','UInkSaveSubsystem::GetSlots',['FInkSaveSlotInfo.SlotIndex,bExists,bValid,ValidationError,SavedAt,DisplayName,MapName,PlayedSeconds'],['slots','canSave','lastError'],'Exactly 3 manual slots; actual empty vs unreadable distinct; SavedAt UTC formatted once.'),
('SettingsVm','UGameUserSettings + frontend reduced motion',['GetOverallScalabilityLevel','GetFrameRateLimit','GEngine fixed-frame effective fallback','bReducedMotion'],['quality','configuredFrameLimit','effectiveFrameLimit','reducedMotion'],'Retain custom/unsupported actual values; no implicit conversion to 30/60.'),
('AnchorVm','UCultivationSubsystem mirror + existing character publisher',['GetParticipantSnapshot(GetAnchorId())','Frontend.GetPresentedCharacterAttributes','GetPresentedCharacterDisciplines'],['identity','realm','attributes','health','qi','cultivationState'],'Only typed real numeric inputs; missing score/attributes unknown. Keep gameplay pawn/scene.'),
('InventoryVm','UInkInventoryModel + WulingInventoryProjection',['GetEntriesView','EntryId','AuthorityInstanceId','ExpiresUtc','Item.ItemId,Count','bEquipped','snapshot.item_instances[].reserved'],['entries','selectedEntryId','filters','search','capacity','actionQuantity'],'Read-only remote projection; no optimistic remove, reward, or temporary identity replacement.'),
('LeaderboardVm','Existing room publisher + UCultivationPrototypeClient',['Frontend room entries','GetLeaderboardEntries','InkCore::ReadBoard'],['scope','category','provided','entries','selfPosition'],'Preserve real Rank and ties/gaps; selfPosition unknown unless explicitly provided. Anchor is sword count, not guild growth.'),
('GuildVm','UWulingPrototypeSubsystem::Snapshot',['guild','counts.members,participating_members,helpers','members[].user,name','member_cursor','identity.role'],['guild','counts','members','cursor','selectedMemberId','permissions'],'Keep RecordRow_i -> Rows -> SelectedMember -> RecordRemove path. Unknown permission never means allowed.'),
('SwordVm','UCultivationSubsystem participant + actual authority records',['GetParticipantSnapshot(exact participant id)','snapshot.records[]'],['participantId','displayName','realm','attributes','honor'],'A viewer sword spirit is not the guild or anchor. Personal page coverage remains partial when only rights records exist.'),
('GiftEffectVm','Existing live gift domain; synced rules catalog not confirmed',['UCultivationSubsystem participant/gift effects where actually exposed','UInkGiftRulesPanel currently unavailable'],['activeEffects','rules'],'No invented gift prices, durations, passive descriptions, or unsupported grants. Catalog absence is a business gap.'),
('MethodVm','Authority loadout + actual catalog',['snapshot.loadout.methods,main,supports,method_names','catalog kind=methods'],['ownedMethods','main','supports','catalog'],'Learning and configuring are original commands; do not invent support-slot entitlement.'),
('EquipmentVm','Authority loadout + inventory/catalog',['snapshot.loadout.equipment,fabao','catalog kind=equipment','snapshot.item_instances'],['slots','fabao','ownedItems','catalog'],'Authority checks major realm, ownership, activity lock and slot validity.'),
('WorldVm','Authority journey/world snapshot',['snapshot.journey.worlds[]','snapshot.round','snapshot.growth','snapshot.identity'],['destinations','selectedEntryId','round','mechanic','resultSource'],'Do not infer map locations/recommended realms or completed stages from decoration.'),
('RaceVm','Authority race/queue state',['snapshot.race_queue','snapshot.race.teams[]','snapshot.identity','journey.resources[]'],['queue','race','currentTeam','phase','scores','eligibleActions'],'Match actual anchor/guild/room identity; queued is not matched. Server decides scoring, qualification and tie outcome.'),
('LeaseVm','Authority lease and selected node',['snapshot.lease','snapshot.journey.resources[]'],['nodeId','status','owner','onlineSeconds','endUtc'],'Expiry uses server time. Never locally grant tier, extend lease or infer ownership from winning art.'),
('RewardVm','Confirmed authority reward receipts and offline object',['snapshot.reward_receipts','snapshot.offline_reward','exact receipt source identity'],['confirmedReceipts','sourceId','offlineTotalQi','acknowledgedQi'],'Only root owns global dequeue. Pages filter by exact event/node/actor source without draining inbox; acknowledge is read acknowledgement, not claim.'),
('EconomyVm','Authority catalog/market/wallet',['snapshot.catalog(kind shop/market/items)','snapshot.market.own_listings','snapshot.wallet'],['catalog','costs','listings','selectedListingId','listingVersion','userDraftPrice'],'No hardcoded cyclic price or fabricated reward. Unknown SKU/cost disables only that purchase, not unrelated page navigation.'),
('ActivityVm','InkJourneyAdapter::Read',['snapshot.activities.day,signed,verse','templates[].activity_id,cycle,enabled','weekly_chests,monthly_chests','month_sign_count'],['serviceDay','signed','verse','templates','weeklyChests','monthlyChests','monthCount'],'The date key is supplied by service, not reconstructed from local HK date. Missing is not false/unclaimed.'),
('LaunchVm','Authority launch snapshot',['launch.id,start,end,status,personal,leaderboard,tasks,guild_milestones,permissions','launch.complete_rounds[]'],['event','personal','tasks','leaders','milestones','confirmedRoundCount'],'confirmedRoundCount=length only after array provided. Rank ties remain returned ranks; numeric rewards only when real permission and reward fields exist.'),
('BreakthroughVm','Authority preview + InkCore gate',['breakthrough_quote.stage.stage_id,from_realm_index,to_realm_index','breakthrough_quote.selection,q,band','breakthrough_quote.kind','growth.realm_index','tribulation.status'],['stage','selection','quote','quality','optionalForecast','canSendValidationRequest'],'q_normalized/readiness/slots/sources/growth range absent: remain unavailable; never manufacture them or require them for native submit. Quote success is not execution authorization.'),
('TribulationVm','Authority attempt',['tribulation.id,status,trial_start,deadline,cleanup_at,helper_count,probability_basis_points,receipt_id','snapshot.server_utc'],['attempt','phase','timers','helpers','probability','exclusiveOutcome'],'Never synthesize three UI stages/roll/helper count. Viewer assist path remains actor-verified live comment, not anchor self-assist.'),
('ItemSourceVm','Authority sources catalog',['catalog kind=sources rows[].source_id,item_id,method,source'],['itemId','sources','selectedSourceId'],'Only method=fixed_selected_reward supports existing ChooseFocus; source information is not a claim entitlement.'),
('GuildBuildVm','Authority building catalog/current guild state',['catalog kind=buildings rows[].building_id,level','snapshot.buildings','guild identity'],['buildings','targetLevel','costs'],'Render only actual catalog fields; send exact building_id/target_level; authority checks supply/contribution/caps.'),
('RecipeVm','Authority recipe catalog',['catalog kind=recipes rows[].recipe_id,ingredients,output_item,output_count','inventory'],['recipes','selectedRecipeId','ingredients','output'],'No generated recipe or consumption formula; Craft and server atomically decide ownership/cost/output.'),
('AppearanceVm','Existing explicit frontend preview state',['Frame_ / Bar_ local preview actions','actual current player identity when available'],['previewFrame','previewBar','identity','previewOnly'],'Preview is labeled preview. No fabricated unlock, no fixed .72 progress as account progress, no persistence claim.'),
('ResourceRankVm','No distinct provider contract confirmed',['current UInkResourceRankingPanel unavailable domain'],['providerState','entries'],'Keep unavailable with true reason; do not use race team score as resource contribution rank without contract.'),
]
yaml_file('vm-contract.yaml',dict(**base,kind='declarative-view-model-not-production-cpp',architecture='../architecture-and-interfaces.md',
    common=dict(field_representation='KnownField status/value/provenance; null for unavailable value, never zero-default',
        collection='provided=false -> items unknown; provided=true and empty -> authoritative empty',
        stable_ids=['EntryId','participant_id','guild_id','user','listing_id','recipe_id','activity_id','attempt_id','round_id','race_id','node_id'],
        time='service day key for eligibility; server_utc/monotonic elapsed for countdown; HK is display timezone only'),
    models=[dict(id=i,owner=o,source_paths=p,fields=f,rule=r,missing_fields='unknown',production_type=None) for i,o,p,f,r in vms]))

actions={}


def action(i,kind,handler,params=None,confirmation=False,extra=None):
    remote=kind=='authority-command'
    actions[i]=dict(id=i,kind=kind,handler_ref=handler,parameters=params or [],confirmation_required=confirmation,
        transport_gate=['active-page','known-actor-role','connected','not-busy','fresh-subject'] if remote else ['active-page'],
        preserves=['existing outbox request identity','profile/rule version','expected_version','authority receipt replay','server final inventory/eligibility validation'] if remote else [],
        result='enqueued-not-settled' if remote else 'existing handler result',rule=extra or '')


for i,legacy in [('start-game','StartGame'),('continue-game','ContinueGame'),('quit-game','RequestQuit'),('toggle-cultivation','ToggleCultivation'),
 ('toggle-reduced-motion','ToggleReducedMotion'),('apply-settings','ApplySettings')]:
    action(i,'existing-local-action','UInkFrontendWidget::TriggerAction('+legacy+')',confirmation=i=='quit-game')
for i,route in [('open-saves','Saves'),('open-settings','Settings'),('open-character','CharacterProfile'),('open-inventory','Inventory'),
 ('open-leaderboard','Leaderboard'),('open-journey','Extensions'),('open-equipment','EquipmentFabao'),('open-methods','CultivationMethods'),
 ('open-breakthrough','BreakthroughPrepare'),('open-appearance','CharacterAppearance'),('open-gift-rules','GiftRules'),
 ('open-launch','LaunchEvent'),('open-launch-rank','LaunchRank'),('open-world','WorldCultivation'),('open-resources','ResourceContest'),('open-wuling','Wuling')]:
    action(i,'navigation','New Root presentation hook -> existing semantic route '+route,extra='Root/host exact C++ signature is integration-owned; never dispatch by a made-up function name.')
action('back','navigation','Existing feature stack / frontend Back',extra='Restore same parent entity/filter/tab/scroll/focus; result pages cannot alias generic live page.')
for i,handler in [('board-category','RankDamage / RankCultivation / RankAnchor'),('board-scope','RankLocal / RankGlobal'),
 ('set-quality','QualityLow / QualityMedium / QualityHigh'),('set-frame-limit','Frame30 / Frame60')]:
    action(i,'existing-local-action','UInkFrontendWidget::TriggerAction('+handler+')',extra='Retain actual unsupported/custom values; no forced default or inferred rank.')
for i,prefix in [('save-slot','SaveSlot_i'),('load-slot','LoadSlot_i'),('delete-slot','DeleteSlot_i')]:
    action(i,'existing-local-action','UInkFrontendWidget::TriggerAction('+prefix+')',['slot_index'],confirmation=i in ['save-slot','delete-slot'],extra='Existing overwrite/delete confirmation; index validation; actual SaveSubsystem LastError. Save confirmation only for overwrite.')
for i,enum in [('inventory-use','Use'),('inventory-equip','Equip'),('inventory-unequip','Unequip'),('inventory-recycle','Sell')]:
    action(i,'authority-command','InkCore::InventoryAction(enum='+enum+')',['EntryId','quantity'],confirmation=i in ['inventory-use','inventory-recycle'],extra='Use current stable instance state; remote projection read-only; validate reserved/expiry/ownership, no clamp-down or optimistic mutation. Legacy local mode delegates to real model only.')
action('inventory-select','local-ui-state','Host stable EntryId selection',['EntryId'],extra='Never use visible index/name as identity.')
action('guild-member-select','local-ui-state','Existing RecordRow_i -> Rows -> SelectedMember',['actual user id'],extra='Preserve full management selection/mutation chain; page does not grant permissions.')
action('select-destination','local-ui-state','Host selected destination state',['exact EntryId'],extra='Map marker art never creates service geography.')
action('month-navigate','local-ui-state','Existing month view navigation',['month_offset'],extra='Browsing another month cannot sign/claim for that month.')
action('inspect-chest','local-ui-state','Host exact milestone detail',['milestone','captured service day'],extra='Displaying reward detail is not claiming.')
action('activity-cycle','navigation','Host retained category state',['daily/limited/permanent/launch'],extra='Cycle names map to actual provider templates; retain parent list.')
action('open-activity-detail','navigation','Existing DailyActivityDetail/TimedActivityDetail + SelectedEntry',['activity_id','cycle'])
action('preview-appearance','local-preview','Existing Frame_ / Bar_ preview actions',['actual selected style'],extra='Preview-only, no server entitlement or persisted equip claim.')
for i,kind,params in [('sign-in','SignIn',['ExpectedDay']),('claim-monthly-chest','MonthlyChest',['ExpectedDay','Milestone','confirmed']),
 ('world-travel','WorldTravel',['exact EntryId','confirmed']),('resource-travel','ResourceTravel',['exact EntryId','confirmed']),
 ('start-launch','StartLaunch',['confirmed'])]:
    action(i,'authority-command','InkJourneyAdapter::Capture/Evaluate/Submit(EAction::'+kind+')',params,confirmation=i!='sign-in',extra='Capture fresh context at click. Sign-in binds the displayed service date. Resource submit is QueueRace, not immediate lease.')
for i,handler in [('quote-breakthrough','InkCore::QuoteBreakthrough'),('prepare-breakthrough','InkCore::PrepareBreakthrough'),('rework-quality','InkCore::PrepareBreakthrough(Rework=true)')]:
    action(i,'authority-command',handler,['selection item_id/count','rework mode'],confirmation=i!='quote-breakthrough',extra='Match native stage+echo and actual mode. Missing display-only fields never veto. Server re-quotes and checks growth/safety/inventory/activity. Quote success is not permission.')
for i,handler in [('cancel-breakthrough','InkCore::CancelOrReconcile'),('reconcile-breakthrough','InkCore::CancelOrReconcile')]:
    action(i,'authority-command',handler,['actual attempt id'],confirmation=i=='cancel-breakthrough',extra='Cancel only Prepare before server trial_start; reconcile only actual Reconcile id; never create a roll/helper result.')
action('create-guild','authority-command','InkCore::CreateGuild',['name'],confirmation=True,extra='Anchor role and known absent guild; real service validates name/rules.')
for i,op,params in [('remove-member','RemoveMember',['user_id']),('rename-guild','RenameGuild',['name']),('dissolve-guild','DissolveGuild',['confirmed']),
 ('cancel-race-queue','CancelRaceQueue',[]),('race-action','SubmitRaceAction',['race_id','kind']),('ack-offline','AcknowledgeOfflineReward',[]),
 ('purchase','Purchase',['sku']),('create-listing','CreateListing',['item_id','count','unit_price']),('buy-listing','BuyListing',['listing_id','listing_version']),
 ('cancel-listing','CancelListing',['listing_id','listing_version']),('learn-method','LearnMethod',['item_id']),('configure-methods','ConfigureMethods',['main','supports']),
 ('activate-fabao','ActivateFabao',[]),('choose-focus','ChooseFocus',['source_id','item_id']),('build-guild','Build',['building_id','target_level']),
 ('start-supported-activity','StartActivity',['activity_id']),('end-supported-activity','EndActivity',[])]:
    action(i,'authority-command','UWulingPrototypeSubsystem::SubmitPrototypeCommand('+op+')',params,
        confirmation=i in ['remove-member','rename-guild','dissolve-guild','purchase','create-listing','buy-listing','cancel-listing','build-guild'],
        extra='Exact existing operation only; no direct HTTP/client ledger writes. StartActivity only actual enabled ACT_SHARED/WEEK_GUARD/WEEK_IRON supported by service; match template/cycle/profile. Race actor/team/heartbeat checked by server. Acknowledge is not an additional grant.')
action('craft','authority-command','UWulingPrototypeSubsystem::Craft',['recipe_id'],confirmation=True,extra='Real catalog/recipe, atomic service input/cost/output validation.')
for i,kind in [('query-shop','shop'),('query-market','market'),('query-sources','sources'),('query-buildings','buildings'),('query-recipes','recipes')]:
    action(i,'authority-command','UWulingPrototypeSubsystem::RequestCatalog(kind='+kind+')',['offset','OwnedOnly'],extra='Read/query uses original queued operation; no rewards/price defaults. Page-specific filters preserved.')
yaml_file('action-contract.yaml',dict(**base,kind='mapping-to-existing-interfaces-not-new-production-functions',
    shared_rules=['Only transport subsystem creates/persists request_id; UI retains request identity and subject across refresh.',
        'Confirm binds exact actor/subject/quantity/cost/version/service day; re-evaluate at click.',
        'Unknown display field cannot block unrelated or native-valid requests; unknown actual action entitlement never defaults allowed.',
        'Viewer AssistTribulation remains verified live-comment actor/event path; no anchor self-assist button.',
        'Root alone owns receipt dequeue; page outcome back has no grant/claim side effect.'],actions=list(actions.values())))

schema={
 '$schema':'https://json-schema.org/draft/2020-12/schema','title':'newUI typed field and action envelope',
 '$defs':{
  'KnownField':{'type':'object','required':['status','value','source_path'],'properties':{
      'status':{'enum':['known','unavailable','not-synced','redacted']},'value':{},'source_path':{'type':['string','null']},'reason':{'type':['string','null']}},
      'allOf':[{'if':{'properties':{'status':{'const':'known'}}},'then':{'properties':{'value':{'not':{'type':'null'}}}},'else':{'properties':{'value':{'type':'null'}}}}]},
  'Collection':{'type':'object','required':['provided','items'],'properties':{'provided':{'type':'boolean'},'items':{'type':['array','null']}},
      'allOf':[{'if':{'properties':{'provided':{'const':False}}},'then':{'properties':{'items':{'type':'null'}}},'else':{'properties':{'items':{'type':'array'}}}}]},
 },
 'type':'object','required':['page_id','domain_state','transport','fields'],
 'properties':{'page_id':{'type':'string'},'domain_state':{'enum':['unknown','loading','empty','data','error','disabled']},
  'transport':{'type':'object','required':['connected','busy'],'properties':{'connected':{'type':'boolean'},'busy':{'type':'boolean'},'stale':{'type':'boolean'},'request_identity':{'type':['string','null']}}},
  'fields':{'type':'object','additionalProperties':{'$ref':'#/$defs/KnownField'}},'collections':{'type':'object','additionalProperties':{'$ref':'#/$defs/Collection'}},
  'source_version':{'type':['integer','null']},'service_day':{'type':['string','null']},'server_utc':{'type':['number','null']}}
}
(MODEL/'view-model.schema.json').write_text(json.dumps(schema,ensure_ascii=False,indent=2),encoding='utf-8')
architecture=NEW/'architecture-and-interfaces.md'
(MODEL/'architecture-binding.json').write_text(json.dumps(dict(**base,authority=str(architecture),sha256=hashlib.sha256(architecture.read_bytes()).hexdigest(),
    planned_root='UInkRebuildFrontendWidget',parent='UInkFrontendWidget',default_package='/Game/newUI/Runtime/WBP_NewUIFrontend',
    main_routes=['Main','HUD','Saves','Settings','Character','Inventory','Leaderboard'],
    owner_page_composition='Independent UUserWidget child, integration-owned host wrapper and UPROPERTY references',
    presentation_hook_cpp_names=None,hook_status='Owner has not specified exact hook signatures; do not invent calls.',
    root_source_production_ready=False,legacy_visual_fallback_allowed=False),ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(dict(vm_models=len(vms),actions=len(actions),architecture_read=True)))
