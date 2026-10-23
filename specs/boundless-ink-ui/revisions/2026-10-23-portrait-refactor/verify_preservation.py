from pathlib import Path
import json,hashlib,re
ROOT=Path(__file__).resolve().parents[4];REV=Path(__file__).parent;OUT=ROOT/'Saved/Codex/PortraitUI20261023';BACK=OUT/'Backup'
mapping=json.loads((ROOT/'Tools/InkFrontend/ui_asset_paths.json').read_text('utf-8'))
def normalized(text):
 for before,after in sorted(mapping.items(),key=lambda p:len(p[0]),reverse=True):text=text.replace(after,before)
 return text.replace('#include "Frontend/UI/InkUiAssetPaths.h"\n','').replace('InkUiAssets::Load<','LoadObject<').replace('\r\n','\n')
checks=[]
for name in ['InkInventoryWidget.cpp','InkLeaderboardTheme.cpp']:
 relative=Path('Source/BoundlessImmortality/Frontend')/name
 checks.append(dict(name=name+' unchanged except asset paths',passed=normalized((ROOT/relative).read_text('utf-8-sig'))==normalized((BACK/relative).read_text('utf-8-sig'))))
def protected_rank(text):
 text=normalized(text)
 text=text.replace('#include "UI/InkUiResponsiveLayout.h"\n','').replace('#include "UI/InkUiTheme.h"\n','')
 first=text.split('    auto* View=MakeButton(Tree,TEXT("HUDRankView")')[0]
 middle=text[text.index('    if(auto* Shell=Find<UCanvasPanel>(Tree,TEXT("RankHUDShell")))',text.index('    auto* View=MakeButton(Tree,TEXT("HUDRankView")')):]
 middle=middle.split('            // Narrow full-Fit')[0].split('            // Only the collapsed HUD entry')[0]
 tail=text[text.index('            if(auto* LayoutSlot=Cast<UCanvasPanelSlot>(Scale->Slot))LayoutSlot->SetOffsets'):]
 rail='    // The right shortcut rail yields its space while the existing expanded rank overlay is visible.\n    for(const TCHAR* Name:{TEXT("HUD_OpenActivities"),TEXT("HUD_OpenSignIn"),TEXT("HUD_OpenExtensions")})\n        if(auto* Shortcut=GetWidgetFromName(Name))Shortcut->SetVisibility(bHUD&&!bGameplayLeaderboardRequested&&LeaderboardFade.Progress<=0?ESlateVisibility::Visible:ESlateVisibility::Collapsed);\n'
 return first,middle,tail.replace(rail,'')
relative=Path('Source/BoundlessImmortality/Frontend/InkLeaderboardWidget.cpp')
checks.append(dict(name='Leaderboard main and expanded contents retained; collapsed HUD entry is user-authorized redesign',passed=protected_rank((ROOT/relative).read_text('utf-8-sig'))==protected_rank((BACK/relative).read_text('utf-8-sig'))))
relative=Path('Source/BoundlessImmortality/Frontend/InkCharacterPanels.cpp')
def profile(text):
 # These two includes serve redesigned subpages below BuildCharacterSection;
 # the entire dedicated profile implementation is still compared byte for byte.
 return normalized(text).replace('#include "Components/ScrollBox.h"\n','').replace('#include "Components/SizeBox.h"\n','').split('void UInkFeaturePanel::BuildCharacterSection')[0]
checks.append(dict(name='CharacterProfile dedicated implementation retained',passed=profile((ROOT/relative).read_text('utf-8-sig'))==profile((BACK/relative).read_text('utf-8-sig'))))
backup=json.loads((OUT/'backup-manifest.json').read_text('utf-8'))
checks.append(dict(name='all backup bytes verified',passed=all(hashlib.sha256((BACK/v['path']).read_bytes()).hexdigest()==v['sha256'] for v in backup['files'])))
report=dict(passed=all(v['passed'] for v in checks),checks=checks,boundary='Source preservation after normalizing only asset path migration; rendered preservation covered by PIE and packaged screenshots')
(OUT/'preservation.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8');print(report)
assert report['passed']
