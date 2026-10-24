"""Recover one exact pre-edit header byte sequence against the first-batch solidified SHA.
The failed legacy grant predicate prevented begin/finish records; this records the exception honestly.
"""
from pathlib import Path
import difflib,json,runpy,re
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
OLD=ROOT/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration'
rec=runpy.run_path(str(OLD/'visible-source-integration/record_source_change.py'),run_name='source_recovery_helpers')
relative='Source/BoundlessImmortality/Frontend/InkFrontendWidget.h'
target,grant=rec['check_grant'](relative)
after=target.read_bytes();text=after.decode('utf-8-sig').replace('\r\n','\n')
new='''    virtual void BuildDefaultTree(UWidgetTree* Tree);
    /** New presentation controllers reuse business state without rebuilding the legacy tree. */
    virtual bool UsesRebuiltPresentation()const{return false;}
    UFUNCTION(BlueprintPure,Category="newUI|Verification") virtual FName GetRuntimeLayoutVersion()const{return NAME_None;}
    UFUNCTION(BlueprintPure,Category="newUI|Verification") virtual FName GetRuntimePageLayoutVersion(FName Page)const{return NAME_None;}
    AInkMenuDirector* GetPresentationDirector()const{return Director.Get();}
    FName GetPresentedLeaderboardCategory()const{return LeaderboardCategory;}
    const TArray<FInkLeaderboardEntry>& GetPresentedGlobalLeaderboard()const{return GlobalLeaderboardEntries;}
'''
hooks='''    virtual void RefreshRebuiltPresentation(){}
    virtual void TickRebuiltPresentation(const FGeometry& Geometry,float DeltaSeconds){}
    virtual bool HandleRebuiltNavigation(FName Action){return false;}
    virtual void SetRebuiltFeedback(const FText& Message){}
'''
assert text.count(new)==1 and text.count(hooks)==1
prior=text.replace(new,'    void BuildDefaultTree(UWidgetTree* Tree);\n').replace(hooks,'')
proof=(OLD/'current-source-proof.json').read_bytes()
expected=next(v['after_sha256'] for v in json.loads(proof)['public_source'] if v['path']==relative)
candidates=[prior.encode(),prior.replace('\n','\r\n').encode()]
def mixed_pattern(value):
    return b''.join(re.escape(line.encode())+b'\r?\n' for line in value.splitlines())
for ending in (b'\n',b'\r\n'):
    mixed=re.sub(mixed_pattern(new),lambda _:b'    void BuildDefaultTree(UWidgetTree* Tree);'+ending,after)
    mixed=re.sub(mixed_pattern(hooks),b'',mixed)
    candidates.append(mixed)
candidates+=[b'\xef\xbb\xbf'+v for v in list(candidates)]
before=next((v for v in candidates if rec['sha'](v)==expected),None)
assert before is not None,'Exact solidified pre-edit SHA not recoverable; stop without claiming a backup'
ledger=rec['read_ledger']();assert len(ledger['changes'])==4 and all(v['state']=='recorded' for v in ledger['changes'])
folder=rec['HERE']/'backups/4';folder.mkdir(parents=True,exist_ok=False)
(folder/'before').write_bytes(before);(folder/'after').write_bytes(after);(folder/'grant.json').write_bytes(grant);(folder/'first-batch-source-proof.json').write_bytes(proof)
(folder/'change.diff').write_text(''.join(difflib.unified_diff(before.decode('utf-8-sig').splitlines(True),after.decode('utf-8-sig').splitlines(True),fromfile='before/'+relative,tofile='after/'+relative)),encoding='utf-8')
row=dict(path=relative,state='recorded',reason='Presentation hooks; recovered exact before bytes after stale grant predicate rejected begin',
    before_sha256=expected,after_sha256=rec['sha'](after),before_exists=True,backup='backups/4',grant_sha256=rec['sha'](grant),
    recorded_at=rec['now'](),backup_timing='After edit; reconstructed exact prior bytes and verified against immutable first-batch source hash',
    metadata=dict(first_batch_source_proof_sha256=rec['sha'](proof)),changed=True)
ledger['changes'].append(row);rec['save'](ledger)
print(json.dumps(row,ensure_ascii=False,indent=2))
