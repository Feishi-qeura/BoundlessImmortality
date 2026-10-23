"""Two actual UE editor/PIE processes and one declared, manually stepped QA clock.

No rendering/device claim. Each UE uses native transport and simulated comments;
the coordinator advances time only after both native clients acknowledge facts.
"""
from pathlib import Path
import json
import subprocess
import threading
import time
import uuid
from Tools.CultivationService.authority import Authority
from Tools.CultivationService.rules import RuleBundle
from Tools.CultivationService.http_server import make_server


def main():
    root=Path(__file__).resolve().parents[2];out=root/'Saved/Codex/GameplayPlan20261021/TwoUE'
    out.mkdir(parents=True,exist_ok=True);run=uuid.uuid4().hex;clock=[1800000000.0]
    authority=Authority(out/(run+'.sqlite3'),RuleBundle.load(root/'docs/design/2026-10-21-wuling-resource-v1.4'),lambda:clock[0])
    server=make_server(authority,18772);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
    def write(value):
        temp=out/'instruction.pending';temp.write_text(json.dumps(dict(value,run=run)),encoding='utf-8');temp.replace(out/'instruction.json')
    write(dict(event='boot',utc=clock[0]))
    processes=[];report=dict(passed=False,boundary='two actual Windows UE PIE processes, NullRHI; manually stepped authority clock; not rendered/device/platform',run=run)
    def wait_for(event,timeout=100):
        end=time.monotonic()+timeout
        while time.monotonic()<end:
            rows=[]
            for side in ('a','b'):
                try:row=json.loads((out/(side+'.json')).read_text(encoding='utf-8'))
                except (FileNotFoundError,json.JSONDecodeError):break
                if row.get('run')!=run or row.get('event')!=event:break
                if row.get('error'):raise RuntimeError(row)
                rows.append(row)
            if len(rows)==2:return rows
            if any(p.poll() is not None for p in processes):raise RuntimeError('UE exited before '+event)
            time.sleep(.1)
        raise TimeoutError(event)
    def advance_presence(target):
        # Clock jumps stay inside the 60-second disconnect grace. Observed online
        # growth remains limited to heartbeat+5; no fabricated continuous presence.
        while clock[0]+4<target:
            clock[0]=min(clock[0]+50,target-2);event='presence-'+str(int(clock[0]))
            write(dict(event=event,utc=clock[0]));wait_for(event,30)
    try:
        editor=Path('F:/UNREAL/UE_5.8/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe')
        for side in ('a','b'):
            args=[str(editor),str(root/'BoundlessImmortality.uproject'),'-ExecCmds=py '+str(root/'Tools/WulingQA/two_ue_client.py'),
                  '-WulingPrototype','-GWLocalTest','-WulingApp=two-ue-'+run,'-WulingRoom=two-ue-'+side,'-WulingUser=ue-anchor-'+side,
                  '-WulingUrl=http://127.0.0.1:18772','-WulingQASide='+side,'-saveddirsuffix=WulingTwoUE'+side,
                  '-abslog='+str(out/(side+'.log')),'-NullRHI','-unattended','-nosplash','-nop4','-nosound','-NoAssetRegistryCache','-NoSaveOnExit']
            processes.append(subprocess.Popen(args,creationflags=subprocess.CREATE_NO_WINDOW))
        ready=wait_for('ready',150);report['pids']=[r['pid'] for r in ready]
        assert len(set(report['pids']))==2
        start=ready[0]['race_start'];assert ready[1]['race_start']==start
        advance_presence(start+119);clock[0]=start+119;write(dict(event='warmup',utc=clock[0]));wait_for('warmup',30)
        events=sorted([(t,'common') for t in (1,61,121,181,241,301)]+[(60,'representative'),(240,'representative'),(90,'anchor'),(270,'anchor')])
        report['events']=[]
        for i,(offset,kind) in enumerate(events):
            advance_presence(start+120+offset)
            clock[0]=start+120+offset;event='action-'+str(i)
            write(dict(event=event,utc=clock[0],kind=kind,slot=int(offset//60),action='护阵' if int(offset//60)%2==0 else '破障'))
            report['events'].append(wait_for(event,30))
        advance_presence(start+601);clock[0]=start+601;write(dict(event='result',utc=clock[0]));result=wait_for('result',30)
        for row in result:
            assert row['score']==1000 and row['stone']==80 and row['material']==12 and row['lease_seconds']==1800,row
        report.update(passed=True,results=result)
        write(dict(event='quit',utc=clock[0]))
        for process in processes:process.wait(40)
        report['exit_codes']=[p.returncode for p in processes]
        assert all(p.returncode==0 for p in processes),report['exit_codes']
    except BaseException as error:report.update(passed=False,error=repr(error))
    finally:
        write(dict(event='quit',utc=clock[0]))
        for process in processes:
            try:process.wait(20)
            except subprocess.TimeoutExpired:process.terminate();process.wait(10)
        server.shutdown();server.server_close()
    (out/'report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(report,ensure_ascii=False,indent=2));return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
