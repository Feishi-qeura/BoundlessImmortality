"""Stage remaining reviewed owner sources and exact Player c/e minimal patches."""
from pathlib import Path
import json,hashlib,re,shutil
HERE=Path(__file__).resolve().parent;TEAM=HERE.parent;ROOT=HERE.parents[3]
STAGE=HERE/'remaining-source-stage'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
rows=[]
def copy_group(owner,revision,folder,target,receipt):
    for source in sorted(folder.glob('*')):
        if source.suffix not in ('.h','.cpp'):continue
        relative='Source/BoundlessImmortality/Frontend/UIRebuild20261024/'+target+'/'+source.name
        dest=STAGE/relative;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,dest)
        rows.append(dict(path=relative,owner=owner,revision=revision,source=str(source),source_sha256=sha(source),receipt=str(receipt),receipt_sha256=sha(receipt),staged=str(dest),staged_sha256=sha(dest),patches=[]))
journey=TEAM/'panel-journey/review-v4'
copy_group('panel-journey','review-v4',journey/'staged-source/Frontend/UIRebuild20261024/Journey','Journey',journey/'status.json')
activities=TEAM/'panel-activities/review-v5'
delta=json.loads((activities/'source-delta-v5.json').read_text(encoding='utf-8-sig'))
for item in delta['files']:
    p=TEAM/item['replacement'];assert sha(p)==item['replacement_sha256']
copy_group('panel-activities','review-v5',activities/'staged-source','Activities',activities/'source-delta-v5.json')
for group in ('c','d','e'):
    folder=TEAM/('panel-player/player-'+group)
    receipt=TEAM/('panel-player/review-v4/receipt.md' if group=='c' else 'panel-player/review-v5/receipt.md')
    copy_group('panel-player','player-'+group,folder/'staged-source/Frontend/UIRebuild20261024/Player','Player',receipt)
assert len(rows)==14,len(rows)

def apply_reviewed_patch(patch):
    lines=patch.read_text(encoding='utf-8-sig').splitlines(keepends=True);index=0;changed=[]
    while index<len(lines):
        if not lines[index].startswith('--- '):index+=1;continue
        old_name=lines[index][4:].split('\t')[0].strip();index+=1
        assert lines[index].startswith('+++ ');new_name=lines[index][4:].split('\t')[0].strip();index+=1
        assert Path(old_name).name==Path(new_name).name
        name=Path(new_name).name;matches=[r for r in rows if Path(r['path']).name==name];assert len(matches)==1,name
        row=matches[0];target=Path(row['staged']);original=target.read_text(encoding='utf-8-sig').splitlines(keepends=True)
        output=[];cursor=0;hunks=0
        while index<len(lines) and not lines[index].startswith('--- '):
            match=re.match(r'@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@',lines[index])
            if not match:index+=1;continue
            start=int(match.group(1))-1;assert start>=cursor
            output.extend(original[cursor:start]);cursor=start;index+=1;hunks+=1
            while index<len(lines) and not lines[index].startswith(('@@ ','--- ')):
                line=lines[index];index+=1
                if line.startswith('\\ No newline'):continue
                if line.startswith(' '):assert original[cursor]==line[1:],(name,cursor);output.append(original[cursor]);cursor+=1
                elif line.startswith('-'):assert original[cursor]==line[1:],(name,cursor);cursor+=1
                elif line.startswith('+'):output.append(line[1:])
                else:break
        assert hunks;output.extend(original[cursor:]);target.write_text(''.join(output),encoding='utf-8')
        row['patches'].append(dict(path=str(patch),sha256=sha(patch),hunks=hunks));row['staged_sha256']=sha(target);changed.append(name)
    return changed

c=TEAM/'panel-player/review-v4/diff/player-c-pc3-02-03.diff'
e=TEAM/'panel-player/review-v5/diff/player-e-breakthrough-confirmation.diff'
patched=apply_reviewed_patch(c)+apply_reviewed_patch(e)
report={'staged_files':len(rows),'production_written':False,'approved':False,'patched':patched,'files':rows}
(HERE/'remaining-source-stage-manifest.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({'staged_files':len(rows),'patched':patched,'production_written':False}))
