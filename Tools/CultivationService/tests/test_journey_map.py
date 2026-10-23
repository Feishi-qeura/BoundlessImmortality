from test_foundation import ServiceCase


class JourneyMapTests(ServiceCase):
    def test_map_exposes_three_open_worlds_and_locked_future(self):
        journey = self.service.snapshot(self.anchor)['journey']
        self.assertEqual([r['id'] for r in journey['worlds'] if r['open']], ['qingyun', 'luoxia', 'xuantie'])
        self.assertEqual(len(journey['worlds']), 8)
        for row in journey['worlds']:
            self.assertTrue(row['name'])
            self.assertTrue(row['difficulty'])
            self.assertTrue(row['description'])
            self.assertEqual(bool(row['rewards']), row['open'])

    def test_resource_count_controls_real_selectable_entries(self):
        self.rules.configs['runtime_balance.json']['map_resource_counts'] = {'NODE_QI': 2, 'NODE_STONE': 0, 'NODE_HERB': 1, 'NODE_IRON': 1, 'NODE_PAGE': 1}
        nodes = self.service.snapshot(self.anchor)['journey']['resources']
        self.assertEqual(len(nodes), 5)
        self.assertEqual(sum(n['node_id'] == 'NODE_QI' for n in nodes), 2)
        self.assertFalse(any(n['node_id'] == 'NODE_STONE' for n in nodes))
        self.assertEqual(len({n['id'] for n in nodes}), len(nodes))
        self.assertTrue(all('description' not in n and 'difficulty' not in n for n in nodes))

    def test_resource_params_follow_rules_and_do_not_invent_owners(self):
        nodes = self.service.snapshot(self.anchor)['journey']['resources']
        rules = {r['node_id']: r for r in self.rules.tables['nodes.csv']}
        self.assertEqual(len(nodes), 5)
        for node in nodes:
            rule = rules[node['node_id']]
            self.assertEqual(node['quantity'], rule['item_count'])
            self.assertEqual(node['interval_seconds'], rule['interval_seconds'])
            self.assertEqual(node['owners'], [])

    def test_world_reward_budget_matches_tables(self):
        row = self.service.snapshot(self.anchor)['journey']['worlds'][0]
        actual = {r['item_id']: r['quantity'] for r in row['rewards']}
        expected = {}
        for r in self.rules.tables['world_rewards.csv']:
            expected[r['item_id']] = expected.get(r['item_id'], 0) + r['count']
        self.assertEqual(actual, expected)

    def test_owner_is_a_live_lease_and_disappears_on_expiry(self):
        from Tools.CultivationService.leases import Leases
        self.create()
        with self.service.store.transaction() as db:
            ctx = self.service._context(db, self.anchor)
            Leases.activate(ctx, 'declared-map-owner-test', 'NODE_QI', 1800)
        nodes = self.service.snapshot(self.anchor)['journey']['resources']
        spring = next(n for n in nodes if n['node_id'] == 'NODE_QI')
        self.assertEqual([o['anchor_id'] for o in spring['owners']], ['anchor'])
        self.assertTrue(spring['owners'][0]['name'])
        self.now += 1801
        nodes = self.service.snapshot(self.anchor)['journey']['resources']
        self.assertFalse(next(n for n in nodes if n['node_id'] == 'NODE_QI')['owners'])
