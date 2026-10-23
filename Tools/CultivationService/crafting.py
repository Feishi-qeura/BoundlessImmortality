from .contracts import require, integer
from .inventory import Inventory
from .guilds import Guilds


class Crafting:
    @staticmethod
    def craft(ctx, recipe_id, count=1):
        ctx.anchor_only()
        integer(count, 1, 100)
        recipe = next((r for r in ctx.rules.tables['items-v1.4/recipes.csv'] if r['recipe_id'] == recipe_id), None)
        require(recipe is not None, 'UNKNOWN_RECIPE')
        require(recipe['recipe_type'] != 'duplicate_salvage', 'USE_DUPLICATE_SALVAGE')
        from .breakthrough import major_index
        realm = ctx.db.execute('SELECT realm FROM growth WHERE scope=? AND user=?', (ctx.scope, ctx.user)).fetchone()[0]
        require(major_index(ctx.rules, realm) >= recipe['min_major'], 'REALM_REQUIRED')
        owner = ctx.user
        if recipe['output_owner'] == 'guild':
            guild = Guilds.current(ctx)
            require(guild is not None, 'GUILD_NOT_FOUND')
            owner = guild['id']
            require(ctx.count(ctx.user, 'public-craft:' + ctx.day) + count <= 5, 'DAILY_LIMIT')
        if recipe['limit_group'] == 'PROP_ALL_6_PER_DAY_AND_ITEM_2':
            require(ctx.count(ctx.user, 'props:' + ctx.day) + count <= 6, 'DAILY_LIMIT')
            require(ctx.count(ctx.user, 'prop:' + recipe['output_item'] + ':' + ctx.day) + count <= 2, 'DAILY_LIMIT')
        ctx.money(ctx.user, 'CUR_STONE', -recipe['stone_cost'] * count, 'craft')
        inputs = [dict(item_id=r['item_id'], count=r['count'] * count) for r in ctx.rules.tables['items-v1.4/recipe_inputs.csv'] if r['recipe_id'] == recipe_id]
        reserved = Inventory.reserve(ctx, inputs, ctx.reservation_key)
        require(recipe['paid_input_allowed'] or not any(r['paid'] for r in reserved), 'PAID_INPUT_FORBIDDEN')
        expiry = min([ctx.window_end] + [r['expiry'] for r in reserved])
        Inventory.consume(ctx, ctx.reservation_key)
        Inventory.grant(ctx, recipe['output_item'], recipe['output_count'] * count, dict(id=ctx.request_id, expiry=expiry, paid=any(r['paid'] for r in reserved)), owner)
        if owner != ctx.user: ctx.count(ctx.user, 'public-craft:' + ctx.day, count)
        if recipe['limit_group'] == 'PROP_ALL_6_PER_DAY_AND_ITEM_2':
            ctx.count(ctx.user, 'props:' + ctx.day, count)
            ctx.count(ctx.user, 'prop:' + recipe['output_item'] + ':' + ctx.day, count)
