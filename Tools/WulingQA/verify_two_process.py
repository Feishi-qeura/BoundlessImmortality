"""Two independent HTTP client processes against an isolated SQLite authority.

The injected authority clock shortens waiting only; clients cannot submit time,
scores, balances or results. This is NOT a two-UE-process or platform test.
"""
from pathlib import Path
import json
import multiprocessing
import threading
import time
import urllib.request
import uuid

from Tools.CultivationService.authority import Authority
from Tools.CultivationService.rules import RuleBundle
from Tools.CultivationService.http_server import make_server


def worker(port, name, pipe):
    import os
    opener=urllib.request.build_opener(urllib.request.ProxyHandler({}))
    def post(route,body,token=''):
        request=urllib.request.Request(f'http://127.0.0.1:{port}/v1/wuling/{route}',data=json.dumps(body).encode(),
            headers={'Content-Type':'application/json','Authorization':'Bearer '+token})
        with opener.open(request,timeout=10) as response:return json.load(response)
    def session(user,role):
        return post('session',dict(app='two-process-qa',room=name,user=user,role=role,development_only=True,simulated=True,profile='prototype_full'))['token']
    def command(token,op,args=None):
        request_id=uuid.uuid4().hex
        for _ in range(10):
            state=post('state',{},token)
            result=post('command',dict(request_id=request_id,expected_version=state['version'],profile='prototype_full',rule_version='wuling-prototype-v1.4.0',command=op,args=args or {}),token)
            if result['error_code']=='STALE_VERSION':continue
            assert result['status']=='success',result
            return result
        raise AssertionError('repeated stale version')
    try:
        anchor=session('anchor-'+name,'anchor')
        guild=command(anchor,'CreateGuild',{'name':'青云'+name})['result']['guild']['id']
        viewer=session('viewer-'+name,'viewer')
        command(viewer,'JoinGuild',{'guild_id':guild})
        heartbeat=lambda:post('sync',dict(active=True,participants=[dict(token=viewer,active=True)]),anchor)
        heartbeat();pipe.send(dict(status='ready',pid=os.getpid()))
        while True:
            instruction=pipe.recv();kind=instruction['kind']
            if kind=='queue':
                heartbeat();command(anchor,'QueueRace',{'node_id':'NODE_STONE'});pipe.send(dict(status='queued'))
            elif kind=='heartbeat':heartbeat();pipe.send(dict(status='heartbeat'))
            elif kind=='event':
                heartbeat();race=post('state',{},anchor)['race']
                args=dict(race_id=race['id'],kind=instruction['action_kind'],action=instruction.get('action','护阵'))
                command(anchor if args['kind']=='anchor' else viewer,'SubmitRaceAction',args)
                pipe.send(dict(status='event'))
            elif kind=='result':
                state=post('state',{},anchor)
                assert state['race']['status']=='Settled',state['race']
                assert all(t['score']==1000 for t in state['race']['teams']),state['race']
                assert state['wallet']['CUR_STONE']==80,state['wallet']
                assert state['inventory']['MAT_BREAK']==12,state['inventory']
                assert state['lease']['end']-state['lease']['start']==1800,state['lease']
                pipe.send(dict(status='success',pid=os.getpid(),wallet=state['wallet'],scores=[t['score'] for t in state['race']['teams']],lease=state['lease']))
                return
    except BaseException as error:
        pipe.send(dict(status='error',error=repr(error)))


def main():
    root=Path(__file__).resolve().parents[2]
    out=root/'Saved/Codex/GameplayPlan20261021/TwoProcess'
    out.mkdir(parents=True,exist_ok=True)
    clock=[1800000000.0]
    authority=Authority(out/(uuid.uuid4().hex+'.sqlite3'),RuleBundle.load(root/'docs/design/2026-10-21-wuling-resource-v1.4'),lambda:clock[0])
    server=make_server(authority,0)
    thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
    processes=[];pipes=[];messages=[]
    try:
        for name in ('甲','乙'):
            parent,child=multiprocessing.Pipe()
            process=multiprocessing.Process(target=worker,args=(server.server_port,name,child))
            process.start();processes.append(process);pipes.append(parent)
        def collect():
            batch=[]
            for pipe in pipes:
                assert pipe.poll(20),'client timeout'
                message=pipe.recv();assert message['status']!='error',message
                batch.append(message);messages.append(message)
            return batch
        def send(**instruction):
            for pipe in pipes:pipe.send(instruction)
            return collect()
        ready=collect();assert ready[0]['pid']!=ready[1]['pid']
        send(kind='queue')
        start=clock[0]
        def advance(target):
            while clock[0]<target:
                clock[0]=min(target,clock[0]+50);send(kind='heartbeat')
        advance(start+119)
        clock[0]=start+121;authority.tick()
        events=[(t,'common') for t in (1,61,121,181,241,301)]+[(60,'representative'),(240,'representative'),(90,'anchor'),(270,'anchor')]
        for offset,kind in sorted(events):
            advance(start+120+offset)
            send(kind='event',action_kind=kind,action='护阵' if int(offset//60)%2==0 else '破障')
        advance(start+601);authority.tick()
        results=send(kind='result')
        for process in processes:process.join(10)
        assert all(p.exitcode==0 for p in processes)
        report=dict(passed=True,boundary='two independent HTTP client OS processes; deterministic server test clock; not UE/device/platform',results=results,messages=len(messages))
    except BaseException as error:
        report=dict(passed=False,error=repr(error),messages=messages)
    finally:
        server.shutdown();server.server_close()
        for process in processes:
            if process.is_alive():process.terminate();process.join(5)
    (out/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(report,ensure_ascii=False,indent=2))
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
