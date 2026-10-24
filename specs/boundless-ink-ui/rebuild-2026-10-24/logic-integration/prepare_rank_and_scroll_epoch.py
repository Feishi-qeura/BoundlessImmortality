"""Promote reviewed Rank v3 and repair actual core scroll/content layout."""
from pathlib import Path
import json,hashlib,runpy
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
rec=runpy.run_path(str(ROOT/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/record_source_change.py'),run_name='rank_scroll_recorder')
def edit(relative,fn,reason):
    path=ROOT/relative;before=path.read_text(encoding='utf-8-sig');after=fn(before);assert after!=before
    rec['begin'](relative,reason);path.write_text(after,encoding='utf-8');rec['finish'](relative)
base='Source/BoundlessImmortality/Frontend/UIRebuild20261024/'
def center(text):
    assert '#include "Components/SizeBoxSlot.h"' not in text
    text=text.replace('#include "Components/SizeBox.h"','#include "Components/SizeBox.h"\n#include "Components/SizeBoxSlot.h"')
    if 'Sizing->SetContent(Label);' in text:
        assert text.count('Sizing->SetContent(Label);')==1
        text=text.replace('Sizing->SetContent(Label);','Sizing->SetContent(Label);\n\t\tif(auto* Slot=Cast<USizeBoxSlot>(Label->Slot))Slot->SetVerticalAlignment(VAlign_Center);')
    else:
        old='\t\tButton->SetContent(Size);';assert text.count(old)==1
        text=text.replace(old,'\t\tif(auto* Slot=Cast<USizeBoxSlot>(Size->GetContent()->Slot))Slot->SetVerticalAlignment(VAlign_Center);\n'+old)
    return text
for name in ('InkPlayerMenuPageWidgets.cpp','InkPlayerVisibleEntryWidgets.cpp','InkPlayerCharacterInventoryWidgets.cpp'):
    edit(base+'Player/'+name,center,'Actual PIE02 polish: center labels inside the filled minimum-touch SizeBox')
def scroll(text):
    for page in ('Character','Inventory'):
        old='\tauto* Scroll = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("Rebuild_'+page+'ScrollV1"));\n\tScroll->SetAllowOverscroll(false);\n';assert text.count(old)==1;text=text.replace(old,'')
    assert text.count('\tScroll->AddChild(Body);')==2;text=text.replace('\tScroll->AddChild(Body);\n','')
    assert text.count('\treturn Scroll;')==2;text=text.replace('\treturn Scroll;','\treturn Body;')
    return text
edit(base+'Player/InkPlayerCharacterInventoryWidgets.cpp',scroll,'Use the new root single page ScrollBox; body composers must not nest an expanding inner scroll viewport')
owner=HERE.parent/'panel-rank/review-v3';status=json.loads((owner/'status.json').read_text(encoding='utf-8-sig'))
for name,expected in status['source_sha256'].items():
    path=owner/'staged-source'/name;assert hashlib.sha256(path.read_bytes()).hexdigest()==expected
for name in status['source_sha256']:
    rec['promote']('panel-rank',str(owner/'status.json'),str(owner/'staged-source'/name),base+'Rank/'+name)
def fonts(text):
    assert text.count('static_cast<float>(Size)')==2
    return text.replace('static_cast<float>(Size)','static_cast<float>(FMath::RoundToInt(FMath::Max(26,Size)*.75f))')
edit(base+'Rank/InkRankWulingPageBase.cpp',fonts,'Shared newUI typography: owner logical sizes use project0.75 Slate points and minimum26 logical pixels')
def avatar_header(text):
    old='    UFUNCTION(BlueprintCallable) void SetParticipantAvatar(const FString& ParticipantId, UTexture2D* Avatar);';assert text.count(old)==1
    return text.replace(old,old+'\n    /** Host invalidates only when the provider/URL identity actually changes. */\n    void InvalidateParticipantAvatar(const FString& ParticipantId);')
edit(base+'Rank/InkRankWulingPages.h',avatar_header,'Expose precise participant avatar invalidation for actual provider/URL changes')
def avatar_impl(text):
    marker='void UInkRankBoardPage::RetryParticipantAvatarAfterHostBackoff';assert text.count(marker)==1
    return text.replace(marker,'void UInkRankBoardPage::InvalidateParticipantAvatar(const FString& ParticipantId)\n{\n    ParticipantAvatarCache.Remove(ParticipantId);AvatarRequestsInFlight.Remove(ParticipantId);\n}\n\n'+marker)
edit(base+'Rank/InkRankWulingPages.cpp',avatar_impl,'Clear only stale identity-scoped avatar cache; preserve request completion/backoff semantics')
print('Promoted exact6 Rank v3 sources and recorded core scroll/theme/cache integration corrections')
