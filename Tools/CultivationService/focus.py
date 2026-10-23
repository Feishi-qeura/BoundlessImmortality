from .contracts import require
from .inventory import Inventory
from .breakthrough import major_index
from .growth import Growth


class Focus:
    @staticmethod
    def choose(ctx,args):
        ctx.anchor_only()
        require(not ctx.entity('activity',ctx.user),'ACTIVITY_BUSY')
        source=next((r for r in ctx.rules.tables['items-v1.4/acquisition_sources.csv'] if r['source_id']==args.get('source_id')),None)
        item=next((r for r in ctx.rules.tables['items-v1.4/item_sources.csv'] if r['source_id']==args.get('source_id') and r['item_id']==args.get('item_id') and r['method']=='fixed_selected_reward'),None)
        require(source and item,'INVALID_FOCUS')
        require(major_index(ctx.rules,Growth.snapshot(ctx)['realm_index'])>=source['min_major'],'REALM_REQUIRED')
        ctx.put('focus',ctx.user,dict(source_id=source['source_id'],item_id=item['item_id']))

    @staticmethod
    def settle(ctx,round_):
        if not ctx.fact('focus-round:'+round_['id'],'focus_round',dict(round_id=round_['id'])):return
        quota='focus-day:'+ctx.day
        if ctx.count(ctx.user,quota)>=6:return
        ctx.count(ctx.user,quota,1)
        selected=round_.get('focus')
        if not selected:return
        row=next((r for r in ctx.rules.tables['items-v1.4/item_sources.csv'] if r['source_id']==selected['source_id'] and r['item_id']==selected['item_id'] and r['method']=='fixed_selected_reward'),None)
        if not row:return
        key='focus-progress:'+selected['source_id']+':'+selected['item_id']
        progress=ctx.count(ctx.user,key,1)
        if progress>=row['required_full_rounds']:
            ctx.count(ctx.user,key,-row['required_full_rounds'])
            Inventory.grant(ctx,row['item_id'],row['quantity'],dict(id=round_['id']+':focus'))
