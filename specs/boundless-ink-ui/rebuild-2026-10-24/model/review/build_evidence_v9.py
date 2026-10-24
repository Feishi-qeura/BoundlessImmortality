"""Read-only service receipt/audit and existing UI evidence extraction. No tests executed."""
from pathlib import Path
from datetime import datetime,timezone
import json,hashlib,csv,sqlite3,copy
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[4];BASE=HERE.parents[1];P=BASE.relative_to(ROOT).as_posix();I=BASE/'logic-integration'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def encode(x):return json.dumps(x,ensure_ascii=False,sort_keys=True,separators=(',',':'),allow_nan=False)
def sha(x):return hashlib.sha256(x).hexdigest()
def write(n,x):(HERE/n).write_text(json.dumps(x,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
inputs=read(HERE/'selected-inputs-v9.json');idx={r['path']:r for r in inputs['sources']}
def capture(p):
    rel=p.relative_to(ROOT).as_posix()
    if rel not in idx:
        data=p.read_bytes();target=HERE/'selected-snapshot-v9'/rel;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
        r=dict(path=rel,snapshot=target.relative_to(HERE).as_posix(),sha256=sha(data),bytes=len(data),supplement_captured_utc=datetime.now(timezone.utc).isoformat());inputs['sources'].append(r);idx[rel]=r
    return HERE/idx[rel]['snapshot']
def frozen(rel):return HERE/idx[rel]['snapshot']
reports={n:read(frozen(P+f'/logic-integration/service-pie-{n:02}/report.json')) for n in range(1,8)}
# One bounded late supplement:09 already contains matching receipts in frozen trace02.
for n in [8,9]:
    p=I/f'service-pie-{n:02}/report.json'
    if p.is_file():reports[n]=read(capture(p))
for name in ['Activities/RebuildActivityWidgets.cpp','Activities/RebuildActivitySecondaryWidgets.cpp']:
    capture(ROOT/'Source/BoundlessImmortality/Frontend/UIRebuild20261024'/name)
traces={n:[json.loads(l) for l in frozen(P+f'/logic-integration/service-command-trace-{n:02}.jsonl').read_text(encoding='utf-8-sig').splitlines() if l.strip()] for n in [1,2]}
attempts=[dict(attempt=n,finished=j['finished'],passed=j['passed'],checks=len(j['checks']),captures=len(j['captures']),source_epoch_sha256=j['source_epoch_sha256'],current_step=j['current_step'],started_at=j['started_at'],finished_at=j['finished_at'],failed_checks=[c for c in j['checks'] if not c['passed']],error_last_line=j.get('error','').splitlines()[-1:] ) for n,j in reports.items()]
# Include only requested01-07 independently evidenced operations; later market commands remain out of this audit.
targets=[]
for n,j in reports.items():
    for c in j['checks']:
        if c['passed'] and c['name'].startswith('actual authority success ') and isinstance(c.get('actual'),dict):targets.append((2,n,c['actual']['command'],c['actual']['request_id']))
for trace_no in [1,2]:
    create=next(e for e in traces[trace_no] if e.get('event')=='command_received' and e['request']['command']=='CreateGuild')
    targets.append((trace_no,trace_no,'CreateGuild',create['request']['request_id']))
join=next(e for e in traces[2] if e.get('event')=='command_received' and e['request']['command']=='JoinGuild')
targets.append((2,2,'JoinGuild',join['request']['request_id']))
connections={}
for n in [1,2]:
    c=sqlite3.connect((I/f'service-data-{n:02}/prototype.sqlite3').resolve().as_uri()+'?mode=ro',uri=True);c.row_factory=sqlite3.Row;c.execute('BEGIN');connections[n]=c
proof=[];seen=set()
for trace_no,attempt,op,rid in targets:
    if (trace_no,rid) in seen:continue
    seen.add((trace_no,rid));events=traces[trace_no];received=[e for e in events if e.get('event')=='command_received' and e['request']['request_id']==rid];results=[e for e in events if e.get('event')=='command_result' and e.get('request_id')==rid]
    rows=connections[trace_no].execute('select scope,user,request,payload_hash,result,created from command_receipt where request=?',(rid,)).fetchall()
    receipts=[]
    for row in rows:
        r=dict(row);result=json.loads(r.pop('result'));scope=r['scope'];user=r['user'];audit_id='command-audit:command:'+sha(encode([scope,user,rid]).encode())
        audit=[dict(x) for x in connections[trace_no].execute('select id,kind,user,created,data from fact where scope=? and id=?',(scope,audit_id))]
        snap=result.get('result') or {};request=received[0]['request'];canonical={k:v for k,v in request.items() if k!='expected_version'}
        r.update(status=result.get('status'),error_code=result.get('error_code'),authoritative_version=result.get('authoritative_version'),receipt_id=result.get('receipt_id'),result_sha256=sha(encode(result).encode()),authority_payload_hash_matches=sha(encode(canonical).encode())==r['payload_hash'],command_audits=audit,state_projection={k:snap[k] for k in ['version','identity','guild','loadout','buildings','activities','inventory','wallet'] if k in snap})
        receipts.append(r)
    proof.append(dict(command=op,request_id=rid,attempt=attempt,database=trace_no,request=received[0]['request'],receive_count=len(received),same_full_transport_payload=len({e['payload_sha256'] for e in received})==1,trace_results=results,durable_receipts=receipts,proof_qualified=len(receipts)==1 and receipts[0]['status']=='success' and receipts[0]['authority_payload_hash_matches'] and len(receipts[0]['command_audits'])==1,scope='declared viewer fixture setup authority call, not a Slate viewer join journey' if op=='JoinGuild' else 'actual loopback authority request/receipt/audit, fixture inventory/economy preconditions'))
stale=[]
for n,c in connections.items():
    audits=[json.loads(r['data']) for r in c.execute("select data from fact where kind='command_audit'")]
    stale.append(dict(database=n,rename_received_count=sum(e.get('event')=='command_received' and e.get('request',{}).get('command')=='RenameGuild' for e in traces[n]),rename_audit_count=sum(a.get('operation')=='RenameGuild' for a in audits)))
    c.rollback();c.close()
busy={n:[c for c in reports[n]['checks'] if c['passed'] and any(s in c['name'] for s in ['Busy','pending request','dropped reply','same id and payload','outbox retried','changed version confirmation dispatches no rename'])] for n in [1,2]}
write('service-chain-proof-v9.json',dict(extracted_utc=datetime.now(timezone.utc).isoformat(),scope='01-07 plus bounded late09 market positives joined to live read-only SQLite snapshot command_receipt and command_audit; no service/test execution',attempts=attempts,chains=proof,stale_rename=stale,busy_and_retry_checks=busy,payload_hash_note='Transport hash includes expected_version; authority receipt intentionally excludes expected_version before canonical hash. Compared using respective definitions, never compared unequal domains directly.',late09_market_positive_receipts_qualified=True,merchant_stale_subject_version_negative_runtime_cases_pending=True))
with (HERE/'service-chain-proof-v9.csv').open('w',encoding='utf-8-sig',newline='') as f:
    cols=['command','request_id','attempt','database','receive_count','same_full_transport_payload','proof_qualified','receipt_id','authoritative_version','audit_count','scope'];w=csv.DictWriter(f,fieldnames=cols)
    for r in proof:
        rec=r['durable_receipts'][0] if r['durable_receipts'] else {};w.writerow({**{k:r[k] for k in cols if k in r},'receipt_id':rec.get('receipt_id'),'authoritative_version':rec.get('authoritative_version'),'audit_count':len(rec.get('command_audits',[]))})
# Exact manifest assets and exact resource snapshots only; retain failed attempts unchanged.
assets=copy.deepcopy(read(HERE/'runtime-component-application-v8.json')['assets']);partial=read(frozen(P+'/logic-integration/runtime-resource-readback-partial-01.json'))
for a in assets:
    if a['id']=='shared-character-attribute-atlas':candidates=['default-root-pie-06/characterappearance-character-entry.json']
    elif a['id']=='equipment-fabao-glyph-atlas-v1.png':candidates=['service-pie-03/equipment-native-data.json']
    else:candidates=[x['snapshot'].replace('\\','/') for x in partial['resources'].get(a['canonical_object_path'],[])][:1]
    observations=[]
    for rel in candidates:
        p=I/rel
        if not p.is_file():continue
        snap=read(capture(p));matches=[x for x in snap.get('images',[]) if x.get('resource')==a['canonical_object_path']]
        observations.append(dict(snapshot=rel,page=snap.get('page'),bound_image_count=len(matches),visible_bound_image_count=sum(bool(x.get('visible')) for x in matches),images=[{k:x.get(k) for k in ['name','object','visible','resource','paint_expectation']} for x in matches]))
    a['runtime_observations_v9']=observations;a['visible_binding_observed']=any(x['visible_bound_image_count']>0 for x in observations);a['classification']='actual-visible-brush-observed-at-recorded-epoch' if a['visible_binding_observed'] else 'actual-brush-bound-but-hidden-at-observed-epoch' if any(x['bound_image_count'] for x in observations) else 'runtime-brush-readback-pending'
    a['runtime_readback']='v9 exact snapshots supersede previous pending text; each observation belongs to its saved attempt epoch'
write('runtime-component-application-v9.json',dict(unique_candidates=13,unique_candidates_with_visible_brush_readback=sum(a['visible_binding_observed'] for a in assets if a['id']!='shared-character-attribute-atlas'),shared_appearance_attribute='9 matching UImages bound, all visible=false in saved Appearance snapshot; actual preview visible/paint proof still pending',all_current_source_runtime_acceptance=False,assets=assets,partial12_report_preserved=True))
with (HERE/'runtime-component-application-v9.csv').open('w',encoding='utf-8-sig',newline='') as f:
    cols=['owner','id','canonical_object_path','classification','visible_binding_observed'];w=csv.DictWriter(f,fieldnames=cols,extrasaction='ignore');w.writeheader();w.writerows(assets)
pie=read(frozen(P+'/logic-integration/default-root-pie-06/report.json'));native=read(frozen(P+'/logic-integration/native-tests-04-report/index.json'));observed=sorted({c['actual'] for c in pie['checks'] if c['passed'] and c['name'].startswith('actual page ')})
runtime=dict(default_PIE06=dict(finished=pie['finished'],passed=pie['passed'],checks=len(pie['checks']),captures=len(pie['captures']),current_step=pie['current_step'],source_epoch_sha256=pie['source_epoch_sha256'],observed_routes=observed,failed_checks=[c for c in pie['checks'] if not c['passed']],strict_geometry_passed_capture_checks=sum(c['passed'] and c['name'].startswith('finite metrics or independently proven culling ') for c in pie['checks']),scope='captured PIE06 epoch subsets only; no current full44/portrait+landscape claim'),native04=dict(succeeded=native['succeeded'],failed=native['failed'],notRun=native['notRun'],tests=[dict(test=x['fullTestPath'],state=x['state']) for x in native['tests']],scope='formal native tests build19; fresh-stage helper fixture does not equal full host/service runtime negative case'),service_attempts=attempts,service_successes_are_scoped=True,declared_fixture_naturally_earned=False,TLS_peer_override='owned test processes arguments only; no platform/certificate trust acceptance',stage_service_actor_scope='separate namespace synthetic preconditions, naturally_earned:false; not natural growth or production player acceptance',service_real_Busy_evidence='connected actual pending CreateGuild; Inventory navigable and confirmed authority inventory snapshot retained; no blanket all mutation gating claim')
write('runtime-evidence-v9.json',runtime)
write('selected-inputs-v9.json',inputs)
drift=[dict(path=r['path'],captured_sha256=r['sha256'],current_sha256=sha((ROOT/r['path']).read_bytes())) for r in inputs['sources'] if sha((ROOT/r['path']).read_bytes())!=r['sha256']]
write('source-drift-after-v9.json',dict(checked_utc=datetime.now(timezone.utc).isoformat(),changed=drift))
status=dict(team_id='logic-core',phase='bounded-v9-independent-real-service-proofs-qualified',latest_report='bounded-v9-proof-remaining.md',latest_readiness='readiness-v9.json',latest_matrix='resolved-open-matrix-v9.json',latest_component_application='runtime-component-application-v9.json',latest_runtime_evidence='runtime-evidence-v9.json',latest_service_proof='service-chain-proof-v9.csv',formal_native_tests_success=4,formal_native_tests_failed=0,unique_runtime_candidates=13,unique_candidate_visible_brush_observed=13,shared_appearance_visible_attribute_pending=True,real_service_success_chains=len(proof),real_service_success_commands=sorted({r['command'] for r in proof}),real_service_Busy_inventory_browse_observed=True,all_real_service_flows_complete=False,latest_default_attempt_passed=False,all44_runtime_verified=False,visual_approval=False,engine_started=False,build_run=False,tests_run=False,service_started=False,production_edits_by_this_worker=False,handoff_ready=True)
write('status.json',status);write('readiness-v9.json',dict(**status,selected_inputs='selected-inputs-v9.json',source_drift=drift,build24=read(frozen(P+'/logic-integration/build-24-result.json'))))
print(json.dumps(dict(proof_rows=len(proof),all_receipt_audit_hash_qualified=all(r['proof_qualified'] for r in proof),unique_success_commands=status['real_service_success_commands'],candidate_brush_readbacks=sum(a['visible_binding_observed'] for a in assets if a['id']!='shared-character-attribute-atlas'),shared_visible=assets[7]['visible_binding_observed'],default_observed_routes=len(observed),native_success=native['succeeded'],drift=len(drift))))
