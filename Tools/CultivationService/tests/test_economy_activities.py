from test_foundation import ServiceCase


class EconomyTests(ServiceCase):
    def seed(self, token, stone=0, items=None):
        from Tools.CultivationService.inventory import Inventory
        with self.service.store.transaction() as db:
            ctx = self.service._context(db, token)
            if stone: ctx.money(ctx.user, 'CUR_STONE', stone, 'test-fixture')
            for item, count in (items or {}).items(): Inventory.grant(ctx,item,count,dict(id='fixture'))

    def test_market_conserves_money_and_items(self):
        self.assertIn('CreateListing',self.service.handlers)
        buyer = self.session('buyer',room='room-b')
        self.seed(self.anchor,10,{'MAT_HERB_T':10})
        self.seed(buyer,100)
        listed=self.command(self.anchor,'CreateListing',{'item_id':'MAT_HERB_T','count':10,'unit_price':8})
        self.assertEqual(listed['status'],'success',listed)
        listing=listed['result']['market']['own_listings'][0]
        result=self.command(buyer,'BuyListing',{'listing_id':listing['id'],'listing_version':0},'buy-one')
        self.assertEqual(result['status'],'success',result)
        self.assertEqual(result['result']['wallet']['CUR_STONE'],20)
        seller=self.service.snapshot(self.anchor)
        self.assertEqual(seller['wallet']['CUR_STONE'],85)
        self.assertEqual(result['result']['inventory']['MAT_HERB_T'],10)
        self.assertEqual(self.command(buyer,'BuyListing',{'listing_id':listing['id'],'listing_version':0})['error_code'],'LISTING_CLOSED')

    def test_daily_sign_once_and_new_hong_kong_day(self):
        viewer=self.session('viewer','viewer')
        one=self.command(viewer,'SignIn')
        self.assertEqual(one['status'],'success',one)
        self.assertEqual(one['result']['growth']['total_qi'],0)
        self.assertEqual(self.command(viewer,'SignIn')['error_code'],'ALREADY_SETTLED')
        self.now+=86400
        two=self.command(viewer,'SignIn')
        self.assertEqual(two['status'],'success',two)
        self.assertEqual(two['result']['activities']['month_sign_count'],2)

    def test_shop_daily_limits_and_atomic_costs(self):
        self.assertIn('Purchase',self.service.handlers)
        self.seed(self.anchor,200,{'MAT_HERB_B':4})
        for i in range(2):
            self.assertEqual(self.command(self.anchor,'Purchase',{'sku':'SKU_HEAL'},f'heal-{i}')['status'],'success')
        self.assertEqual(self.command(self.anchor,'Purchase',{'sku':'SKU_HEAL'})['error_code'],'DAILY_LIMIT')
        state=self.service.snapshot(self.anchor)
        self.assertEqual(state['wallet']['CUR_STONE'],100)
        self.assertEqual(state['inventory']['ITEM_HEAL'],2)

    def test_client_request_id_cannot_consume_market_escrow(self):
        self.assertIn('CreateListing',self.service.handlers)
        buyer=self.session('buyer',room='room-b')
        self.seed(self.anchor,10,{'MAT_HERB_T':11})
        self.seed(buyer,100)
        listed=self.command(self.anchor,'CreateListing',{'item_id':'MAT_HERB_T','count':10,'unit_price':8})
        listing=listed['result']['market']['own_listings'][0]
        self.command(self.anchor,'BindMaterial',{'item_id':'MAT_HERB_T','count':1},'market:'+listing['id'])
        bought=self.command(buyer,'BuyListing',{'listing_id':listing['id'],'listing_version':0})
        self.assertEqual(bought['status'],'success',bought)
        self.assertEqual(bought['result']['inventory'].get('MAT_HERB_T',0),10)

    def test_building_debits_public_owner_once(self):
        self.assertIn('Build',self.service.handlers)
        gid=self.create()
        from Tools.CultivationService.inventory import Inventory
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,self.anchor)
            ctx.money(gid,'CUR_SUPPLY',20,'fixture')
            for item in ('GUILD_MAT_01','GUILD_MAT_02'): Inventory.grant(ctx,item,2,{'id':'fixture'},gid)
        reply=self.command(self.anchor,'Build',{'building_id':'BUILD_01','target_level':1})
        self.assertEqual(reply['status'],'success',reply)
        self.assertEqual(reply['result']['guild']['supply'],0)
        self.assertEqual(self.command(self.anchor,'Build',{'building_id':'BUILD_01','target_level':1})['error_code'],'STALE_BUILDING_LEVEL')

    def test_launch_is_once_per_anchor_and_relative_window(self):
        self.create()
        result=self.command(self.anchor,'StartLaunchEvent')
        self.assertEqual(result['status'],'success',result)
        event=result['result']['launch']
        self.assertEqual(event['end']-event['start'],604800)
        self.assertEqual(self.command(self.anchor,'StartLaunchEvent')['error_code'],'ALREADY_SETTLED')
        self.assertFalse(event['rank_numeric_rewards'])
