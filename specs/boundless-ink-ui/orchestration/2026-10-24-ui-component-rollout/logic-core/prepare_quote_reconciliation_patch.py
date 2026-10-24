"""Generate an UNAPPLIED minimal gate correction against frozen current source."""
import difflib
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[4]
changes = {}


def edit(path, replacements):
    before = (ROOT / path).read_text(encoding='utf-8-sig')
    after = before
    for old, new in replacements:
        if after.count(old) != 1:
            raise RuntimeError(f'Anchor count {after.count(old)}: {path}: {old[:80]}')
        after = after.replace(old, new, 1)
    changes[path] = before, after


edit('Source/BoundlessImmortality/Frontend/UiRollout20261024/LogicCore/InkCoreAdapter.cpp', [
    ('{if(Selection.IsEmpty())return false;', '{if(Selection.IsEmpty()||Selection.Num()>64)return false;'),
    ('    D.CanSubmit=InkCorePolicy::CanSubmitQuote(D.Facts);', '''    // Display completeness remains diagnostic. A preview is not a submit permission:
    // the authoritative Prepare/Rework handler re-quotes and checks inventory/state.
    double FromRealm=0,ToRealm=0,CurrentRealm=0;
    const bool NativeQuote=!CoreAdapterString(Stage,TEXT("stage_id")).IsEmpty()&&
        CoreAdapterNumber(Stage,TEXT("from_realm_index"),FromRealm)&&FromRealm>=0&&FromRealm<=33&&FMath::FloorToDouble(FromRealm)==FromRealm&&
        CoreAdapterNumber(Stage,TEXT("to_realm_index"),ToRealm)&&ToRealm>FromRealm&&ToRealm<=33&&FMath::FloorToDouble(ToRealm)==ToRealm&&
        CoreAdapterNumber(Growth,TEXT("realm_index"),CurrentRealm)&&CurrentRealm>=0&&CurrentRealm<=33&&FMath::FloorToDouble(CurrentRealm)==CurrentRealm&&
        (Rework?ToRealm<=CurrentRealm:FromRealm==CurrentRealm);
    const bool ExplicitReadinessAllowsRequest=!CoreAdapterField(Q,TEXT("readiness"))||(HasReady&&Ready);
    D.CanSubmit=NativeQuote&&D.Facts.HasSelection&&D.Facts.SelectionMatches&&D.Facts.KindMatches&&
        !D.Facts.AttemptActive&&ExplicitReadinessAllowsRequest;'''),
    ('TEXT("报价字段不完整、材料槽位无效或服务尚未确认准备度")',
     'TEXT("报价身份、阶段或材料选择无效，或服务明确拒绝；缺少展示字段不影响发送校验请求")'),
])

panel = 'Source/BoundlessImmortality/Frontend/UI/InkWulingToolsPanel.cpp'
old_gate = '            BreakSubmissionReady=HasStage&&HasQ&&HasNormalizedQ&&HasQuality&&HasGrowth&&HasReadiness&&HasSlots&&SlotFieldsComplete&&HasSources&&HasChance&&ReadyByService&&SelectionMatches&&SlotSelectionMatches&&CoreSlotsValid&&QuoteKindMatches;'
edit(panel, [
    (old_gate, old_gate.replace('BreakSubmissionReady=', 'const bool BreakDisplayComplete=') +
     '\n            BreakSubmissionReady=InkCore::EvaluateQuote(State,MaterialSelection,bRework).CanSubmit;'),
    ('            if(!BreakSubmissionReady)Details+=TEXT("报价字段不完整或服务未确认准备度，暂不可提交。\\n");',
     '            if(!BreakDisplayComplete)Details+=TEXT("部分展示资料未提供；保留未知值，提交时由服务校验。\\n");\n'
     '            if(!BreakSubmissionReady)Details+=InkCore::EvaluateQuote(State,MaterialSelection,bRework).Reason+TEXT("\\n");'),
    ('            if(!SlotSelectionMatches)Details+=TEXT("提交门控：")',
     '            if(HasSlots&&!SlotSelectionMatches)Details+=TEXT("展示资料：")'),
    ('            if(HasSlotCategories&&!CoreSlotsValid)Details+=TEXT("提交门控：报价核心材料槽位数量不符合当前操作规则或数量结构无效。\\n");',
     '            if(HasSlotCategories&&!CoreSlotsValid)Details+=TEXT("展示资料：材料槽位与选择不一致，分类资料待核对；实际材料资格由服务提交时重验。\\n");'),
    ('!HasReadiness?FString(TEXT("待同步"))',
     '!HasReadiness?FString(TEXT("服务未提供；提交时校验"))'),
])

tests = 'Source/BoundlessImmortality/Frontend/UiRollout20261024/LogicCore/InkCoreAdapterTests.cpp'
edit(tests, [
    ('"growth":{"name":"fixture-current"}', '"growth":{"name":"fixture-current","realm_index":21}'),
    ('"breakthrough_quote":{"target_stage":"fixture-target"',
     '"breakthrough_quote":{"stage":{"stage_id":"BR04","from_realm_index":21,"to_realm_index":22},"target_stage":"fixture-target"'),
    ('TestFalse(TEXT("malformed slot is inert"),InkCore::EvaluateQuote(S,Selection).CanSubmit);',
     'TestTrue(TEXT("malformed display slot cannot invalidate a matching native quote request"),InkCore::EvaluateQuote(S,Selection).CanSubmit);'),
    ('TestFalse(TEXT("missing normalized Q never becomes zero"),InkCore::EvaluateQuote(S,Selection).CanSubmit);',
     '''TestTrue(TEXT("missing normalized Q stays unknown and does not block a valid request"),InkCore::EvaluateQuote(S,Selection).CanSubmit);
    Q->RemoveField(TEXT("growth_min"));Q->RemoveField(TEXT("growth_max"));Q->RemoveField(TEXT("sources"));Q->RemoveField(TEXT("readiness"));Q->RemoveField(TEXT("slots"));Q->RemoveField(TEXT("probability_basis_points"));Q->RemoveField(TEXT("q"));Q->RemoveField(TEXT("quality_band"));
    TestTrue(TEXT("display-only fields absent still permit authoritative submit validation"),InkCore::EvaluateQuote(S,Selection).CanSubmit);
    Q->SetBoolField(TEXT("readiness"),false);TestFalse(TEXT("explicit server readiness refusal is preserved"),InkCore::EvaluateQuote(S,Selection).CanSubmit);
    Q->RemoveField(TEXT("readiness"));Q->GetObjectField(TEXT("stage"))->RemoveField(TEXT("stage_id"));
    TestFalse(TEXT("missing actual quote stage identity remains blocked"),InkCore::EvaluateQuote(S,Selection).CanSubmit);'''),
])

patch = []
baseline = []
for path, (before, after) in changes.items():
    baseline.append(dict(path=path, sha256=hashlib.sha256((ROOT / path).read_bytes()).hexdigest()))
    patch.extend(difflib.unified_diff(before.splitlines(keepends=True), after.splitlines(keepends=True), fromfile='a/'+path, tofile='b/'+path))
(HERE / 'quote-submission-minimal-unapplied.patch').write_text(''.join(patch), encoding='utf-8')
(HERE / 'quote-submission-patch-baseline.json').write_text(json.dumps(baseline, indent=2), encoding='utf-8')
print('Prepared 3-file UNAPPLIED patch; no frozen source was edited.')
