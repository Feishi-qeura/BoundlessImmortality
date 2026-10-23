import importlib
import json
from pathlib import Path
import shutil
import tempfile
import threading
import unittest

ROOT = Path(__file__).resolve().parents[3]
DESIGN = ROOT / 'docs/design/2026-10-21-wuling-resource-v1.4'


class ServiceCase(unittest.TestCase):
    def setUp(self):
        self.assertTrue((ROOT / 'Tools/CultivationService/authority.py').exists(), 'Authority service is not implemented')
        module = importlib.import_module('Tools.CultivationService.authority')
        self.rules = importlib.import_module('Tools.CultivationService.rules').RuleBundle.load(DESIGN)
        sandbox = ROOT / 'Saved/Codex/GameplayPlan20261021/TestDatabases'
        sandbox.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=sandbox)
        self.addCleanup(self.temp.cleanup)
        self.now = 1800000000.0
        self.db = Path(self.temp.name) / 'test.sqlite3'
        self.service = module.Authority(self.db, self.rules, lambda: self.now)
        self.anchor = self.session('anchor')

    def session(self, user, role='anchor', room='room-a', app='test-app'):
        return self.service.open_session(dict(development_only=True, simulated=True, profile='prototype_full',
                                              app=app, room=room, user=user, role=role))['token']

    def command(self, token, command, args=None, request=None, version=None):
        state = self.service.snapshot(token)
        return self.service.execute(token, dict(request_id=request or f'{command}-{state["version"]}',
            expected_version=state['version'] if version is None else version, profile='prototype_full',
            rule_version=self.rules.version, command=command, args=args or {}))

    def create(self):
        reply = self.command(self.anchor, 'CreateGuild', {'name': '青云武灵'})
        self.assertEqual(reply['status'], 'success', reply)
        return reply['result']['guild']['id']

