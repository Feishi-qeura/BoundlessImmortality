"""Close the reviewed v4 metadata stage without changing any image or old ZIP.

One-shot operation. Existing generation scripts must not be rerun for verification.
"""
from pathlib import Path
if (Path(__file__).resolve().parents[1]/'evidence/history-archive-index.json').exists():
    raise RuntimeError('Historical generator frozen after UE integration. Use the external baseline in a separate workspace to reproduce; never overwrite archived evidence.')

import copy
import hashlib
import json
import subprocess
import sys
import zipfile

import yaml
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT.parents[1]
PACK = 'home-components-trim-v2'
ZIP_REL = f'packages/{PACK}-png.zip'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(rel):
    return yaml.safe_load((ROOT / rel).read_text(encoding='utf-8'))


def write(rel, text):
    path = ROOT / rel
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text.rstrip() + '\n', encoding='utf-8')


def dump(rel, data):
    write(rel, yaml.safe_dump(data, allow_unicode=True, sort_keys=False, width=110))


def jsave(rel, data):
    write(rel, json.dumps(data, ensure_ascii=False, indent=2))


def main():
    assert not (ROOT / ZIP_REL).exists(), 'Refuse to replace an existing package'
    snapshot = ROOT / 'evidence/pre-finalization-snapshot'
    assert not snapshot.exists(), 'One-shot finalizer already started; inspect disk before recovery'
    contract = load('contracts/sprite-contract-candidate-v2.yaml')
    manifest_rel = f'assets/sprites/{PACK}/sprite-manifest.yaml'
    manifest = load(manifest_rel)
    assert contract['items'] == manifest['items'] and contract['policy_revision'] == 'v4'
    plan = load('contracts/home-slice-correction-v4.yaml')
    v4 = load('evidence/home-slice-correction-v4-visual-review.json')
    v3 = load('evidence/home-slice-correction-visual-review.json')
    old = load('evidence/home-9slice-visual-review.json')
    baseline = load('evidence/preservation-baseline.json')
    assert len(baseline) == 670
    assert all((GAME / b['path']).is_file() and sha(GAME / b['path']) == b['sha256'] for b in baseline)
    for item in manifest['items']:
        assert sha(ROOT / item['path']) == item['sha256']
        assert sha(ROOT / item['source_path']) == item['source_sha256']
    for test in plan['tests'] + plan['guides']:
        assert sha(ROOT / test['path']) == test['sha256']
    carried = old['inspected'][:3] + [r for r in v3['inspected'] if r['result'] in {'visual-pass-at-review-scale', 'guide-placement-pass'}]
    assert len(carried) == 6
    assert all(sha(ROOT / r['path']) == r['sha256'] for r in carried)
    preview = next(t for t in plan['tests'] if t['component_id'] == 'home-rank-paper' and t['variant'] == 'narrow')
    assert preview['sha256'] == '596683dd1d232a4d7c93674ba3d00ae4eb91a2fd86057b7b8de2b800ab5f461e'
    rank = next(i for i in manifest['items'] if i['component_id'] == 'home-rank-paper')
    with Image.open(ROOT / rank['path']) as source, Image.open(ROOT / preview['path']) as actual:
        expected = Image.new('RGBA', (1260, 399))
        expected.paste(source.resize((1260, 319), Image.Resampling.LANCZOS), (0, 40))
        assert actual.mode == expected.mode and actual.size == expected.size and actual.tobytes() == expected.tobytes()

    # Snapshot every existing PNG and ZIP, including obsolete versions and review evidence.
    preserved = {p.relative_to(ROOT).as_posix(): sha(p) for p in ROOT.rglob('*') if p.is_file() and p.suffix.lower() in {'.png', '.zip'}}
    jsave('evidence/pre-finalization-preserved-files.json', dict(date='2026-10-22', timezone='Asia/Hong_Kong', files=preserved))
    for rel in ['contracts/sprite-contract.yaml', 'contracts/sprite-contract-candidate-v2.yaml', manifest_rel,
                'contracts/home-slice-correction-v4.yaml', 'evidence/delivery-status.json',
                'evidence/home-slice-correction-v4-visual-review.json', 'evidence/home-9slice-visual-review.json',
                'README.md', 'quickstart.md', 'component-handoff.md', 'tasks.md', 'components.md', 'plan.md']:
        target = snapshot / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes((ROOT / rel).read_bytes())

    # Record provenance honestly: the current turn verifies bytes, reuses the prior
    # reported inspection, and does not claim to have reopened the image.
    jsave('evidence/v4-review-reuse-provenance.json', dict(
        date='2026-10-22', timezone='Asia/Hong_Kong',
        user_handoff='上一聊天已经查看 narrow Fit v4 并报告通过，但审核结论尚未落盘，新ZIP尚未生成。',
        corroborating_chat=dict(thread_id='01a16246-883b-7211-b3a5-78c3bf0c8698',
            message_id='msg_05dcf86a5374ae76016ada0ad82b38819bae734b4c3fa7a311',
            timestamp_utc='2026-10-22T13:08:42.299Z',
            excerpt='原聊天已口头报告最后一张 v4 图通过，但磁盘上的审核状态仍是“待审”，新 ZIP 也尚未生成。',
            local_rollout='rollout-2026-10-22T14-22-31-01a16246-883b-7211-b3a5-78c3bf0c8698.jsonl',
            line=252),
        evidence_kind='User-provided handoff corroborated by saved chat report; original detailed v4 inspection record was not recovered.',
        current_turn_checks=['preview SHA256 matches v4 plan and generation checks',
                             'exact deterministic RGBA Fit reconstruction',
                             'six carried accepted-image hashes match saved reviews'],
        preview_path=preview['path'], preview_sha256=preview['sha256'],
        current_turn_images_reopened=0,
        limitation='No new visual observations are claimed. Prior reported visual pass is reused within offline preview scope; user approval and runtime remain unverified.'))
    record = copy.deepcopy(preview)
    record.update(result='visual-pass-at-review-scale', displayed_dimensions=[1260, 399],
        findings='复用上一聊天已查看并报告通过的 narrow Fit v4 结论；本轮核对图片哈希与确定性RGBA重建，未重新打开图片，不新增细节性视觉观察。',
        scope='Previously reported offline narrow Fit preview pass only; no UE sampling, text/hit layout, device or user acceptance.',
        review_method='reused-prior-reported-inspection', current_turn_reopened=False,
        issue_resolution='rank-paper-v2-cloud-tips-cross-fixed-endcaps',
        provenance='evidence/v4-review-reuse-provenance.json')
    v4.update(status='author-visual-review-passed', complete=True, inspection_complete=True,
        visual_acceptance_passed=True, inspected_count=1, inspected=[record], pending=[],
        user_approved=False, current_turn_images_reopened=0,
        evidence_reuse='evidence/v4-review-reuse-provenance.json',
        open_issues=[], resolved_issues=['top-tile-v2-ornament-crosses-fixed-regions', 'rank-paper-v2-cloud-tips-cross-fixed-endcaps'])
    jsave('evidence/home-slice-correction-v4-visual-review.json', v4)
    for issue in old['issues']:
        issue['acceptance_blocking'] = False
        issue['status'] = 'resolved-in-v3-author-review' if issue['component_id'] == 'home-top-tile' else 'resolved-in-v4-author-review'
        issue['next_check'] = 'Current offline correction accepted; engine sampling and user acceptance remain outside this stage.'
        issue['proposed_remediation']['status'] = 'author-reviewed-pass-activated-v4'
        issue.setdefault('resolution', {}).update(
            policy='full/Fit' if issue['component_id'] == 'home-top-tile' else 'aspect-dependent Fit / height-locked X-only nine-slice',
            successor_review='evidence/home-slice-correction-v4-visual-review.json',
            original_v2_previews_remain_failed=True, user_approved=False, candidate_activated=True)
        if 'correction_progress' in issue:
            issue['correction_progress']['v4_review'] = 'passed-reused-prior-report'
            issue['resolution']['visual_evidence'] = preview['path']
    old['current_corrected_policy'] = dict(revision='v4', author_visual_review='passed', original_v2_results_preserved=True,
        evidence='evidence/home-slice-correction-v4-visual-review.json')
    jsave('evidence/home-9slice-visual-review.json', old)

    review = dict(complete=True, text_free=True, user_approved=False, author_visual_acceptance_passed=True,
        evidence='evidence/home-slice-correction-v4-visual-review.json',
        provenance='evidence/v4-review-reuse-provenance.json',
        notes=['Author offline review complete; v4 narrow inspection reused from prior reported pass.',
               'Current metadata active for this delivery. Derived policy is not user-approved; UE integration is not performed.'])
    for data in [contract, manifest]:
        data.update(status='generated', candidate_only=False, review=copy.deepcopy(review))
    contract['activation'] = dict(date='2026-10-22', timezone='Asia/Hong_Kong',
        basis='Completed author offline review and explicit user request to finalize delivery',
        snapshot='evidence/pre-finalization-snapshot/contracts/sprite-contract.yaml')
    manifest['activation'] = copy.deepcopy(contract['activation'])
    dump(manifest_rel, manifest)
    dump('contracts/sprite-contract.yaml', contract)
    candidate = load('contracts/sprite-contract-candidate-v2.yaml')
    candidate['superseded_by'] = 'contracts/sprite-contract.yaml'
    candidate['historical_snapshot'] = True
    dump('contracts/sprite-contract-candidate-v2.yaml', candidate)
    plan.update(status='author-reviewed-active', candidate_only=False, active_sprite_contract_unchanged=False,
        active_sprite_contract='contracts/sprite-contract.yaml', user_approved=False,
        activation_basis='User requested delivery finalization after review; this is not derived-art approval.')
    dump('contracts/home-slice-correction-v4.yaml', plan)

    # Portable metadata references the archived items; unchanged workspace paths
    # remain explicit provenance, never implied bundled source files.
    portable = copy.deepcopy(manifest)
    portable['source']['external_provenance_only'] = True
    portable['source']['note'] = 'Source sheet and original component files remain in the workspace; render dependencies are items/ PNGs.'
    for item in portable['items']:
        item['workspace_path'] = item['path']
        item['path'] = 'items/' + Path(item['path']).name
    portable_contract = copy.deepcopy(contract)
    portable_contract['source'] = copy.deepcopy(portable['source'])
    portable_contract['items'] = copy.deepcopy(portable['items'])
    portable_contract['output'] = dict(items_dir='items', manifest_path='sprite-manifest.yaml', package_path=Path(ZIP_REL).name)
    payload = {}
    def add_yaml(name, value):
        payload[name] = yaml.safe_dump(value, allow_unicode=True, sort_keys=False, width=110).encode('utf-8')
    add_yaml('sprite-manifest.yaml', portable)
    add_yaml('sprite-contract.yaml', portable_contract)
    for item in manifest['items']:
        payload['items/' + Path(item['path']).name] = (ROOT / item['path']).read_bytes()
    preview_paths = [t['path'] for t in plan['tests']] + [g['path'] for g in plan['guides']]
    track_plan = load('contracts/home-9slice-review.yaml')
    preview_paths += [t['path'] for t in track_plan['tests'] if t['component_id'] == 'home-bar-track']
    preview_paths += ['evidence/9slice-review-v2/home-bar-track-guides-v2.png']
    assert len(set(preview_paths)) == 12
    for rel in preview_paths + ['contracts/home-slice-correction-v4.yaml', 'contracts/home-9slice-review.yaml',
            'contracts/home-state-recipes.yaml', 'evidence/home-slice-correction-v4-visual-review.json',
            'evidence/home-slice-correction-visual-review.json', 'evidence/home-9slice-visual-review.json',
            'evidence/v4-review-reuse-provenance.json', 'evidence/home-slice-correction-v4-checks.json',
            'evidence/approval-home-components.json']:
        payload[rel] = (ROOT / rel).read_bytes()
    readme = '''首页独立切图交付：home-components-trim-v2，参数修订 v4。
18张 items/ PNG与旧v1切图字节相同；新增的是已审核渲染策略与证据。
先读取sprite-manifest.yaml、sprite-contract.yaml与contracts/home-slice-correction-v4.yaml。
轨道：Margin [260,260,60,60]，按高度统一缩放后锁高，仅横向伸缩。
顶部底板：整图等比Fit居中，文字和热点跟随实际图形范围。
排行纸底：宽高比小于1575/399时整图Fit；否则按高度统一缩放后锁高横向九宫格，Margin [820,400,115,125]。
排行纸底九宫格的视觉宽度下限为折算399px高度时1575px；技术切片极限1221px不可作为允许下限。
Fit窄预览1260×399，实际图形1260×319，上下40px透明余量。单一Box margin不能表达响应分支。
共2项九宫格策略（排行纸底有条件），16项full；9控件×7态为63条共享素材规则，不是63张新PNG。
12张当前验证预览置于evidence/。v2/v3旧失败记录保留为历史；其中旧失败图片引用只供工作区溯源，不属于本包当前预览。
原图、溯源合图和其他历史契约路径为外部工作区来源；渲染只依赖本包items/文件和当前策略。
v4视觉通过结论复用上一聊天的已查看报告，本轮只核对哈希与确定性RGBA重建，未重新查看图片。
作者离线审核完成，派生切图与参数尚未获用户批准。组件本身已批准。
包内file-checksums.json覆盖其他全部条目；最终ZIP哈希、严格校验与保护核对见工作区evidence/home-trim-final-checks.json。
本阶段未导入UE，未生成Atlas/UMG/uasset，未执行构建、PIE、Cook、设备或平台测试。
'''
    payload['README.txt'] = readme.encode('utf-8')
    payload['file-checksums.json'] = (json.dumps({name: hashlib.sha256(data).hexdigest() for name, data in sorted(payload.items())},
        ensure_ascii=False, indent=2) + '\n').encode('utf-8')
    with zipfile.ZipFile(ROOT / ZIP_REL, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(payload.items()):
            info = zipfile.ZipInfo(name, date_time=(2026, 10, 22, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, data)
    assets = load('contracts/asset-manifest.yaml')
    assert all(a['id'] != f'{PACK}-package' for a in assets['assets'])
    assets['assets'].append(dict(id=f'{PACK}-package', kind='package', screen_id='home', path=ZIP_REL,
        status='generated', approved=False, prompt_path='prompts/pipeline/home-components-trim-v1.md',
        source_tool='deterministic-reuse-only-metadata-packaging',
        source_assets=[i['source_asset_id'] for i in manifest['items']],
        sha256=sha(ROOT / ZIP_REL), policy_revision='v4', author_review='passed',
        review_evidence='evidence/home-slice-correction-v4-visual-review.json'))
    dump('contracts/asset-manifest.yaml', assets)

    # Documentation is current; historical scripts and old packages remain untouched.
    write('README.md', '''# 无极仙道 · 水墨UI交付
当前阶段：**首页独立切图与v4渲染策略的作者离线审核、交付收尾已完成**。
首页v2、规范、屏幕地图及18个源组件已获用户批准；派生切图与九宫格策略尚未获用户批准。

- [当前PNG交付包](packages/home-components-trim-v2-png.zip)：18张原字节复用的透明PNG、12张当前预览、契约、审核记录及逐文件SHA256。
- [当前交付契约](contracts/sprite-contract.yaml)、[v4响应策略](contracts/home-slice-correction-v4.yaml)、[使用与复现](quickstart.md)、[开发交接](component-handoff.md)。
- [v4审核记录](evidence/home-slice-correction-v4-visual-review.json)、[审核复用来源](evidence/v4-review-reuse-provenance.json)、[最终校验](evidence/home-trim-final-checks.json)、[交付状态](evidence/delivery-status.json)。
- [组件总览](component-review.html)、[组件拆解](components.md)、[63条状态规则](contracts/home-state-recipes.yaml)、[已批准首页](assets/pages/home-v2.png)。

顶部底板整图Fit；排行纸底按1575/399宽高比分支选择Fit或锁高横向九宫格。文字及热点跟随实际Fit图形范围，单一Box margin不能表达该分支。
复用此前审核：轨道3图、v3必要通过图3张与v4 narrow通过报告；本轮未重新打开图片，只核对已有证据、哈希与确定性像素重建。
原素材、历史预览与旧ZIP全部保留；切换前元数据见evidence/pre-finalization-snapshot。
本轮未导入UE、未生成Atlas/UMG/uasset，未执行构建、PIE、Cook、设备或平台验证。其余页面视觉扩展不属于本次交付收尾。
''')
    write('quickstart.md', '''# 使用与真实状态
项目：D:/demo/BoundlessImmortality/specs/boundless-ink-ui；2026-10-22，Asia/Hong_Kong。
首页独立切图与v4参数交付收尾完成。源页面、规范、屏幕地图及18个源组件已批准；派生切图与参数的user_approved仍为false。

## 当前文件与解压使用
使用packages/home-components-trim-v2-png.zip。解压后items/为18张透明无字PNG，sprite-manifest.yaml与sprite-contract.yaml是包内路径版本；file-checksums.json覆盖其余全部条目。
磁盘当前契约是contracts/sprite-contract.yaml，当前清单是assets/sprites/home-components-trim-v2/sprite-manifest.yaml。
清单复用assets/sprites/home-components-trim-v1/items原PNG路径，文件名保留trim-v1表示像素版本；pack-v2表示元数据版本，不是重新生图。
原组件、切图、历史预览、旧ZIP均不覆盖。旧ZIP内状态是历史生成快照；新ZIP内审核与当前清单一致。切换前元数据保存在evidence/pre-finalization-snapshot。

## 当前渲染参数
2项九宫格策略、16项full/Fit。Margin顺序left/right/top/bottom，单位为裁切后原生像素。
轨道：Margin [260,260,60,60]，按目标高度统一缩放后锁高，仅横向伸缩。
顶部底板：整图等比Fit并居中，保留透明余量，不再使用v2全轴九宫格。
排行纸底：容器宽高比小于1575/399时整图Fit居中；达到或超过时先按目标高度统一缩放，再锁高横向九宫格，Margin [820,400,115,125]。
按399px高度折算，九宫格视觉策略下限1575px，已验证原生1575与宽2205；技术极限1221px不作为允许的视觉下限。
窄预览1260×399，图形1260×319，原点[0,40]。Fit实际图形范围定义文字及热点布局；单个UE Box margin不能表达宽高比分支，未来集成必须实现该分支。
其余不规则底板、头像、框、图标、云带整图等比；血量/修为填充按真实fraction遮罩。9控件×7态是63条共享素材规则，不是63张PNG。
标题马善政体，其余文字和所有数字霞鹜文楷；可见生命用词“血量”。

## 审核复用及边界
原v2计划10图与v3必要4图的历史结果保留，原失败预览仍为失败。v3轨道之外新增通过：顶部底板narrow Fit、排行纸底guide与wide；v3排行narrow纸纹压成竖带，已被v4整图Fit替代。
v4 narrow已在上一聊天查看并报告通过，本轮依用户接续说明及保存聊天的佐证复用该结论；原详细审核记录未恢复，不补写未经观察的视觉细节。
本轮重新打开图片0张。当前图片SHA256、v4契约及确定性RGBA重建一致。详见evidence/v4-review-reuse-provenance.json。
最终验证包括源图/切图RGBA、合图区域、预览重建、分割线、旧包与全部既有PNG、670份保护基线、新包CRC和逐项SHA256、严格项目校验。结果见evidence/home-trim-final-checks.json。
这些检查证明离线文件与契约一致；UE导入、Atlas、引擎JSON、UMG/uasset、构建、PIE、Cook、设备和平台验证均未执行。

## 复现命令
环境：Python 3.12.14，PyYAML 6.0.3，Pillow 12.3.0。
```powershell
$env:PYTHONUTF8='1'
& 'C:/Users/zxxuh/.codex/skills/gameui/.venv/Scripts/python.exe' 'D:/demo/BoundlessImmortality/specs/boundless-ink-ui/authoring/verify_home_trim_delivery.py'
```
该命令读取当前图与ZIP，更新最终报告和严格校验日志；不生成或修改素材。其内部运行gameui/scripts/validate_project.py --strict。
authoring/finalize_home_trim_delivery.py为一次性收尾记录，已有新包时拒绝重跑。build_home_slices.py、build_home_slice_corrections.py、build_home_rank_narrow_fallback.py及更早生成脚本不得作为复核命令，以免回退契约、批准状态或覆盖旧包。
当前阶段无剩余落盘任务；用户对派生策略的最终接受与UE集成属于后续决策，不自动执行。
''')
    write('component-handoff.md', '''# 首页组件交接
状态：18个源组件已批准；独立切图与v4渲染策略的作者离线审核及交付收尾完成，派生参数user_approved=false。

## 当前入口
- packages/home-components-trim-v2-png.zip：18张RGBA切图、12张当前预览、当前契约、审核证据和file-checksums.json。
- contracts/sprite-contract.yaml与assets/sprites/home-components-trim-v2/sprite-manifest.yaml为磁盘当前契约和清单。
- evidence/delivery-status.json与evidence/home-trim-final-checks.json为最新状态和校验结果。
原PNG字节复用、语义名及component ID不变；头像/修为采用已批准v2，旧v1保留停用。63条控件状态共享基础纹理和参数，文字独立渲染。

## 装配约束
1. 保留真实3D场景、主角、相机、运行HUD及角色/背包资产。本阶段不导入UE。
2. 页面941×1672只供参考，逻辑720×1280约乘0.765，不能将生图像素直接当UE坐标。热点按SafeZone和88×88逻辑单位核对。
3. 轨道Margin [260,260,60,60]，按高度统一缩放后锁高，仅拉伸X。顶部底板整图Fit，禁止复用原v2全轴九宫格。
4. 排行纸底容器宽高比小于1575/399时整图Fit；否则按高度统一缩放后锁高X轴九宫格，Margin [820,400,115,125]。按399px高度折算，允许九宫格下限1575px，技术切片下限1221px不可作为视觉下限。必须实现条件分支，单一Box margin不足。
5. Fit居中透明余量不是有效图形区。文字与热点随实际图形范围布局；窄预览1260×399内图形1260×319，原点[0,40]。其余16项full策略保留整体比例。
6. 头像圆形mask后叠框；血量/修为填充按真实fraction遮罩，动态数值单独TextBlock。未知头像沿用现有占位，不将修士肖像当真人。标题用马善政体，其余文字及所有数字霞鹜文楷，可见生命用词“血量”。
7. 九控件七态见contracts/home-state-recipes.yaml，pending不模拟成功，返回保持可用；绑定ActionId前核查实际源码。

## 证据与保留
v2/v3原失败预览与结果保留，仅当前修订策略通过作者离线审核。v4 narrow结论按用户要求复用上一聊天已查看报告，并有保存聊天佐证；本轮0张图片重看，不新增未经观察的视觉细节。图片哈希与RGBA重建已复核。
原素材、所有历史PNG与旧ZIP不变，切换前契约快照在evidence/pre-finalization-snapshot。
未来获准集成时另行验证UI压缩、sRGB、Alpha混合、采样、Mip与设备表现。当前没有UMG/uasset、Atlas、引擎导出、构建、PIE、Cook、设备或平台验收。
''')
    tasks = (ROOT / 'tasks.md').read_text(encoding='utf-8')
    tasks = tasks.replace('- [ ] [correction-v4-visual] 只剩1张narrow Fit v4必要图待查；通过后再启用新元数据、打新ZIP并完成交付。',
        '- [x] [correction-v4-visual] 复用上一聊天narrow Fit v4已查看通过结论，核对哈希及RGBA重建；已补写来源与审核，本轮未重新查看图片。')
    tasks = tasks.replace('- [ ] [trim-delivery] 完成视觉审核后整理本阶段最终交付；派生切图/九宫格不冒充用户已批准。',
        '- [x] [trim-delivery] 启用v4当前交付契约，生成home-components-trim-v2新ZIP并完成最终校验；原PNG和历史ZIP保留，派生参数user_approved=false。')
    tasks = tasks.replace('严格校验，0错误/0警告。视觉审核仍未完成。', '严格校验，0错误/0警告；当时视觉审核未完成，现由v4收尾证据接续。')
    write('tasks.md', tasks)
    components = (ROOT / 'components.md').read_text(encoding='utf-8')
    components = components.replace('9-slice暂不批准：有机墨边、特殊牌匾和云尾不能直接写死Margin。',
        '当前v4参数已完成作者离线审核，派生参数仍未获用户批准；轨道锁高横向九宫格，排行纸底按宽高比分支选择Fit或锁高横向九宫格，顶部底板整图Fit。不得直接将Margin套用于所有容器。')
    components = components.replace('素材尚待单独审核；首页获批不等于所有组件自动获批。后续图集/切图/UE接线按实际素材验收继续。',
        '18个源组件已有独立用户批准；切图与v4参数已交付home-components-trim-v2，详见component-handoff.md及evidence/delivery-status.json。派生参数user_approved=false，Atlas与UE接线未执行。')
    write('components.md', components)
    planning = (ROOT / 'plan.md').read_text(encoding='utf-8')
    planning = planning.replace('# 分阶段执行计划\n', '# 分阶段执行计划\n当前状态：截至2026-10-22，首页v2、规范/地图及18个源组件已批准，切图与v4参数交付收尾完成；以下顺序保留为原始阶段计划，具体执行状态见tasks.md和evidence/delivery-status.json。未执行Atlas或UE导入。\n', 1)
    write('plan.md', planning)

    result = subprocess.run([sys.executable, str(ROOT / 'authoring/verify_home_trim_delivery.py')],
        capture_output=True, text=True, encoding='utf-8')
    assert result.returncode == 0, result.stdout + result.stderr
    report = load('evidence/home-trim-final-checks.json')
    status = load('evidence/delivery-status.json')
    status.update(stage='home-trim-v2-v4-delivery-complete', stage_complete=True,
        derivative_pack=PACK, derivative_user_approved=False, candidate_only=False,
        active_contract='contracts/sprite-contract.yaml',
        active_manifest=manifest_rel, final_validation='evidence/home-trim-final-checks.json',
        numerical_checks=dict(passed=True, evidence='evidence/home-trim-final-checks.json',
            retained_rgba=18, preview_reconstructions=report['deterministic_preview_checks'],
            guide_pixel_checks=report['guide_pixel_checks']),
        protected_files=report['protected_files'],
        visual_review=dict(complete=True, inspected=1, required=1, pending=0,
            evidence='evidence/home-slice-correction-v4-visual-review.json', status='author-visual-review-passed',
            accepted_images=7, current_turn_images_reopened=0, evidence_reused=True,
            provenance='evidence/v4-review-reuse-provenance.json', open_issues=[],
            resolved_issues=v4['resolved_issues'], user_approved=False),
        strict_validation=report['strict_validation'],
        package=dict(**report['package'], status='delivered-author-reviewed-pending-user-acceptance',
            user_approved=False, metadata_snapshot='v4 current author review'),
        old_packages_preserved=True, previous_package='D:/demo/BoundlessImmortality-UIArchive/20261022-home-v2/files/specs/boundless-ink-ui/packages/home-components-trim-v1-png.zip',
        correction_checks='evidence/home-trim-final-checks.json',
        generation_checks_snapshot='evidence/home-slice-correction-v4-checks.json',
        remaining=[], follow_up_outside_current_scope=['User acceptance of derived rendering policy',
            'Separately authorized UE integration and runtime/device verification'])
    status.pop('candidate_pack', None)
    status.pop('candidate_contract', None)
    status['v2_review']['historical_snapshot'] = True
    status['v2_review']['open_issues_at_v2_review'] = status['v2_review'].pop('open_issues', [])
    status['v2_review']['current_open_issues'] = []
    status['v3_review']['historical_snapshot'] = True
    jsave('evidence/delivery-status.json', status)
    assert all(sha(ROOT / rel) == digest for rel, digest in preserved.items())
    print(result.stdout.strip())


if __name__ == '__main__':
    main()
