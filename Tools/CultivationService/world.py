import math
import bisect
from .contracts import require, uid, CommandContext
from .guilds import Guilds
from .inventory import Inventory
from .growth import Growth
from .focus import Focus
from .loadouts import Loadouts
from .combat import Combat


class World:
    REGIONS = {'qingyun': '青云山', 'luoxia': '落霞谷', 'xuantie': '玄铁岭'}

    @staticmethod
    def start(ctx, preset, route, focus=None, region_id='qingyun'):
        ctx.anchor_only()
        require(isinstance(region_id, str) and region_id in World.REGIONS, 'REGION_NOT_OPEN')
        require(not ctx.entity('activity', ctx.user), 'ACTIVITY_BUSY')
        template = next((r for r in ctx.rules.tables['encounters.csv'] if r['preset_id'] == preset), None)
        require(template and route in ('guard', 'gather'), 'INVALID_ARGUMENT')
        template=dict(template)
        factor=ctx.rules.configs['runtime_balance.json']['flight_health_factor']
        for key in ('normal_health','obstacle_health','boss_health'):template[key]=max(1,math.ceil(template[key]*factor))
        template['flight_health_factor']=factor
        id_ = uid()
        round_ = dict(id=id_, anchor=ctx.user, room=ctx.room, start=ctx.now, elapsed=0, status='Preparing', stage=0,
                      region_id=region_id, region_name=World.REGIONS[region_id],
                      region_mechanic=dict(ctx.rules.configs['runtime_balance.json']['region_mechanics'][region_id]),
                      preset=preset, route=route, template=template, targets=[], wave=0, completed=[],
                      participants={}, last_action={}, anchor_health=1000, recovery_until=0,
                      rules_hash=ctx.rules.sha256, focus=ctx.entity('focus',ctx.user),loadout=Loadouts.get(ctx))
        launch=ctx.entity('launch',ctx.user)
        round_['boundaries']=[60,220,380,540,600] if launch and launch['status']=='Active' else [60,240,420,540,600]
        ctx.put('world', id_, round_)
        ctx.put('activity', ctx.user, dict(kind='world', id=id_))
        ctx.put('last_world', ctx.user, dict(id=id_))

    @staticmethod
    def targets(round_, stage):
        template = round_['template']
        count = template['stage1_targets_per_wave'] if stage == 1 else (template['obstacle_count'] if stage == 2 else 1)
        hp = template['normal_health'] if stage == 1 else (template['obstacle_health'] if stage == 2 else template['boss_health'])
        round_['targets'] = [dict(id=uid(), health=hp, max_health=hp, defense=template['target_defense']) for _ in range(count)]
        kind=round_.get('region_mechanic',{}).get('kind')
        for target in round_['targets']:
            if kind=='gather_route' and stage==1:
                target['sealed']=not round_.get('gather_route_open',False)
            if kind=='weakpoint' and stage==2:
                target.update(weakpoint_required=True,weak_until=round_.get('weak_until',0))

    @staticmethod
    def advance(ctx):
        for round_ in ctx.entities('world'):
            if round_['status'] in ('Completed', 'Interrupted'): continue
            elapsed = max(0, ctx.now - round_['start'])
            Combat.advance(ctx,round_)
            World.complete_targets(ctx,round_)
            round_['elapsed'] = min(600, elapsed)
            stage = bisect.bisect_right(round_['boundaries'][:4],elapsed)
            if elapsed >= 600:
                round_['status'] = 'Completed'
                round_['targets'] = []
                World.finish(ctx, round_)
                ctx.put('activity', round_['anchor'], None)
            elif stage == 4:
                round_['status'], round_['targets'] = 'Settling', []
            elif stage > 0:
                round_['status'] = 'Running'
                if stage != round_['stage']:
                    round_['stage'], round_['wave'] = stage, 0
                    World.targets(round_, stage)
                    round_['pressure_cursor']=ctx.now
            for target in round_['targets']:target['mechanic_locked']=not Combat.target_open(target,ctx.now)
            ctx.put('world', round_['id'], round_)

    @staticmethod
    def submit(ctx, round_id, action, target_id=None,choice=None):
        round_ = ctx.entity('world', round_id)
        require(round_ and round_['room'] == ctx.room, 'ROUND_CLOSED')
        World.advance(ctx)
        round_ = ctx.entity('world', round_id)
        require(round_['status'] == 'Running', 'ROUND_CLOSED')
        require(round_['stage'] not in round_['completed'],'STAGE_COMPLETED')
        require(ctx.session['active'] and ctx.session['heartbeat'] >= ctx.now - 5, 'PARTICIPATION_REQUIRED')
        require(ctx.now - round_['last_action'].get(ctx.user, 0) >= 1, 'ACTION_COOLDOWN')
        require(action in ('strike', '护阵', '破障', '共修', 'route'), 'INVALID_ACTION')
        if action=='route':require(choice in ('guard','gather'),'INVALID_ROUTE')
        stage = round_['stage']
        if action == 'strike':
            target = next((t for t in round_['targets'] if t['id'] == target_id and t['health'] > 0), None)
            require(target is not None, 'TARGET_CLOSED')
            require(not target.get('sealed'),'GATHER_ROUTE_REQUIRED')
            require(Combat.target_open(target,ctx.now),'WEAKPOINT_CLOSED')
            damage = Combat.resolve_hit(ctx,round_,target)
            target['health'] = max(0, target['health'] - damage)
        else:
            kind=round_.get('region_mechanic',{}).get('kind')
            if kind=='gather_route' and stage==1 and action=='route' and choice=='gather':
                round_['gather_route_open']=True
                for target in round_['targets']:target['sealed']=False
            if kind=='weakpoint' and stage==2 and action=='破障':
                round_['weak_until']=ctx.now+round_['region_mechanic']['weakpoint_seconds']
                for target in round_['targets']:target['weak_until']=round_['weak_until']
            correct = (action == '护阵' and stage == 1) or (action == '破障' and stage == 2) or (action == '共修' and stage == 3)
            if correct:
                # Responses are recorded as mechanics, not unbounded damage or an automatic stage win.
                key = f"response:{round_id}:{ctx.user}:{action}"
                if ctx.count(ctx.user, key) < 2:
                    ctx.count(ctx.user, key, 1)
                    ctx.fact(f'{key}:{ctx.count(ctx.user,key)}', 'world_response', dict(round_id=round_id, action=action, stage=stage))
                    contribution_key='contribution-response:'+round_id
                    if ctx.role=='viewer' and ctx.count(ctx.user,contribution_key)<2:
                        ctx.count(ctx.user,contribution_key,1);World.contribution(ctx,ctx.user,5,round_)
        participant = round_['participants'].setdefault(ctx.user, dict(stages=[], role=ctx.role, guild_id=(Guilds.current(ctx) or {}).get('id')))
        if stage not in participant['stages']: participant['stages'].append(stage)
        round_['last_action'][ctx.user] = ctx.now
        if action!='strike':participant['free_response']=True
        ctx.fact('action:'+ctx.reservation_key, 'world_action', dict(round_id=round_id, action=action, stage=stage))
        World.complete_targets(ctx,round_)
        for target in round_['targets']:target['mechanic_locked']=not Combat.target_open(target,ctx.now)
        ctx.put('world', round_id, round_)
        from .launch_event import LaunchEvent
        LaunchEvent.on_action(ctx,round_,action,stage)

    @staticmethod
    def complete_targets(ctx,round_):
        stage=round_['stage']
        if round_['targets'] and all(t['health'] <= 0 for t in round_['targets']):
            if stage == 1 and round_['wave'] + 1 < round_['template']['stage1_waves']:
                round_['wave'] += 1
                World.targets(round_, stage)
            else:
                round_['targets'] = []
                if stage not in round_['completed']:
                    round_['completed'].append(stage)
                    World.stage_reward(ctx, round_, stage)

    @staticmethod
    def stage_reward(ctx, round_, stage):
        if not ctx.fact(f"world:{round_['id']}:stage:{stage}", 'stage_reward', dict(round_id=round_['id'], stage=stage), round_['anchor']): return
        for reward in ctx.rules.tables['world_rewards.csv']:
            if reward['stage'] == stage:
                Inventory.grant(ctx, reward['item_id'], reward['count'], dict(id=f"{round_['id']}:{stage}"), round_['anchor'])
        for user,participant in round_.get('participants',{}).items():
            if participant['role']=='viewer' and stage in participant['stages']:
                World.contribution(ctx,user,{1:2,2:3,3:5}[stage],round_)

    @staticmethod
    def contribution(ctx,user,amount,round_):
        from datetime import datetime,timedelta,timezone
        day=datetime.fromtimestamp(round_['start']+600,timezone(timedelta(hours=8))).strftime('%Y-%m-%d')
        key='contribution:'+day
        delta=min(amount,max(0,120-ctx.count(user,key)))
        if delta:ctx.money(user,'STAT_CONTRIB',delta,'world_contribution',round_['id']);ctx.count(user,key,delta)

    @staticmethod
    def finish(ctx, round_,include_focus=True,launch=True):
        from .effects import Effects
        owner=ctx.for_user(round_['anchor']);combat=Effects.combat(owner);combat['safe']=True;ctx.put('combat',owner.user,combat)
        from .launch_event import LaunchEvent
        if launch:LaunchEvent.on_complete(ctx.for_user(round_['anchor']),round_)
        if len(round_['completed']) != 3: return
        if not ctx.fact('world-complete:' + round_['id'], 'world_complete', round_, round_['anchor']): return
        guild = Guilds.current(ctx, round_['anchor'])
        if include_focus and len(round_['participants'].get(round_['anchor'],{}).get('stages',[]))==3:
            Focus.settle(ctx.for_user(round_['anchor']),round_)
        for user, participant in round_['participants'].items():
            if len(participant['stages']) != 3: continue
            key = 'world-bonus:' + ctx.day
            if ctx.count(user, key) < 6:
                Growth.add(ctx, user, 60, dict(id=round_['id']+':complete', kind=round_.get('reward_kind','world')))
                ctx.count(user, key, 1)
            if guild and participant['role'] == 'viewer' and participant['guild_id'] == guild['id']:
                member_key, guild_key = 'supply:' + ctx.day, 'supply:' + ctx.day
                if ctx.count(user, member_key) < 6 and ctx.count(guild['id'], guild_key) < 100:
                    ctx.money(guild['id'], 'CUR_SUPPLY', 1, 'world', round_['id'])
                    ctx.count(user, member_key, 1)
                    ctx.count(guild['id'], guild_key, 1)
        completed_people=sum(len(p['stages'])==3 for p in round_['participants'].values())
        if guild and completed_people >= 2 and ctx.count(guild['id'], 'reputation:' + ctx.day) < 6:
            ctx.money(guild['id'], 'STAT_REPUTATION', 10, 'world', round_['id'])
            ctx.count(guild['id'], 'reputation:' + ctx.day, 1)

    @staticmethod
    def interrupt(ctx, round_id):
        ctx.anchor_only()
        round_ = ctx.entity('world', round_id)
        require(round_ and round_['anchor'] == ctx.user and round_['status'] not in ('Completed', 'Interrupted'), 'ROUND_CLOSED')
        round_['status'], round_['targets'] = 'Interrupted', []
        ctx.put('world', round_id, round_)
        ctx.put('activity', ctx.user, None)
        from .effects import Effects
        combat=Effects.combat(ctx);combat['safe']=True;ctx.put('combat',ctx.user,combat)
