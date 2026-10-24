"""Preserve confirmed Data projections while only mutations are transport-busy."""
from pathlib import Path
import runpy
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
rec=runpy.run_path(str(ROOT/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/record_source_change.py'),run_name='busy_projection_recorder')
def edit(relative,replacements):
    path=ROOT/relative;text=path.read_text(encoding='utf-8-sig')
    for old,new in replacements:
        assert text.count(old)==1,(relative,old,text.count(old));text=text.replace(old,new)
    rec['begin'](relative,'I3-01 keep confirmed character/inventory rows and selected identity during transport Busy; gate only mutations')
    path.write_text(text,encoding='utf-8');rec['finish'](relative)
base='Source/BoundlessImmortality/Frontend/UIRebuild20261024/'
edit(base+'InkRebuildFrontendWidget.cpp',[
    ('PlayerState(InkCorePolicy::EState State,bool Busy)','PlayerState(InkCorePolicy::EState State)'),
    ('    if(Busy)return E::Pending;\n',''),
    ('PlayerState(View.CharacterState,View.Transport.Busy),FText())','PlayerState(View.CharacterState),View.Transport.Busy?FText::FromString(TEXT("请求处理中，保留已确认的数据")):FText())'),
    ('PlayerState(View.InventoryState,View.Transport.Busy)','PlayerState(View.InventoryState)'),
    ('PlayerTexture(TEXT("T_InventoryCategoryGlyphAtlas_v1")),Provider,FText(),FMath::Max(112.f,LastSafeSize.X-60))','PlayerTexture(TEXT("T_InventoryCategoryGlyphAtlas_v1")),Provider,View.Transport.Busy?FText::FromString(TEXT("请求处理中，保留已确认的数据")):FText(),FMath::Max(112.f,LastSafeSize.X-60),View.Transport.Busy)')])
edit(base+'Player/InkPlayerCharacterInventoryWidgets.h',[
    ('float AvailableWidth=672.f);','float AvailableWidth=672.f, bool bMutationsBusy=false);')])
edit(base+'Player/InkPlayerCharacterInventoryWidgets.cpp',[
    ('if (State == E::Data) return;','if (State == E::Data && Message.IsEmpty()) return;'),
    ('const FText& StateMessage, const float AvailableWidth)','const FText& StateMessage, const float AvailableWidth, const bool bMutationsBusy)'),
    ('ActionIds[Index], ActionLabels[Index], Index == 0, Enabled[Index]);','ActionIds[Index], ActionLabels[Index], Index == 0, Enabled[Index] && !bMutationsBusy);')])
print('Recorded busy projection fix in 3 source files')
