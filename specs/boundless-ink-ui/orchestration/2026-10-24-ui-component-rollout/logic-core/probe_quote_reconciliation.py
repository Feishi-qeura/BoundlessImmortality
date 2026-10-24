"""Narrow real-service request-chain probe. No UE, HTTP listener, Source or service edits."""
import hashlib
import json
from pathlib import Path
import sys
import uuid

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[4]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT))
from Tools.CultivationService.authority import Authority
from Tools.CultivationService.rules import RuleBundle
from Tools.CultivationService.inventory import Inventory

INPUTS = [
    'Tools/CultivationService/authority.py', 'Tools/CultivationService/breakthrough.py',
    'Tools/CultivationService/tribulation.py', 'Tools/CultivationService/inventory.py',
    'Tools/CultivationService/http_server.py',
    'Source/BoundlessImmortality/Wuling/WulingPrototypeSubsystem.cpp',
    'Source/BoundlessImmortality/Frontend/UiRollout20261024/LogicCore/InkCoreAdapter.cpp',
    'Source/BoundlessImmortality/Frontend/UiRollout20261024/LogicCore/InkCoreMaterialBinding.cpp',
    'Source/BoundlessImmortality/Frontend/UI/InkWulingToolsPanel.cpp',
]


def hashes():
    return {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest() for name in INPUTS}


before = hashes()
RUN = HERE / 'quote-reconciliation-probe' / ('run-' + uuid.uuid4().hex)
RUN.mkdir(parents=True)
checks = []
trace = []
rules = RuleBundle.load(ROOT / 'docs/design/2026-10-21-wuling-resource-v1.4')


def check(condition, label):
    checks.append(dict(check=label, passed=bool(condition)))
    if not condition:
        raise AssertionError(label)


class Case:
    def __init__(self, name):
        self.name = name
        self.now = 1800000000.0
        self.service = Authority(RUN / (name + '.sqlite3'), rules, lambda: self.now)
        self.service.random_draw = lambda _: 0
        self.token = self.session('anchor', 'anchor')

    def session(self, user, role):
        return self.service.open_session(dict(development_only=True, simulated=True,
            profile='prototype_full', app='logic-core-reconciliation', room='room-a', user=user, role=role))['token']

    def seed(self, owned=True, safe=True):
        with self.service.store.transaction() as db:
            ctx = self.service._context(db, self.token)
            db.execute('UPDATE growth SET realm=21,qi=10000 WHERE scope=? AND user=?', (ctx.scope, ctx.user))
            if owned:
                Inventory.grant(ctx, 'MAT_BR04_C01_G1', 1, {'id': 'reconciliation-fixture'})
            if not safe:
                ctx.put('combat', ctx.user, dict(health=1000, max_health=1000, last_combat=0, safe=False))

    def command(self, operation, args, version=None, request=None, token=None):
        token = token or self.token
        state = self.service.snapshot(token)
        body = dict(request_id=request or ('probe-' + uuid.uuid4().hex),
            profile='prototype_full', rule_version=rules.version,
            expected_version=state['version'] if version is None else version,
            command=operation, args=args)
        # Exactly the JSON request/response objects handled by /v1/wuling/command.
        # This intentionally does not claim an HTTP or UE runtime test.
        response = json.loads(json.dumps(self.service.execute(token, body), ensure_ascii=False, allow_nan=False))
        index = len(trace)
        request_file = f'{index:02d}-{self.name}-{operation}-request.json'
        response_file = f'{index:02d}-{self.name}-{operation}-response.json'
        (RUN / request_file).write_text(json.dumps(body, ensure_ascii=False, indent=2), encoding='utf-8')
        (RUN / response_file).write_text(json.dumps(response, ensure_ascii=False, indent=2), encoding='utf-8')
        trace.append(dict(case=self.name, operation=operation, request=request_file, response=response_file,
                          status=response['status'], error_code=response['error_code']))
        return body, response


