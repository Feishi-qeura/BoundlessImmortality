from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
from .contracts import encode, RuleError


def make_server(authority, port=18770):
    class Server(ThreadingHTTPServer):
        daemon_threads = True

        def service_actions(self):
            try: authority.tick()
            except RuleError: pass  # Failed transactions roll back; clients remain pending rather than receiving a false receipt.

    class Handler(BaseHTTPRequestHandler):
        def setup(self):
            super().setup()
            self.connection.settimeout(5)

        def log_message(self, format_, *args):
            # Default request paths may be logged; bearer tokens and bodies never are.
            pass

        def do_GET(self):
            if self.path == '/health':
                self.reply(200, dict(service='wuling-local-authority', protocol=1,
                    profile='prototype_full', rule_version=authority.rules.version,
                    rules_hash=authority.rules.sha256))
            else:
                self.reply(404, dict(error_code='NOT_FOUND'))

        def reply(self, status, response):
            payload = encode(response).encode('utf-8')
            self.send_response(status)
            self.send_header('Content-Type', 'application/json; charset=utf-8')
            self.send_header('Content-Length', str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)

        def do_POST(self):
            status, response = 200, None
            try:
                length = int(self.headers.get('Content-Length', '0'))
                if not 0 < length <= 262144:
                    raise RuleError('INVALID_BODY_SIZE')
                def invalid_constant(_): raise RuleError('INVALID_NUMBER')
                body = json.loads(self.rfile.read(length), parse_constant=invalid_constant)
                if not isinstance(body, dict): raise RuleError('INVALID_ARGUMENT')
                token = self.headers.get('Authorization', '').removeprefix('Bearer ')
                if self.path == '/v1/wuling/session': response = authority.open_session(body)
                elif self.path == '/v1/wuling/state': response = authority.snapshot(token, body.get('cursor', 0))
                elif self.path == '/v1/wuling/sync': response = authority.sync(token, body)
                elif self.path == '/v1/wuling/command': response = authority.execute(token, body)
                else:
                    status, response = 404, dict(error_code='NOT_FOUND')
            except (ValueError, TypeError, KeyError) as error:
                code = str(error) if isinstance(error, RuleError) else 'INVALID_ARGUMENT'
                status, response = (401 if code == 'UNAUTHENTICATED' else 400), dict(error_code=code)
            except Exception:
                status, response = 500, dict(error_code='PENDING_RECONCILE')
            self.reply(status, response)

    return Server(('127.0.0.1', port), Handler)
