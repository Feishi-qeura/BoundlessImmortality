"""Read-only persisted authority audit. Never exports bearer/session records."""
from pathlib import Path
import json,sqlite3,hashlib,time,csv
from datetime import datetime,timezone
HERE=Path(__file__).resolve().parent;F=json.loads((HERE/'service-fixture-02.json').read_text());DB=Path(F['database'])
db=sqlite3.connect('file:'+DB.as_posix()+'?mode=ro',uri=True);db.row_factory=sqlite3.Row
scope=hashlib.sha256((F['app']+'|prototype_full|'+F['rules_version']).encode()).hexdigest();user=F['user'];checks=[]
def check(name,value,actual=None):checks.append(dict(name=name,passed=bool(value),actual=actual))
def entity(kind,id):
 r=db.execute('SELECT data FROM entity WHERE scope=? AND kind=? AND id=?',(scope,kind,id)).fetchone();return json.loads(r['data']) if r else None
wallet={x['currency']:x['amount'] for x in db.execute('SELECT currency,amount FROM wallet WHERE scope=? AND owner=?',(scope,user))}
inventory={x['item']:x['n'] for x in db.execute('SELECT item,sum(count) n FROM item_instance WHERE scope=? AND owner=? AND reserved IS NULL AND expiry>? AND count>0 GROUP BY item',(scope,user,time.time()))}
guilds=[dict(r) for r in db.execute('SELECT id,anchor,name,dissolved FROM guild WHERE scope=? AND anchor=?',(scope,user))]
check('one persisted guild despite dropped post-commit reply',len(guilds)==1,guilds)
recipe_rows=list(csv.DictReader((HERE.parents[3]/'docs/design/2026-10-21-wuling-resource-v1.4/tables/items-v1.4/recipes.csv').open(encoding='utf-8-sig')));craft_cost=int(next(x for x in recipe_rows if x['recipe_id']==F['recipe'])['stone_cost'])
check('actual wallet matches declared fixture less actual shop/recipe/market/listing costs',wallet.get('CUR_STONE')==500-10-craft_cost-8-1,dict(wallet=wallet,actual_recipe_stone_cost=craft_cost))
check('actual tradeable herb12 fixture plus2 purchased restored after withdrawal',inventory.get('MAT_HERB_T')==14,inventory)
check('actual book consumed once and craft core consumed twice',inventory.get(F['book'],0)==0 and inventory.get('MAT_BR01_C01_G1')==4)
loadout=entity('loadout',user);check('actual method learned and configured; equipment unequipped',loadout and F['method'] in loadout['methods'] and loadout['main']==F['method'] and not loadout['equipment'],loadout)
if guilds:
 gid=guilds[0]['id'];build=entity('building',gid+':BUILD_01');check('actual building target1 persisted',build and build['level']==1,build)
 materials={x['item']:x['n'] for x in db.execute('SELECT item,sum(count) n FROM item_instance WHERE scope=? AND owner=? AND reserved IS NULL AND count>0 GROUP BY item',(scope,gid))}
 supply=db.execute('SELECT amount FROM wallet WHERE scope=? AND owner=? AND currency=?',(scope,gid,'CUR_SUPPLY')).fetchone();check('actual guild material and supply costs debited once',materials.get('GUILD_MAT_01')==6 and materials.get('GUILD_MAT_02')==6 and supply and supply['amount']==80,dict(materials=materials,supply=supply['amount'] if supply else None))
 member=db.execute('SELECT guild_id FROM membership WHERE scope=? AND user=?',(scope,'newui-member-fixture')).fetchone();check('actual removed member remains unassigned',member and member['guild_id'] is None)
listings=[json.loads(x['data']) for x in db.execute('SELECT data FROM entity WHERE scope=? AND kind=?',(scope,'listing'))]
check('actual seller listing sold and own listing cancelled',len(listings)==2 and sorted(x['status'] for x in listings)==['Cancelled','Sold'],[dict(id=x['id'],status=x['status'],version=x['version']) for x in listings])
trace=[json.loads(x) for x in (HERE/'service-command-trace-02.jsonl').read_text().splitlines()]
creates=[x for x in trace if x.get('event')=='command_received' and x['request']['command']=='CreateGuild'];pairs={(x['request']['request_id'],x['payload_sha256']) for x in creates}
check('actual repeated create uses one request and payload hash',len(creates)>=2 and len(pairs)==1,dict(attempts=len(creates),unique_pairs=len(pairs)))
if creates:
 request=creates[0]['request']['request_id'];n=db.execute('SELECT count(*) FROM command_receipt WHERE scope=? AND user=? AND request=?',(scope,user,request)).fetchone()[0];check('one authoritative persisted idempotency receipt',n==1,n)
db.close();report=dict(checked_at=datetime.now(timezone.utc).isoformat(),read_only=True,database=str(DB),declared_test_fixtures=True,naturally_earned=False,checks=checks,passed=all(x['passed'] for x in checks))
(HERE/'actual-service-persisted-audit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps(dict(passed=report['passed'],checks=len(checks),failed=[x for x in checks if not x['passed']]),ensure_ascii=False))
