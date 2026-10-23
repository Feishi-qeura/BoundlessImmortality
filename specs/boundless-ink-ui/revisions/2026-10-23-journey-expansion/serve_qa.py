"""Explicit isolated UI fixture. Never uses the player's database or real identities."""
import sys,json,time,uuid
from pathlib import Path
ROOT=Path(__file__).resolve().parents[4];sys.path.insert(0,str(ROOT))
from Tools.CultivationService.authority import Authority
from Tools.CultivationService.rules import RuleBundle
from Tools.CultivationService.http_server import make_server

OUT=ROOT/'Saved/Codex/JourneyExpansion20261023';OUT.mkdir(parents=True,exist_ok=True)
authority=Authority(OUT/(uuid.uuid4().hex+'.sqlite3'),RuleBundle.load(ROOT/'docs/design/2026-10-21-wuling-resource-v1.4'))
session=authority.open_session(dict(app='journey-qa',room='journey-room',user='journey-anchor',role='anchor',development_only=True,simulated=True,profile='prototype_full'))
token=session['token']
# A declared three-day attendance fixture enables a normal monthly claim in PIE.
# It does not itself grant any reward, item, money or receipt.
with authority.store.transaction() as db:
    ctx=authority._context(db,token);ctx.count(ctx.user,'sign-month:'+ctx.day[:7],3)
metadata=dict(boundary='Isolated simulated UI QA. Attendance count=3 is an explicit fixture; claiming uses the real transaction. No seeded economy or owned resource.',app='journey-qa',room='journey-room',user='journey-anchor',port=18778)
(OUT/'fixture.json').write_text(json.dumps(metadata,ensure_ascii=False,indent=2),encoding='utf-8')
server=make_server(authority,18778)
print(json.dumps(metadata,ensure_ascii=False),flush=True)
try:server.serve_forever()
finally:server.server_close()
