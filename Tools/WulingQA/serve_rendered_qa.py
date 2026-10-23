"""Explicit development fixtures for rendered UE QA; never uses the default DB."""
from pathlib import Path
import argparse
import json
import uuid
import time
import hashlib
from dataclasses import replace
from Tools.CultivationService.authority import Authority
from Tools.CultivationService.rules import RuleBundle
from Tools.CultivationService.http_server import make_server
from Tools.CultivationService.inventory import Inventory


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--port',type=int,default=18771);parser.add_argument('--short-visuals',action='store_true');parser.add_argument('--freeze-clock',action='store_true');args=parser.parse_args()
    root=Path(__file__).resolve().parents[2]
    out=root/'Saved/Codex/GameplayPlan20261021/Rendered'
    out.mkdir(parents=True,exist_ok=True)
    db_path=out/(uuid.uuid4().hex+'.sqlite3')
    rules=RuleBundle.load(root/'docs/design/2026-10-21-wuling-resource-v1.4')
    if args.short_visuals:
        next(r for r in rules.tables['items-v1.4/prop_items.csv'] if r['item_id']=='PROP_03')['duration_seconds']=8
        rules=replace(rules,sha256=hashlib.sha256((rules.sha256+'|declared-QA-PROP03-8seconds').encode()).hexdigest())
    fixture_epoch=time.time()
    authority=Authority(db_path,rules,(lambda:fixture_epoch) if args.freeze_clock else time.time)
    # Deterministic failing draw is a declared test fixture, not a production probability change.
    authority.random_draw=lambda _:9999
    session=authority.open_session(dict(app='wuling-rendered-v14',room='rendered-room',user='render-anchor',role='anchor',development_only=True,simulated=True,profile='prototype_full'))
    token=session['token'];state=authority.snapshot(token)
    authority.execute(token,dict(request_id='fixture-guild',expected_version=state['version'],profile='prototype_full',rule_version=authority.rules.version,command='CreateGuild',args={'name':'青云测试武灵'}))
    with authority.store.transaction() as db:
        ctx=authority._context(db,token)
        guild=ctx.db.execute('SELECT id FROM guild WHERE scope=? AND anchor=?',(ctx.scope,ctx.user)).fetchone()[0]
        db.execute('UPDATE growth SET realm=21,qi=20000 WHERE scope=? AND user=?',(ctx.scope,ctx.user))
        ctx.money(ctx.user,'CUR_STONE',2000,'declared_qa_fixture');ctx.money(guild,'CUR_SUPPLY',500,'declared_qa_fixture')
        items={'MAT_BREAK':100,'MAT_HERB_B':100,'MAT_IRON_B':100,'MAT_PAGE':100,'MAT_HERB_T':20,'MAT_IRON_T':20,
               'MAT_BR04_C01_G4':1,'MAT_BR04_G01_G4':3,'MAT_BR04_E01_G4':4,'MAT_BR04_T01_G4':1,
               'EQ_ROBE_T1':1,'EQ_CROWN_T1':1,'EQ_BOOTS_T1':1,'EQ_PENDANT_T1':1,'FABAO_01':1,
               'BOOK_METHOD_EDGE_BOOK':2,'BOOK_METHOD_GUARD_BOOK':1,'BOOK_METHOD_ECHO_BOOK':1,'PROP_01':2,'ITEM_HEAL':2,'ITEM_SHIELD':2}
        for item,count in items.items():
            if item not in ctx.rules.items:raise ValueError('fixture references unknown item: '+item)
            Inventory.grant(ctx,item,count,dict(id='declared-rendered-fixture'))
        if args.short_visuals:Inventory.grant(ctx,'PROP_03',2,dict(id='declared-rendered-fixture',label='隔离道具生命周期测试'))
        for item in ('GUILD_MAT_01','GUILD_MAT_02','GUILD_MAT_03','GUILD_MAT_04'):Inventory.grant(ctx,item,24,dict(id='declared-rendered-fixture'),guild)
    metadata=dict(db=str(db_path),port=args.port,app='wuling-rendered-v14',room='rendered-room',user='render-anchor',
                  boundary='explicit simulated identities, seeded QA inventory and qi, deterministic roll 9999; no real economic progress',items=items)
    metadata['visual_fixture_seconds']=8 if args.short_visuals else None
    metadata['frozen_clock']=args.freeze_clock
    metadata['fixture_epoch']=fixture_epoch if args.freeze_clock else None
    (out/'fixture.json').write_text(json.dumps(metadata,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(metadata,ensure_ascii=False),flush=True)
    server=make_server(authority,args.port)
    try:server.serve_forever()
    except KeyboardInterrupt:pass
    finally:server.server_close()


if __name__=='__main__':main()
