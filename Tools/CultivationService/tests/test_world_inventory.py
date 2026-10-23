from test_foundation import ServiceCase


class WorldInventoryTests(ServiceCase):
    def test_action_request_cannot_preempt_stage_reward_fact_key(self):
        from Tools.CultivationService.world import World
        self.command(self.anchor,'StartWorldRound');self.now+=61
        self.service.sync(self.anchor,{'active':True})
        world=self.service.snapshot(self.anchor)['round']
        key='world:'+world['id']+':stage:1'
        self.assertEqual(self.command(self.anchor,'SubmitAction',dict(round_id=world['id'],action='护阵'),key)['status'],'success')
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,self.anchor);World.stage_reward(ctx,world,1)
        self.assertGreater(self.service.snapshot(self.anchor)['wallet'].get('CUR_STONE',0),0)

    def test_inventory_craft_atomic_and_expiry(self):
        self.assertIn('Craft', self.service.handlers)
        from Tools.CultivationService.inventory import Inventory
        with self.service.store.transaction() as db:
            ctx = self.service._context(db, self.anchor)
            Inventory.grant(ctx, 'MAT_BREAK', 20, {'id': 'seed', 'expiry': self.now + 1000})
            ctx.money(ctx.user, 'CUR_STONE', 60, 'seed')
        recipe = next(r for r in self.rules.tables['items-v1.4/recipes.csv'] if r['output_item'] == 'MAT_BR01_C01_G1')
        result = self.command(self.anchor, 'Craft', {'recipe_id': recipe['recipe_id']}, 'craft-one')
        self.assertEqual(result['status'], 'success', result)
        self.assertEqual(result['result']['wallet']['CUR_STONE'], 0)
        self.assertEqual(result['result']['inventory'].get('MAT_BREAK', 0), 0)
        self.assertEqual(result['result']['inventory']['MAT_BR01_C01_G1'], 1)
        before = self.service.snapshot(self.anchor)['inventory']
        failure = self.command(self.anchor, 'Craft', {'recipe_id': recipe['recipe_id']})
        self.assertEqual(failure['error_code'], 'INSUFFICIENT_FUNDS')
        self.assertEqual(self.service.snapshot(self.anchor)['inventory'], before)

    def test_binding_preserves_paid_source_and_expiry(self):
        self.assertIn('BindMaterial', self.service.handlers)
        from Tools.CultivationService.inventory import Inventory
        with self.service.store.transaction() as db:
            ctx = self.service._context(db, self.anchor)
            Inventory.grant(ctx, 'MAT_HERB_T', 2, {'id': 'seed', 'expiry': self.now + 1000, 'paid': True})
        result = self.command(self.anchor, 'BindMaterial', {'item_id': 'MAT_HERB_T', 'count': 2})
        self.assertEqual(result['status'], 'success', result)
        self.assertEqual(result['result']['inventory']['MAT_HERB_B'], 2)
        with self.service.store.transaction() as db:
            row = db.execute('SELECT * FROM item_instance WHERE item=? AND count>0', ('MAT_HERB_B',)).fetchone()
            self.assertEqual(row['expiry'], self.now + 1000)
            self.assertEqual(row['paid'], 1)

    def test_world_preparation_and_no_automatic_success(self):
        self.create()
        reply = self.command(self.anchor, 'StartWorldRound', {'preset': 'ENC_SOLO', 'route': 'guard'})
        self.assertEqual(reply['status'], 'success', reply)
        self.assertEqual(reply['result']['round']['status'], 'Preparing')
        self.now += 61
        state = self.service.snapshot(self.anchor)
        self.assertEqual(state['round']['stage'], 1)
        self.assertEqual(len(state['round']['targets']), 4)
        self.now += 600
        state = self.service.snapshot(self.anchor)
        self.assertEqual(state['round']['status'], 'Completed')
        self.assertEqual(state['wallet'].get('CUR_STONE', 0), 0)

    def test_world_complete_fixed_rewards_once(self):
        self.create()
        self.command(self.anchor, 'StartWorldRound', {'preset': 'ENC_SOLO', 'route': 'guard'})
        for stage_start in (61, 241, 421):
            start = self.service.snapshot(self.anchor)['round']['start']
            self.now = start + stage_start
            self.service.sync(self.anchor, {'active': True})
            for hit in range(200):
                state = self.service.snapshot(self.anchor)
                targets = [t for t in state['round']['targets'] if t['health'] > 0]
                if not targets: break
                target = targets[0]
                self.now += 1.01
                reply = self.command(self.anchor, 'SubmitAction', {'round_id': state['round']['id'], 'action': 'strike', 'target_id': target['id']}, f'hit-{stage_start}-{hit}')
                self.assertEqual(reply['status'], 'success', reply)
                self.service.sync(self.anchor, {'active': True})
            self.assertLess(hit, 199)
        self.now = start + 601
        state = self.service.snapshot(self.anchor)
        self.assertEqual(state['wallet']['CUR_STONE'], 80)
        self.assertEqual(state['inventory']['MAT_BREAK'], 12)
        self.assertEqual(state['inventory']['MAT_HERB_T'], 1)
        self.assertEqual(state['inventory']['MAT_IRON_T'], 1)
        self.assertEqual(state['inventory']['MAT_PAGE'], 2)
        self.service.snapshot(self.anchor)
        self.assertEqual(self.service.snapshot(self.anchor)['wallet']['CUR_STONE'], 80)

    def test_partial_viewer_does_not_create_collective_reputation(self):
        gid=self.create()
        viewer=self.session('viewer','viewer')
        self.command(viewer,'JoinGuild',{'guild_id':gid})
        from Tools.CultivationService.world import World
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,self.anchor)
            round_={'id':'partial-fixture','anchor':'anchor','start':self.now-600,'completed':[1,2,3],
                    'participants':{'anchor':{'stages':[1,2,3],'role':'anchor','guild_id':gid},'viewer':{'stages':[3],'role':'viewer','guild_id':gid}}}
            World.finish(ctx,round_)
        self.assertEqual(self.service.snapshot(self.anchor)['guild']['reputation'],0)

    def test_personal_contribution_daily_cap_is_separate_from_launch_score(self):
        from Tools.CultivationService.world import World
        self.assertTrue(hasattr(World,'contribution'))
        viewer=self.session('viewer','viewer')
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,viewer)
            for n in range(8):
                round_={'id':f'contrib-{n}','start':self.now}
                for amount in (2,3,5,5,5):World.contribution(ctx,ctx.user,amount,round_)
        self.assertEqual(self.service.snapshot(viewer)['wallet']['STAT_CONTRIB'],120)
