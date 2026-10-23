from test_foundation import ServiceCase


class RaceTests(ServiceCase):
    def test_qi_bonus_integrates_until_exact_lease_expiry(self):
        from Tools.CultivationService.leases import Leases
        self.create();self.service.sync(self.anchor,{'active':True})
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,self.anchor);Leases.activate(ctx,'fixture','NODE_QI',5)
        self.now+=5
        self.assertEqual(self.service.snapshot(self.anchor)['growth']['total_qi'],6)

    def team(self,user,room):
        anchor=self.session(user,'anchor',room)
        gid=self.command(anchor,'CreateGuild',{'name':'青云'+user})['result']['guild']['id']
        viewer=self.session(user+'-viewer','viewer',room)
        self.command(viewer,'JoinGuild',{'guild_id':gid})
        self.service.sync(anchor,{'active':True,'participants':[{'token':viewer,'active':True}]})
        return anchor,viewer

    def test_two_teams_match_and_freeze_real_representatives(self):
        self.assertIn('QueueRace',self.service.handlers)
        a,va=self.team('甲','a');b,vb=self.team('乙','b')
        for token in (a,b):
            result=self.command(token,'QueueRace',{'node_id':'NODE_STONE'})
            self.assertEqual(result['status'],'success',result)
        state=self.service.snapshot(a)
        self.assertEqual(state['race']['status'],'Ready')
        self.assertEqual(len(state['race']['teams']),2)
        self.now+=119
        for anchor,viewer in ((a,va),(b,vb)):self.service.sync(anchor,{'active':True,'participants':[{'token':viewer,'active':True}]})
        self.now+=2
        state=self.service.snapshot(a)
        self.assertEqual(state['race']['status'],'Running')
        self.assertTrue(all(len(t['representatives'])==1 for t in state['race']['teams']))

    def test_no_fake_opponents_and_queue_cancellation(self):
        self.assertIn('QueueRace',self.service.handlers)
        a,viewer=self.team('甲','a')
        self.command(a,'QueueRace',{'node_id':'NODE_QI'})
        self.now+=91
        state=self.service.snapshot(a)
        self.assertIsNone(state['race'])
        self.assertEqual(state['race_queue']['status'],'Queued')
        self.assertEqual(self.command(a,'CancelRaceQueue')['status'],'success')

    def test_lease_only_effective_online_buckets_and_utc_expiry(self):
        self.assertIn('QueueRace',self.service.handlers)
        from Tools.CultivationService.leases import Leases
        a,viewer=self.team('甲','a')
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,a)
            Leases.activate(ctx,'fixture-race','NODE_STONE',1800)
        for _ in range(600):
            self.now+=1
            self.service.sync(a,{'active':True})
        state=self.service.snapshot(a)
        self.assertEqual(state['wallet']['CUR_STONE'],20)
        self.service.sync(a,{'active':False})
        self.now+=1201
        state=self.service.snapshot(a)
        self.assertEqual(state['wallet']['CUR_STONE'],20)
        self.assertEqual(state['lease']['status'],'Expired')

    def test_tied_full_scores_grant_both_teams_stage_budget_and_lease(self):
        a,va=self.team('甲','a');b,vb=self.team('乙','b')
        for token in (a,b):self.command(token,'QueueRace',{'node_id':'NODE_STONE'})
        start=self.service.snapshot(a)['race']['start']
        def advance_to(target):
            while self.now<target:
                self.now=min(target,self.now+4)
                for anchor,viewer in ((a,va),(b,vb)):self.service.sync(anchor,{'active':True,'participants':[{'token':viewer,'active':True}]})
        advance_to(start+119)
        events=[(t,'common') for t in (1,61,121,181,241,301)]+[(60,'representative'),(240,'representative'),(90,'anchor'),(270,'anchor')]
        for offset,kind in sorted(events):
            advance_to(start+120+offset)
            for anchor,viewer in ((a,va),(b,vb)):
                self.service.sync(anchor,{'active':True,'participants':[{'token':viewer,'active':True}]})
                race=self.service.snapshot(anchor)['race']
                token=anchor if kind=='anchor' else viewer
                args={'race_id':race['id'],'kind':kind,'action':'护阵' if int(offset//60)%2==0 else '破障'}
                result=self.command(token,'SubmitRaceAction',args,f'event-{offset}-{kind}-{token[:8]}')
                self.assertEqual(result['status'],'success',result)
        for second in range(int(self.now-start)+1,601):
            self.now=start+second
            for anchor,viewer in ((a,va),(b,vb)):self.service.sync(anchor,{'active':True,'participants':[{'token':viewer,'active':True}]})
        self.now=start+601
        for token in (a,b):
            state=self.service.snapshot(token)
            self.assertEqual([t['score'] for t in state['race']['teams']],[1000,1000])
            self.assertEqual(state['wallet']['CUR_STONE'],80)
            self.assertEqual(state['inventory']['MAT_BREAK'],12)
            self.assertEqual(state['lease']['end']-state['lease']['start'],1800)
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,va)
            self.assertEqual(ctx.count(ctx.user,'world-bonus:'+ctx.day),1)

    def test_disconnected_anchor_forfeits_rank_but_keeps_confirmed_stage_rewards(self):
        a,va=self.team('甲','a');b,vb=self.team('乙','b')
        for token in (a,b):self.command(token,'QueueRace',{'node_id':'NODE_STONE'})
        from Tools.CultivationService.races import Races
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,a);race=ctx.entities('race')[0]
            race['status']='Running'
            for team in race['teams']:team.update(eligible=True,score=600,common=[0,1,2,3,4,5],representatives=['fixture'])
            ctx.put('race',race['id'],race)
        self.service.sync(a,{'active':False});self.now+=601
        state=self.service.snapshot(a)
        self.assertIsNone(state['lease'])
        self.assertEqual(state['wallet']['CUR_STONE'],80)
