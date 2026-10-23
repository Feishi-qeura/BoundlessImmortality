from .contracts import require, integer, uid
from .inventory import Inventory


class Market:
    @staticmethod
    def create(ctx, args):
        ctx.anchor_only()
        bounds = {'MAT_HERB_T': (4,12), 'MAT_IRON_T': (6,18)}
        item, count, price = args.get('item_id'), integer(args.get('count'),1,100), integer(args.get('unit_price'),1,18)
        require(item in bounds and bounds[item][0] <= price <= bounds[item][1], 'ITEM_NOT_TRADEABLE')
        require(sum(r['seller']==ctx.user and r['status']=='Listed' for r in ctx.entities('listing')) < 5, 'LISTING_LIMIT')
        id_ = uid()
        reserved = Inventory.reserve(ctx,[dict(item_id=item,count=count)],'market:'+id_)
        require(all(not r['paid'] and r['binding']=='unbound' for r in reserved), 'ITEM_NOT_TRADEABLE')
        expiry = min(r['expiry'] for r in reserved)
        require(expiry-ctx.now>=3600,'ASSET_EXPIRING')
        ctx.money(ctx.user,'CUR_STONE',-1,'listing_fee',id_)
        ctx.put('listing',id_,dict(id=id_,seller=ctx.user,item_id=item,count=count,unit_price=price,status='Listed',version=0,
                                 created=ctx.now,end=min(expiry,ctx.now+86400),original_expiry=expiry))

    @staticmethod
    def buy(ctx,args):
        ctx.anchor_only()
        listing=ctx.entity('listing',args.get('listing_id'))
        require(listing and listing['status']=='Listed' and listing['end']>ctx.now,'LISTING_CLOSED')
        require(listing['version']==args.get('listing_version'),'STALE_VERSION')
        require(listing['seller']!=ctx.user,'SELF_PURCHASE')
        total=listing['count']*listing['unit_price']
        tax=(total+19)//20
        ctx.money(ctx.user,'CUR_STONE',-total,'market_buy',listing['id'])
        ctx.money(listing['seller'],'CUR_STONE',total-tax,'market_sale',listing['id'])
        ctx.db.execute('UPDATE item_instance SET owner=?,reserved=NULL,version=version+1 WHERE scope=? AND reserved=?', (ctx.user,ctx.scope,'market:'+listing['id']))
        listing.update(status='Sold',buyer=ctx.user,tax=tax,version=listing['version']+1)
        ctx.put('listing',listing['id'],listing)

    @staticmethod
    def cancel(ctx,args):
        ctx.anchor_only()
        listing=ctx.entity('listing',args.get('listing_id'))
        require(listing and listing['seller']==ctx.user,'FORBIDDEN')
        require(listing['status']=='Listed','LISTING_CLOSED')
        require(listing['version']==args.get('listing_version'),'STALE_VERSION')
        Inventory.release(ctx,'market:'+listing['id'])
        listing.update(status='Cancelled',version=listing['version']+1)
        ctx.put('listing',listing['id'],listing)

    @staticmethod
    def expire(ctx):
        for listing in ctx.entities('listing'):
            if listing['status']=='Listed' and ctx.now>=listing['end']:
                Inventory.release(ctx,'market:'+listing['id'],listing['seller'])
                listing.update(status='Expired',version=listing['version']+1)
                ctx.put('listing',listing['id'],listing)

    @staticmethod
    def snapshot(ctx):
        rows=ctx.entities('listing')
        return dict(own_listings=[dict(r,name=ctx.rules.items[r['item_id']]['name']) for r in rows if r['seller']==ctx.user],
                    listings=[{k:v for k,v in r.items() if k not in ('seller','buyer')} for r in rows if r['status']=='Listed'])
