from .contracts import require, integer, uid, encode


class Inventory:
    @staticmethod
    def grant(ctx, item_id, count, source, owner=None):
        integer(count, 1)
        require(item_id in ctx.rules.items, 'UNKNOWN_ITEM')
        template = ctx.rules.items[item_id]
        owner = owner or ctx.user
        granted_count = count
        if template['max_stack'] == 0:
            if item_id == 'STAT_QI':
                from .growth import Growth
                Growth.add(ctx, owner, count)
            else:
                ctx.money(owner, item_id, count, 'reward', source.get('id', ''))
            from .reward_receipts import RewardReceipts
            RewardReceipts.record(ctx, owner, item_id, count, source)
            return []
        expiry = min(ctx.window_end, source.get('expiry', ctx.window_end))
        require(expiry > ctx.now, 'ASSET_EXPIRED')
        result = []
        while count:
            n = min(count, template['max_stack'])
            id_ = uid()
            ctx.db.execute('INSERT INTO item_instance VALUES(?,?,?,?,?,?,?,?,?,?,NULL,0)',
                           (id_, ctx.scope, owner, item_id, n, template['binding'], int(source.get('paid', False) or template.get('paid_origin', False)), ctx.now, expiry, encode(source)))
            result.append(id_)
            ctx.db.execute('INSERT INTO item_ledger VALUES(?,?,?,?,?,?,?)',
                           (uid(),ctx.scope,owner,item_id,n,int(source.get('paid',False) or template.get('paid_origin',False)),ctx.now))
            count -= n
        from .reward_receipts import RewardReceipts
        RewardReceipts.record(ctx, owner, item_id, granted_count, source)
        return result

    @staticmethod
    def reserve(ctx, requirements, reservation_id, owner=None):
        owner = owner or ctx.user
        combined = {}
        for requirement in requirements:
            item, count = requirement['item_id'], integer(requirement['count'], 1)
            combined[item] = combined.get(item, 0) + count
        plan = []
        for item, count in combined.items():
            rows = ctx.db.execute('SELECT * FROM item_instance WHERE scope=? AND owner=? AND item=? AND count>0 AND expiry>? AND reserved IS NULL ORDER BY expiry,id', (ctx.scope, owner, item, ctx.now)).fetchall()
            for row in rows:
                n = min(count, row['count'])
                plan.append((dict(row), n))
                count -= n
                if not count: break
            require(count == 0, 'INSUFFICIENT_FUNDS')
        result = []
        for row, n in plan:
            if n == row['count']:
                ctx.db.execute('UPDATE item_instance SET reserved=?,version=version+1 WHERE id=?', (reservation_id, row['id']))
                reserved = row['id']
            else:
                reserved = uid()
                ctx.db.execute('UPDATE item_instance SET count=count-?,version=version+1 WHERE id=?', (n, row['id']))
                ctx.db.execute('INSERT INTO item_instance VALUES(?,?,?,?,?,?,?,?,?,?,?,0)',
                               (reserved, ctx.scope, owner, row['item'], n, row['binding'], row['paid'], row['created'], row['expiry'], row['source'], reservation_id))
            result.append(dict(row, id=reserved, count=n, reserved=reservation_id))
        return result

    @staticmethod
    def consume(ctx, reservation_id, owner=None):
        for row in ctx.db.execute('SELECT owner,item,count,paid FROM item_instance WHERE scope=? AND reserved=? AND owner=? AND count>0',(ctx.scope,reservation_id,owner or ctx.user)).fetchall():
            ctx.db.execute('INSERT INTO item_ledger VALUES(?,?,?,?,?,?,?)',(uid(),ctx.scope,row['owner'],row['item'],-row['count'],row['paid'],ctx.now))
        ctx.db.execute('UPDATE item_instance SET count=0,reserved=NULL,version=version+1 WHERE scope=? AND reserved=? AND owner=?', (ctx.scope, reservation_id,owner or ctx.user))

    @staticmethod
    def release(ctx, reservation_id, owner=None):
        ctx.db.execute('UPDATE item_instance SET reserved=NULL,version=version+1 WHERE scope=? AND reserved=? AND owner=?', (ctx.scope, reservation_id,owner or ctx.user))

    @staticmethod
    def bind(ctx, args):
        ctx.anchor_only()
        conversion = {'MAT_HERB_T': 'MAT_HERB_B', 'MAT_IRON_T': 'MAT_IRON_B'}
        require(args.get('item_id') in conversion, 'ITEM_BOUND')
        inputs = Inventory.reserve(ctx, [dict(item_id=args['item_id'], count=args.get('count', 1))], ctx.reservation_key)
        for row in inputs:
            Inventory.grant(ctx, conversion[row['item']], row['count'], dict(id=ctx.request_id, expiry=row['expiry'], paid=bool(row['paid'])))
        Inventory.consume(ctx, ctx.reservation_key)
