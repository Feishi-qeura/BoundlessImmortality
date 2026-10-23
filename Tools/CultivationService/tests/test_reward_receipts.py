from test_foundation import ServiceCase
from Tools.CultivationService.inventory import Inventory
from Tools.CultivationService.growth import Growth
from Tools.CultivationService.world import World
from Tools.CultivationService.contracts import require


class RewardReceiptTests(ServiceCase):
    def receipts(self, token=None):
        return self.service.snapshot(token or self.anchor)['reward_receipts']

    def test_no_balance_inference_fixtures_trade_refunds_or_daily_placeholder(self):
        with self.service.store.transaction() as db:
            ctx = self.service._context(db, self.anchor)
            Inventory.grant(ctx, 'MAT_BREAK', 3, dict(id='fixture'))
            Inventory.grant(ctx, 'CUR_STONE', 7, dict(id='fixture'))
            Inventory.grant(ctx, 'MAT_BREAK', 1, dict(id='race-not-a-real-race:anchor:1'))
            ctx.money(ctx.user, 'STAT_CONTRIB', 5, 'world_contribution', 'race-not-a-real-race')
            Growth.add(ctx, ctx.user, 20)
            for reason in ('market_sale', 'refund', 'recycle', 'seed'):
                ctx.money(ctx.user, 'CUR_STONE', 1, reason, 'fixture')
        self.assertEqual(self.receipts(), dict(entries=[], high_watermark=0))
        self.assertEqual(self.command(self.anchor, 'SignIn')['status'], 'success')
        self.assertEqual(self.receipts(), dict(entries=[], high_watermark=0))

    def test_stage_wallet_and_items_group_and_poll_never_regrant(self):
        round_ = self.command(self.anchor, 'StartWorldRound')['result']['round']
        with self.service.store.transaction() as db:
            ctx = self.service._context(db, self.anchor)
            World.stage_reward(ctx, round_, 1)
        receipt = self.receipts()['entries'][0]
        expected = {r['item_id']: r['count'] for r in self.rules.tables['world_rewards.csv'] if r['stage'] == 1}
        self.assertEqual({r['item_id']: r['quantity'] for r in receipt['items']}, expected)
        self.assertEqual(receipt['source_label'], '世界历练')
        before = self.service.snapshot(self.anchor)
        for _ in range(3):
            self.assertEqual(self.receipts()['entries'][0], receipt)
        after = self.service.snapshot(self.anchor)
        self.assertEqual(before['wallet'], after['wallet'])
        self.assertEqual(before['inventory'], after['inventory'])
        with self.service.store.transaction() as db:
            World.stage_reward(self.service._context(db, self.anchor), round_, 1)
        self.assertEqual(len(self.receipts()['entries']), 1)

    def test_explicit_qi_grant_not_passive_growth_and_owner_isolation(self):
        viewer = self.session('reward-viewer', 'viewer')
        with self.service.store.transaction() as db:
            ctx = self.service._context(db, self.anchor)
            Growth.add(ctx, 'reward-viewer', 60, dict(id='round:complete', kind='world'))
        self.now += 10
        self.assertEqual(self.receipts()['entries'], [])
        entries = self.receipts(viewer)['entries']
        self.assertEqual(entries[0]['items'], [dict(item_id='STAT_QI', name='修为', quantity=60)])
        self.assertEqual(len(entries), 1)  # Ten seconds passive settlement is not a new popup.

    def test_craft_replay_returns_same_receipt_and_failed_craft_does_not_append(self):
        with self.service.store.transaction() as db:
            ctx = self.service._context(db, self.anchor)
            Inventory.grant(ctx, 'MAT_BREAK', 20, dict(id='seed'))
            ctx.money(ctx.user, 'CUR_STONE', 60, 'seed')
        recipe = next(r for r in self.rules.tables['items-v1.4/recipes.csv'] if r['output_item'] == 'MAT_BR01_C01_G1')
        first = self.command(self.anchor, 'Craft', dict(recipe_id=recipe['recipe_id']), 'receipt-craft')
        replay = self.command(self.anchor, 'Craft', dict(recipe_id=recipe['recipe_id']), 'receipt-craft')
        self.assertEqual(first, replay)
        self.assertEqual(len(self.receipts()['entries']), 1)
        self.assertEqual(first['result']['reward_receipts']['entries'][0]['items'][0]['quantity'], 1)
        self.assertEqual(self.command(self.anchor, 'Craft', dict(recipe_id=recipe['recipe_id']))['error_code'], 'INSUFFICIENT_FUNDS')
        self.assertEqual(len(self.receipts()['entries']), 1)

    def test_receipt_and_grant_roll_back_in_same_command_savepoint(self):
        def fail_after_grant(ctx, args):
            Inventory.grant(ctx, 'MAT_BREAK', 2, dict(id='rollback', kind='world'))
            require(False, 'TEST_FAILURE')
        self.service.handlers['TestReceiptRollback'] = fail_after_grant
        self.assertEqual(self.command(self.anchor, 'TestReceiptRollback')['error_code'], 'TEST_FAILURE')
        self.assertEqual(self.receipts(), dict(entries=[], high_watermark=0))
        self.assertEqual(self.service.snapshot(self.anchor)['inventory'].get('MAT_BREAK', 0), 0)

    def test_real_monthly_entitlement_has_one_receipt_daily_has_none(self):
        for _ in range(3):
            self.assertEqual(self.command(self.anchor, 'SignIn')['status'], 'success')
            self.now += 86400
        self.assertEqual(self.receipts()['entries'], [])
        claimed = self.command(self.anchor, 'ClaimSignInChest', dict(milestone=3), 'monthly-receipt')
        self.assertEqual(claimed['status'], 'success')
        item = self.receipts()['entries'][0]['items'][0]
        self.assertEqual(item, dict(item_id='COSMETIC_SIGN_3', name='3日签到纪念外观', quantity=1))
        self.command(self.anchor, 'ClaimSignInChest', dict(milestone=3), 'monthly-receipt')
        self.assertEqual(len(self.receipts()['entries']), 1)

    def test_positive_direct_wallet_credit_is_grouped_once_with_items(self):
        with self.service.store.transaction() as db:
            ctx = self.service._context(db, self.anchor)
            ctx.put('world', 'direct-world', dict(id='direct-world', status='Completed'))
            Inventory.grant(ctx, 'MAT_BREAK', 2, dict(id='direct-world:bonus'))
            ctx.money(ctx.user, 'CUR_STONE', 15, 'world', 'direct-world:bonus')
            ctx.money(ctx.user, 'CUR_STONE', 7, 'refund', 'direct-world:bonus')
        receipt = self.receipts()['entries'][0]
        self.assertEqual({r['item_id']:r['quantity'] for r in receipt['items']}, {'MAT_BREAK':2, 'CUR_STONE':15})
        self.assertEqual(self.receipts()['entries'][0], receipt)

    def test_projection_retention_bounded_with_monotonic_sequence(self):
        with self.service.store.transaction() as db:
            ctx = self.service._context(db, self.anchor)
            for index in range(140):
                Inventory.grant(ctx, 'MAT_BREAK', 1, dict(id='grant-'+str(index), kind='world'))
        projection = self.receipts()
        self.assertEqual(projection['high_watermark'], 140)
        self.assertEqual(len(projection['entries']), 128)
        self.assertEqual(projection['entries'][0]['sequence'], 13)
        self.assertEqual(projection['entries'][-1]['sequence'], 140)

    def test_real_matched_race_settlement_and_contribution_receipts(self):
        teams = []
        for name, room in (('甲', 'race-a'), ('乙', 'race-b')):
            anchor = self.session(name, room=room)
            guild = self.command(anchor, 'CreateGuild', dict(name='青云'+name))['result']['guild']['id']
            viewer = self.session(name+'剑灵', 'viewer', room)
            self.command(viewer, 'JoinGuild', dict(guild_id=guild))
            self.service.sync(anchor, dict(active=True, participants=[dict(token=viewer, active=True)]))
            teams.append((anchor, viewer))
        for anchor, _ in teams:
            self.assertEqual(self.command(anchor, 'QueueRace', dict(node_id='NODE_STONE'))['status'], 'success')
        race = self.service.snapshot(teams[0][0])['race']
        self.now = race['start']+119
        for anchor, viewer in teams:
            self.service.sync(anchor, dict(active=True, participants=[dict(token=viewer, active=True)]))
        self.now += 2
        for anchor, viewer in teams:
            self.service.sync(anchor, dict(active=True, participants=[dict(token=viewer, active=True)]))
            reply = self.command(viewer, 'SubmitRaceAction', dict(race_id=race['id'], kind='common', action='护阵'))
            self.assertEqual(reply['status'], 'success', reply)
        # Freeze the remaining confirmed stage inputs, then exercise the real
        # Races.advance/settle path. No synthetic world entity is ever created.
        with self.service.store.transaction() as db:
            ctx = self.service._context(db, teams[0][0])
            race = ctx.entity('race', race['id'])
            for team in race['teams']:
                team.update(common=[0,1,2,3,4,5], score=600)
                for participant in team['participants'].values():
                    participant['stages'] = [1,2,3]
            ctx.put('race', race['id'], race)
        self.now = race['start']+601
        for anchor, viewer in teams:
            state = self.service.snapshot(anchor)
            entries = state['reward_receipts']['entries']
            stages = [entry for entry in entries if entry['source_id'].rsplit(':',1)[-1] in ('1','2','3')]
            self.assertEqual(len(stages), 3)
            for stage, receipt in enumerate(stages, 1):
                expected = {r['item_id']:r['count'] for r in self.rules.tables['world_rewards.csv'] if r['stage']==stage}
                self.assertEqual({item['item_id']:item['quantity'] for item in receipt['items']}, expected)
                self.assertEqual(receipt['source_label'], '灵脉竞采')
            viewer_entries = self.receipts(viewer)['entries']
            contribution = [entry for entry in viewer_entries if entry['source_id']=='race-'+race['id']]
            self.assertEqual(contribution[0]['items'][0]['quantity'], 5)
            self.assertEqual(contribution[0]['source_label'], '灵脉竞采')
            complete = [entry for entry in viewer_entries if entry['source_id'].endswith(':complete')]
            self.assertEqual(complete[0]['source_label'], '灵脉竞采')
            self.assertEqual(complete[0]['items'][0]['quantity'], 60)
            self.assertEqual(self.service.snapshot(anchor)['reward_receipts']['entries'], entries)
