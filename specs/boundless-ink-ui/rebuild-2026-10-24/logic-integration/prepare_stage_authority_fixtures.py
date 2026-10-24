"""Declared isolated branch preconditions; never client snapshots or production data."""
from pathlib import Path
import json,sys
from datetime import datetime,timezone
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];sys.path.insert(0,str(ROOT))
from Tools.CultivationService.authority import Authority
from Tools.CultivationService.rules import RuleBundle
from Tools.CultivationService.inventory import Inventory
from Tools.CultivationService.growth import Growth
OUT=HERE/'service-stage-fixtures.json';assert not OUT.exists(),'Do not silently seed a second time'
DB=HERE/'service-data-02/prototype.sqlite3';assert DB.exists()
RULES=RuleBundle.load(ROOT/'docs/design/2026-10-21-wuling-resource-v1.4');AUTH=Authority(DB,RULES)
APP='NewUIStageAcceptance20261024';rows=[]
for user,stage_id,items in [('newui-stage-cancel','BR04',{'MAT_BR04_C01_G1':2}),('newui-stage-rework','BR01',{'MAT_BR01_C01_G1':2,'MAT_BR01_C01_G2':2})]:
 stage=next(x for x in RULES.tables['items-v1.4/breakthrough_stages.csv'] if x['stage_id']==stage_id)
 body=dict(app=APP,room=user+'-room',user=user,role='anchor',display_name='声明的阶段验证样本',development_only=True,simulated=True,profile='prototype_full')
 token=AUTH.open_session(body)['token']
 with AUTH.store.transaction() as db:
  ctx=AUTH._context(db,token);before=Growth.snapshot(ctx)
  db.execute('UPDATE growth SET realm=?,qi=? WHERE scope=? AND user=?',(stage['from_realm_index'],RULES.realms[stage['from_realm_index']]['anchor_qi_to_next'],ctx.scope,ctx.user))
  ctx.money(ctx.user,'CUR_STONE',500,'declared-stage-test-fixture','stage-precondition-20261024')
  for item,count in items.items():Inventory.grant(ctx,item,count,dict(id='declared-stage-test-fixture',expiry=ctx.now+7200))
  after=Growth.snapshot(ctx)
 rows.append(dict(app=APP,room=body['room'],user=user,stage=stage,before=before,after=after,items=items))
OUT.write_text(json.dumps(dict(created_at=datetime.now(timezone.utc).isoformat(),database=str(DB),declared_test_preconditions=True,naturally_earned=False,scope='Separate app namespace and two synthetic users. Real rule stage realm and qi threshold; backend actions remain unchanged.',actors=rows),ensure_ascii=False,indent=2),encoding='utf-8')
print('Two declared stage actors prepared; no tokens exported')
