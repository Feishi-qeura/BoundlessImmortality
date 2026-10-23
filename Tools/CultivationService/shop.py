from .contracts import require
from .inventory import Inventory


class Shop:
    @staticmethod
    def purchase(ctx,args):
        sku=next((r for r in ctx.rules.tables['shop.csv'] if r['sku']==args.get('sku')),None)
        require(sku is not None,'UNKNOWN_SKU')
        require(sku['owner_type']==ctx.role,'FORBIDDEN')
        key='shop:'+sku['sku']+':'+ctx.day
        require(ctx.count(ctx.user,key)<sku['daily_limit'],'DAILY_LIMIT')
        require(not sku['unique_window'] or ctx.count(ctx.user,'sku-once:'+sku['sku'])==0,'ALREADY_SETTLED')
        ctx.money(ctx.user,'CUR_STONE',-sku['stone_cost'],'shop')
        requirements=[]
        for i in (1,2):
            item,count=sku['extra_item_'+str(i)],sku['extra_count_'+str(i)]
            if not item or not count: continue
            if ctx.rules.items[item]['max_stack']==0: ctx.money(ctx.user,item,-count,'shop')
            else: requirements.append(dict(item_id=item,count=count))
        rows=Inventory.reserve(ctx,requirements,ctx.reservation_key)
        expiry=min([ctx.window_end]+[r['expiry'] for r in rows])
        Inventory.consume(ctx,ctx.reservation_key)
        Inventory.grant(ctx,sku['output_item'],sku['output_count'],dict(id=ctx.request_id,expiry=expiry,paid=any(r['paid'] for r in rows)))
        ctx.count(ctx.user,key,1)
        if sku['unique_window']: ctx.count(ctx.user,'sku-once:'+sku['sku'],1)

    @staticmethod
    def recycle(ctx,args):
        ctx.anchor_only()
        from .contracts import integer
        count=integer(args.get('count'),1,100)
        item=args.get('item_id')
        table=next((r for r in ctx.rules.tables['market.csv'] if r['item_id']==item),None)
        require(table is not None,'ITEM_NOT_TRADEABLE')
        # Prices are explicit in the supplied table, never inferred from a player's sale price.
        price=table['npc_recycle_unit_price']
        require(ctx.count(ctx.user,'recycle:'+ctx.day)+count*price<=50,'DAILY_LIMIT')
        rows=Inventory.reserve(ctx,[dict(item_id=item,count=count)],ctx.reservation_key)
        require(not any(r['paid'] for r in rows),'ITEM_NOT_TRADEABLE')
        Inventory.consume(ctx,ctx.reservation_key)
        ctx.money(ctx.user,'CUR_STONE',count*price,'recycle')
        ctx.count(ctx.user,'recycle:'+ctx.day,count*price)
