from pathlib import Path
import runpy
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
rec=runpy.run_path(str(ROOT/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/record_source_change.py'),run_name='rank_root_recorder')
def put(relative,fn):
    p=ROOT/relative;before=p.read_text(encoding='utf-8-sig');after=fn(before);assert after!=before
    rec['begin'](relative,'New root leaderboard: reviewed typed page, category/scope actions, exact asynchronous avatar identity and provider projection')
    p.write_text(after,encoding='utf-8');rec['finish'](relative)
base='Source/BoundlessImmortality/Frontend/'
def frontend(text):
    old='    void CompleteLeaderboardAvatar(const FString& Key,UTexture2D* Texture);';assert text.count(old)==1
    return text.replace(old,'    virtual void CompleteLeaderboardAvatar(const FString& Key,UTexture2D* Texture);')
put(base+'InkFrontendWidget.h',frontend)
def header(text):
    text=text.replace('class UCanvasPanel;','class UInkRankBoardPage;\nclass UInkLeaderboardAvatarRequest;\nclass UCanvasPanel;',1)
    marker='    /** Mount a reviewed owner component';assert marker in text
    text=text.replace(marker,'    virtual void CompleteLeaderboardAvatar(const FString& Key,UTexture2D* Texture)override;\n'+marker)
    marker='    UFUNCTION() void HandleNewInventorySearch';assert marker in text
    text=text.replace(marker,'''    UPROPERTY(Transient) TObjectPtr<UInkRankBoardPage> NewRankBoard;
    UPROPERTY(Transient) TMap<FString,TObjectPtr<UInkLeaderboardAvatarRequest>> NewRankRequests;
    UPROPERTY(Transient) TMap<FString,TObjectPtr<UTexture2D>> NewRankAvatarCache;
    TArray<FInkLeaderboardEntry> NewRankRows;
    TMap<FString,FString> NewRankAvatarSubjects,NewRankPendingIds;
    TMap<FString,double> NewRankAvatarRetryAt;
    FString NewRankContentKey;
    void RefreshNewLeaderboard();
    void PollNewRankAvatars();
    UFUNCTION() void HandleNewRankCategory(FName Category);
    UFUNCTION() void HandleNewRankScope(FName Scope);
    UFUNCTION() void HandleNewRankRetry();
    UFUNCTION() void HandleNewRankAvatar(FString ParticipantId);
'''+marker)
    return text
put(base+'UIRebuild20261024/InkRebuildFrontendWidget.h',header)
def cpp(text):
    old='RefreshMainAndHud();RefreshLocalPage(CurrentPage);';assert text.count(old)==1
    text=text.replace(old,'RefreshMainAndHud();RefreshLocalPage(CurrentPage);RefreshNewLeaderboard();PollNewRankAvatars();')
    old='const float BodyY=Page==TEXT("Inventory")?240.f:136.f;';assert text.count(old)==1
    text=text.replace(old,'''if(Page==TEXT("Leaderboard"))if(auto* Heading=GetWidgetFromName(TEXT("NewUIHeading_Leaderboard")))Heading->SetVisibility(ESlateVisibility::Collapsed);
        const float BodyY=Page==TEXT("Inventory")?240.f:Page==TEXT("Leaderboard")?24.f:136.f;''')
    return text
put(base+'UIRebuild20261024/InkRebuildFrontendWidget.cpp',cpp)
print('Connected independent Rank page to default root and real board provider/actions')
