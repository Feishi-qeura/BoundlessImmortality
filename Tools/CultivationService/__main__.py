from pathlib import Path
import argparse
import json
import os
import sys
import threading
from .authority import Authority
from .rules import RuleBundle
from .http_server import make_server


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description='Loopback-only isolated Wuling prototype; no real identity or payments.')
    parser.add_argument('--port', type=int, default=18770)
    parser.add_argument('--db', type=Path, default=root / 'Saved/WulingPrototype/v1.4/prototype.sqlite3')
    parser.add_argument('--rules', type=Path, default=root / 'docs/design/2026-10-21-wuling-resource-v1.4')
    parser.add_argument('--ready-file', type=Path, help='Write the bound loopback port atomically for an owning launcher.')
    parser.add_argument('--parent-stdin', action='store_true', help='Stop when the owning launcher closes stdin.')
    args = parser.parse_args()
    if args.port != 0 and not 1024 <= args.port <= 65535: parser.error('port must be 0 or 1024..65535')
    authority = Authority(args.db, RuleBundle.load(args.rules))
    server = make_server(authority, args.port)
    if args.ready_file:
        args.ready_file.parent.mkdir(parents=True, exist_ok=True)
        pending = args.ready_file.with_suffix('.pending')
        pending.write_text(json.dumps(dict(port=server.server_port, pid=os.getpid())), encoding='utf-8')
        pending.replace(args.ready_file)
    if args.parent_stdin:
        def monitor_parent():
            try:
                while sys.stdin.buffer.read(1):
                    pass
            finally:
                server.shutdown()
        threading.Thread(target=monitor_parent, daemon=True).start()
    print(f'Wuling {authority.rules.version} listening on 127.0.0.1:{server.server_port}; isolated DB {args.db}', flush=True)
    try: server.serve_forever()
    except KeyboardInterrupt: pass
    finally:
        server.server_close()
        if args.ready_file:
            args.ready_file.unlink(missing_ok=True)


if __name__ == '__main__': main()
