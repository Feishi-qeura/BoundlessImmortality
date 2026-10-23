from .contracts import require
from .inventory import Inventory
from .growth import Growth
from .breakthrough import major_index


class Loadouts:
    @staticmethod
    def unequip(ctx,args):
        ctx.anchor_only();require(not ctx.entity('activity',ctx.user),'ACTIVITY_BUSY')
        loadout=Loadouts.get(ctx);item=args.get('item_id')
        require(item and (item==loadout['fabao'] or item in loadout['equipment'].values()),'NOT_EQUIPPED')
        loadout['equipment']={slot:value for slot,value in loadout['equipment'].items() if value!=item}
        if loadout['fabao']==item:loadout['fabao']=''
        ctx.put('loadout',ctx.user,loadout)

    @staticmethod
    def get(ctx):
        return ctx.entity('loadout',ctx.user,dict(equipment={},fabao='',methods={},main='',supports=[]))

    @staticmethod
    def equip(ctx,args):
        ctx.anchor_only()
        require(not ctx.entity('activity',ctx.user),'ACTIVITY_BUSY')
        item=args.get('item_id')
        equipment=next((r for r in ctx.rules.tables['items-v1.4/equipment_items.csv'] if r['item_id']==item),None)
        fabao=next((r for r in ctx.rules.tables['items-v1.4/fabao_items.csv'] if r['item_id']==item),None)
        row=equipment or fabao
        require(row,'NOT_EQUIPMENT')
        require(major_index(ctx.rules,Growth.snapshot(ctx)['realm_index'])>=row['min_major'],'REALM_REQUIRED')
        available=ctx.db.execute('SELECT 1 FROM item_instance WHERE scope=? AND owner=? AND item=? AND expiry>? AND count>0 AND reserved IS NULL',(ctx.scope,ctx.user,item,ctx.now)).fetchone()
        require(available,'INSUFFICIENT_FUNDS')
        loadout=Loadouts.get(ctx)
        if equipment:loadout['equipment'][row['slot']]=item
        else:loadout['fabao']=item
        ctx.put('loadout',ctx.user,loadout)

    @staticmethod
    def learn(ctx,args):
        ctx.anchor_only()
        require(not ctx.entity('activity',ctx.user),'ACTIVITY_BUSY')
        item=args.get('item_id')
        row=next((r for r in ctx.rules.tables['items-v1.4/method_items.csv'] if r['item_id']==item),None)
        require(row,'UNKNOWN_METHOD_ITEM')
        require(major_index(ctx.rules,Growth.snapshot(ctx)['realm_index'])>=row['min_major'],'REALM_REQUIRED')
        loadout=Loadouts.get(ctx)
        method=loadout['methods'].get(row['method_id'])
        if row['item_type']=='study':require(method and method['level']<6,'METHOD_NOT_STUDYABLE')
        rows=Inventory.reserve(ctx,[dict(item_id=item,count=1)],ctx.reservation_key)
        Inventory.consume(ctx,ctx.reservation_key)
        if row['item_type']=='book':
            if method:Inventory.grant(ctx,'MAT_PAGE',2,dict(id=ctx.request_id,expiry=min(r['expiry'] for r in rows),paid=any(r['paid'] for r in rows)))
            else:loadout['methods'][row['method_id']]=dict(level=1,progress=0)
        else:
            method['progress']+=row['study_units']
            costs=[1,2,3,4,6]
            while method['level']<6 and method['progress']>=costs[method['level']-1]:
                method['progress']-=costs[method['level']-1];method['level']+=1
        ctx.put('loadout',ctx.user,loadout)

    @staticmethod
    def configure(ctx,args):
        ctx.anchor_only();require(not ctx.entity('activity',ctx.user),'ACTIVITY_BUSY')
        loadout=Loadouts.get(ctx);main=args.get('main','');supports=args.get('supports',[])
        require(isinstance(supports,list) and all(isinstance(s,str) for s in supports),'INVALID_ARGUMENT')
        chosen=([main] if main else [])+supports
        require(len(chosen)==len(set(chosen)),'DUPLICATE_METHOD')
        slots=ctx.rules.realms[Growth.snapshot(ctx)['realm_index']]['support_slots']
        require(len(supports)<=slots and all(m in loadout['methods'] for m in chosen),'METHOD_NOT_UNLOCKED')
        loadout.update(main=main,supports=supports);ctx.put('loadout',ctx.user,loadout)

    @staticmethod
    def stats(ctx,loadout=None):
        loadout=loadout or Loadouts.get(ctx)
        caps=dict(attack_add=.2,defense_add=.25,health_add=.25,move_speed_add=.10)
        result=dict.fromkeys(caps,0)
        for row in ctx.rules.tables['items-v1.4/equipment_items.csv']:
            if row['item_id'] in loadout['equipment'].values():
                available=ctx.db.execute('SELECT 1 FROM item_instance WHERE scope=? AND owner=? AND item=? AND expiry>? AND count>0 AND reserved IS NULL',(ctx.scope,ctx.user,row['item_id'],ctx.now)).fetchone()
                if available:result[row['stat']]+=row['additive_value']
        return {k:min(caps[k],v) for k,v in result.items()}

    @staticmethod
    def attributes(ctx):
        from .launch_event import LaunchEvent
        realm=Growth.snapshot(ctx)['realm_index']
        stats=Loadouts.stats(ctx) if ctx.role=='anchor' else dict(attack_add=0,defense_add=0,health_add=0,move_speed_add=0)
        from .combat import Combat
        quality=Combat.quality(ctx) if ctx.role=='anchor' else 1
        combat=ctx.entity('combat',ctx.user,{})
        move=combat.get('effects',{}).get('move_speed_add',{})
        movement=min(.10,stats['move_speed_add']+(move.get('magnitude',0) if move.get('end',0)>ctx.now else 0))
        return dict(attack=10*(1+.12*realm)*quality*(1+stats['attack_add']),
                    max_health=1000*(1+.15*realm)*quality*(1+stats['health_add']) if ctx.role=='anchor' else 0,
                    defense=20*(1+.10*realm)*quality*(1+stats['defense_add'])*(1+LaunchEvent.bonus(ctx)) if ctx.role=='anchor' else 0,
                    move_speed_add=movement,quality_factor=quality)
