from .contracts import require, uid
from .guilds import Guilds
from .inventory import Inventory


class Leases:
    @staticmethod
    def current(ctx):
        guild=Guilds.current(ctx)
        if not guild:return None
        rows=[r for r in ctx.entities('lease') if r['guild_id']==guild['id']]
        return max(rows,key=lambda r:r['start']) if rows else None

    @staticmethod
    def activate(ctx,race_id,node_id,duration_seconds,start_utc=None):
        guild=Guilds.current(ctx)
        require(guild and guild['anchor']==ctx.user,'GUILD_NOT_FOUND')
        require(not any(r['guild_id']==guild['id'] and r['end']>ctx.now and r['status']=='Active' for r in ctx.entities('lease')),'LEASE_ACTIVE')
        require(node_id in {r['node_id'] for r in ctx.rules.tables['nodes.csv']},'UNKNOWN_NODE')
        start=ctx.now if start_utc is None else start_utc
        lease=dict(id=uid(),guild_id=guild['id'],anchor=ctx.user,room=ctx.room,node_id=node_id,race_id=race_id,
                   start=start,end=min(start+duration_seconds,ctx.window_end),cursor=start,online_seconds=0,issued_buckets=0,status='Active')
        ctx.put('lease',lease['id'],lease)
        return lease

    @staticmethod
    def settle(ctx):
        nodes={r['node_id']:r for r in ctx.rules.tables['nodes.csv']}
        for lease in ctx.entities('lease'):
            if lease['status']!='Active':continue
            end=min(ctx.now,lease['end'])
            actor=ctx.db.execute('SELECT * FROM session WHERE scope=? AND user=? AND room=? AND revoked=0',(ctx.scope,lease['anchor'],lease['room'])).fetchone()
            if actor and actor['active']:
                online_end=min(end,actor['heartbeat']+5)
                lease['online_seconds']+=max(0,online_end-max(lease['cursor'],lease['start']))
            lease['cursor']=max(lease['cursor'],ctx.now)
            node=nodes[lease['node_id']]
            if node['item_id']:
                buckets=int(lease['online_seconds']//node['interval_seconds'])
                delta=buckets-lease['issued_buckets']
                if delta>0:
                    Inventory.grant(ctx,node['item_id'],delta*node['item_count'],dict(id=lease['id']+':'+str(buckets)),lease['anchor'])
                    lease['issued_buckets']=buckets
            if ctx.now>=lease['end']:lease['status']='Expired'
            ctx.put('lease',lease['id'],lease)

    @staticmethod
    def qi_bonus(ctx):
        lease=Leases.current(ctx)
        if not lease or lease['node_id']!='NODE_QI' or not lease['start']<=ctx.now<lease['end']:return 0
        host=ctx.db.execute('SELECT 1 FROM session WHERE scope=? AND user=? AND room=? AND active=1 AND revoked=0 AND heartbeat>=?',(ctx.scope,lease['anchor'],ctx.room,ctx.now-5)).fetchone()
        return .2 if host and ctx.room==lease['room'] else 0
