from .contracts import require, integer
from .guilds import Guilds
from .inventory import Inventory


class Construction:
    @staticmethod
    def build(ctx,args):
        ctx.anchor_only()
        guild=Guilds.current(ctx)
        require(guild is not None,'GUILD_NOT_FOUND')
        level=integer(args.get('target_level'),1,3)
        building=args.get('building_id')
        row=next((r for r in ctx.rules.tables['items-v1.4/guild_buildings.csv'] if r['building_id']==building and r['level']==level),None)
        require(row is not None,'UNKNOWN_BUILDING')
        key=guild['id']+':'+building
        current=ctx.entity('building',key,dict(level=0))
        require(current['level']==row['prerequisite_level'],'STALE_BUILDING_LEVEL')
        ctx.money(guild['id'],'CUR_SUPPLY',-row['supply_cost'],'construction')
        requirements=[dict(item_id=row['material_'+str(i)],count=row['material_'+str(i)+'_count']) for i in (1,2)]
        Inventory.reserve(ctx,requirements,ctx.reservation_key,owner=guild['id'])
        Inventory.consume(ctx,ctx.reservation_key,owner=guild['id'])
        ctx.put('building',key,dict(guild_id=guild['id'],building_id=building,level=level,completed=ctx.now))
