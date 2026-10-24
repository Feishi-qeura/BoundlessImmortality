"""Static Source/Config evidence, separate from package/runtime acceptance."""
from pathlib import Path
import re,json,hashlib
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
mapping=json.loads((HERE.parent/'migration/reference-replacement-plan.json').read_text(encoding='utf-8-sig'))['resolved_package_replacements']
stage=json.loads((HERE/'canonical-source-stage-review.json').read_text(encoding='utf-8'))
future={'/Game/newUI/Runtime/WBP_NewUIFrontend'}|{'/Game/newUI/Components/Player/'+n for n in ('T_MainActionOrnament_v1','T_HudRouteGlyphAtlas_v1','T_CharacterAttributeGlyphAtlas_v1','T_InventoryCategoryGlyphAtlas_v1')}
runtime=list((ROOT/'Source/BoundlessImmortality/Frontend').rglob('*.cpp'))+[ROOT/'Source/BoundlessImmortality/LiveWeapons'/n for n in ('LiveGMControlPanel.cpp','LiveGMOverlay.cpp','LiveTitleEffectsComponent.cpp')]
problems=[];pending=[];literal_count=0
for file in runtime:
    source=file.read_text(encoding='utf-8-sig');relative=file.relative_to(ROOT).as_posix()
    if file.name not in ('InkUiAssetPaths.cpp','InkRebuildCanonicalAndBusyTests.cpp') and '/Game/UI' in source:problems.append({'file':relative,'reason':'retired runtime root literal remains'})
    for match in re.finditer(r'"(/Game/newUI/[^"\r\n]+)"',source):
        literal=match.group(1)
        if '%' in literal or literal.endswith('/'):continue
        package=literal.split('.',1)[0];literal_count+=1
        if package in future:pending.append({'file':relative,'package':package});continue
        physical=ROOT/('Content/'+package[len('/Game/'):]+'.uasset')
        if not physical.is_file():problems.append({'file':relative,'literal':literal,'reason':'canonical file absent'})
config=(ROOT/'Config/DefaultEngine.ini').read_text(encoding='utf-8-sig')
redirects=re.findall(r'\+PackageRedirects=\(OldName="([^"]+)",NewName="([^"]+)"\)',config)
ui=[(a,b) for a,b in redirects if a.startswith('/Game/UI/')]
for old,new in ui:
    if mapping.get(old)!=new:problems.append({'old':old,'new':new,'reason':'redirect not equal exact resolved canonical mapping'})
if len(ui)!=len(mapping) or len({a for a,b in ui})!=len(ui):problems.append({'reason':'UI redirect coverage or uniqueness mismatch','redirects':len(ui),'mapping':len(mapping)})
report={'static_only':True,'source_epoch_sha256':hashlib.sha256((HERE/'source-epoch.json').read_bytes()).hexdigest(),'canonical_runtime_literal_count':literal_count,'exact_compatibility_and_redirect_count':len(mapping),'intentional_historical_helper_exceptions':stage['exceptions'],'pending_new_default_assets':pending,'problems':problems,'passed':not problems,'runtime_verified':False}
(HERE/'canonical-references-audit.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps({'passed':report['passed'],'problems':problems,'pending_new_assets':len({r['package'] for r in pending}),'literal_count':literal_count,'redirects':len(ui)}))
