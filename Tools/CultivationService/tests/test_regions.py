from test_foundation import ServiceCase


class RegionTests(ServiceCase):
    def test_luoxia_requires_free_gather_route_before_herb_targets_take_damage(self):
        self.command(self.anchor, 'StartWorldRound', {'region_id': 'luoxia'})
        self.now += 61
        state = self.service.sync(self.anchor, {'active': True})
        target = state['round']['targets'][0]
        hit = dict(round_id=state['round']['id'], action='strike', target_id=target['id'])
        rejected = self.command(self.anchor, 'SubmitAction', hit, 'sealed-hit')
        self.assertEqual(rejected.get('error_code'), 'GATHER_ROUTE_REQUIRED', rejected)
        self.assertEqual(self.service.snapshot(self.anchor)['round']['targets'][0]['health'], target['health'])
        opened = self.command(self.anchor, 'SubmitAction', dict(round_id=state['round']['id'], action='route', choice='gather'))
        self.assertEqual(opened['status'], 'success', opened)
        self.assertFalse(opened['result']['round']['targets'][0]['mechanic_locked'])
        self.now += 1.01
        self.service.sync(self.anchor, {'active': True})
        self.assertEqual(self.command(self.anchor, 'SubmitAction', hit, 'open-hit')['status'], 'success')

    def test_xuantie_obstacles_require_timed_free_break_response(self):
        self.command(self.anchor, 'StartWorldRound', {'region_id': 'xuantie'})
        self.now += 241
        state = self.service.sync(self.anchor, {'active': True})
        target = state['round']['targets'][0]
        hit = dict(round_id=state['round']['id'], action='strike', target_id=target['id'])
        self.assertEqual(self.command(self.anchor, 'SubmitAction', hit, 'closed-hit').get('error_code'), 'WEAKPOINT_CLOSED')
        viewer = self.session('free-helper', 'viewer')
        self.service.sync(viewer, {'active': True})
        result = self.command(viewer, 'SubmitAction', dict(round_id=state['round']['id'], action='破障'))
        self.assertEqual(result['status'], 'success', result)
        target = result['result']['round']['targets'][0] if result['result']['round'] else self.service.snapshot(self.anchor)['round']['targets'][0]
        self.assertFalse(target['mechanic_locked'])
        self.now += 1.01
        self.assertEqual(self.command(self.anchor, 'SubmitAction', hit, 'weak-hit')['status'], 'success')
        self.now += 8
        self.service.sync(self.anchor, {'active': True})
        self.assertEqual(self.command(self.anchor, 'SubmitAction', hit, 'expired-hit').get('error_code'), 'WEAKPOINT_CLOSED')

    def test_first_three_regions_start_and_keep_public_reward_budget(self):
        worlds = (('qingyun', '青云山'), ('luoxia', '落霞谷'), ('xuantie', '玄铁岭'))
        for region, name in worlds:
            with self.subTest(region=region):
                result = self.command(self.anchor, 'StartWorldRound', {'region_id': region, 'route': 'guard'})
                self.assertEqual(result['status'], 'success', result)
                round_ = result['result']['round']
                self.assertEqual(round_.get('region_id'), region)
                self.assertEqual(round_.get('region_name'), name)
                self.assertEqual(round_['preset'], 'ENC_SOLO')
                self.assertEqual(round_['boundaries'], [60, 240, 420, 540, 600])
                reply = self.command(self.anchor, 'InterruptWorldRound', {'round_id': round_['id']})
                self.assertEqual(reply['status'], 'success', reply)
                self.assertEqual(reply['result']['wallet'].get('CUR_STONE', 0), 0)

    def test_future_and_unknown_regions_cannot_be_started_via_command(self):
        for region in ('xuanbing', 'yaoshou', 'chiyan', 'xingyun', 'taixu', 'unknown', '', 2, None):
            with self.subTest(region=region):
                result = self.command(self.anchor, 'StartWorldRound', {'region_id': region})
                self.assertEqual(result.get('error_code'), 'REGION_NOT_OPEN', result)
                self.assertIsNone(self.service.snapshot(self.anchor)['round'])

    def test_legacy_world_command_defaults_to_qingyun(self):
        result = self.command(self.anchor, 'StartWorldRound')
        self.assertEqual(result['result']['round'].get('region_id'), 'qingyun')

    def test_three_regions_complete_and_settle_the_same_budget_once(self):
        self.create()
        for count, region in enumerate(('qingyun', 'luoxia', 'xuantie'), 1):
            result = self.command(self.anchor, 'StartWorldRound', {'region_id': region})
            start = result['result']['round']['start']
            for stage_time in (61, 241, 421):
                self.now = start + stage_time
                self.service.sync(self.anchor, {'active': True})
                if region == 'luoxia' and stage_time == 61:
                    state = self.service.snapshot(self.anchor)
                    self.command(self.anchor, 'SubmitAction', dict(round_id=state['round']['id'], action='route', choice='gather'))
                    self.now += 1.01
                    self.service.sync(self.anchor, {'active': True})
                for hit in range(200):
                    state = self.service.snapshot(self.anchor)
                    targets = [t for t in state['round']['targets'] if t['health'] > 0]
                    if not targets:
                        break
                    if region == 'xuantie' and stage_time == 241:
                        self.now += 1.01
                        self.service.sync(self.anchor, {'active': True})
                        response = self.command(self.anchor, 'SubmitAction', dict(round_id=state['round']['id'], action='破障'), f'weak-{hit}')
                        self.assertEqual(response['status'], 'success', response)
                        self.now += 1.01
                        self.service.sync(self.anchor, {'active': True})
                    self.now += 1.01
                    reply = self.command(self.anchor, 'SubmitAction', dict(round_id=state['round']['id'],
                        action='strike', target_id=targets[0]['id']), f'{region}-{stage_time}-{hit}')
                    self.assertEqual(reply['status'], 'success', reply)
                    self.service.sync(self.anchor, {'active': True})
                self.assertLess(hit, 199)
            self.now = start + 601
            for _ in range(2):
                state = self.service.snapshot(self.anchor)
                self.assertEqual(state['round']['status'], 'Completed')
                self.assertEqual(state['wallet']['CUR_STONE'], 80 * count)
                self.assertEqual(state['inventory']['MAT_BREAK'], 12 * count)