class FoundationTests(ServiceCase):
    def test_offline_acknowledgement_never_grants_again(self):
        viewer=self.session('offline-viewer','viewer');self.now+=100
        state=self.service.snapshot(viewer)
        self.assertEqual(state['growth']['total_qi'],10)
        self.assertEqual(state['offline_reward']['total_qi'],10)
        result=self.command(viewer,'AcknowledgeOfflineReward')
        self.assertEqual(result['status'],'success',result)
        self.assertEqual(result['result']['offline_reward']['acknowledged_qi'],10)
        self.assertEqual(result['result']['growth']['total_qi'],10)

    def test_background_tick_does_not_mutate_mismatched_rules(self):
        self.service.sync(self.anchor,{'active':True})
        with self.service.store.transaction() as db:
            db.execute("UPDATE namespace SET rules_hash='old-frozen-hash'")
        self.now+=3;self.service.tick()
        with self.service.store.transaction() as db:
            row=db.execute('SELECT total FROM growth').fetchone()
            self.assertEqual(row['total'],0)

    def test_rules_typed_and_complete(self):
        self.assertEqual(len(self.rules.tables['items-v1.4/item_master.csv']), 730)
        self.assertEqual(len(self.rules.realms), 34)
        self.assertFalse(self.rules.items['MAT_HERB_B']['tradeable'])
        self.assertTrue(self.rules.items['MAT_HERB_T']['tradeable'])
        self.assertEqual(self.rules.realms[0]['viewer_qi_to_next'], 120)
        self.assertEqual(len(self.rules.sha256), 64)

    def test_missing_table_refused(self):
        copy = Path(self.temp.name) / 'design'
        shutil.copytree(DESIGN / 'tables', copy / 'tables')
        (copy / 'tables/realm_costs.csv').unlink()
        with self.assertRaises(ValueError):
            type(self.rules).load(copy)

    def test_release_and_fake_official_identity_refused(self):
        for changes in ({'profile': 'douyin_release_candidate'}, {'simulated': False}, {'role': 'admin'}):
            body = dict(development_only=True, simulated=True, profile='prototype_full', app='app',room='room',user='u',role='anchor')
            body.update(changes)
            with self.assertRaises(ValueError):
                self.service.open_session(body)

    def test_receipt_replay_precedes_stale_version(self):
        version = self.service.snapshot(self.anchor)['version']
        first = self.command(self.anchor, 'CreateGuild', {'name': '青云武灵'}, 'same', version)
        replay = self.command(self.anchor, 'CreateGuild', {'name': '青云武灵'}, 'same', version)
        self.assertEqual(first, replay)
        conflict = self.command(self.anchor, 'CreateGuild', {'name': '玄铁武灵'}, 'same', version)
        self.assertEqual(conflict['error_code'], 'REQUEST_ID_CONFLICT')

    def test_stale_version_and_unknown_command(self):
        self.create()
        self.assertEqual(self.command(self.anchor, 'CreateGuild', {'name': '别的武灵'}, version=0)['error_code'], 'STALE_VERSION')
        self.assertEqual(self.command(self.anchor, 'MintCurrency')['error_code'], 'UNKNOWN_COMMAND')

    def test_membership_rejoin_and_personal_preservation(self):
        gid = self.create()
        viewer = self.session('viewer', 'viewer')
        self.assertEqual(self.command(viewer, 'JoinGuild', {'guild_id': gid})['status'], 'success')
        self.assertEqual(self.command(viewer, 'JoinGuild', {'guild_id': gid})['error_code'], 'ALREADY_MEMBER')
        self.command(viewer, 'LeaveGuild')
        self.assertEqual(self.command(viewer, 'JoinGuild', {'guild_id': gid})['error_code'], 'REJOIN_WAIT')
        self.now += 86400
        self.assertEqual(self.command(viewer, 'JoinGuild', {'guild_id': gid})['status'], 'success')

    def test_concurrent_creation_commits_once(self):
        barrier = threading.Barrier(2)
        replies = []
        def worker(index):
            barrier.wait()
            replies.append(self.command(self.anchor, 'CreateGuild', {'name': '青云武灵'}, f'concurrent-{index}', 0))
        threads = [threading.Thread(target=worker, args=(i,)) for i in range(2)]
        for thread in threads: thread.start()
        for thread in threads: thread.join()
        self.assertEqual(sum(r['status'] == 'success' for r in replies), 1)

    def test_session_takeover_revokes_old_heartbeat(self):
        viewer = self.session('viewer', 'viewer')
        self.service.sync(viewer, {'active': True})
        replacement = self.session('viewer', 'viewer', 'room-b')
        with self.assertRaises(ValueError):
            self.service.sync(viewer, {'active': True})
        self.assertEqual(self.service.snapshot(replacement)['identity']['room'], 'room-b')

    def test_anchor_sync_reports_moved_participant_without_reclaim(self):
        viewer=self.session('viewer','viewer')
        self.service.sync(self.anchor,{'active':True,'participants':[{'token':viewer,'active':True}]})
        moved=self.session('viewer','viewer','room-b')
        state=self.service.sync(self.anchor,{'active':True,'participants':[{'token':viewer,'active':True}]})
        self.assertIn('viewer',state['revoked_participants'])
        self.assertEqual(self.service.snapshot(moved)['identity']['room'],'room-b')

    def test_growth_online_heartbeat_and_auto_advance(self):
        viewer = self.session('viewer', 'viewer')
        self.service.sync(viewer, {'active': True})
        for _ in range(130):
            self.now += 1
            self.service.sync(viewer, {'active': True})
        growth = self.service.snapshot(viewer)['growth']
        self.assertEqual(growth['realm_index'], 1)
        self.assertAlmostEqual(growth['qi'], 10)

    def test_offline_capped_cursor_consumes_whole_interval(self):
        viewer = self.session('viewer', 'viewer')
        self.service.sync(viewer, {'active': False})
        self.now += 90000
        self.service.sync(viewer, {'active': False})
        first = self.service.snapshot(viewer)['growth']['total_qi']
        self.assertAlmostEqual(first, 2880)
        self.service.sync(viewer, {'active': False})
        self.assertEqual(self.service.snapshot(viewer)['growth']['total_qi'], first)

    def test_viewer_cannot_create_guild(self):
        viewer = self.session('viewer', 'viewer')
        self.assertEqual(self.command(viewer, 'CreateGuild', {'name': '青云武灵'})['error_code'], 'FORBIDDEN')

    def test_restart_restores_receipt(self):
        self.create()
        module = importlib.import_module('Tools.CultivationService.authority')
        restarted = module.Authority(self.db, self.rules, lambda: self.now)
        self.assertEqual(restarted.snapshot(self.anchor)['guild']['name'], '青云武灵')


if __name__ == '__main__':
    unittest.main()
