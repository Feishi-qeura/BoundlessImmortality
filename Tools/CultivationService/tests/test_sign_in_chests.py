from test_foundation import ServiceCase
from datetime import datetime, timezone


class SignInChestTests(ServiceCase):
    def setUp(self):
        super().setUp()
        self.now = datetime(2026, 10, 5, 4, tzinfo=timezone.utc).timestamp()

    def test_week_contains_server_dates_and_daily_claim_is_sign_in(self):
        state = self.service.snapshot(self.anchor)['activities']
        self.assertEqual(len(state['weekly_chests']), 7)
        self.assertTrue(state['weekly_chests'][0]['eligible'])
        self.assertFalse(state['weekly_chests'][1]['eligible'])
        reply = self.command(self.anchor, 'SignIn')
        self.assertTrue(reply['result']['activities']['weekly_chests'][0]['claimed'])
        self.assertEqual(reply['result']['growth']['total_qi'], 0)
        self.assertEqual(self.command(self.anchor, 'SignIn')['error_code'], 'ALREADY_SETTLED')

    def test_month_requires_threshold_claim_receipt_and_replay(self):
        self.assertEqual(self.command(self.anchor, 'ClaimSignInChest', {'milestone': 3})['error_code'], 'MILESTONE_LOCKED')
        for _ in range(3):
            self.assertEqual(self.command(self.anchor, 'SignIn')['status'], 'success')
            self.now += 86400
        chest = self.service.snapshot(self.anchor)['activities']['monthly_chests'][0]
        self.assertTrue(chest['eligible'])
        self.assertFalse(chest['claimed'])
        reply = self.command(self.anchor, 'ClaimSignInChest', {'milestone': 3}, 'chest-3')
        self.assertEqual(reply['status'], 'success', reply)
        self.assertTrue(reply['result']['activities']['monthly_chests'][0]['claimed'])
        replay = self.command(self.anchor, 'ClaimSignInChest', {'milestone': 3}, 'chest-3')
        self.assertEqual(reply['receipt_id'], replay['receipt_id'])
        self.assertEqual(self.command(self.anchor, 'ClaimSignInChest', {'milestone': 3})['error_code'], 'ALREADY_SETTLED')
        other = self.session('other')
        self.assertFalse(self.service.snapshot(other)['activities']['monthly_chests'][0]['claimed'])
        self.now = datetime(2026, 11, 1, 4, tzinfo=timezone.utc).timestamp()
        self.assertFalse(self.service.snapshot(self.anchor)['activities']['monthly_chests'][0]['claimed'])

    def test_old_auto_granted_cosmetics_are_preserved_as_claimed(self):
        with self.service.store.transaction() as db:
            ctx = self.service._context(db, self.anchor)
            ctx.put('cosmetic', ctx.user+':month:'+ctx.day[:7]+':3', dict(owner=ctx.user, month=ctx.day[:7], milestone=3))
        self.assertTrue(self.service.snapshot(self.anchor)['activities']['monthly_chests'][0]['claimed'])
        self.assertEqual(self.command(self.anchor, 'ClaimSignInChest', {'milestone': 3})['error_code'], 'ALREADY_SETTLED')

    def test_invalid_milestones_cannot_create_rewards(self):
        for value in (True, 0, 2, 28, '3', 3.0):
            self.assertEqual(self.command(self.anchor, 'ClaimSignInChest', {'milestone': value})['error_code'], 'INVALID_ARGUMENT')
