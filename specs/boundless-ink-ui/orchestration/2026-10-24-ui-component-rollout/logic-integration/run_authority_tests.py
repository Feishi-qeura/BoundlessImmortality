"""Run real authority tests with isolated Windows-friendly temporary databases.

Python's TemporaryDirectory(mode=0700) creates inaccessible directories on this
managed host. Only substitute directory allocation, never service/test behavior.
"""
import io
import json
from pathlib import Path
import shutil
import sys
import unittest
from unittest.mock import patch
import uuid

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[4]
sys.dont_write_bytecode = True
sys.path[:0] = [str(ROOT), str(ROOT / 'Tools/CultivationService/tests')]


class IsolatedDirectory:
    def __init__(self, *args, **kwargs):
        parent = HERE / 'test-databases'
        parent.mkdir(exist_ok=True)
        target = parent / str(uuid.uuid4())
        # 0777 uses inherited workspace ACLs instead of special Windows 0700 ACLs.
        target.mkdir(mode=0o777)
        self.name = str(target)

    def cleanup(self):
        target = Path(self.name).resolve()
        if target.parent != (HERE / 'test-databases').resolve():
            raise RuntimeError('Refusing cleanup outside owned test-databases')
        if target.exists():
            shutil.rmtree(target)


def main():
    names = [
        'test_journey_map', 'test_sign_in_chests', 'test_sign_in_placeholders',
        'test_launch_progress',
        'test_economy_activities.EconomyTests.test_daily_sign_once_and_new_hong_kong_day',
        'test_economy_activities.EconomyTests.test_launch_is_once_per_anchor_and_relative_window',
    ]
    stream = io.StringIO()
    suite = unittest.defaultTestLoader.loadTestsFromNames(names)
    with patch('test_foundation.tempfile.TemporaryDirectory', IsolatedDirectory):
        result = unittest.TextTestRunner(stream=stream, verbosity=2).run(suite)
    log = stream.getvalue()
    (HERE / 'authority-tests-isolated.log').write_text(log, encoding='utf-8')
    report = dict(tests_run=result.testsRun, errors=len(result.errors), failures=len(result.failures),
                  passed=result.wasSuccessful(), scope='real Python authority; not C++ adapter or UE runtime',
                  fixture_override='temporary directory allocation only; per-test UUID under owned directory')
    (HERE / 'authority-test-result.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(log)
    return 0 if result.wasSuccessful() else 1


if __name__ == '__main__':
    raise SystemExit(main())
