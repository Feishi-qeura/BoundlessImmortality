"""One transaction boundary for all new-rule commands and durable receipts."""
import hashlib
import math
import time
import secrets
from .contracts import CommandContext, RuleError, require, encode, integer, uid
from .store import Store
from .guilds import Guilds
from .growth import Growth
from .crafting import Crafting
from .inventory import Inventory
from .breakthrough import Breakthrough
from .world import World
from .tribulation import Tribulation
from .market import Market
from .shop import Shop
from .construction import Construction
from .activities import Activities
from .launch_event import LaunchEvent
from .leases import Leases
from .races import Races
from .focus import Focus
from .loadouts import Loadouts
from .online_intervals import OnlineIntervals
from .presentation import Presentation
from .journey import Journey
from .effects import Effects
from .combat import Combat
from .catalog import Catalog
from .reward_receipts import RewardReceipts


class Authority:
    def __init__(self, db, rules, clock=time.time):
        self.store, self.rules, self.clock = Store(db), rules, clock
        self.random_draw = secrets.randbelow
        self.handlers = {
            'Craft': lambda c,a: Crafting.craft(c, a.get('recipe_id'), a.get('count', 1)),
            'BindMaterial': Inventory.bind,
            'StartWorldRound': lambda c,a: World.start(c, a.get('preset', 'ENC_SOLO'), a.get('route', 'guard'), a.get('focus'), a.get('region_id', 'qingyun')),
            'SubmitAction': lambda c,a: World.submit(c, a.get('round_id'), a.get('action'), a.get('target_id'),a.get('choice')),
            'InterruptWorldRound': lambda c,a: World.interrupt(c, a.get('round_id')),
            'ClaimSettlement': lambda c,a: None,
            'Breakthrough': Breakthrough.minor,
            'PrepareBreakthrough': Tribulation.prepare,
            'AssistTribulation': Tribulation.assist,
            'CancelBreakthrough': Tribulation.cancel,
            'ReconcileBreakthrough': Tribulation.reconcile,
            'ReworkQuality': Breakthrough.rework,
            'QuoteBreakthrough': Breakthrough.preview,
            'QuoteReworkQuality':Breakthrough.rework_preview,
            'CreateListing': Market.create, 'BuyListing': Market.buy, 'CancelListing': Market.cancel,
            'Purchase': Shop.purchase, 'Recycle': Shop.recycle,
            'Build': Construction.build, 'SignIn': Activities.sign_in, 'ClaimSignInChest': Activities.claim_chest,
            'StartLaunchEvent': LaunchEvent.start,
            'QueueRace': Races.queue, 'CancelRaceQueue': Races.cancel, 'SubmitRaceAction': Races.submit,
            'ChooseFocus': Focus.choose, 'Equip': Loadouts.equip, 'Unequip':Loadouts.unequip, 'LearnMethod': Loadouts.learn,
            'ConfigureMethods': Loadouts.configure,
            'ActivateFabao': lambda c,a: Effects.activate(c,a,True), 'UseProp': Effects.activate,
            'ActivateMethod': Combat.activate_method,
            'QueryCatalog': Catalog.query,
            'StartActivity': Activities.start, 'EndActivity': Activities.end,
            'SelectHonorTitle':LaunchEvent.select_honor,
            'AcknowledgeOfflineReward':Growth.acknowledge_offline,
        }

    def _now(self):
        now = float(self.clock())
        require(math.isfinite(now), 'INVALID_CLOCK')
        return now

    def _context(self, db, token, now=None):
        row = db.execute('SELECT s.*,i.role,i.name FROM session s JOIN identity i USING(scope,user) WHERE token=? AND revoked=0', (token,)).fetchone()
        require(row is not None, 'UNAUTHENTICATED')
        ctx = CommandContext(db, self, dict(row), self._now() if now is None else now)
        scope = db.execute('SELECT * FROM namespace WHERE id=?', (ctx.scope,)).fetchone()
        require(scope['rules_hash'] == self.rules.sha256, 'RULE_HASH_MISMATCH')
        require(ctx.now < scope['end'], 'ASSET_EXPIRED')
        return ctx

    def open_session(self, body):
        require(body.get('development_only') is True and body.get('simulated') is True and body.get('profile') == 'prototype_full', 'FEATURE_DISABLED')
        require(body.get('role') in ('anchor', 'viewer'), 'INVALID_ROLE')
        for key in ('app', 'room', 'user'):
            require(isinstance(body.get(key), str) and 0 < len(body[key]) <= 256 and not any(ord(c) < 32 for c in body[key]), 'INVALID_IDENTITY')
        now = self._now()
        scope = hashlib.sha256((body['app'] + '|prototype_full|' + self.rules.version).encode()).hexdigest()
        with self.store.transaction() as db:
            db.execute('INSERT OR IGNORE INTO namespace VALUES(?,?,?,?,?,?,?,0)',
                       (scope, body['app'], 'prototype_full', self.rules.version, self.rules.sha256, now, now + 2592000))
            existing = db.execute('SELECT * FROM identity WHERE scope=? AND user=?', (scope, body['user'])).fetchone()
            require(not existing or existing['role'] == body['role'], 'IDENTITY_ROLE_CONFLICT')
            old = db.execute('SELECT token,generation FROM session WHERE scope=? AND user=? AND revoked=0', (scope, body['user'])).fetchone()
            if old:
                prior=self._context(db,old['token'],now)
                Growth.settle(prior,now);OnlineIntervals.record(prior,False)
            db.execute('UPDATE session SET revoked=1,active=0 WHERE scope=? AND user=?', (scope, body['user']))
            db.execute('INSERT INTO identity VALUES(?,?,?,?) ON CONFLICT(scope,user) DO UPDATE SET name=excluded.name',
                       (scope, body['user'], body['role'], str(body.get('display_name', body['user']))[:128]))
            db.execute('INSERT OR IGNORE INTO growth(scope,user,cursor) VALUES(?,?,?)', (scope, body['user'], now))
            token = uid() + uid()
            db.execute('INSERT INTO session VALUES(?,?,?,?,?,0,?,0)', (token, scope, body['user'], body['room'], old['generation'] + 1 if old else 1, now))
            ctx = self._context(db, token, now)
            return dict(token=token, state=self._snapshot(ctx))

    def sync(self, token, body):
        with self.store.transaction() as db:
            ctx = self._context(db, token)
            Leases.settle(ctx)
            Growth.settle(ctx, ctx.now)
            active = body.get('active', False)
            require(type(active) is bool, 'INVALID_ARGUMENT')
            OnlineIntervals.record(ctx,active)
            if active and (not ctx.session['active'] or ctx.session['heartbeat']<ctx.now-5):
                db.execute('UPDATE growth SET offline_seconds=0 WHERE scope=? AND user=?', (ctx.scope, ctx.user))
            db.execute('UPDATE session SET active=?,heartbeat=? WHERE token=?', (int(active), ctx.now, token))
            if active:db.execute('INSERT OR IGNORE INTO visit VALUES(?,?,?,?)',(ctx.scope,ctx.user,ctx.day,ctx.now))
            revoked = []
            for person in body.get('participants', []):
                try:
                    other = self._context(db, person.get('token', ''), ctx.now)
                except ValueError:
                    prior=db.execute('SELECT user FROM session WHERE token=? AND scope=? AND room=? AND revoked=1',(person.get('token',''),ctx.scope,ctx.room)).fetchone()
                    if prior:revoked.append(prior['user'])
                    continue
                require(ctx.role == 'anchor' and other.scope == ctx.scope and other.room == ctx.room and other.role == 'viewer', 'FORBIDDEN')
                Growth.settle(other, ctx.now)
                present = person.get('active') is True and active
                OnlineIntervals.record(other,present)
                if present and (not other.session['active'] or other.session['heartbeat']<ctx.now-5):
                    db.execute('UPDATE growth SET offline_seconds=0 WHERE scope=? AND user=?', (other.scope, other.user))
                db.execute('UPDATE session SET active=?,heartbeat=? WHERE token=?', (int(present), ctx.now, person['token']))
                if present:db.execute('INSERT OR IGNORE INTO visit VALUES(?,?,?,?)',(other.scope,other.user,other.day,ctx.now))
            ctx = self._context(db, token, ctx.now)
            self._advance(ctx)
            state = self._snapshot(ctx, body.get('cursor', 0))
            state['revoked_participants'] = revoked
            return state

    def snapshot(self, token, cursor=0):
        with self.store.transaction() as db:
            ctx = self._context(db, token)
            Growth.settle(ctx, ctx.now)
            self._advance(ctx)
            return self._snapshot(ctx, cursor)

    def _advance(self, ctx):
        World.advance(ctx)
        Tribulation.advance(ctx)
        Market.expire(ctx)
        Races.advance(ctx)
        Leases.settle(ctx)
        LaunchEvent.advance(ctx)

    def tick(self):
        with self.store.transaction() as db:
            now=self._now()
            scopes={}
            for row in db.execute('SELECT s.*,i.role,i.name FROM session s JOIN identity i USING(scope,user) JOIN namespace n ON n.id=s.scope WHERE s.revoked=0 AND n.end>?',(now,)).fetchall():
                frozen=db.execute('SELECT rules_hash FROM namespace WHERE id=?',(row['scope'],)).fetchone()
                if frozen['rules_hash']!=self.rules.sha256:continue
                ctx=CommandContext(db,self,dict(row),now)
                Growth.settle(ctx,now)
                scopes[ctx.scope]=ctx
            for ctx in scopes.values():self._advance(ctx)

    def online_bonus(self,ctx):
        return Leases.qi_bonus(ctx)+LaunchEvent.bonus(ctx)

    def online_bonus_seconds(self,ctx,start,end):
        return OnlineIntervals.bonus_seconds(ctx,start,end)

    def _snapshot(self, ctx, cursor=0):
        cursor = integer(cursor, 0, 1000000)
        scope = dict(ctx.db.execute('SELECT * FROM namespace WHERE id=?', (ctx.scope,)).fetchone())
        guild = Guilds.current(ctx)
        members, participating, helpers, total = [], 0, 0, 0
        if guild:
            total = ctx.db.execute('SELECT count(*) FROM membership WHERE scope=? AND guild_id=?', (ctx.scope, guild['id'])).fetchone()[0]
            members = [dict(r) for r in ctx.db.execute('SELECT m.user,i.name FROM membership m JOIN identity i USING(scope,user) WHERE m.scope=? AND guild_id=? ORDER BY m.user LIMIT 20 OFFSET ?', (ctx.scope, guild['id'], cursor))]
            guild['supply'] = ctx.money(guild['id'], 'CUR_SUPPLY')
            guild['reputation'] = ctx.money(guild['id'], 'STAT_REPUTATION')
            for row in ctx.db.execute('SELECT s.user,m.guild_id FROM session s LEFT JOIN membership m USING(scope,user) JOIN identity i USING(scope,user) WHERE s.scope=? AND s.room=? AND s.revoked=0 AND s.active=1 AND s.heartbeat>=? AND i.role=\'viewer\'', (ctx.scope, ctx.room, ctx.now - 5)):
                if row['guild_id'] == guild['id']: participating += 1
                else: helpers += 1
        wallet = {r['currency']: r['amount'] for r in ctx.db.execute('SELECT * FROM wallet WHERE scope=? AND owner=?', (ctx.scope, ctx.user))}
        inventory = {r['item']: r['n'] for r in ctx.db.execute('SELECT item,sum(count) n FROM item_instance WHERE scope=? AND owner=? AND expiry>? AND reserved IS NULL GROUP BY item', (ctx.scope, ctx.user, ctx.now))}
        state = dict(version=scope['version'], server_utc=ctx.now, manifest=dict(rule_version=self.rules.version, rules_hash=self.rules.sha256, profile='prototype_full', window_end=scope['end']),
                     identity=dict(user=ctx.user, role=ctx.role, room=ctx.room, generation=ctx.session['generation'], active=bool(ctx.session['active'])),
                     guild=guild, counts=dict(members=total, participating_members=participating, helpers=helpers),
                     members=members, growth=Growth.snapshot(ctx), wallet=wallet, inventory=inventory,
                     round=ctx.entity('world', (ctx.entity('last_world', ctx.user) or {}).get('id', '')))
        if cursor + 20 < total: state['next_cursor'] = cursor + 20
        last_attempt = ctx.entity('last_attempt', ctx.user)
        state['tribulation'] = ctx.entity('attempt',last_attempt['id']) if last_attempt else None
        state['market']=Market.snapshot(ctx) if ctx.role=='anchor' else None
        state['activities']=Activities.snapshot(ctx)
        state['launch']=LaunchEvent.snapshot(ctx)
        state['honors']=LaunchEvent.honors(ctx)
        state['offline_reward']=ctx.entity('offline_reward',ctx.user)
        state['member_cursor']=cursor
        state['lease']=Leases.current(ctx)
        state['race']=Races.snapshot(ctx)
        state['race_queue']=ctx.entity('queue',ctx.user)
        state['focus']=ctx.entity('focus',ctx.user)
        state['loadout']=Loadouts.get(ctx)
        state['loadout']['method_names']={r['method_id']:r['name'].removesuffix('原典') for r in ctx.rules.tables['items-v1.4/method_items.csv'] if r['item_type']=='book'}
        state['guild_inventory']={r['item']:r['n'] for r in ctx.db.execute('SELECT item,sum(count) n FROM item_instance WHERE scope=? AND owner=? AND count>0 AND expiry>? AND reserved IS NULL GROUP BY item',(ctx.scope,guild['id'] if guild else '',ctx.now))}
        state['combat']=Effects.combat(ctx)
        state['presentation']=Presentation.snapshot(ctx,state['round'])
        state['journey']=Journey.snapshot(ctx)
        state['breakthrough_quote']=ctx.entity('breakthrough_quote',ctx.user)
        state['catalog']=Catalog.snapshot(ctx)
        state['reward_receipts']=RewardReceipts.snapshot(ctx)
        state['buildings']=[r for r in ctx.entities('building') if guild and r['guild_id']==guild['id']]
        state['item_instances']=[dict(r,definition=ctx.rules.items[r['item']]) for r in ctx.db.execute('SELECT id,item,count,binding,paid,source,expiry,reserved FROM item_instance WHERE scope=? AND owner=? AND count>0 AND expiry>? ORDER BY expiry,id',(ctx.scope,ctx.user,ctx.now))]
        state['records'] = []
        for person in ctx.db.execute('SELECT s.*,i.role,i.name FROM session s JOIN identity i USING(scope,user) WHERE s.scope=? AND s.room=? AND s.revoked=0', (ctx.scope,ctx.room)).fetchall():
            person_ctx = ctx.for_user(person['user'])
            g = Growth.snapshot(person_ctx)
            material = ctx.db.execute('SELECT coalesce(sum(count),0) FROM item_instance WHERE scope=? AND owner=? AND item=\'MAT_BREAK\' AND reserved IS NULL AND expiry>?', (ctx.scope,person['user'],ctx.now)).fetchone()[0]
            honor=LaunchEvent.honors(person_ctx)
            caption=next((t['name'] for t in honor['owned'] if t['title_id']==honor['selected']),'')
            state['records'].append(dict(user=person['user'],role=person['role'],realm_index=g['realm_index'],realm_name=g['name'],qi=g['qi'],qi_to_next=g['qi_to_next'],material_count=material,material_cost=g['material_to_next'],rate=(1+self.online_bonus(person_ctx)) if person['active'] and person['heartbeat']>=ctx.now-5 else 0,attributes=Loadouts.attributes(person_ctx),honor_title=caption,name=person['name'],offline_reward=person_ctx.entity('offline_reward',person_ctx.user)))
        return state

    def execute(self, token, command):
        with self.store.transaction() as db:
            ctx = self._context(db, token)
            require(isinstance(command, dict), 'INVALID_ARGUMENT')
            request = command.get('request_id')
            require(isinstance(request, str) and 0 < len(request) <= 128, 'INVALID_REQUEST_ID')
            ctx.request_id = request
            version = db.execute('SELECT version FROM namespace WHERE id=?', (ctx.scope,)).fetchone()[0]
            def response(error='', receipt='', result=None):
                return dict(request_id=request, status='error' if error else 'success', error_code=error,
                            authoritative_version=version, receipt_id=receipt, result=result)
            payload = {k: v for k, v in command.items() if k != 'expected_version'}
            hash_ = hashlib.sha256(encode(payload).encode()).hexdigest()
            prior = db.execute('SELECT * FROM command_receipt WHERE scope=? AND user=? AND request=?', (ctx.scope, ctx.user, request)).fetchone()
            if prior:
                return __import__('json').loads(prior['result']) if prior['payload_hash'] == hash_ else response('REQUEST_ID_CONFLICT')
            if command.get('profile') != 'prototype_full': return response('FEATURE_DISABLED')
            if command.get('rule_version') != self.rules.version: return response('RULE_VERSION_MISMATCH')
            if type(command.get('expected_version')) is not int or command['expected_version'] != version: return response('STALE_VERSION')
            operation, args = command.get('command'), command.get('args', {})
            ctx.reward_operation = operation
            if not isinstance(args, dict): return response('INVALID_ARGUMENT')
            if set(args)&{'owner','owner_id','roll','helpers','damage','amount','score','online_seconds','occurred_at'}:return response('UNTRUSTED_FIELD')
            Growth.settle(ctx, ctx.now)
            db.execute('SAVEPOINT command_mutation')
            try:
                if operation in ('CreateGuild', 'JoinGuild', 'LeaveGuild', 'RemoveMember', 'RenameGuild', 'DissolveGuild'):
                    Guilds.apply(ctx, operation, args)
                elif operation in self.handlers:
                    self.handlers[operation](ctx, args)
                    if operation == 'ClaimSignInChest':
                        milestone = args['milestone']
                        RewardReceipts.record(ctx, ctx.user, 'COSMETIC_SIGN_'+str(milestone), 1,
                            dict(id=ctx.user+':'+ctx.day[:7]+':'+str(milestone), kind='monthly'),
                            name=str(milestone)+'日签到纪念外观')
                else:
                    raise RuleError('UNKNOWN_COMMAND')
            except RuleError as error:
                db.execute('ROLLBACK TO command_mutation')
                db.execute('RELEASE command_mutation')
                return response(str(error))
            db.execute('RELEASE command_mutation')
            ctx.fact('command-audit:'+ctx.reservation_key,'command_audit',dict(operation=operation))
            version += 1
            db.execute('UPDATE namespace SET version=? WHERE id=?', (version, ctx.scope))
            result = response(receipt=uid(), result=self._snapshot(ctx))
            db.execute('INSERT INTO command_receipt VALUES(?,?,?,?,?,?)', (ctx.scope, ctx.user, request, hash_, encode(result), ctx.now))
            return result
