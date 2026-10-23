"""Presentation receipts recorded in the SAME transaction as a confirmed grant.

Balances and inventory differences are deliberately never interpreted as rewards.
The existing fact/entity/counter store keeps this additive and migration-free.
"""
import hashlib
import json
from .contracts import encode, uid


class RewardReceipts:
    LABELS = {'world': '世界历练', 'focus': '定向历练', 'lease': '灵脉采集',
              'launch': '青云初启', 'craft': '炼制完成', 'purchase': '商店兑换',
              'bind': '材料转化', 'learn': '典籍研习', 'monthly': '月度签到纪念',
              'race': '灵脉竞采'}
    COMMANDS = {'Craft': 'craft', 'Purchase': 'purchase',
                'BindMaterial': 'bind', 'LearnMethod': 'learn'}

    @staticmethod
    def source(ctx, source):
        """Accept explicit authority sources or identify a durable activity source."""
        source = source or {}
        source_id = source.get('id', '')
        kind = source.get('kind', '')
        if not source_id:
            return None
        if kind in RewardReceipts.LABELS:
            return dict(id=source_id, kind=kind)
        root = source_id.split(':', 1)[0]
        # Races use synthetic world-round IDs and never create a world entity.
        # Require the actual durable race; a matching text prefix alone is not
        # evidence of a reward source. This also covers contribution wallet rows.
        if root.startswith('race-') and ctx.entity('race', root[5:]):
            return dict(id=source_id, kind='race')
        if ctx.entity('world', root):
            return dict(id=source_id, kind='focus' if source_id.endswith(':focus') else 'world')
        if ctx.entity('lease', root):
            return dict(id=source_id, kind='lease')
        if any(event and event.get('id') == root for event in ctx.entities('launch')):
            return dict(id=source_id, kind='launch')
        operation = getattr(ctx, 'reward_operation', '')
        if source_id == ctx.request_id and operation in RewardReceipts.COMMANDS:
            return dict(id=source_id, kind=RewardReceipts.COMMANDS[operation])
        return None

    @staticmethod
    def record(ctx, owner, item_id, quantity, source, name=None, key=None, created=None):
        if quantity <= 0:
            return
        source = RewardReceipts.source(ctx, source)
        if not source:
            return  # Fixtures, transfers, refunds and unspecified credits are excluded.
        occurred = ctx.now if created is None else created
        receipt_id = hashlib.sha256(encode([owner, source, occurred]).encode()).hexdigest()
        fact_key = 'reward-grant:' + (key or uid())
        if not ctx.fact(fact_key, 'reward_grant', dict(receipt_id=receipt_id,
                         item_id=item_id, quantity=quantity, source=source), owner):
            return
        receipt = ctx.entity('reward_receipt', receipt_id)
        if not receipt:
            receipt = dict(receipt_id=receipt_id, owner=owner,
                           sequence=ctx.count(owner, 'reward-receipt-sequence', 1),
                           title='获得奖励', source_label=RewardReceipts.LABELS[source['kind']],
                           source_id=source['id'], created_utc=occurred, items=[])
        item = next((entry for entry in receipt['items'] if entry['item_id'] == item_id), None)
        if item:
            item['quantity'] += quantity
        else:
            definition = ctx.rules.items.get(item_id, {})
            receipt['items'].append(dict(item_id=item_id,
                name=name or definition.get('name', item_id), quantity=quantity))
        ctx.put('reward_receipt', receipt_id, receipt)

    @staticmethod
    def capture_wallet(ctx):
        # These direct authority credits bypass Inventory.grant. Currency grants
        # through Inventory are already recorded there and must not be counted twice.
        rows = ctx.db.execute("""SELECT w.* FROM wallet_ledger w
            WHERE w.scope=? AND w.owner=? AND w.amount>0
            AND w.reason IN ('launch','world','world_contribution')
            AND NOT EXISTS (SELECT 1 FROM fact f WHERE f.scope=w.scope
                AND f.id='reward-grant:wallet:'||w.id) ORDER BY w.rowid""",
            (ctx.scope, ctx.user)).fetchall()
        for row in rows:
            RewardReceipts.record(ctx, row['owner'], row['currency'], row['amount'],
                dict(id=row['source']), key='wallet:'+row['id'], created=row['created'])

    @staticmethod
    def snapshot(ctx):
        RewardReceipts.capture_wallet(ctx)
        rows = ctx.db.execute("""SELECT data FROM entity WHERE scope=? AND kind='reward_receipt'
            AND json_extract(data,'$.owner')=? ORDER BY json_extract(data,'$.sequence') DESC LIMIT 128""",
            (ctx.scope, ctx.user)).fetchall()
        return dict(high_watermark=ctx.count(ctx.user, 'reward-receipt-sequence'),
                    entries=list(reversed([json.loads(row['data']) for row in rows])))
