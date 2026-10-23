"""New-rule damage only. Delayed/status damage never re-enters hit triggers."""
from .contracts import require
from .loadouts import Loadouts
from .effects import Effects
from .growth import Growth


class Combat:
    @staticmethod
    def target_open(target,now):
        return not target.get('sealed',False) and (not target.get('weakpoint_required',False) or now<target.get('weak_until',0))

    @staticmethod
    def quality(ctx):
        # Quality entity keys carry the owner; avoid including another anchor's snapshots.
        keys=[ctx.user+':'+s['stage_id'] for s in ctx.rules.tables['items-v1.4/breakthrough_stages.csv']]
        placeholders=','.join('?' for _ in keys)
        rows=[__import__('json').loads(r[0]) for r in ctx.db.execute('SELECT data FROM entity WHERE scope=? AND kind=\'quality\' AND id IN ('+placeholders+')', (ctx.scope,*keys))]
        return 1+min(.10,sum((r['band']['live_snapshot_factor']-1)/10 for r in rows))

    @staticmethod
    def resolve_hit(ctx,round_,target):
        owner=ctx.for_user(round_['anchor'])
        loadout=round_['loadout'];main=loadout['main'];supports=loadout['supports']
        stats=Loadouts.stats(owner,loadout)
        realm=Growth.snapshot(ctx)['realm_index']
        attack=10*(1+.12*realm)
        if ctx.user==round_['anchor']:attack*=Combat.quality(owner)*(1+stats['attack_add'])
        state=Effects.combat(owner);effects=state['effects']
        armor_ignore=0
        ignore=effects.get('next_hit_armor_ignore',{})
        if ctx.user==round_['anchor'] and ignore.get('end',0)>ctx.now and ignore.get('charges',0)>0:
            armor_ignore=min(.6,ignore['magnitude']);ignore['charges']-=1
        defense=max(0,target['defense']*(1-armor_ignore))
        base=attack*attack/(attack+defense)
        mechanics=round_.setdefault('mechanics',{}).setdefault(ctx.user,dict(hits=0,cooldowns={}))
        mechanics['hits']+=1;hits=mechanics['hits'];cooldowns=mechanics['cooldowns']
        level=loadout['methods'].get(main,{}).get('level',1)
        factor=1.2 if level>=6 else (1.1 if level>=3 else 1)
        multiplier=1;support_add=0
        if main=='METHOD_EDGE' and hits%5==0:multiplier=1.8*factor
        if main=='METHOD_STAR' and (ctx.now-round_['start'])%20<6:multiplier*=1+.2*factor
        if main=='METHOD_SURGE' and effects.get('method_surge',{}).get('end',0)>ctx.now:multiplier*=1+.25*factor
        marked=effects.get('method_pact',{}).get('end',0)>ctx.now
        if main=='METHOD_PACT' and marked and ctx.user!=round_['anchor']:
            from .guilds import Guilds
            if (Guilds.current(ctx) or {}).get('id')==(Guilds.current(owner) or {}).get('id'):multiplier*=1+.15*factor
        delayed=round_.setdefault('delayed',[])
        if main=='METHOD_ECHO' and hits%4==0:delayed.append(dict(target=target['id'],due=ctx.now+.5,damage=base*.5*factor))
        for method in supports:
            if method=='METHOD_STAR':support_add+=.05
            if method=='METHOD_SURGE' and state['health']<state['max_health']*.5:support_add+=.1
            if method=='METHOD_EDGE' and effects.get('charged',{}).get('end',0)>ctx.now:
                support_add+=.1;effects['charged']['end']=0
            if method=='METHOD_PACT' and marked and cooldowns.get('pact',0)<=ctx.now:
                support_add+=.05;cooldowns['pact']=ctx.now+20
            if method=='METHOD_ECHO' and cooldowns.get('echo',0)<=ctx.now:
                delayed.append(dict(target=target['id'],due=ctx.now+.5,damage=base*.1));cooldowns['echo']=ctx.now+20
        ether=main=='METHOD_ETHER' or ('METHOD_ETHER' in supports and cooldowns.get('ether',0)<=ctx.now)
        if ether:
            dot=target.setdefault('ether',dict(stacks=0,last_tick=ctx.now,end=ctx.now))
            dot.update(stacks=min(5,dot['stacks']+1),end=ctx.now+5,damage=base*.05*(factor if main=='METHOD_ETHER' else 1))
            cooldowns['ether']=ctx.now+20
        for key in ('next_strike_add','next_hits_add','normal_obstacle_hit_add'):
            effect=effects.get(key,{})
            if ctx.user==round_['anchor'] and effect.get('end',0)>ctx.now and effect.get('charges',1)>0:
                if key=='normal_obstacle_hit_add' and round_['stage']!=2:continue
                support_add+=effect['magnitude'];effect['charges']=effect.get('charges',1)-1
        ctx.put('combat',owner.user,state)
        from .launch_event import LaunchEvent
        return base*multiplier*(1+min(.5,support_add))*(1+LaunchEvent.bonus(ctx))

    @staticmethod
    def incoming(state,damage,now):
        shield=state['effects'].get('shield_max_hp',{})
        if shield.get('end',0)>now:
            absorbed=min(shield.get('remaining',0),damage)
            shield['remaining']=shield.get('remaining',0)-absorbed;damage-=absorbed
        applied=min(state['health'],max(0,damage));state['health']-=applied
        return applied

    @staticmethod
    def activate_method(ctx,args):
        ctx.anchor_only()
        activity=ctx.entity('activity',ctx.user)
        require(activity and activity['kind']=='world','NO_TARGET')
        round_=ctx.entity('world',activity['id']);main=round_['loadout']['main']
        state=Effects.combat(ctx)
        require(main in ('METHOD_GUARD','METHOD_SURGE','METHOD_PACT'),'NO_ACTIVE_METHOD')
        require(state['cooldowns'].get(main,0)<=ctx.now,'COOLDOWN')
        if main=='METHOD_SURGE':
            require(state['health']>1,'INSUFFICIENT_HEALTH');state['health']*=.9
        key={'METHOD_GUARD':'method_guard','METHOD_SURGE':'method_surge','METHOD_PACT':'method_pact'}[main]
        state['effects'][key]=dict(end=ctx.now+(20 if main=='METHOD_PACT' else 6))
        state['cooldowns'][main]=ctx.now+(20 if main=='METHOD_PACT' else 30)
        ctx.put('combat',ctx.user,state)

    @staticmethod
    def advance(ctx,round_):
        owner=ctx.for_user(round_['anchor']);state=Effects.combat(owner)
        now=ctx.now
        for delayed in list(round_.get('delayed',[])):
            if delayed['due']>now:continue
            target=next((t for t in round_['targets'] if t['id']==delayed['target']),None)
            if target and Combat.target_open(target,now):target['health']=max(0,target['health']-delayed['damage'])
            round_['delayed'].remove(delayed)
        for target in round_['targets']:
            dot=target.get('ether')
            if dot:
                ticks=max(0,int(min(now,dot['end'])-dot['last_tick']))
                if Combat.target_open(target,now):target['health']=max(0,target['health']-ticks*dot['damage']*dot['stacks'])
                dot['last_tick']+=ticks
            slow=state['effects'].get('npc_slow',{})
            target['move_speed']=55*(1-slow['magnitude']) if slow.get('end',0)>now and round_['stage']!=3 else 55
        burn=state['effects'].get('burn_attack_per_tick',{})
        if burn.get('round_id')==round_['id']:
            ticks=max(0,int(min(now,burn['end'])-burn['last_tick']))
            target=next((t for t in round_['targets'] if t['health']>0 and Combat.target_open(t,now)),None)
            if target:target['health']=max(0,target['health']-ticks*burn['magnitude']*burn.get('base_attack',Loadouts.attributes(owner)['attack']))
            burn['last_tick']+=ticks
        if round_['status']=='Running' and any(t['health']>0 for t in round_['targets']):
            interval=round_['template']['boss_attack_interval_seconds'] if round_['stage']==3 else round_['template']['enemy_attack_interval_seconds']
            cursor=round_.get('pressure_cursor',now)
            attacks=max(0,int((now-cursor)//interval));round_['pressure_cursor']=cursor+attacks*interval
            attack=round_['template']['boss_attack'] if round_['stage']==3 else round_['template']['enemy_attack']
            reduction=.2 if state['effects'].get('method_guard',{}).get('end',0)>now else 0
            defense=Loadouts.attributes(owner)['defense']
            if 'METHOD_GUARD' in round_['loadout']['supports'] and state['health']<state['max_health']*.5:defense*=1.1
            damage=attack*attack/(attack+defense)*(1-min(.6,reduction))*attacks
            Combat.incoming(state,damage,now)
            if attacks:state['last_combat']=now;state['safe']=False
            if state['health']<=0:
                round_['recovery_until']=now+30;round_['targets']=[]
        if round_.get('recovery_until',0) and now>=round_['recovery_until']:
            state['health']=state['max_health'];round_['recovery_until']=0
            if round_['status']=='Running' and round_['stage'] not in round_['completed'] and not round_['targets']:
                from .world import World
                World.targets(round_,round_['stage'])
                round_['pressure_cursor']=now
        round_['anchor_health']=state['health'];ctx.put('combat',owner.user,state)
