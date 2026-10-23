from test_foundation import ServiceCase


class LaunchProgressTests(ServiceCase):
    def test_final_buff_modifies_sword_damage_and_anchor_defense(self):
        from Tools.CultivationService.loadouts import Loadouts
        from Tools.CultivationService.combat import Combat
        self.create();self.command(self.anchor,'StartLaunchEvent');self.service.sync(self.anchor,{'active':True})
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,self.anchor);event=ctx.entity('launch',ctx.user)
            event.update(buff_start=ctx.now,buff_end=ctx.now+1200);ctx.put('launch',ctx.user,event)
            self.assertEqual(Loadouts.attributes(ctx)['defense'],22)
            round_=dict(anchor=ctx.user,start=ctx.now,stage=1,loadout=Loadouts.get(ctx))
            self.assertAlmostEqual(Combat.resolve_hit(ctx,round_,dict(id='t',health=100,defense=10)),5.5)

    def test_owned_title_remains_visible_after_departure(self):
        gid=self.create();viewer=self.session('viewer','viewer')
        self.command(viewer,'JoinGuild',{'guild_id':gid})
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,viewer)
            ctx.put('enrollment',ctx.user,dict(user=ctx.user,home_guild=gid,title=dict(title_id='QingyunFeiyan',name='青云踏飞雁',expiry=ctx.now+100)))
        self.command(viewer,'LeaveGuild')
        self.assertEqual(self.service.snapshot(viewer)['honors']['owned'][0]['name'],'青云踏飞雁')

    def test_pre_enrollment_stages_are_not_retroactively_completed(self):
        from Tools.CultivationService.launch_event import LaunchEvent
        gid=self.create();viewer=self.session('viewer','viewer')
        self.command(viewer,'JoinGuild',{'guild_id':gid});self.command(self.anchor,'StartLaunchEvent')
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,viewer)
            round_={'id':'late-enrollment','anchor':'anchor','start':self.now,'completed':[1,2,3],'participants':{'viewer':dict(stages=[1,2,3],role='viewer',guild_id=gid)}}
            for action,stage in [('破障',2),('共修',3)]:LaunchEvent.on_action(ctx,round_,action,stage)
            LaunchEvent.on_complete(ctx.for_user('anchor'),round_)
        self.assertEqual(self.service.snapshot(viewer)['launch']['personal']['complete_count'],0)

    def test_cross_room_assist_does_not_advance_home_launch(self):
        from Tools.CultivationService.launch_event import LaunchEvent
        gid=self.create();viewer=self.session('viewer','viewer')
        self.command(viewer,'JoinGuild',{'guild_id':gid});self.command(self.anchor,'StartLaunchEvent')
        other=self.session('other','anchor','room-b')
        self.command(other,'CreateGuild',{'name':'青云乙'})
        viewer=self.session('viewer','viewer','room-b')
        self.command(other,'StartWorldRound');self.now+=61
        self.service.sync(viewer,{'active':True})
        state=self.service.snapshot(other)
        result=self.command(viewer,'SubmitAction',{'round_id':state['round']['id'],'action':'护阵'})
        self.assertEqual(result['status'],'success',result)
        self.assertIsNone(self.service.snapshot(viewer)['launch']['personal'])

    def test_numeric_permission_off_still_records_task_without_qi(self):
        self.rules.configs['launch_event.json']['profiles']['prototype_full']['task_numeric_rewards']=False
        gid=self.create();viewer=self.session('viewer','viewer')
        self.command(viewer,'JoinGuild',{'guild_id':gid});self.command(self.anchor,'StartLaunchEvent')
        from Tools.CultivationService.launch_event import LaunchEvent
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,viewer)
            round_={'id':'first-free-response','anchor':'anchor','start':self.now,'completed':[], 'participants':{}}
            LaunchEvent.on_action(ctx,round_,'护阵',1)
        state=self.service.snapshot(viewer)
        self.assertIn('L01',state['launch']['personal']['tasks'])
        self.assertEqual(state['growth']['total_qi'],0)

    def test_free_facts_award_tasks_once_and_title(self):
        from Tools.CultivationService.launch_event import LaunchEvent
        self.assertTrue(hasattr(LaunchEvent,'on_action'))
        gid=self.create()
        viewer=self.session('viewer','viewer')
        self.command(viewer,'JoinGuild',{'guild_id':gid})
        self.command(self.anchor,'StartLaunchEvent')
        with self.service.store.transaction() as db:
            viewer_ctx=self.service._context(db,viewer)
            anchor_ctx=self.service._context(db,self.anchor)
            for n in range(12):
                round_={'id':f'world-{n}','anchor':'anchor','start':self.now+n*600,'completed':[1,2,3],
                        'participants':{'anchor':{'stages':[1,2,3],'role':'anchor','guild_id':gid,'free_response':True},'viewer':{'stages':[1,2,3],'role':'viewer','guild_id':gid}}}
                for action,stage in [('route',1),('护阵',1),('护阵',1),('破障',2),('破障',2),('共修',3)]:
                    LaunchEvent.on_action(viewer_ctx,round_,action,stage)
                LaunchEvent.on_complete(anchor_ctx,round_)
                LaunchEvent.on_complete(anchor_ctx,round_)
        state=self.service.snapshot(viewer)
        self.assertEqual(state['growth']['total_qi'],780)
        self.assertEqual(state['launch']['personal']['title']['name'],'青云踏飞雁')
        self.assertEqual(state['launch']['personal']['score'],120)
        self.assertEqual(self.service.snapshot(self.anchor)['wallet']['CUR_STONE'],200)
        self.assertEqual(self.service.snapshot(self.anchor)['inventory']['MAT_BREAK'],60)
