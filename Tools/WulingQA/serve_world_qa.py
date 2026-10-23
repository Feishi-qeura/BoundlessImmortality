"""Unseeded realm-zero flight QA, with explicit server-only idle-time skips."""
from pathlib import Path
import json
import threading
import time
import uuid
from Tools.CultivationService.authority import Authority
from Tools.CultivationService.rules import RuleBundle
from Tools.CultivationService.http_server import make_server


def main():
    root=Path(__file__).resolve().parents[2];out=root/'Saved/Codex/GameplayPlan20261021/WorldFlight';out.mkdir(parents=True,exist_ok=True)
    origin=time.monotonic();offset=[0.0];clock=lambda:1800000000+time.monotonic()-origin+offset[0]
    db=out/(uuid.uuid4().hex+'.sqlite3');service=Authority(db,RuleBundle.load(root/'docs/design/2026-10-21-wuling-resource-v1.4'),clock)
    (out/'fixture.json').write_text(json.dumps(dict(db=str(db),app='world-flight-v14',user='world-anchor',room='world-flight',boundary='no seeded items/qi, realm zero; server skips unused phase wait only')),encoding='utf-8')
    (out/'control.json').write_text('{}',encoding='utf-8')
    stop=threading.Event()
    def control():
        while not stop.wait(.1):
            try:
                request=json.loads((out/'control.json').read_text(encoding='utf-8'))
                desired=request.get('utc',0)
                if desired>clock():offset[0]+=desired-clock()
            except (FileNotFoundError,json.JSONDecodeError):pass
    thread=threading.Thread(target=control,daemon=True);thread.start();server=make_server(service,18773)
    print(json.dumps(dict(db=str(db),port=18773)),flush=True)
    try:server.serve_forever()
    except KeyboardInterrupt:pass
    finally:stop.set();server.server_close()


if __name__=='__main__':main()
