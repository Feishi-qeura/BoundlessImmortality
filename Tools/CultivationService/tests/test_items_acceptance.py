from test_foundation import ServiceCase


class ItemAcceptanceTests(ServiceCase):
    def seed(self,items,realm=8):
        from Tools.CultivationService.inventory import Inventory
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,self.anchor)
            db.execute('UPDATE growth SET realm=?,qi=100000 WHERE scope=? AND user=?',(realm,ctx.scope,ctx.user))
            for item,count in items.items():Inventory.grant(ctx,item,count,dict(id='declared-test-fixture',expiry=ctx.now+1000))

    def test_mixed_materials_floor_once_and_core_commit_does_not_charge_old_cost(self):
        self.seed({'MAT_BR01_C01_G1':1,'MAT_BR01_G01_G1':2,'MAT_BR01_G02_G2':1,'MAT_BREAK':7,'PROP_02':1})
        selection=[dict(item_id=item,count=count) for item,count in {'MAT_BR01_C01_G1':1,'MAT_BR01_G01_G1':2,'MAT_BR01_G02_G2':1}.items()]
        result=self.command(self.anchor,'PrepareBreakthrough',{'selection':selection})
        self.assertEqual(result['status'],'success',result)
        state=result['result'];self.assertEqual(state['tribulation']['quote']['q'],14)
        self.assertEqual(state['growth']['realm_index'],9)
        self.assertEqual(state['inventory']['MAT_BREAK'],7)
        self.assertEqual(state['wallet'].get('CUR_STONE',0),0)
        self.assertEqual(state['combat']['weakness_until']-self.now,44)
        used=self.command(self.anchor,'UseProp',dict(item_id='PROP_02'))
        self.assertEqual(used['status'],'success',used)
        self.assertEqual(used['result']['combat']['weakness_until']-self.now,34)

    def test_rework_keeps_core_score_without_rerolling_or_multiplying_old_quality(self):
        self.seed({'MAT_BR01_C01_G1':1,'MAT_BR01_G01_G4':3,'MAT_BR01_E01_G4':3})
        first=self.command(self.anchor,'PrepareBreakthrough',dict(selection=[dict(item_id='MAT_BR01_C01_G1',count=1)]))
        self.assertEqual(first['status'],'success',first)
        self.service.random_draw=lambda _: (_ for _ in ()).throw(AssertionError('rework must never draw'))
        selection=[dict(item_id='MAT_BR01_G01_G4',count=3),dict(item_id='MAT_BR01_E01_G4',count=3)]
        result=self.command(self.anchor,'ReworkQuality',dict(selection=selection))
        self.assertEqual(result['status'],'success',result)
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,self.anchor);quality=ctx.entity('quality',ctx.user+':BR01')
            self.assertEqual(quality['core_score'],10);self.assertEqual(quality['q'],55)
            self.assertEqual(quality['band']['live_snapshot_factor'],1.02)
        self.assertEqual(result['result']['growth']['realm_index'],9)

    def test_missing_cleansable_status_preserves_prop_and_cooldown(self):
        self.seed({'PROP_01':1})
        result=self.command(self.anchor,'UseProp',dict(item_id='PROP_01'))
        self.assertEqual(result['error_code'],'NO_TARGET')
        state=self.service.snapshot(self.anchor)
        self.assertEqual(state['inventory']['PROP_01'],1)
        self.assertEqual(state['combat']['cooldowns'],{})

    def test_unqualified_prepare_reserves_nothing(self):
        self.seed({'MAT_BR01_C01_G1':1})
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,self.anchor)
            ctx.put('combat',ctx.user,dict(health=49,max_health=100,last_combat=0,safe=True))
        result=self.command(self.anchor,'PrepareBreakthrough',dict(selection=[dict(item_id='MAT_BR01_C01_G1',count=1)]))
        self.assertEqual(result['error_code'],'NOT_SAFE')
        self.assertIsNone(self.service.snapshot(self.anchor)['item_instances'][0]['reserved'])

    def test_all_new_bound_items_and_public_materials_are_rejected_by_market(self):
        from Tools.CultivationService.market import Market
        from Tools.CultivationService.contracts import RuleError
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,self.anchor)
            for item in self.rules.items:
                if item in ('MAT_HERB_T','MAT_IRON_T'):continue
                with self.subTest(item=item),self.assertRaisesRegex(RuleError,'ITEM_NOT_TRADEABLE'):
                    Market.create(ctx,dict(item_id=item,count=1,unit_price=4))