selection = dict(selection=[dict(item_id='MAT_BR04_C01_G1', count=1)])
normal = Case('normal')
normal.seed()
_, quoted = normal.command('QuoteBreakthrough', selection)
check(quoted['status'] == 'success', 'real quote succeeds')
quote = quoted['result']['breakthrough_quote']
missing = [key for key in ['q_normalized', 'growth_min', 'growth_max', 'readiness', 'slots', 'sources'] if key not in quote]
check(len(missing) == 6, 'six presentation-only fields actually absent in successful raw result')
check(quote['stage']['from_realm_index'] == 21 and quote['stage']['to_realm_index'] == 22, 'stage indices available at nested real paths')
_, stale = normal.command('PrepareBreakthrough', selection, version=0)
check(stale['error_code'] == 'STALE_VERSION', 'stale version rejected before mutation')
body, prepared = normal.command('PrepareBreakthrough', selection)
check(prepared['status'] == 'success', 'prepare succeeds with the same six fields absent')
attempt_id = prepared['result']['tribulation']['id']
check(prepared['result']['tribulation']['status'] == 'Prepare', 'real attempt entered Prepare')
_, replay = normal.command('PrepareBreakthrough', selection, request=body['request_id'], version=body['expected_version'])
check(replay == prepared, 'same request id replays receipt instead of another attempt')
check(normal.service.snapshot(normal.token)['tribulation']['id'] == attempt_id, 'replay retains the one actual attempt')

normal.now += 76
state = normal.service.snapshot(normal.token)
check(state['tribulation']['status'] == 'Succeeded', 'fixture authority settlement unlocks real rework prerequisite')
with normal.service.store.transaction() as db:
    ctx = normal.service._context(db, normal.token)
    old = ctx.entity('quality', ctx.user + ':BR04')
    options = [row for row in rules.tables['items-v1.4/breakthrough_material_items.csv']
               if row['stage_id'] == 'BR04' and row['category'] == 'core' and row['quality_weight'] > old['core_score']]
    upgrade = max(options, key=lambda row: row['quality_weight'])
    Inventory.grant(ctx, upgrade['item_id'], 1, {'id': 'rework-fixture'})
rework_selection = dict(selection=[dict(item_id=upgrade['item_id'], count=1)])
_, rework_quote = normal.command('QuoteReworkQuality', rework_selection)
check(rework_quote['status'] == 'success', 'real rework quote succeeds')
check(rework_quote['result']['breakthrough_quote']['kind'] == 'rework', 'rework mode attested at quote.kind')
check('probability_basis_points' not in rework_quote['result']['breakthrough_quote'], 'rework quote itself provides no random trial probability')
_, reworked = normal.command('ReworkQuality', rework_selection)
check(reworked['error_code'] == 'ACTIVITY_BUSY', 'rework quote does not bypass authoritative attempt cleanup')
normal.now += 8
normal.service.snapshot(normal.token)
_, reworked = normal.command('ReworkQuality', rework_selection)
check(reworked['status'] == 'success', 'real rework commits without normalized/readiness/slots/sources')

unowned = Case('unowned')
unowned.seed(owned=False)
_, unowned_quote = unowned.command('QuoteBreakthrough', selection)
check(unowned_quote['status'] == 'success', 'successful quote does not certify inventory ownership')
_, unowned_prepare = unowned.command('PrepareBreakthrough', selection)
check(unowned_prepare['error_code'] == 'INSUFFICIENT_FUNDS', 'actual prepare still checks current inventory')

unsafe = Case('unsafe')
unsafe.seed(safe=False)
_, unsafe_quote = unsafe.command('QuoteBreakthrough', selection)
check(unsafe_quote['status'] == 'success', 'successful quote does not certify safe combat state')
_, unsafe_prepare = unsafe.command('PrepareBreakthrough', selection)
check(unsafe_prepare['error_code'] == 'NOT_SAFE', 'actual prepare still checks authoritative safety')

direct = Case('direct-no-preview')
direct.seed()
_, direct_prepare = direct.command('PrepareBreakthrough', selection)
check(direct_prepare['status'] == 'success', 'service protocol does not require a prior quote object for legal prepare')
viewer = direct.session('viewer', 'viewer')
_, viewer_prepare = direct.command('PrepareBreakthrough', selection, token=viewer)
check(viewer_prepare['error_code'] == 'FORBIDDEN', 'viewer cannot submit anchor breakthrough')

after = hashes()
check(before == after, 'all consumed frozen Source/service inputs unchanged')
report = dict(test_only=True, scope='narrow in-process actual Authority protocol; no HTTP/UE runtime',
              checks=checks, passed=all(row['passed'] for row in checks), requests=trace,
              missing_normal_quote_fields=missing, frozen_hashes_before=before, frozen_hashes_after=after,
              run_directory=str(RUN))
(HERE / 'quote-reconciliation-evidence.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps(dict(passed=report['passed'], checks=len(checks), requests=len(trace), run_directory=str(RUN))))
