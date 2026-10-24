"""Apply only after owned PIE normal exit and fresh engine conflict check."""
from pathlib import Path
import runpy
ROOT=Path.cwd();BASE='Source/BoundlessImmortality/Frontend/UIRebuild20261024/'
REC=runpy.run_path(str(ROOT/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/record_source_change.py'),run_name='actual_pie06_and_guid_payload_repair')
def edit(path,transform,reason):
 target=ROOT/path;before=target.read_text(encoding='utf-8-sig');after=transform(before);assert after!=before,path
 REC['begin'](path,reason);target.write_text(after,encoding='utf-8');REC['finish'](path)
def back(source):
 old='if(PageId==TEXT("OfflineRewards")&&!WidgetTree->FindWidget(TEXT("NewFeatureBack")))'
 new='if((PageId==TEXT("OfflineRewards")||PageId==TEXT("Activities")||PageId==TEXT("DailyActivities")||PageId==TEXT("TimedActivities")||PageId==TEXT("DailyActivityDetail")||PageId==TEXT("TimedActivityDetail")||PageId==TEXT("LaunchEvent"))&&!WidgetTree->FindWidget(TEXT("NewFeatureBack")))'
 assert source.count(old)==1
 return source.replace(old,new)
def equipment(source):
 old='bool IdentityMatches=Entry&&Entry->Item.ItemId==P.ItemId;'
 new='bool IdentityMatches=Entry&&P.EntryId==SelectedInventoryEntry&&(P.ItemId.IsNone()||Entry->Item.ItemId==P.ItemId);'
 assert source.count(old)==1;source=source.replace(old,new)
 old='if(D::String(Row,TEXT("method_id"))==P.ItemId.ToString()&&D::String(Row,TEXT("item_id"))==Entry->Item.ItemId.ToString()'
 new='if(P.EntryId==SelectedInventoryEntry&&D::String(Row,TEXT("method_id"))==P.ItemId.ToString()&&D::String(Row,TEXT("item_id"))==Entry->Item.ItemId.ToString()'
 assert source.count(old)==1
 return source.replace(old,new)
edit(BASE+'InkRebuildFeatureHost.cpp',back,'PIE06 actual Activities has no Back. The same owner Hub/category/LaunchEvent pages also omit Back; provide one existing typed host Back with ancestor restoration.')
edit(BASE+'InkRebuildFeaturePlayer.cpp',equipment,'Equipment typed payload carries actual instance GUID and no item ID. Validate current selected GUID plus optional supplied ID against fresh model; keep required book-to-method mapping for LearnMethod.')

def preserve_scroll(source):
 old='    RetirePurePage();UWidget* Body=nullptr;'
 new='    TMap<FName,float> ScrollOffsets;\n    TFunction<void(UWidget*,bool)> PreserveScroll=[&](UWidget* Widget,bool Restore)\n    {\n        if(!Widget)return;\n        if(auto* Scroll=Cast<UScrollBox>(Widget))\n        {if(Restore){if(const float* Offset=ScrollOffsets.Find(Scroll->GetFName()))Scroll->SetScrollOffset(*Offset);}else ScrollOffsets.Add(Scroll->GetFName(),Scroll->GetScrollOffset());}\n        if(auto* Panel=Cast<UPanelWidget>(Widget))for(auto* Child:Panel->GetAllChildren())PreserveScroll(Child,Restore);\n    };\n    PreserveScroll(PurePage,false);RetirePurePage();UWidget* Body=nullptr;'
 assert source.count(old)==1;source=source.replace(old,new)
 old='MountPure(PureScroll,TEXT("newui-r1-player-feature-")+PageId.ToString());}'
 new='MountPure(PureScroll,TEXT("newui-r1-player-feature-")+PageId.ToString());PreserveScroll(PurePage,true);}'
 assert source.count(old)==1
 return source.replace(old,new)
edit(BASE+'InkRebuildFeaturePlayer.cpp',preserve_scroll,'Service PIE01 actual book row was scrolled then provider refresh rebuilt all pure widgets and reset outer offset0, blocking Slate click. Preserve current pure page named scroll offsets through confirmed data refresh; actor reset retires page before capture.')
