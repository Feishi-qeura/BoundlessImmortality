from .contracts import require


class Growth:
    @staticmethod
    def settle(ctx, now):
        row = ctx.db.execute('SELECT * FROM growth WHERE scope=? AND user=?', (ctx.scope, ctx.user)).fetchone()
        if not row:
            return
        start, end = row['cursor'], min(now, ctx.window_end)
        elapsed = max(0, end - start)
        online_end = min(end, ctx.session['heartbeat'] + 5) if ctx.session['active'] else start
        online = max(0, online_end - start)
        offline = elapsed - online
        offline_total = row['offline_seconds']
        offline_qi = 0 if ctx.role == 'anchor' else .1 * min(offline, max(0, 28800 - offline_total))
        bonus=ctx.authority.online_bonus_seconds(ctx,start,online_end) if online else 0
        grant = online + bonus + offline_qi
        if offline_qi:
            receipt=ctx.entity('offline_reward',ctx.user,dict(total_qi=0,acknowledged_qi=0,seconds=0))
            receipt.update(total_qi=receipt['total_qi']+offline_qi,seconds=receipt['seconds']+min(offline,max(0,28800-offline_total)),last_settled_utc=now)
            ctx.put('offline_reward',ctx.user,receipt)
        qi, realm = row['qi'] + grant, row['realm']
        if ctx.role == 'viewer':
            while realm < 33 and qi >= ctx.rules.realms[realm]['viewer_qi_to_next']:
                qi -= ctx.rules.realms[realm]['viewer_qi_to_next']
                realm += 1
        ctx.db.execute('UPDATE growth SET qi=?,total=total+?,realm=?,cursor=?,offline_seconds=? WHERE scope=? AND user=?',
                       (qi, grant, realm, max(row['cursor'], now), offline_total + offline, ctx.scope, ctx.user))

    @staticmethod
    def add(ctx, user, amount, source=None):
        row = ctx.db.execute('SELECT g.*,i.role FROM growth g JOIN identity i USING(scope,user) WHERE g.scope=? AND g.user=?', (ctx.scope, user)).fetchone()
        require(row is not None, 'IDENTITY_NOT_FOUND')
        qi, realm = row['qi'] + amount, row['realm']
        if row['role'] == 'viewer':
            while realm < 33 and qi >= ctx.rules.realms[realm]['viewer_qi_to_next']:
                qi -= ctx.rules.realms[realm]['viewer_qi_to_next']
                realm += 1
        require(qi >= 0, 'INSUFFICIENT_QI')
        ctx.db.execute('UPDATE growth SET qi=?,total=total+?,realm=? WHERE scope=? AND user=?', (qi, amount, realm, ctx.scope, user))
        if source:
            from .reward_receipts import RewardReceipts
            RewardReceipts.record(ctx, user, 'STAT_QI', amount, source)

    @staticmethod
    def snapshot(ctx):
        row = dict(ctx.db.execute('SELECT * FROM growth WHERE scope=? AND user=?', (ctx.scope, ctx.user)).fetchone())
        rule = ctx.rules.realms[row['realm']]
        return dict(realm_index=row['realm'], name=rule['name'], qi=row['qi'], total_qi=row['total'],
                    qi_to_next=rule['anchor_qi_to_next'] if ctx.role == 'anchor' else rule['viewer_qi_to_next'],
                    stone_to_next=rule['anchor_stone_to_next'] if ctx.role == 'anchor' else 0,
                    material_to_next=rule['anchor_material_to_next'] if ctx.role == 'anchor' else 0)

    @staticmethod
    def acknowledge_offline(ctx,args):
        receipt=ctx.entity('offline_reward',ctx.user)
        require(receipt is not None,'NO_OFFLINE_REWARD')
        receipt['acknowledged_qi']=receipt['total_qi'];ctx.put('offline_reward',ctx.user,receipt)
