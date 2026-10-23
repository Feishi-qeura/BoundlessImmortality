from test_foundation import ServiceCase
from datetime import datetime, timezone


class SignInPlaceholderTests(ServiceCase):
    def test_seven_distinct_placeholders_record_without_economic_rewards(self):
        self.now = datetime(2026, 10, 5, 4, tzinfo=timezone.utc).timestamp()
        before = self.service.snapshot(self.anchor)
        template = next(row for row in before['activities']['templates'] if row['activity_id']=='ACT_SIGN')
        self.assertEqual(template['reward_budget'], 'seven_daily_placeholders_no_grant')
        self.assertEqual(before['activities']['sign_in_qi_reward'], 0)
        weekly = before['activities']['weekly_chests']
        self.assertEqual(len(weekly), 7)
        self.assertEqual(len({day['reward'] for day in weekly}), 7)
        self.assertTrue(all(day.get('placeholder') for day in weekly))
        for index in range(7):
            reply = self.command(self.anchor, 'SignIn', request='placeholder-day-'+str(index))
            self.assertEqual(reply['status'], 'success', reply)
            state = reply['result']
            self.assertEqual(state['growth']['total_qi'], before['growth']['total_qi'])
            self.assertEqual(state['wallet'], before['wallet'])
            self.assertEqual(state['inventory'], before['inventory'])
            self.assertTrue(state['activities']['weekly_chests'][index]['claimed'])
            replay = self.command(self.anchor, 'SignIn', request='placeholder-day-'+str(index))
            self.assertEqual(reply['receipt_id'], replay['receipt_id'])
            self.now += 86400
