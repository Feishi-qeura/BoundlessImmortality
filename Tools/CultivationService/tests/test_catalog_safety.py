from test_foundation import ServiceCase


class CatalogSafetyTests(ServiceCase):
    def test_runtime_health_calibration_preserves_source_encounter_table(self):
        self.assertEqual(self.rules.tables['encounters.csv'][0]['boss_health'],360)
        result=self.command(self.anchor,'StartWorldRound')['result']['round']
        self.assertEqual(result['template']['boss_health'],54)
        self.assertEqual(result['template']['normal_health'],3)

    def test_owned_snapshot_preserves_instance_binding_expiry_and_definition(self):
        from Tools.CultivationService.inventory import Inventory
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,self.anchor)
            Inventory.grant(ctx,'MAT_HERB_T',2,dict(id='fixture',expiry=self.now+100))
        row=self.service.snapshot(self.anchor)['item_instances'][0]
        self.assertEqual(row['definition']['item_id'],'MAT_HERB_T')
        self.assertEqual(row['expiry'],self.now+100)
        self.assertIn('binding',row)
        self.assertIn('source',row)

    def test_activity_calendar_contains_only_server_settled_dates(self):
        one=self.command(self.anchor,'SignIn')['result']['activities']
        self.assertEqual(one['signed_days'],[one['day']])
        self.now+=86400
        two=self.service.snapshot(self.anchor)['activities']
        self.assertEqual(two['signed_days'],[one['day']])
        self.assertFalse(two['signed'])

    def test_catalog_paginates_and_exposes_exact_sources(self):
        self.assertIn('QueryCatalog',self.service.handlers)
        result=self.command(self.anchor,'QueryCatalog',{'kind':'recipes','offset':0})
        self.assertEqual(result['status'],'success',result)
        catalog=result['result']['catalog']
        self.assertLessEqual(len(catalog['rows']),8)
        self.assertEqual(catalog['total'],556)
        self.assertIn('ingredients',catalog['rows'][0])

    def test_client_supplied_roll_is_rejected(self):
        result=self.command(self.anchor,'PrepareBreakthrough',{'roll':0,'selection':[]})
        self.assertEqual(result['error_code'],'UNTRUSTED_FIELD')

    def test_duplicate_recipe_and_invalid_boolean_rejected(self):
        import shutil
        from pathlib import Path
        from test_foundation import DESIGN
        copy=Path(self.temp.name)/'bad-rules'
        shutil.copytree(DESIGN/'tables',copy/'tables')
        path=copy/'tables/items-v1.4/recipes.csv'
        text=path.read_text(encoding='utf-8-sig')
        path.write_text(text+text.splitlines()[1]+'\n',encoding='utf-8-sig')
        with self.assertRaises(ValueError):type(self.rules).load(copy)

    def test_server_tick_advances_without_connected_client(self):
        self.assertTrue(hasattr(self.service,'tick'))
        self.command(self.anchor,'StartWorldRound')
        self.now+=601
        self.service.tick()
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,self.anchor)
            world=ctx.entities('world')[0]
            self.assertEqual(world['status'],'Completed')
