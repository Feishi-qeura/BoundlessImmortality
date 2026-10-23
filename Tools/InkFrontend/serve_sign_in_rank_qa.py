"""Isolated sign-in authority fixture for rendered UI QA; never opens the normal save DB."""
import sys,time
from pathlib import Path
from datetime import datetime,timedelta
from zoneinfo import ZoneInfo
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT))
from Tools.CultivationService.authority import Authority
from Tools.CultivationService.rules import RuleBundle
from Tools.CultivationService.http_server import make_server
out=ROOT/'Saved/Codex/SignInRank20261022'
db=out/('qa-'+str(time.time_ns())+'.sqlite3')
service=Authority(db,RuleBundle.load(ROOT/'docs/design/2026-10-21-wuling-resource-v1.4'))
token=service.open_session(dict(development_only=True,simulated=True,profile='prototype_full',app='signin-rank-qa',room='qa',user='qa-anchor',role='anchor'))['token']
with service.store.transaction() as conn:
    ctx=service._context(conn,token)
    if ctx.count(ctx.user,'fixture-seeded')==0:
        today=datetime.strptime(ctx.day,'%Y-%m-%d').date()
        for i in range(1,8):
            day=(today-timedelta(days=i)).isoformat()
            ctx.count(ctx.user,'sign:'+day,1)
        ctx.count(ctx.user,'sign-month:'+ctx.day[:7],7)
        ctx.count(ctx.user,'fixture-seeded',1)
server=make_server(service,18773)
print('Isolated sign-in/rank fixture ready on port 18773',flush=True)
try:server.serve_forever()
finally:server.server_close()
