"""Assemble review documents and verify the archive; writes documentation only."""
from pathlib import Path
from datetime import datetime,timezone
import csv
import hashlib
import json
import os
import re
import zipfile

P=Path(__file__).resolve().parents[1]
PROJECT=Path(r'D:\demo\BoundlessImmortality')
ARCHIVE=P.parent/'武灵修仙策划包_v1.4_分类物品与天劫.zip'


def write(name,s): (P/name).write_text(s,encoding='utf-8')


def block(f):
    return '\n\n### '+str(f.relative_to(P)).replace('\\','/')+'\n\n```'+('json' if f.suffix=='.json' else 'csv')+'\n'+f.read_text(encoding='utf-8-sig').rstrip()+'\n```\n'


def assemble():
    docs=sorted(P.glob('[0-9][0-9]-*.md'))
    header='# 武灵修仙完整策划案 v1.4\n\n2026-10-21。含开服活动、分类物品与天劫助力。本文含规格和行级配置，全部新增游戏文件仍待实施。启用物品扩展时以15/16及items-v1.4参数为准，旧表仅作兼容，不重复导入旧物品或重复扣费。\n'
    parts=[header]
    for f in docs:parts+=['\n\n---\n\n',f.read_text(encoding='utf-8')]
    parts+=['\n\n---\n\n# 基础配置与UI清单\n\n基础经济沿v1.1，扩展覆盖项按15。资产文件盘点另见压缩包ui-existing-assets.csv。\n']
    for f in sorted((P/'tables').iterdir()):
        if f.suffix in ('.csv','.json') and f.name!='ui-existing-assets.csv':parts.append(block(f))
    parts+=['\n\n# v1.4分类配置\n\n此处含完整行配置；item_master索引与schema字段字典另见压缩包，避免与各分类表重复灌入。\n']
    for f in sorted((P/'tables/items-v1.4').iterdir()):
        if f.suffix in ('.csv','.json') and f.name not in ('item_master.csv','schema.json'):parts.append(block(f))
    write('完整策划案.md',''.join(parts))
    parts=['# 分类物品与天劫 AI交接完整版 v1.4\n\n本轮规格与完整分类参数。CSV/JSON行数据在正文末；压缩包另有总索引、字段字典和Excel。使用时不要把分类表与总索引作为两套物品重复发放。正式开关保持关闭，未生成游戏代码或UI资产。\n\n']
    for f in docs:
        if int(f.name[:2])>=15:parts+=['\n\n---\n\n',f.read_text(encoding='utf-8')]
    parts+=['\n\n# 行级配置附录\n']
    for f in sorted((P/'tables/items-v1.4').iterdir()):
        if f.suffix in ('.csv','.json') and f.name not in ('item_master.csv','schema.json'):parts.append(block(f))
    write('物品与天劫_AI交接完整版.md',''.join(parts))
    parts=['# UI专项交接完整版 v1.4\n\n39页面/子页、90文件任务均为计划。物品与天劫参数按15/16，新增39页不代表已实现39页。\n']
    for f in docs:
        if int(f.name[:2])>=11:parts+=['\n\n---\n\n',f.read_text(encoding='utf-8')]
    for name in ['ui-screens.csv','ui-files.csv','ui-art-requests.csv','ui-acceptance.csv','ui-launch-delta.csv','launch-acceptance.csv']:
        parts.append(block(P/'tables'/name))
    parts.append(block(P/'tables/items-v1.4/ui-extension.csv'))
    write('UI专项交接完整版.md',''.join(parts))
    parts=['# 青云初启开服活动交接 v1.4\n\n沿用v1.3活动；新增材料天劫另按15/16集成，不修改观众免费任务预算。\n\n',(P/'14-青云初启开服限定活动策划案.md').read_text(encoding='utf-8')]
    for name in ['launch_event.json','launch_tasks.csv','launch_guild_milestones.csv','launch_rank_candidates.csv','launch-acceptance.csv','ui-launch-delta.csv']:
        parts.append(block(P/'tables'/name))
    write('开服限定活动交接完整版.md',''.join(parts))


