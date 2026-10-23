import importlib.util
import json
import threading
import urllib.request
from test_foundation import ServiceCase


class HttpTests(ServiceCase):
    def test_health_identifies_rules_without_opening_a_session(self):
        from Tools.CultivationService.http_server import make_server
        server = make_server(self.service, 0)
        self.addCleanup(server.server_close)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        self.addCleanup(server.shutdown)
        opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
        with opener.open(f'http://127.0.0.1:{server.server_port}/health', timeout=3) as response:
            health = json.load(response)
        self.assertEqual(health, dict(service='wuling-local-authority', protocol=1,
            profile='prototype_full', rule_version=self.rules.version, rules_hash=self.rules.sha256))
        with self.service.store.transaction() as db:
            self.assertEqual(db.execute('SELECT count(*) FROM session').fetchone()[0], 1)

    def test_loopback_http_matches_ue_contract(self):
        self.assertIsNotNone(importlib.util.find_spec('Tools.CultivationService.http_server'))
        from Tools.CultivationService.http_server import make_server
        server = make_server(self.service, 0)
        self.addCleanup(server.server_close)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        self.addCleanup(server.shutdown)
        body = json.dumps({'cursor': 0}).encode()
        request = urllib.request.Request(f'http://127.0.0.1:{server.server_port}/v1/wuling/state', data=body,
            headers={'Content-Type': 'application/json', 'Authorization': f'Bearer {self.anchor}'})
        opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
        with opener.open(request, timeout=3) as response:
            state = json.load(response)
        self.assertEqual(state['identity']['user'], 'anchor')
        self.assertEqual(state['manifest']['rule_version'], self.rules.version)
