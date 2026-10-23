import importlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]


class LauncherTests(unittest.TestCase):
    def test_portrait_default_and_explicit_display_override(self):
        from Tools.WindowsRelease.launcher import display_arguments
        self.assertEqual(display_arguments([]), ['-windowed', '-ResX=540', '-ResY=960'])
        for explicit in [['-ResX=1280', '-ResY=720'], ['-fullscreen'], ['-windowedfullscreen']]:
            self.assertEqual(display_arguments(explicit), explicit)

    def test_packaged_service_ready_and_owned_process_cleanup(self):
        launcher = importlib.import_module('Tools.WindowsRelease.launcher')
        from Tools.CultivationService.rules import RuleBundle
        rules = ROOT / 'docs/design/2026-10-21-wuling-resource-v1.4'
        expected = RuleBundle.load(rules).sha256
        with tempfile.TemporaryDirectory() as folder:
            data = Path(folder) / 'userdata'
            with launcher.LocalAuthority(sys.executable, ROOT, rules, data, expected) as service:
                self.assertTrue(service.endpoint.startswith('http://127.0.0.1:'))
                self.assertIsNone(service.process.poll())
                pid = service.process.pid
                self.assertEqual(service.health()['rules_hash'], expected)
                self.assertTrue((data / 'prototype.sqlite3').is_file())
            self.assertIsNotNone(service.process.poll())
            self.assertEqual(service.process.pid, pid)
            self.assertFalse(service.ready_file.exists())
            # Original ledger survives a subsequent service process.
            with launcher.LocalAuthority(sys.executable, ROOT, rules, data, expected) as restarted:
                self.assertEqual(restarted.health()['rules_hash'], expected)

    def test_lock_prevents_same_profile_second_launch_and_releases_on_error(self):
        launcher = importlib.import_module('Tools.WindowsRelease.launcher')
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'profile.lock'
            with launcher.ProfileLock(path):
                with self.assertRaisesRegex(RuntimeError, 'PROFILE_ALREADY_RUNNING'):
                    with launcher.ProfileLock(path):
                        self.fail('second launcher took profile lock')
            with launcher.ProfileLock(path):
                pass

    def test_wrong_rules_never_start_game_and_clean_up_service(self):
        launcher = importlib.import_module('Tools.WindowsRelease.launcher')
        rules = ROOT / 'docs/design/2026-10-21-wuling-resource-v1.4'
        with tempfile.TemporaryDirectory() as folder:
            service = launcher.LocalAuthority(sys.executable, ROOT, rules, Path(folder), '0' * 64)
            with self.assertRaisesRegex(RuntimeError, 'RULE_HASH_MISMATCH'):
                with service:
                    self.fail('invalid authority accepted')
            self.assertIsNotNone(service.process.poll())
            self.assertFalse(service.ready_file.exists())

    def test_short_startup_failure_has_diagnostic_and_no_hanging_process(self):
        launcher = importlib.import_module('Tools.WindowsRelease.launcher')
        with tempfile.TemporaryDirectory() as folder:
            service = launcher.LocalAuthority(sys.executable, ROOT, Path(folder) / 'missing rules', Path(folder), '0' * 64)
            with self.assertRaisesRegex(RuntimeError, 'AUTHORITY_EXITED'):
                with service:
                    self.fail('missing rules accepted')
            self.assertIsNotNone(service.process.poll())

    def test_userdata_identity_survives_restart_and_rejects_corruption(self):
        launcher = importlib.import_module('Tools.WindowsRelease.launcher')
        with tempfile.TemporaryDirectory() as folder:
            data = Path(folder)
            identity = launcher.local_identity(data)
            self.assertEqual(launcher.local_identity(data), identity)
            self.assertEqual(len(identity), 32)
            path = data / 'identity.json'
            path.write_text('{"id":"broken"}', encoding='utf-8')
            with self.assertRaisesRegex(RuntimeError, 'INVALID_LOCAL_IDENTITY'):
                launcher.local_identity(data)
            self.assertEqual(path.read_text(encoding='utf-8'), '{"id":"broken"}')

    def test_connect_rejects_non_loopback_and_wrong_protocol(self):
        launcher = importlib.import_module('Tools.WindowsRelease.launcher')
        for url in ('http://example.com:18770', 'http://127.0.0.1:12', 'http://127.0.0.1:70000',
                    'http://127.0.0.1:18770/remote', 'https://127.0.0.1:18770'):
            with self.subTest(url=url), self.assertRaisesRegex(RuntimeError, 'INVALID_LOCAL_ENDPOINT'):
                launcher.read_health(url)
        with self.assertRaisesRegex(RuntimeError, 'AUTHORITY_PROTOCOL_MISMATCH'):
            launcher.validate_health(dict(rules_hash='0' * 64), '0' * 64)


if __name__ == '__main__':
    unittest.main()
