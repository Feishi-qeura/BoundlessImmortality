from test_foundation import ServiceCase


class CombatTests(ServiceCase):
    def test_quality_ownership_is_exact_even_when_identity_contains_colon(self):
        from Tools.CultivationService.combat import Combat
        self.session('anchor:child')
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,self.anchor)
            ctx.put('quality','anchor:child:BR01',dict(band=dict(live_snapshot_factor=1.1)))
            self.assertEqual(Combat.quality(ctx),1)

    def test_burn_uses_actual_frozen_anchor_attack(self):
        from Tools.CultivationService.combat import Combat
        from Tools.CultivationService.effects import Effects
        self.command(self.anchor,'StartWorldRound');self.now+=61;self.service.snapshot(self.anchor)
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,self.anchor);world=ctx.entities('world')[0]
            state=Effects.combat(ctx);state['effects']['burn_attack_per_tick']=dict(round_id=world['id'],end=ctx.now+5,last_tick=ctx.now-1,magnitude=.1,base_attack=40)
            ctx.put('combat',ctx.user,state);before=world['targets'][0]['health'];Combat.advance(ctx,world)
            self.assertEqual(before-world['targets'][0]['health'],min(4,before))

    def test_incoming_damage_uses_authoritative_defense_and_recovery_rebuilds_targets(self):
        from Tools.CultivationService.effects import Effects
        from Tools.CultivationService.combat import Combat
        from Tools.CultivationService.loadouts import Loadouts
        from Tools.CultivationService.world import World
        self.command(self.anchor,'StartWorldRound')
        self.now+=61;self.service.snapshot(self.anchor)
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,self.anchor)
            db.execute('UPDATE growth SET realm=10 WHERE scope=? AND user=?',(ctx.scope,ctx.user))
            world=ctx.entities('world')[0];state=Effects.combat(ctx)
            before=state['health'];ctx.put('combat',ctx.user,state)
            world['pressure_cursor']=ctx.now-4
            Combat.advance(ctx,world)
            after=Effects.combat(ctx)['health']
            defense=Loadouts.attributes(ctx)['defense']
            self.assertAlmostEqual(before-after,100/(10+defense))
            state=Effects.combat(ctx);state['health']=.01;ctx.put('combat',ctx.user,state)
            world['pressure_cursor']=ctx.now-4;Combat.advance(ctx,world);ctx.put('world',world['id'],world)
            self.assertEqual(world['targets'],[])
        self.now+=31;self.service.snapshot(self.anchor)
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,self.anchor);world=ctx.entities('world')[0]
            self.assertGreater(len(world['targets']),0)
            self.assertEqual(world['completed'],[])

    def test_edge_every_fifth_actual_hit_and_echo_is_delayed(self):
        from importlib.util import find_spec
        self.assertIsNotNone(find_spec('Tools.CultivationService.combat'))
        from Tools.CultivationService.combat import Combat
        with self.service.store.transaction() as db:
            ctx=self.service._context(db,self.anchor)
            round_={'anchor':ctx.user,'start':ctx.now,'stage':1,'loadout':{'equipment':{},'methods':{'METHOD_EDGE':{'level':1}},'main':'METHOD_EDGE','supports':[]},'mechanics':{},'delayed':[]}
            target={'id':'target','defense':10,'health':1000}
            values=[Combat.resolve_hit(ctx,round_,target) for _ in range(5)]
            self.assertEqual(values[:4],[5,5,5,5])
            self.assertEqual(values[4],9)
            round_['loadout'].update(main='METHOD_ECHO',methods={'METHOD_ECHO':{'level':1}})
            round_['mechanics']={}
            for _ in range(4):Combat.resolve_hit(ctx,round_,target)
            self.assertEqual(len(round_['delayed']),1)
            self.assertEqual(round_['delayed'][0]['damage'],2.5)

    def test_damage_overflow_and_shield_absorption(self):
        from importlib.util import find_spec
        self.assertIsNotNone(find_spec('Tools.CultivationService.combat'))
        from Tools.CultivationService.combat import Combat
        state={'health':100,'max_health':100,'effects':{'shield_max_hp':{'remaining':20,'end':self.now+10}},'statuses':{},'cooldowns':{}}
        self.assertEqual(Combat.incoming(state,30,self.now),10)
        self.assertEqual(state['health'],90)
        self.assertEqual(state['effects']['shield_max_hp']['remaining'],0)
