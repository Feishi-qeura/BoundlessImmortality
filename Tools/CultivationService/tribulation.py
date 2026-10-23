from .contracts import require, uid
from .growth import Growth
from .inventory import Inventory
from .breakthrough import Breakthrough


def probability(helpers, failures):
    return min(10000, 5000 + min(50, helpers) * 100 + min(5, failures) * 1000)


class Tribulation:
    @staticmethod
    def prepare(ctx, args):
        ctx.anchor_only()
        require(ctx.rules.configs['items-v1.4/item_system.json']['profiles']['prototype_full']['tribulation'],'FEATURE_DISABLED')
        require(not ctx.entity('activity', ctx.user), 'ACTIVITY_BUSY')
        previous = ctx.entity('last_attempt', ctx.user)
        if previous:
            prior = ctx.entity('attempt', previous['id'])
            require(prior['status'] == 'Cancelled' or ctx.now >= prior.get('settled_at',0) + 60, 'RETRY_WAIT')
        growth = Growth.snapshot(ctx)
        require(growth['qi'] >= growth['qi_to_next'] > 0, 'INSUFFICIENT_QI')
        combat = ctx.entity('combat', ctx.user, dict(health=1000, max_health=1000, last_combat=0, safe=True))
        require(combat['health'] >= combat['max_health'] * .5 and combat['safe'] and ctx.now - combat['last_combat'] >= 15, 'NOT_SAFE')
        quote = Breakthrough.quote(ctx, args.get('selection', []))
        id_ = uid()
        rows = Inventory.reserve(ctx, quote['selection'], id_)
        require(all(r['expiry'] >= ctx.now + 83 for r in rows), 'MATERIAL_EXPIRING')
        pity_key = ctx.user + ':' + quote['stage']['stage_id']
        attempt = dict(id=id_, anchor=ctx.user, room=ctx.room, stage=quote['stage']['stage_id'], status='Prepare', start=ctx.now,
                       trial_start=ctx.now+15, deadline=ctx.now+75, cleanup_at=ctx.now+83, quote=quote, reserved_qi=growth['qi_to_next'],
                       helper_ids=[], helper_count=0, failures=ctx.count(ctx.user, 'pity:' + pity_key),
                       probability_basis_points=probability(0, ctx.count(ctx.user,'pity:'+pity_key)),
                       roll=None, receipt_id=None, rules_hash=ctx.rules.sha256)
        ctx.put('attempt', id_, attempt)
        ctx.put('last_attempt', ctx.user, dict(id=id_))
        ctx.put('activity', ctx.user, dict(kind='tribulation', id=id_))
        if not quote['stage']['tribulation']:
            Tribulation.commit(ctx, attempt, True)
            attempt['cleanup_at'] = ctx.now
            ctx.put('attempt', id_, attempt)
            ctx.put('activity', ctx.user, None)

    @staticmethod
    def assist(ctx, args):
        attempt = ctx.entity('attempt', args.get('attempt_id'))
        require(attempt and attempt['room'] == ctx.room and ctx.now >= attempt['trial_start'] and ctx.now < attempt['deadline'] and attempt['status'] in ('Prepare','Trial'), 'ASSIST_CLOSED')
        require(ctx.role == 'viewer' and ctx.user != attempt['anchor'], 'SELF_ASSIST_FORBIDDEN')
        require(ctx.session['active'] and ctx.session['heartbeat'] >= ctx.now - 5, 'PARTICIPATION_REQUIRED')
        event = args.get('event_id')
        require(isinstance(event,str) and 0 < len(event) <= 128, 'INVALID_EVENT')
        if ctx.user not in attempt['helper_ids']:
            ctx.fact(f"assist:{attempt['id']}:{ctx.user}", 'tribulation_assist', dict(attempt_id=attempt['id'], event_id=event))
            attempt['helper_ids'].append(ctx.user)
            attempt['helper_count'] = len(attempt['helper_ids'])
            attempt['probability_basis_points'] = probability(attempt['helper_count'], attempt['failures'])
            ctx.put('attempt', attempt['id'], attempt)

    @staticmethod
    def cancel(ctx, args):
        ctx.anchor_only()
        attempt = ctx.entity('attempt', args.get('attempt_id'))
        require(attempt and attempt['anchor'] == ctx.user and attempt['status'] == 'Prepare' and ctx.now < attempt['trial_start'], 'CANNOT_CANCEL')
        Inventory.release(ctx, attempt['id'])
        attempt['status'] = 'Cancelled'
        ctx.put('attempt', attempt['id'], attempt)
        ctx.put('activity', ctx.user, None)

    @staticmethod
    def reconcile(ctx,args):
        ctx.anchor_only()
        attempt=ctx.entity('attempt',args.get('attempt_id'))
        require(attempt and attempt['anchor']==ctx.user,'FORBIDDEN')
        if attempt['receipt_id']:return
        require(attempt['status']=='Reconcile','PENDING_RECONCILE')
        rows=ctx.db.execute('SELECT expiry,count FROM item_instance WHERE scope=? AND owner=? AND reserved=?',(ctx.scope,ctx.user,attempt['id'])).fetchall()
        require(rows and all(r['count']>0 and r['expiry']>=attempt['cleanup_at'] for r in rows),'PENDING_RECONCILE')
        # Original qualification was frozen during Trial; reconciliation neither renews nor copies an expired stack.
        if attempt['roll'] is None:attempt['roll']=ctx.authority.random_draw(10000)
        attempt['probability_basis_points']=probability(len(attempt['helper_ids']),attempt['failures'])
        Tribulation.commit(ctx,attempt,attempt['roll']<attempt['probability_basis_points'])
        ctx.put('activity',ctx.user,None)

    @staticmethod
    def commit(ctx, attempt, success):
        if attempt['receipt_id']: return
        growth = Growth.snapshot(ctx)
        if success:
            require(growth['qi'] >= attempt['reserved_qi'], 'PENDING_RECONCILE')
            Inventory.consume(ctx, attempt['id'])
            ctx.db.execute('UPDATE growth SET qi=qi-?,realm=realm+1 WHERE scope=? AND user=?', (attempt['reserved_qi'], ctx.scope, ctx.user))
            ctx.put('quality', ctx.user+':'+attempt['stage'], attempt['quote'])
            from .effects import Effects
            state=Effects.combat(ctx);state['weakness_until']=ctx.now+attempt['quote']['weakness_seconds'];ctx.put('combat',ctx.user,state)
            ctx.count(ctx.user, 'pity:'+ctx.user+':'+attempt['stage'], -attempt['failures'])
            attempt['status'] = 'Succeeded'
        else:
            Inventory.release(ctx, attempt['id'])
            loss = min(1500, growth['qi'])
            ctx.db.execute('UPDATE growth SET qi=qi-? WHERE scope=? AND user=?', (loss, ctx.scope, ctx.user))
            ctx.count(ctx.user, 'pity:'+ctx.user+':'+attempt['stage'], 1)
            attempt['status'], attempt['loss_qi'] = 'Failed', loss
        attempt['receipt_id'], attempt['settled_at'] = uid(), ctx.now
        attempt['reserved_qi'] = 0
        ctx.fact('tribulation-result:'+attempt['id'], 'tribulation_result', attempt)
        ctx.put('attempt', attempt['id'], attempt)

    @staticmethod
    def advance(ctx):
        for attempt in ctx.entities('attempt'):
            if attempt['status'] == 'Cancelled': continue
            owner = ctx.for_user(attempt['anchor'])
            if attempt['status'] == 'Prepare' and ctx.now >= attempt['trial_start']:
                rows = ctx.db.execute('SELECT expiry FROM item_instance WHERE scope=? AND reserved=? AND count>0', (ctx.scope,attempt['id'])).fetchall()
                if not rows or any(row[0] <= ctx.now for row in rows):
                    Inventory.release(ctx, attempt['id'],attempt['anchor'])
                    attempt['status'] = 'Cancelled'
                    ctx.put('activity', attempt['anchor'], None)
                else:
                    attempt['status'] = 'Trial'
            if attempt['status'] == 'Trial' and ctx.now >= attempt['deadline']:
                # A prolonged outage beyond original material expiry must be reconciled explicitly.
                expiry = ctx.db.execute('SELECT min(expiry) FROM item_instance WHERE scope=? AND reserved=? AND count>0',(ctx.scope,attempt['id'])).fetchone()[0]
                if expiry is None or expiry <= ctx.now:
                    attempt['status'] = 'Reconcile'
                else:
                    if attempt['roll'] is None: attempt['roll'] = ctx.authority.random_draw(10000)
                    attempt['probability_basis_points'] = probability(len(attempt['helper_ids']), attempt['failures'])
                    Tribulation.commit(owner, attempt, attempt['roll'] < attempt['probability_basis_points'])
            if attempt['status'] in ('Succeeded','Failed') and ctx.now >= attempt['cleanup_at']:
                activity = ctx.entity('activity', attempt['anchor'])
                if activity and activity['id'] == attempt['id']: ctx.put('activity', attempt['anchor'], None)
            ctx.put('attempt', attempt['id'], attempt)
