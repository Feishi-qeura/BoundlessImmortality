from .contracts import require
from .inventory import Inventory
from .loadouts import Loadouts
from .growth import Growth
from .breakthrough import major_index


class Effects:
    @staticmethod
    def combat(ctx):
        maximum=Loadouts.attributes(ctx)['max_health'] if ctx.role=='anchor' else 0
        state=ctx.entity('combat',ctx.user)
        if state:
            old=state['max_health']
            if old!=maximum and old>0:state['health']=state['health']/old*maximum
            state['max_health']=maximum
            return state
        return dict(health=maximum,max_health=maximum,last_combat=0,safe=True,statuses={},effects={},cooldowns={})

    @staticmethod
    def activate(ctx,args,fabao=False):
        ctx.anchor_only()
        activity=ctx.entity('activity',ctx.user)
        require(not activity or activity['kind']=='world','ACTIVITY_BUSY')
        item=Loadouts.get(ctx)['fabao'] if fabao else args.get('item_id')
        table='fabao_items.csv' if fabao else 'prop_items.csv'
        row=next((r for r in ctx.rules.tables['items-v1.4/'+table] if r['item_id']==item),None)
        if fabao:
            require(ctx.db.execute('SELECT 1 FROM item_instance WHERE scope=? AND owner=? AND item=? AND count>0 AND reserved IS NULL AND expiry>?',(ctx.scope,ctx.user,item,ctx.now)).fetchone(),'INSUFFICIENT_FUNDS')
        if item=='ITEM_HEAL':row=dict(effect_key='heal_max_hp',magnitude=.25,duration_seconds=0,cooldown_seconds=60,min_major=0,shared_cooldown='HEAL_90')
        if item=='ITEM_SHIELD':row=dict(effect_key='shield_max_hp',magnitude=.10,duration_seconds=60,cooldown_seconds=60,min_major=0,shared_cooldown='SHIELD')
        require(row is not None,'UNKNOWN_EFFECT')
        require(major_index(ctx.rules,Growth.snapshot(ctx)['realm_index'])>=row['min_major'],'REALM_REQUIRED')
        state=Effects.combat(ctx)
        state.setdefault('effects',{});state.setdefault('statuses',{});state.setdefault('cooldowns',{})
        key=row['effect_key'];cooldown=key if fabao else row.get('shared_cooldown',key)
        require(state['cooldowns'].get(cooldown,0)<=ctx.now,'COOLDOWN')
        magnitude,duration=row['magnitude'],row['duration_seconds']
        if key=='heal_max_hp':
            require(0<state['health']<state['max_health'],'NO_TARGET')
            state['health']=min(state['max_health'],state['health']+state['max_health']*magnitude)
        elif key.startswith('cleanse_') or key=='cold_cleanse':
            status='cold' if key=='cold_cleanse' else key.removeprefix('cleanse_')
            require(state['statuses'].get(status,{}).get('stacks',0)>0,'NO_TARGET')
            state['statuses'][status]['stacks']=max(0,state['statuses'][status]['stacks']-magnitude)
        elif key=='post_break_weakness_reduce_seconds':
            require(state.get('weakness_until',0)>ctx.now,'NO_TARGET')
            state['weakness_until']=max(ctx.now,state['weakness_until']-magnitude)
        elif key=='shield_max_hp':
            prior=state['effects'].get('shield_max_hp',{})
            amount=max(prior.get('remaining',0) if prior.get('end',0)>ctx.now else 0,state['max_health']*magnitude)
            state['effects'][key]=dict(end=ctx.now+duration,magnitude=magnitude,remaining=amount)
        elif key in ('npc_slow','burn_attack_per_tick','normal_obstacle_hit_add'):
            require(activity and activity['kind']=='world','NO_TARGET')
            world=ctx.entity('world',activity['id'])
            require(world['status']=='Running' and world['targets'],'NO_TARGET')
            require(key!='npc_slow' or world['stage']!=3,'TARGET_IMMUNE')
            require(key!='normal_obstacle_hit_add' or world['stage']==2,'NO_TARGET')
            state['effects'][key]=dict(end=ctx.now+duration,magnitude=magnitude,last_tick=ctx.now,round_id=activity['id'])
            if key=='burn_attack_per_tick':state['effects'][key]['base_attack']=Loadouts.attributes(ctx)['attack']
        elif key=='local_replay_marker':
            require(ctx.entity('last_attempt',ctx.user),'NO_TARGET')
            state['replay_marker']=ctx.entity('last_attempt',ctx.user)
        elif key in ('next_strike_add','next_hits_add','next_hit_armor_ignore','hazard_damage_reduction','negative_duration_reduction','move_speed_add','cold_duration_reduce',
                     'cue_advance_seconds','public_route_preview','meditation_visual','world_cue_visual','rift_cue_visual','break_ceremony_visual','ceremony_message'):
            state['effects'][key]=dict(end=ctx.now+duration,magnitude=magnitude,charges=3 if key=='next_hits_add' else 1)
        else:require(False,'EFFECT_NOT_IMPLEMENTED')
        if not fabao:
            Inventory.reserve(ctx,[dict(item_id=item,count=1)],ctx.reservation_key)
            Inventory.consume(ctx,ctx.reservation_key)
        state['cooldowns'][cooldown]=ctx.now+max(row['cooldown_seconds'],60 if not fabao else 0)
        ctx.put('combat',ctx.user,state)
