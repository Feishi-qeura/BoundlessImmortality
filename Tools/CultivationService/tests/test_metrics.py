from test_foundation import ServiceCase
from Tools.CultivationService.inventory import Inventory


class MetricsTests(ServiceCase):
    def test_subday_active_span_and_interrupted_world_are_counted_correctly(self):
        from Tools.CultivationService.metrics import Metrics
        viewer=self.session('metrics-viewer','viewer')
        self.service.sync(viewer,{'active':True});self.now+=10
        self.service.sync(viewer,{'active':True})
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,self.anchor)
            ctx.put('world','interrupted',dict(start=self.now,status='Interrupted',completed=[1,2,3],participants={}))
        report=Metrics.export(self.db,self.now-1,self.now+1)['scopes'][0]
        self.assertEqual(report['active_viewers'],1)
        self.assertEqual(report['worlds_started'],1)
        self.assertEqual(report['worlds_completed'],0)
        self.assertEqual(report['completion_rate'],0)

    def test_participation_counts_action_time_in_cross_boundary_round(self):
        from Tools.CultivationService.metrics import Metrics
        viewer=self.session('crossday-viewer','viewer')
        self.service.sync(viewer,{'active':True})
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,viewer)
            ctx.put('world','previous-window',dict(start=self.now-10,status='Running',completed=[],participants={ctx.user:dict(role='viewer',stages=[1])}))
            ctx.fact('cross-boundary-action','world_action',dict(round_id='previous-window',action='护阵',stage=1))
        report=Metrics.export(self.db,self.now-1,self.now+1)['scopes'][0]
        self.assertEqual(report['worlds_started'],0)
        self.assertEqual(report['participating_viewers'],1)
        self.assertEqual(report['participation_rate'],1)

    def test_output_cannot_overwrite_database_or_sqlite_sidecars(self):
        import os
        from pathlib import Path
        from Tools.CultivationService.metrics import Metrics
        for target in (self.db,Path(str(self.db)+'-wal'),Path(str(self.db)+'-shm')):
            with self.assertRaisesRegex(ValueError,'OUTPUT_OVERWRITES_DATABASE'):
                Metrics.validate_output(self.db,target)
        alias=self.db.with_name('database-alias.json');os.link(self.db,alias)
        with self.assertRaisesRegex(ValueError,'OUTPUT_OVERWRITES_DATABASE'):
            Metrics.validate_output(self.db,alias)
        Metrics.validate_output(self.db,self.db.with_name('audit.json'))

    def test_export_is_deduplicated_scoped_and_keeps_revenue_unknown(self):
        from Tools.CultivationService.metrics import Metrics
        self.create()
        self.service.sync(self.anchor, {'active': True})
        self.command(self.anchor, 'SignIn', request='metrics-sign')
        self.command(self.anchor, 'SignIn', request='metrics-sign')
        with self.service.store.transaction() as db:
            ctx = self.service._context(db, self.anchor)
            Inventory.grant(ctx, 'MAT_HERB_T', 3, {'id':'source-one'})
        self.command(self.anchor, 'BindMaterial', {'item_id':'MAT_HERB_T','count':2})
        report = Metrics.export(self.db, self.now-1, self.now+1)
        scope = report['scopes'][0]
        self.assertIsNone(scope['real_revenue'])
        self.assertEqual(scope['commands']['SignIn'], 1)
        self.assertEqual(scope['item_flows']['MAT_HERB_T']['granted'], 3)
        self.assertEqual(scope['item_flows']['MAT_HERB_T']['consumed'], 2)
        self.assertEqual(scope['item_flows']['MAT_HERB_B']['granted'], 2)
        self.assertNotIn(self.anchor, str(report))
        self.assertNotIn('source-one', str(report))
        self.assertEqual(Metrics.export(self.db,self.now+10,self.now+20)['scopes'][0]['commands'], {})

    def test_rejected_transaction_does_not_create_consumption_audit(self):
        from Tools.CultivationService.metrics import Metrics
        self.create()
        result=self.command(self.anchor,'BindMaterial',{'item_id':'MAT_HERB_T','count':2})
        self.assertEqual(result['status'],'error')
        scope=Metrics.export(self.db,self.now-1,self.now+1)['scopes'][0]
        self.assertEqual(scope['item_flows'],{})
        self.assertNotIn('BindMaterial',scope['commands'])
