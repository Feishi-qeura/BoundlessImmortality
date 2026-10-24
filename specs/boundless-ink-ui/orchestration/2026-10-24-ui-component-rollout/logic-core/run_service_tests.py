"""Run real authority regressions with every temporary DB inside this team's directory."""
import json
from pathlib import Path
import sys
import tempfile
import unittest
import uuid

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[4]
sys.dont_write_bytecode = True
sys.path[:0] = [str(ROOT), str(ROOT / 'Tools/CultivationService/tests')]
DB_ROOT = HERE / 'test-databases'
DB_ROOT.mkdir(exist_ok=True)
class LocalTestDirectory:
    # Python's mkdtemp(mode=0700) creates unreadable ACLs in this Windows sandbox.
    # Ordinary mkdir inherits the authorized workspace ACL. Keep each isolated DB
    # as local evidence rather than recursively deleting any directory.
    def __init__(self, *args, **kwargs):
        path = DB_ROOT / ('run-' + uuid.uuid4().hex)
        path.mkdir()
        self.name = str(path)

    def cleanup(self):
        pass

    def __enter__(self):
        return self.name

    def __exit__(self, *_):
        self.cleanup()


tempfile.TemporaryDirectory = LocalTestDirectory
from test_foundation import ServiceCase


class AdapterContractProbe(ServiceCase):
    def test_real_quote_records_missing_presentation_fields_without_inventing_them(self):
        from Tools.CultivationService.inventory import Inventory
        with self.service.store.transaction() as db:
            ctx = self.service._context(db, self.anchor)
            db.execute('UPDATE growth SET realm=21,qi=10000 WHERE scope=? AND user=?', (ctx.scope, ctx.user))
            Inventory.grant(ctx, 'MAT_BR04_C01_G1', 1, {'id': 'logic-core-probe'})
        selection = [{'item_id': 'MAT_BR04_C01_G1', 'count': 1}]
        result = self.command(self.anchor, 'QuoteBreakthrough', {'selection': selection})
        self.assertEqual(result['status'], 'success', result)
        snapshot = result['result']
        quote = snapshot['breakthrough_quote']
        self.assertEqual(quote['selection'], selection)
        self.assertIsInstance(quote['q'], int)
        self.assertIn('probability_basis_points', quote)
        wanted = ['q_normalized', 'growth_min', 'growth_max', 'readiness', 'slots', 'sources']
        missing = [key for key in wanted if key not in quote]
        # This is a recorded contract gap, not a passing populated UI acceptance.
        self.assertEqual(missing, wanted)
        (HERE / 'real-quote-probe.json').write_text(json.dumps({
            'test_only': True, 'source': 'Tools/CultivationService/breakthrough.py',
            'snapshot': snapshot, 'missing_presentation_fields': missing,
            'ui_submission_expected': 'blocked_missing_authority_fields',
        }, ensure_ascii=False, indent=2), encoding='utf-8')


if __name__ == '__main__':
    suite = unittest.TestSuite()
    for name in ['test_foundation', 'test_world_inventory', 'test_tribulation', 'test_focus_loadouts', 'test_metrics']:
        suite.addTests(unittest.defaultTestLoader.loadTestsFromName(name))
    suite.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(AdapterContractProbe))
    with (HERE / 'service-tests.log').open('w', encoding='utf-8') as stream:
        result = unittest.TextTestRunner(stream=stream, verbosity=2).run(suite)
    report = dict(tests_run=result.testsRun, failures=len(result.failures), errors=len(result.errors),
                  skipped=len(result.skipped), passed=result.wasSuccessful(),
                  scope='real Python authority; not Unreal adapter runtime or PIE')
    (HERE / 'service-test-result.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report))
    raise SystemExit(0 if result.wasSuccessful() else 1)