def verify():
    issues=[];links=0
    for f in P.rglob('*.md'):
        if 'node_modules' in f.parts:continue
        s=f.read_text(encoding='utf-8')
        if sum(line.lstrip().startswith('```') for line in s.splitlines())%2:issues.append(str(f.relative_to(P))+': unmatched code fence')
        for m in re.finditer(r'!?\[[^\]\n]*\]\(([^)\n]+)\)',s):
            target=m.group(1).strip().strip('<>').split('#',1)[0]
            if not target or re.match(r'^[a-z]+://',target,re.I):continue
            links+=1
            if target.endswith(ARCHIVE.name):continue
            if not (f.parent/target).exists():issues.append(str(f.relative_to(P))+': missing '+target)
    if issues:raise RuntimeError(issues)
    reports={n:json.loads((P/n).read_text(encoding='utf-8')) for n in ['validation-report.json','launch-planning-validation.json','ui-planning-validation.json','item-system-validation.json']}
    assert all(r['passed'] for r in reports.values())
    for report,base in [('validation-report.json',P/'tables'),('item-system-validation.json',P/'tables/items-v1.4')]:
        for name,value in reports[report].get('source_hashes',reports[report].get('source_sha256',{})).items():
            assert hashlib.sha256((base/name).read_bytes()).hexdigest()==value,('stale report',report,name)
    with (P/'tables/ui-existing-assets.csv').open(encoding='utf-8-sig',newline='') as h:assets=list(csv.DictReader(h))
    sources=json.loads((P/'ui-audit/source-inspection.json').read_text(encoding='utf-8'))
    changed_assets=[];changed_sources=[]
    for r in assets:
        rel=r.get('file') or r.get('path') or r.get('relative_path');f=PROJECT/rel
        if not f.is_file() or hashlib.sha256(f.read_bytes()).hexdigest().lower()!=r['sha256'].lower():changed_assets.append(rel)
    for r in sources:
        f=PROJECT/r['path']
        if not f.is_file() or hashlib.sha256(f.read_bytes()).hexdigest().lower()!=r['sha256'].lower():changed_sources.append(r['path'])
    export=json.loads((P/'workbook-review/export-validation.json').read_text(encoding='utf-8'))
    assert export['xlsx_integrity'] and not export['formula_errors'] and export['filterable_tables']==15
    return dict(document_version='1.4',generated_at_utc=datetime.now(timezone.utc).isoformat(),numbered_docs=16,
                baseline_checks=reports['validation-report.json']['check_count'],launch_checks=reports['launch-planning-validation.json']['check_count'],
                ui_planning_checks=reports['ui-planning-validation.json']['check_count'],item_checks=reports['item-system-validation.json']['check_count'],
                counts=reports['item-system-validation.json']['counts'],screens=39,planned_ui_file_tasks=90,
                pending_ui_cases=284,pending_launch_cases=30,pending_item_cases=30,markdown_links_checked=links,markdown_issues=issues,
                unchanged_assets_since_current_audit=len(assets)-len(changed_assets),asset_changes=changed_assets,
                unchanged_sources_since_current_audit=len(sources)-len(changed_sources),source_changes=changed_sources,
                source_or_game_assets_modified_by_this_delivery=False,runtime_tests_run=False,
                workbook_sheets=18,workbook_formula_errors=0,scope='Documentation, CSV/JSON arithmetic/references and XLSX structure; no game/platform implementation claims')


if __name__=='__main__':
    assemble()
    report=verify()
    write('delivery-integrity.json',json.dumps(report,ensure_ascii=False,indent=2)+'\n')
    with zipfile.ZipFile(ARCHIVE,'w',zipfile.ZIP_DEFLATED,compresslevel=7) as z:
        for folder,dirs,files in os.walk(P,followlinks=False):
            dirs[:]=[d for d in dirs if d not in ('node_modules','__pycache__')]
            for name in sorted(files):
                f=Path(folder)/name;z.write(f,arcname=str(Path(P.name)/f.relative_to(P)))
    with zipfile.ZipFile(ARCHIVE) as z:
        assert z.testzip() is None
        for f in ['15-分类物品与武灵经济扩展策划案.md','16-天劫与观众助力策划案.md','武灵修仙_物品与突破配置_v1.4.xlsx','tables/items-v1.4/breakthrough_material_items.csv']:
            assert z.read(P.name+'/'+f)==(P/f).read_bytes()
        report['archive_files']=len(z.infolist())
    print(json.dumps({'archive':str(ARCHIVE),'archive_bytes':ARCHIVE.stat().st_size,'report':report},ensure_ascii=False,indent=2))
