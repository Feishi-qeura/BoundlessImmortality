"""Read-only atlas projection. Entry counts never create grants or new rules."""
from .world import World


class Journey:
    WORLD = (
        ('qingyun', '青云山', .22, .65, '护阵 · 常规', '青山灵泉，守护阵眼。与剑灵一同完成三阶段历练，积累修为与破境资材。'),
        ('luoxia', '落霞谷', .50, .48, '采集封印 · 进阶', '霞谷药田被灵障封锁。第一阶段选择免费采集路线解封，再完成三阶段历练。'),
        ('xuantie', '玄铁岭', .32, .22, '限时弱点 · 挑战', '玄铁矿脉盘踞山岭。第二阶段回应破障，打开限时弱点，合力完成历练。'),
        ('xuanbing', '玄冰原', .76, .18, '未开放', '冰封北域，待后续章节开放。'),
        ('yaoshou', '妖兽林', .84, .72, '未开放', '林海深处，待后续章节开放。'),
        ('chiyan', '赤炎洲', .72, .43, '未开放', '赤炎诸岛，待后续章节开放。'),
        ('xingyun', '星陨台', .9, .40, '未开放', '星光遗址，待后续章节开放。'),
        ('taixu', '太虚境', .67, .88, '未开放', '太虚秘境，待后续章节开放。'),
    )
    RESOURCE_POSITIONS = {'NODE_QI': (.23, .72), 'NODE_STONE': (.75, .22), 'NODE_HERB': (.22, .23), 'NODE_IRON': (.77, .60), 'NODE_PAGE': (.48, .48)}

    @staticmethod
    def snapshot(ctx):
        totals = {}
        for row in ctx.rules.tables['world_rewards.csv']:
            totals[row['item_id']] = totals.get(row['item_id'], 0) + row['count']
        rewards = [dict(item_id=k, name=ctx.rules.items.get(k, {}).get('name', k), quantity=v) for k, v in totals.items()]
        worlds = [dict(id=i, name=n, x=x, y=y, open=i in World.REGIONS, difficulty=d, description=desc,
                       rewards=rewards if i in World.REGIONS else []) for i, n, x, y, d, desc in Journey.WORLD]
        # Several anchors may hold the same resource type in separate valid races.
        # Display that true set rather than inventing a single exclusive owner.
        owners = {}
        for lease in ctx.entities('lease'):
            if lease.get('status') != 'Active' or lease.get('end', 0) <= ctx.now:
                continue
            anchor = lease['anchor']
            identity = ctx.db.execute('SELECT name FROM identity WHERE scope=? AND user=?', (ctx.scope, anchor)).fetchone()
            owner = dict(anchor_id=anchor, name=identity['name'] if identity else anchor, end=lease['end'])
            entries = owners.setdefault(lease['node_id'], [])
            if not any(r['anchor_id'] == anchor for r in entries):
                entries.append(owner)
        counts = ctx.rules.configs['runtime_balance.json'].get('map_resource_counts', {})
        resources = []
        for rule in ctx.rules.tables['nodes.csv']:
            node = rule['node_id']
            count = counts.get(node, 1)
            count = max(0, min(8, count)) if type(count) is int else 0
            x, y = Journey.RESOURCE_POSITIONS.get(node, (.5, .5))
            for slot in range(count):
                resources.append(dict(id=f'{node}:{slot+1}', node_id=node, name=rule['name'], index=slot+1, entry_count=count,
                    x=x, y=y, open=True, quantity=rule['item_count'], item_id=rule['item_id'],
                    item_name=ctx.rules.items.get(rule['item_id'], {}).get('name', rule['item_id']),
                    interval_seconds=rule['interval_seconds'], qi_additive=rule['qi_additive'],
                    lease_max_seconds=rule['lease_max_seconds'], owners=owners.get(node, [])))
        return dict(worlds=worlds, resources=resources)
