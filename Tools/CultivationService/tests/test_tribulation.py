from test_foundation import ServiceCase


class TribulationTests(ServiceCase):
    def test_server_feature_flag_refuses_prepare(self):
        self.rules.configs['items-v1.4/item_system.json']['profiles']['prototype_full']['tribulation']=False
        self.assertEqual(self.command(self.anchor,'PrepareBreakthrough',{'selection':[]})['error_code'],'FEATURE_DISABLED')

    def prepare(self):
        self.assertIn('PrepareBreakthrough', self.service.handlers)
        from Tools.CultivationService.inventory import Inventory
        with self.service.store.transaction() as db:
            ctx = self.service._context(db, self.anchor)
            db.execute('UPDATE growth SET realm=21,qi=10000 WHERE scope=? AND user=?', (ctx.scope, ctx.user))
            Inventory.grant(ctx, 'MAT_BR04_C01_G1', 1, {'id': 'seed'})
        result = self.command(self.anchor, 'PrepareBreakthrough', {'selection': [{'item_id':'MAT_BR04_C01_G1','count':1}]})
        self.assertEqual(result['status'], 'success', result)
        return result['result']['tribulation']

    def test_failed_attempt_keeps_materials_and_persistent_roll(self):
        self.service.random_draw = lambda _: 9999
        attempt = self.prepare()
        self.now += 76
        state = self.service.snapshot(self.anchor)
        self.assertEqual(state['tribulation']['status'], 'Failed')
        self.assertEqual(state['growth']['realm_index'], 21)
        self.assertEqual(state['growth']['qi'], 8500)
        self.assertEqual(state['inventory']['MAT_BR04_C01_G1'], 1)
        self.service.random_draw = lambda _: 0
        repeated = self.service.snapshot(self.anchor)
        self.assertEqual(repeated['tribulation']['roll'], 9999)
        self.assertEqual(repeated['growth']['qi'], 8500)

    def test_distinct_helpers_cap_and_cutoff(self):
        attempt = self.prepare()
        self.now += 16
        for index in range(51):
            token = self.session(f'helper-{index}', 'viewer')
            self.service.sync(token, {'active': True})
            for repeat in range(2):
                result = self.command(token, 'AssistTribulation', {'attempt_id': attempt['id'], 'event_id': f'event-{index}-{repeat}'}, f'assist-{index}-{repeat}')
                self.assertEqual(result['status'], 'success', result)
        state = self.service.snapshot(self.anchor)
        self.assertEqual(state['tribulation']['helper_count'], 51)
        self.assertEqual(state['tribulation']['probability_basis_points'], 10000)
        self.now += 60
        result = self.command(token, 'AssistTribulation', {'attempt_id': attempt['id'], 'event_id':'late'}, 'late')
        self.assertEqual(result['error_code'], 'ASSIST_CLOSED')
        self.assertEqual(self.service.snapshot(self.anchor)['growth']['realm_index'], 22)

    def test_prepare_cancel_and_late_cancel(self):
        attempt = self.prepare()
        self.assertEqual(self.command(self.anchor, 'CancelBreakthrough', {'attempt_id':attempt['id']})['status'], 'success')
        self.assertEqual(self.service.snapshot(self.anchor)['inventory']['MAT_BR04_C01_G1'], 1)

    def test_probability_integer_boundaries_and_pity(self):
        from importlib.util import find_spec
        self.assertIsNotNone(find_spec('Tools.CultivationService.tribulation'))
        from Tools.CultivationService.tribulation import probability
        self.assertEqual(probability(0,0),5000)
        self.assertEqual(probability(1,0),5100)
        self.assertEqual(probability(50,0),10000)
        self.assertEqual(probability(0,5),10000)
        self.assertTrue(4999 < probability(0,0))
        self.assertFalse(5000 < probability(0,0))

    def test_sixth_attempt_pity_succeeds_without_helpers(self):
        self.service.random_draw=lambda _:9999
        first=self.prepare()
        for n in range(6):
            if n:
                with self.service.store.transaction() as db:
                    ctx=self.service._context(db,self.anchor)
                    db.execute('UPDATE growth SET qi=10000 WHERE scope=? AND user=?',(ctx.scope,ctx.user))
                result=self.command(self.anchor,'PrepareBreakthrough',{'selection':[{'item_id':'MAT_BR04_C01_G1','count':1}]},f'prepare-{n}')
                self.assertEqual(result['status'],'success',result)
            self.now+=76
            state=self.service.snapshot(self.anchor)
            self.assertEqual(state['tribulation']['status'],'Succeeded' if n==5 else 'Failed')
            self.now+=61
        self.assertEqual(state['growth']['realm_index'],22)
        self.assertEqual(state['inventory'].get('MAT_BR04_C01_G1',0),0)

    def test_outage_reconciliation_does_not_extend_material_expiry(self):
        self.assertIn('ReconcileBreakthrough',self.service.handlers)
        self.service.random_draw=lambda _:9999
        attempt=self.prepare()
        with self.service.store.transaction() as db:
            db.execute('UPDATE item_instance SET expiry=? WHERE reserved=?',(self.now+84,attempt['id']))
        self.now+=16
        self.service.tick()
        self.now+=90
        state=self.service.snapshot(self.anchor)
        self.assertEqual(state['tribulation']['status'],'Reconcile')
        result=self.command(self.anchor,'ReconcileBreakthrough',{'attempt_id':attempt['id']})
        self.assertEqual(result['status'],'success',result)
        self.assertEqual(result['result']['tribulation']['status'],'Failed')
        self.assertEqual(result['result']['inventory'].get('MAT_BR04_C01_G1',0),0)
        with self.service.store.transaction() as db:
            row=db.execute('SELECT count,expiry,reserved FROM item_instance WHERE item=?',('MAT_BR04_C01_G1',)).fetchone()
            self.assertEqual(row['count'],1)
            self.assertEqual(row['expiry'],attempt['start']+84)
            self.assertIsNone(row['reserved'])
