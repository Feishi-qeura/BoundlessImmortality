from test_foundation import ServiceCase


class FocusLoadoutTests(ServiceCase):
    def test_unequip_does_not_remove_owned_item_or_reset_cooldown(self):
        self.seed({'FABAO_01':1})
        self.command(self.anchor,'Equip',{'item_id':'FABAO_01'})
        result=self.command(self.anchor,'Unequip',{'item_id':'FABAO_01'})
        self.assertEqual(result['status'],'success',result)
        self.assertEqual(result['result']['loadout']['fabao'],'')
        self.assertEqual(result['result']['inventory']['FABAO_01'],1)

    def seed(self,items):
        from Tools.CultivationService.inventory import Inventory
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,self.anchor)
            for item,count in items.items():Inventory.grant(ctx,item,count,{'id':'fixture'})

    def test_focus_shared_daily_six_budget(self):
        self.assertIn('ChooseFocus',self.service.handlers)
        selected={'source_id':'CH_BR01_C','item_id':'MAT_BR01_C01_G1'}
        self.assertEqual(self.command(self.anchor,'ChooseFocus',selected)['status'],'success')
        from Tools.CultivationService.focus import Focus
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,self.anchor)
            for n in range(8):Focus.settle(ctx,dict(id=f'round-{n}',focus=selected))
        self.assertEqual(self.service.snapshot(self.anchor)['inventory']['MAT_BR01_C01_G1'],6)

    def test_equipment_requires_ownership_and_safe_state(self):
        self.assertIn('Equip',self.service.handlers)
        self.assertEqual(self.command(self.anchor,'Equip',{'item_id':'EQ_ROBE_T1'})['error_code'],'INSUFFICIENT_FUNDS')
        self.seed({'EQ_ROBE_T1':1})
        self.assertEqual(self.command(self.anchor,'Equip',{'item_id':'EQ_ROBE_T1'})['status'],'success')
        state=self.service.snapshot(self.anchor)
        self.assertEqual(state['loadout']['equipment']['robe'],'EQ_ROBE_T1')
        self.command(self.anchor,'StartWorldRound')
        self.assertEqual(self.command(self.anchor,'Equip',{'item_id':'EQ_ROBE_T1'})['error_code'],'ACTIVITY_BUSY')

    def test_method_book_unlock_and_duplicate_salvage(self):
        self.assertIn('LearnMethod',self.service.handlers)
        self.seed({'BOOK_METHOD_EDGE_BOOK':2})
        for n in range(2):self.assertEqual(self.command(self.anchor,'LearnMethod',{'item_id':'BOOK_METHOD_EDGE_BOOK'},f'learn-{n}')['status'],'success')
        state=self.service.snapshot(self.anchor)
        self.assertEqual(state['loadout']['methods']['METHOD_EDGE']['level'],1)
        self.assertEqual(state['inventory']['MAT_PAGE'],2)
        self.assertEqual(self.command(self.anchor,'ConfigureMethods',{'main':'METHOD_EDGE','supports':['METHOD_EDGE']})['error_code'],'DUPLICATE_METHOD')

    def test_full_health_fabao_does_not_consume_cooldown(self):
        self.assertIn('ActivateFabao',self.service.handlers)
        self.seed({'FABAO_01':1})
        self.command(self.anchor,'Equip',{'item_id':'FABAO_01'})
        self.assertEqual(self.command(self.anchor,'ActivateFabao')['error_code'],'NO_TARGET')
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,self.anchor)
            ctx.put('combat',ctx.user,dict(health=500,max_health=1000,last_combat=0,safe=True,statuses={},effects={},cooldowns={}))
        reply=self.command(self.anchor,'ActivateFabao')
        self.assertEqual(reply['status'],'success',reply)
        self.assertEqual(reply['result']['combat']['health'],600)
        self.assertEqual(self.command(self.anchor,'ActivateFabao')['error_code'],'COOLDOWN')
