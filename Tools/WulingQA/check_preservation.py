"""Compare the immutable pre-work baseline; never restore or modify game assets."""
from pathlib import Path
import difflib
import hashlib
import json

ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'Saved/Codex/GameplayPlan20261021'
GAMEPLAY={
    'Cultivation/CultivationHealthComponent.cpp','Cultivation/CultivationHealthComponent.h','Cultivation/CultivationPrototypeClient.cpp','Cultivation/CultivationSubsystem.cpp','Cultivation/CultivationSubsystem.h',
    'Frontend/InkCharacterPanels.cpp','Frontend/InkFeatureGroupPanel.cpp','Frontend/InkFeaturePanel.cpp','Frontend/InkFeaturePanel.h','Frontend/InkFeaturePanels.cpp',
    'Frontend/InkFrontendWidget.cpp','Frontend/InkGameplayBridge.cpp','Frontend/InkInventoryModel.cpp','Frontend/InkInventoryModel.h','Frontend/InkInventoryWidget.cpp','Frontend/InkMenuDirector.h',
    'Frontend/InkRewardsPanels.cpp','Frontend/InkSignInSample.cpp','Frontend/InkSignInShell.cpp','Frontend/InkWulingPanel.cpp',
    'LiveWeapons/LiveWeaponSwarm.cpp','LiveWeapons/LiveWeaponSwarm.h','Wuling/WulingPrototypeSubsystem.cpp','Wuling/WulingPrototypeSubsystem.h'}
GAMEPLAY={'Source/BoundlessImmortality/'+s for s in GAMEPLAY}
# Separate user-authorized UI chat, explicitly recorded in coordination.md. No rollback.
CONCURRENT={
    'Frontend/InkAnchorStatusWidget.cpp','Frontend/InkFrontendExtensions.cpp','Frontend/InkFrontendWidget.h','Frontend/InkHudLayout.cpp',
    'Frontend/Tests/InkUiLayoutTests.cpp','Frontend/UI/InkUiResponsiveLayout.cpp','Frontend/UI/InkUiTheme.cpp','Frontend/UI/InkUiTheme.h','Frontend/UI/InkUiViewModels.cpp'}
CONCURRENT={'Source/BoundlessImmortality/'+s for s in CONCURRENT}|{'Source/BoundlessImmortalityEditor/Private/InkFrontendEditorLibrary.cpp'}


def main():
    baseline=json.loads((OUT/'preservation-baseline.json').read_text(encoding='utf-8'))
    diff_dir=OUT/'SourceDiffs';diff_dir.mkdir(exist_ok=True)
    report=dict(boundary='272 original protected files; concurrent authorized UI changes explicitly separated',checked=len(baseline),unchanged=0,changes=[],unexpected=[])
    for row in baseline:
        name=row['path'];path=ROOT/name;actual=hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else None
        if actual==row['sha256']:report['unchanged']+=1;continue
        category='authorized_gameplay_source' if name in GAMEPLAY else ('concurrent_authorized_ui_source' if name in CONCURRENT else 'unexpected')
        entry=dict(path=name,before=row['sha256'],after=actual,category=category);report['changes'].append(entry)
        if category=='unexpected':report['unexpected'].append(entry)
        if path.suffix in ('.cpp','.h') and path.is_file():
            before=(OUT/'Backup'/name).read_text(encoding='utf-8-sig');after=path.read_text(encoding='utf-8-sig')
            diff=''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='baseline/'+name,tofile='current/'+name))
            (diff_dir/(name.replace('/','__')+'.diff')).write_text(diff,encoding='utf-8')
    report['protected_assets_design_config_plugins_unchanged']=not any(not e['path'].startswith('Source/') for e in report['changes'])
    report['passed']=not report['unexpected'] and report['protected_assets_design_config_plugins_unchanged']
    (OUT/'preservation-final.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(dict(passed=report['passed'],checked=report['checked'],unchanged=report['unchanged'],authorized_changes=len(report['changes']),unexpected=report['unexpected']),ensure_ascii=False))
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
