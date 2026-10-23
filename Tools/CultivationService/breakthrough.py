from .contracts import require
from .inventory import Inventory
from .growth import Growth


def major_index(rules, realm):
    return sum(s['to_realm_index'] <= realm for s in rules.tables['items-v1.4/breakthrough_stages.csv'])


class Breakthrough:
    @staticmethod
    def preview(ctx,args):
        quote=Breakthrough.quote(ctx,args.get('selection',[]))
        from .tribulation import probability
        quote['probability_basis_points']=probability(0,ctx.count(ctx.user,'pity:'+ctx.user+':'+quote['stage']['stage_id'])) if quote['stage']['tribulation'] else 10000
        ctx.put('breakthrough_quote',ctx.user,quote)

    @staticmethod
    def quote(ctx, selection, stage=None, old_core=0):
        ctx.anchor_only()
        growth = Growth.snapshot(ctx)
        stage = stage or next((s for s in ctx.rules.tables['items-v1.4/breakthrough_stages.csv'] if s['from_realm_index'] == growth['realm_index']), None)
        require(stage is not None, 'NOT_MAJOR_TRANSITION')
        require(isinstance(selection, list) and len(selection) <= 64, 'INVALID_SELECTION')
        from .contracts import integer
        materials = {r['item_id']: r for r in ctx.rules.tables['items-v1.4/breakthrough_material_items.csv']}
        counts = dict(core=0, guard=0, essence=0, treasure=0)
        weights = dict(core=old_core, guard=0, essence=0, treasure=0)
        for selected in selection:
            require(isinstance(selected, dict), 'INVALID_SELECTION')
            row = materials.get(selected.get('item_id'))
            require(row and row['stage_id'] == stage['stage_id'], 'WRONG_TRANSITION_MATERIAL')
            count = integer(selected.get('count'), 1, 100)
            category = row['category']
            counts[category] += count
            weights[category] += row['quality_weight'] * count
        require(counts['core'] == 1 if not old_core else counts['core'] in (0,1), 'CORE_REQUIRED')
        for category in ('guard', 'essence', 'treasure'):
            require(counts[category] <= stage[category + '_max_count'], 'MATERIAL_LIMIT')
        if old_core and counts['core']:
            weights['core'] -= old_core
            require(weights['core'] > old_core, 'CORE_NOT_UPGRADED')
        guard = weights['guard'] // stage['guard_max_count']
        q = min(100, weights['core'] + guard + weights['essence'] // stage['essence_max_count'] + weights['treasure'])
        band = next(b for b in ctx.rules.tables['items-v1.4/quality_bands.csv'] if b['q_min'] <= q <= b['q_max'])
        return dict(stage=stage, q=q, band=band, core_score=weights['core'], weakness_seconds=60-(60*guard//15), selection=selection)

    @staticmethod
    def rework(ctx, args):
        require(not ctx.entity('activity', ctx.user), 'ACTIVITY_BUSY')
        quote=Breakthrough.rework_quote(ctx,args)
        Inventory.reserve(ctx, quote['selection'], ctx.reservation_key)
        Inventory.consume(ctx, ctx.reservation_key)
        ctx.put('quality', ctx.user + ':' + quote['stage']['stage_id'], quote)

    @staticmethod
    def rework_preview(ctx,args):
        quote=Breakthrough.rework_quote(ctx,args);quote['kind']='rework'
        ctx.put('breakthrough_quote',ctx.user,quote)

    @staticmethod
    def rework_quote(ctx,args):
        ctx.anchor_only()
        realm = Growth.snapshot(ctx)['realm_index']
        stage = next((s for s in reversed(ctx.rules.tables['items-v1.4/breakthrough_stages.csv']) if s['to_realm_index'] <= realm), None)
        require(stage is not None, 'NOT_MAJOR_TRANSITION')
        old = ctx.entity('quality', ctx.user + ':' + stage['stage_id'])
        require(old is not None, 'NO_QUALITY_SNAPSHOT')
        quote = Breakthrough.quote(ctx, args.get('selection', []), stage, old['core_score'])
        require(quote['q'] > old['q'], 'QUALITY_NOT_UPGRADED')
        return quote

    @staticmethod
    def minor(ctx, args):
        ctx.anchor_only()
        require(not ctx.entity('activity', ctx.user), 'ACTIVITY_BUSY')
        growth = Growth.snapshot(ctx)
        require(growth['realm_index'] < 33, 'MAX_REALM')
        stage = next((s for s in ctx.rules.tables['items-v1.4/breakthrough_stages.csv'] if s['from_realm_index'] == growth['realm_index']), None)
        if stage:
            require(False, 'CORE_SELECTION_REQUIRED')
        require(growth['qi'] >= growth['qi_to_next'], 'INSUFFICIENT_QI')
        ctx.money(ctx.user, 'CUR_STONE', -growth['stone_to_next'], 'breakthrough')
        Inventory.reserve(ctx, [dict(item_id='MAT_BREAK', count=growth['material_to_next'])], ctx.reservation_key)
        Inventory.consume(ctx, ctx.reservation_key)
        ctx.db.execute('UPDATE growth SET qi=qi-?,realm=realm+1 WHERE scope=? AND user=?', (growth['qi_to_next'], ctx.scope, ctx.user))
