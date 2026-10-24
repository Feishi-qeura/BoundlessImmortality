from pathlib import Path
import runpy,hashlib,json
R=Path.cwd();H=R/'specs/boundless-ink-ui/rebuild-2026-10-24/logic-integration';B='Source/BoundlessImmortality/Frontend/UIRebuild20261024/'
rec=runpy.run_path(str(R/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/record_source_change.py'),run_name='family_recorder')
def write(p,s,reason):
 rec['begin'](p,reason);(R/p).write_text(s,encoding='utf-8');rec['finish'](p)
def edit(p,fn,reason):
 s=(R/p).read_text(encoding='utf-8-sig');n=fn(s);assert n!=s,p;write(p,n,reason)
for family in ['Player','Journey','Rank','Activities']:
 p=H/('InkRebuildFeature'+family+'.cpp.stage');write(B+'InkRebuildFeature'+family+'.cpp',p.read_text(encoding='utf-8'),'Bind actual owner '+family+' pages through newUI host; preserve native service actions and optional data truth')
edit(B+'Journey/InkJourneyNewWidgets.h',lambda s:s.replace('    FVector2D NormalizedPosition = FVector2D(.5f, .5f);','    FVector2D NormalizedPosition = FVector2D::ZeroVector;\n\n    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="newUI|Journey")\n    bool bPositionKnown = false;'),'Do not fabricate geography when map coordinates are absent')
def journey(s):
 s=s.replace('for(const auto& E:Entries){ const FString Group=EntryGroup(E);','for(const auto& E:Entries){ if(!E.bPositionKnown)continue; const FString Group=EntryGroup(E);')
 old='if(E&&IsResourceMap()){ for(const auto& Candidate:Entries){if(EntryGroup(Candidate)!=EntryGroup(*E))continue;'
 assert old in s;s=s.replace(old,'for(const auto& Candidate:Entries){if(Candidate.bPositionKnown&&!(E&&IsResourceMap()&&EntryGroup(Candidate)==EntryGroup(*E)))continue;')
 s=s.replace('&UInkJourneyMapPageWidget::HandlePinSelected);} } if(E){','&UInkJourneyMapPageWidget::HandlePinSelected);} if(E){')
 return s
edit(B+'Journey/InkJourneyNewWidgets.cpp',journey,'Known destinations without service coordinates remain selectable in a list; never center invented map pins')
def host(s):
 s=s.replace('#include "Components/CanvasPanel.h"','#include "Components/CanvasPanel.h"\n#include "Components/Border.h"')
 s=s.replace('Stable->RemoveField(TEXT("server_utc"));','if(PageId!=TEXT("Tribulation")&&PageId!=TEXT("RaceLive"))Stable->RemoveField(TEXT("server_utc"));')
 s=s.replace('    if(!Class)return;','''    if(PageId==TEXT("Market")&&!WidgetTree->FindWidget(TEXT("NewFeatureQuerySell")))InkRebuildUI::Action(WidgetTree,RootCanvas,FrontendOwner,TEXT("NewFeatureQuerySell"),TEXT("Ext_QueryItems"),FText::FromString(TEXT("读取可寄售物品")),FVector4(24,LastSize.Y-88,300,88));
    if(!Class)return;''')
 s=s.replace('    Move(TEXT("NewFeatureBack"),','    Move(TEXT("NewFeatureQuerySell"),FVector4(24,LastSize.Y-88,300,88));\n    Move(TEXT("NewFeatureBack"),')
 s=s.replace('    const FString Id=Action.ToString();','    const FString Id=Action.ToString();if(Id==TEXT("QueryItems")){QueryCatalog(TEXT("items"),0,true);return;}')
 return s
edit(B+'InkRebuildFeatureHost.cpp',host,'Complete owner host widget includes, real service clocks, and actual sell-inventory query action')
def econ(s):
 s=s.replace('bPriceRulesKnown','bPriceValidationAvailable')
 if 'void URebuildConsignmentWidget::HandleQuantityChanged' in s:
  s=s.replace('    ViewModel.DraftQuantityText = TextIn.ToString();','    ViewModel.DraftQuantityText = TextIn.ToString();\n    FRebuildActivityActionRequest Draft;Draft.Action=TEXT("ui.market.draft");Draft.QuantityText=ViewModel.DraftQuantityText;Draft.PriceText=ViewModel.DraftUnitPriceText;OnActionRequested.Broadcast(Draft);')
  s=s.replace('    ViewModel.DraftUnitPriceText = TextIn.ToString();','    ViewModel.DraftUnitPriceText = TextIn.ToString();\n    FRebuildActivityActionRequest Draft;Draft.Action=TEXT("ui.market.draft");Draft.QuantityText=ViewModel.DraftQuantityText;Draft.PriceText=ViewModel.DraftUnitPriceText;OnActionRequested.Broadcast(Draft);')
 return s
for suffix in ['h','cpp']:edit(B+'Activities/RebuildEconomyWidgets.'+suffix,econ,'Preserve typed market drafts across snapshot refresh; server validates user-entered price without invented price rules')
edit(B+'InkRebuildFeatureActivities.cpp',lambda s:s.replace('Vm.bPriceRulesKnown=false;Vm.bCreateAllowed=false;','Vm.bPriceValidationAvailable=CanDispatch();Vm.bCreateAllowed=CanDispatch()&&Vm.bInventoryProvided;'),'Allow confirmed actual-inventory listing requests through existing authoritative server validation')
edit(B+'InkRebuildFeatureRank.cpp',lambda s:s.replace('int32 Total=0;if(D::Integer(Page,TEXT("member_total"),Total)&&Vm.Cursor+Rows.Num()<Total)Vm.NextCursor=Vm.Cursor+Rows.Num();','D::Integer(Page,TEXT("next_cursor"),Vm.NextCursor);').replace('Vm.PageSize=8','Vm.PageSize=20'),'Use actual service next_cursor rather than inventing member totals')
