from .contracts import require, integer
from .guilds import Guilds
from .loadouts import Loadouts


class Catalog:
    @staticmethod
    def query(ctx,args):
        kind=args.get('kind','items')
        require(kind in ('items','recipes','sources','equipment','methods','props','buildings','shop','market'),'INVALID_CATALOG')
        offset=integer(args.get('offset',0),0,10000)
        ctx.put('catalog_query',ctx.user,dict(kind=kind,offset=offset,owned=args.get('owned',False) is True))

    @staticmethod
    def snapshot(ctx):
        query=ctx.entity('catalog_query',ctx.user,dict(kind='items',offset=0,owned=True))
        kind=query['kind'];tables=ctx.rules.tables
        counts={r['item']:r['n'] for r in ctx.db.execute('SELECT item,sum(count) n FROM item_instance WHERE scope=? AND owner=? AND expiry>? AND count>0 AND reserved IS NULL GROUP BY item',(ctx.scope,ctx.user,ctx.now))}
        if kind=='recipes':
            rows=[dict(r,ingredients=[dict(i,name=ctx.rules.items[i['item_id']]['name']) for i in tables['items-v1.4/recipe_inputs.csv'] if i['recipe_id']==r['recipe_id']]) for r in tables['items-v1.4/recipes.csv']]
        elif kind=='items':rows=list(ctx.rules.items.values())
        elif kind=='sources':
            source={r['source_id']:r for r in tables['items-v1.4/acquisition_sources.csv']}
            rows=[dict(r,name=ctx.rules.items[r['item_id']]['name'],source=source[r['source_id']]) for r in tables['items-v1.4/item_sources.csv']]
        elif kind=='equipment':rows=tables['items-v1.4/equipment_items.csv']+tables['items-v1.4/fabao_items.csv']
        elif kind=='methods':rows=tables['items-v1.4/method_items.csv']
        elif kind=='props':rows=tables['items-v1.4/prop_items.csv']
        elif kind=='buildings':rows=tables['items-v1.4/guild_buildings.csv']
        elif kind=='shop':rows=[dict(r,name=ctx.rules.items[r['output_item']]['name']) for r in tables['shop.csv']]
        elif kind=='market':
            from .market import Market
            market=Market.snapshot(ctx);own={r['id'] for r in market['own_listings']}
            rows=[dict(r,name=ctx.rules.items[r['item_id']]['name'],is_own=r['id'] in own) for r in market['listings']]
        expiries={r['item']:r['expiry'] for r in ctx.db.execute('SELECT item,min(expiry) expiry FROM item_instance WHERE scope=? AND owner=? AND expiry>? AND count>0 AND reserved IS NULL GROUP BY item',(ctx.scope,ctx.user,ctx.now))}
        rows=[dict(r,owned_count=counts.get(r.get('item_id',r.get('output_item','')),0),earliest_expiry=expiries.get(r.get('item_id',''),0)) for r in rows]
        if query['owned'] and kind in ('items','equipment','methods','props'):rows=[r for r in rows if r['owned_count']>0]
        return dict(kind=kind,offset=query['offset'],total=len(rows),rows=rows[query['offset']:query['offset']+8])
