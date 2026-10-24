"""Prepare an exact reviewable patch; never writes the shared source files."""
from pathlib import Path
import difflib
import hashlib
import json

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[4]
changes = {}


def edit(relative, replacements):
    path = ROOT / relative
    before = path.read_text(encoding='utf-8-sig')
    after = before
    for old, new in replacements:
        if after.count(old) != 1:
            raise RuntimeError(f'Expected exactly one anchor in {relative}: {old[:100]}')
        after = after.replace(old, new, 1)
    changes[relative] = (before, after)


edit('Source/BoundlessImmortality/Frontend/InkFrontendWidget.h', [
    ('    const FText& GetPresentedCharacterName()const{return CharacterName;}',
     '    const FText& GetPresentedCharacterName()const{return CharacterName;}\n'
     '    const TArray<FInkLeaderboardEntry>& GetPresentedRoomLeaderboard()const{return LeaderboardEntries;}\n'
     '    bool HasPresentedRoomLeaderboard()const{return bRolloutRoomLeaderboardProvided;}'),
    ('    UPROPERTY(Transient) TArray<FInkLeaderboardEntry> LeaderboardEntries;',
     '    bool bRolloutRoomLeaderboardProvided=false;\n'
     '    UPROPERTY(Transient) TArray<FInkLeaderboardEntry> LeaderboardEntries;'),
])
edit('Source/BoundlessImmortality/Frontend/InkFrontendWidget.cpp', [
    ('{if(Scope!=TEXT("Global")&&Scope!=TEXT("Local"))return;auto& Target=',
     '{if(Scope!=TEXT("Global")&&Scope!=TEXT("Local"))return;if(Scope==TEXT("Local"))bRolloutRoomLeaderboardProvided=true;auto& Target='),
])
include = '#include "Frontend/UiRollout20261024/LogicCore/InkCoreAdapter.h"\n'
edit('Source/BoundlessImmortality/Frontend/UI/InkWulingToolsPanel.cpp', [
    ('#include "InkWulingToolsPanel.h"\n', '#include "InkWulingToolsPanel.h"\n' + include),
    ('    if(Id==TEXT("ToolPrepare")){if(bRework)W->SubmitPrototypeCommand(TEXT("ReworkQuality"),TEXT("{\\\"selection\\\":")+SelectionJson()+TEXT("}"));else W->PrepareBreakthrough(SelectionJson());return;}',
     '    if(Id==TEXT("ToolPrepare")){const auto Result=InkCore::PrepareBreakthrough(this,MaterialSelection,bRework,FrontendOwner&&FrontendOwner->CurrentPage==PageId);FeedbackMessage(Result.Message);return;}'),
])
# Existing inventory action branch delegates to the same adapter used by new shells.
relative = 'Source/BoundlessImmortality/Frontend/InkInventoryWidget.cpp'
path = ROOT / relative
before = path.read_text(encoding='utf-8-sig')
start = before.index('            auto Args=MakeShared<FJsonObject>();Args->SetStringField(TEXT("item_id"),E->Item.ItemId.ToString());')
end_marker = '            SetPageFeedback(FText::FromString(W->SubmitPrototypeCommand(Op,Json)?TEXT("请求已记录，等待服务确认"):W->GetStatus()));return true;'
end = before.index(end_marker, start) + len(end_marker)
replacement = '''            const auto CoreAction=Action==TEXT("InventoryEquip")?(E->bEquipped?InkCorePolicy::EInventoryAction::Unequip:InkCorePolicy::EInventoryAction::Equip):
                Action==TEXT("InventorySell")?InkCorePolicy::EInventoryAction::Sell:InkCorePolicy::EInventoryAction::Use;
            const auto Result=InkCore::InventoryAction(this,SelectedInventoryEntry,CoreAction,InventoryActionQuantity,CurrentPage==TEXT("Inventory"));
            SetPageFeedback(FText::FromString(Result.Message));return true;'''
after = before[:start] + replacement + before[end:]
after = include + after
changes[relative] = before, after

patch = []
baselines = []
for relative, (before, after) in changes.items():
    baselines.append(dict(path=relative, sha256=hashlib.sha256((ROOT / relative).read_bytes()).hexdigest()))
    patch.extend(difflib.unified_diff(before.splitlines(keepends=True), after.splitlines(keepends=True),
                                     fromfile='a/' + relative, tofile='b/' + relative))
(HERE / 'integration.patch').write_text(''.join(patch), encoding='utf-8')
(HERE / 'integration-baseline.json').write_text(json.dumps(baselines, indent=2), encoding='utf-8')
print(f'Prepared {len(changes)} shared-file changes. No shared files edited.')
