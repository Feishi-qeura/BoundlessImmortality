"""Write a bounded evidence summary and a separate acceptance matrix; never edit design originals."""
import csv
import json
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'Saved/Codex/UIRefactor20261021/Resume20261021'

def read(relative):
    path = OUT / relative
    return json.loads(path.read_text(encoding='utf-8-sig')) if path.is_file() else {}

def run():
    pie = read('Verification/pie-report.json')
    contract = read('Verification/hud-contract.json')
    native = read('FinalUIAutomation/index.json')
    protected = read('preservation-final.json')
    assets = read('delivery-asset-audit.json')
    glyph = read('GlyphVerification/glyph-report.json')
    summary = {
        'scope': 'UI stage one, current workspace; references remain unopened',
        'native': {k: native.get(k) for k in ['succeeded', 'failed', 'notRun', 'inProcess']},
        'native_failures': [t['fullTestPath'] for t in native.get('tests', []) if t['state'] != 'Success'],
        'rendered': {'checks': len(pie.get('checks', [])), 'captures': len(pie.get('captures', [])), 'finished': pie.get('sequence_finished', False), 'passed': pie.get('passed', False), 'failed': [c['name'] for c in pie.get('checks', []) if not c['passed']], 'errors': pie.get('errors', [])},
        'geometry_contract': {'passed': contract.get('passed', False), 'checks': len(contract.get('checks', [])), 'failed': [c['name'] for c in contract.get('checks', []) if not c['passed']]},
        'protected_assets_preserved': protected.get('protected_preserved', False),
        'manual_ui_assets_unchanged_this_resume': protected.get('manual_ui_assets_unchanged_in_resume', False),
        'materials_audited': len(assets.get('materials', [])),
        'font_locations_audited': len(assets.get('source_font_locations', [])),
        'glyph_followup': {'captures':len(glyph.get('captures',[])), 'manual_review':glyph.get('manual_review','pending'), 'finished':glyph.get('sequence_finished',False), 'note':'Full matrix/native suite preceded the narrow glyph patch; build-glyph.log and GlyphVerification verify that final change.'},
        'remaining_gates': ['Reference comparison and user visual review remain pending.', 'Rendered extreme-number/no-avatar stress cases and full save-confirmation interactions are not all covered.', 'Engine startup Condition failed messages remain unclassified; suite test results do not clear these.', 'No mobile/device/Cook/production-service/platform validation in this UI resume.'],
    }
    summary['implementation_checks_passed'] = bool(native) and native.get('failed') == 0 and pie.get('passed', False) and pie.get('sequence_finished', False) and contract.get('passed', False) and protected.get('protected_preserved', False) and glyph.get('manual_review')=='pass'
    with (ROOT / 'docs/design/2026-10-21-ink-ui-standard-v1/验收矩阵.csv').open(encoding='utf-8-sig', newline='') as handle:
        rows = list(csv.DictReader(handle))
    for row in rows:
        row['status'] = 'not_run'
        row['evidence_path'] = ''
        row['notes'] = '首段未覆盖本项的完整验收；不以代码或文件存在代替验收。'
        if row['id'] == 'BASE-01':
            row.update(status='pass', evidence_path='resume-baseline.json; ../preservation-baseline.json; ../Baseline/baseline-report.json', notes='当前恢复点602文件；原基线与原有截图保留。')
        elif row['id'] == 'BASE-02' and protected.get('manual_ui_assets_unchanged_in_resume'):
            row.update(status='pass', evidence_path='preservation-final.json; ../asset-audit.json', notes='本轮未改WBP或材质资产；七个默认值和依赖按实际加载报告记录。')
        elif row['id'] == 'BASE-03' and protected.get('protected_preserved'):
            row.update(status='pass', evidence_path='preservation-final.json', notes='原52条保护记录一致；并行玩法改动单列，不回滚，不计作本UI任务改动。')
        elif row['id'].startswith('VIS-') or row['id'] == 'SHIP-04':
            row.update(status='blocked', evidence_path='Verification/pie-report.json; Verification/hud-contract.json', notes='已有运行截图/几何仅证明所列断言；遵守用户暂不读参考图片的约束，参考视觉比较与确认未完成。')
        elif row['id'] == 'SHIP-01':
            row.update(status='not_run', evidence_path='build-inventory-final.log; FinalUIAutomation/index.json', notes='本轮只做Editor构建及列出的原生回归；未执行Game构建，不能将整项标pass。')
    with (OUT / 'acceptance-matrix.csv').open('w', encoding='utf-8-sig', newline='') as handle:
        writer = csv.DictWriter(handle, fieldnames=rows[0].keys())
        writer.writeheader(); writer.writerows(rows)
    summary['acceptance_status_counts'] = dict(Counter(row['status'] for row in rows))
    (OUT / 'delivery-summary.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(summary, ensure_ascii=False, indent=2))
    return summary

if __name__ == '__main__':
    run()
