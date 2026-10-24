#include "InkRebuildFrontendWidget.h"
#include "Rank/InkRankWulingPages.h"
#include "Frontend/InkLeaderboardAvatarRequest.h"
#include "Frontend/InkLeaderboardTheme.h"
#include "Frontend/UiRollout20261024/LogicCore/InkCoreAdapter.h"
#include "Cultivation/CultivationRules.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Engine/Font.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"

namespace
{
EInkRankPageState RebuildBoardState(InkCorePolicy::EState State)
{
    switch(State)
    {
    case InkCorePolicy::EState::Data:return EInkRankPageState::Data;
    case InkCorePolicy::EState::Empty:return EInkRankPageState::Empty;
    case InkCorePolicy::EState::Loading:return EInkRankPageState::Loading;
    case InkCorePolicy::EState::Offline:return EInkRankPageState::Offline;
    case InkCorePolicy::EState::Error:return EInkRankPageState::Error;
    default:return EInkRankPageState::Unavailable;
    }
}
FString RebuildAvatarKey(FName Scope,const FInkLeaderboardEntry& Entry)
{return Scope.ToString()+TEXT("|")+Entry.ParticipantId+TEXT("|")+Entry.AvatarUrl;}
}

void UInkRebuildFrontendWidget::RefreshNewLeaderboard()
{
    if(CurrentPage!=TEXT("Leaderboard")||!GetOwningPlayer())return;
    if(!NewRankBoard)
    {
        NewRankBoard=CreateWidget<UInkRankBoardPage>(GetOwningPlayer());if(!NewRankBoard)return;
        NewRankBoard->SetSharedThemeFonts(
            LoadObject<UFont>(nullptr,TEXT("/Game/newUI/Ink/Fonts/Shared/F_InkDisplay.F_InkDisplay")),
            LoadObject<UFont>(nullptr,TEXT("/Game/newUI/Ink/Fonts/Shared/F_InkBody.F_InkBody")));
        NewRankBoard->OnCategoryRequested.AddUniqueDynamic(this,&ThisClass::HandleNewRankCategory);
        NewRankBoard->OnScopeRequested.AddUniqueDynamic(this,&ThisClass::HandleNewRankScope);
        NewRankBoard->OnRetryRequested.AddUniqueDynamic(this,&ThisClass::HandleNewRankRetry);
        NewRankBoard->OnAvatarRequested.AddUniqueDynamic(this,&ThisClass::HandleNewRankAvatar);
        MountPageComponent(TEXT("Leaderboard"),NewRankBoard,TEXT("newui-r1-rank-leaderboard"));
        if(auto* Outer=GetWidgetFromName(TEXT("NewUIScroll_Leaderboard")))Outer->RemoveFromParent();
        NewRankBoard->RemoveFromParent();if(auto* Page=PageCanvases.FindRef(TEXT("Leaderboard")).Get())Page->AddChildToCanvas(NewRankBoard);
    }
    const auto Size=LastSafeSize.IsNearlyZero()?FVector2D(720,1280):LastSafeSize;
    if(auto* Header=Cast<USizeBox>(NewRankBoard->WidgetTree->FindWidget(TEXT("NewRankHeaderSize"))))Header->SetHeightOverride(Size.X>Size.Y?88:176);
    if(auto* Layout=Cast<UCanvasPanelSlot>(NewRankBoard->Slot))Layout->SetOffsets(FMargin(24,24,Size.X-48,Size.Y-96));
    const auto Board=InkCore::ReadBoard(this,GetPresentedRoomLeaderboard(),LeaderboardScope,
        GetPresentedLeaderboardCategory(),HasPresentedRoomLeaderboard());
    FString Key=Board.Scope.ToString()+TEXT("|")+Board.Category.ToString()+FString::FromInt(int32(Board.State))+TEXT("|")+Board.Status;
    for(const auto& Row:Board.Entries)
    {FString Text;FInkLeaderboardEntry::StaticStruct()->ExportText(Text,&Row,nullptr,nullptr,PPF_None,nullptr);Key+=TEXT("|")+Text;}
    if(Key==NewRankContentKey)return;
    TMap<FString,FString> CurrentSubjects;
    for(const auto& Row:Board.Entries)CurrentSubjects.Add(Row.ParticipantId,RebuildAvatarKey(Board.Scope,Row));
    for(const auto& Pair:NewRankAvatarSubjects)
    {
        const auto* Current=CurrentSubjects.Find(Pair.Key);
        if(!Current||*Current!=Pair.Value)
        {
            NewRankBoard->InvalidateParticipantAvatar(Pair.Key);
            if(auto* Request=NewRankRequests.FindRef(Pair.Value).Get())Request->Cancel();
            NewRankRequests.Remove(Pair.Value);NewRankPendingIds.Remove(Pair.Value);
            NewRankAvatarCache.Remove(Pair.Value);NewRankAvatarRetryAt.Remove(Pair.Value);
        }
    }
    NewRankRows=Board.Entries;NewRankAvatarSubjects=MoveTemp(CurrentSubjects);NewRankContentKey=Key;
    FInkNewRankBoardData Data;Data.Category=Board.Category;Data.Scope=Board.Scope;
    Data.Title=FText::FromString(InkLeaderboardTheme::CategoryTitle(Board.Category));
    Data.MetricLabel=FText::FromString(InkLeaderboardTheme::MetricLabel(Board.Category));
    Data.ProviderStatus=FText::FromString(Board.Status);Data.State=RebuildBoardState(Board.State);
    Data.SelfPosition=FText::FromString(TEXT("本人名次未提供"));
    const auto* Cultivation=GetGameInstance()?GetGameInstance()->GetSubsystem<UCultivationSubsystem>():nullptr;
    const FString LocalSelf=Board.Scope==TEXT("Local")&&Cultivation?Cultivation->GetAnchorId():FString();
    for(const auto& Entry:Board.Entries)
    {
        FInkNewRankRow Row;Row.Rank=Entry.Rank;Row.ParticipantId=Entry.ParticipantId;
        Row.DisplayName=Entry.DisplayName;Row.EarnedTitle=Entry.TitleId.IsNone()?FText():CultivationRules::TitleName(Entry.TitleId);
        Row.Realm=Entry.Realm;Row.Metric=Entry.Metric;
        Row.Avatar=Entry.AvatarTexture.LoadSynchronous();
        if(!Row.Avatar)Row.Avatar=NewRankAvatarCache.FindRef(RebuildAvatarKey(Board.Scope,Entry));
        Data.Rows.Add(Row);
        if(!LocalSelf.IsEmpty()&&Entry.ParticipantId==LocalSelf)
        {Data.SelfPosition=FText::Format(FText::FromString(TEXT("本人名次：{0}")),FText::AsNumber(Entry.Rank));Data.SelfMetric=Entry.Metric;}
    }
    NewRankBoard->SetBoardData(Data);
}

void UInkRebuildFrontendWidget::HandleNewRankCategory(FName Category)
{
    if(Category!=TEXT("Damage")&&Category!=TEXT("CultivationSpeed")&&Category!=TEXT("Anchor"))return;
    TriggerAction(Category==TEXT("Damage")?TEXT("RankDamage"):Category==TEXT("CultivationSpeed")?TEXT("RankCultivation"):TEXT("RankAnchor"));
}
void UInkRebuildFrontendWidget::HandleNewRankScope(FName Scope)
{if(Scope==TEXT("Local")||Scope==TEXT("Global"))TriggerAction(Scope==TEXT("Global")?TEXT("RankGlobal"):TEXT("RankLocal"));}
void UInkRebuildFrontendWidget::HandleNewRankRetry()
{NewRankContentKey.Empty();RefreshNewLeaderboard();}

void UInkRebuildFrontendWidget::HandleNewRankAvatar(FString ParticipantId)
{
    if(!NewRankBoard)return;
    const auto* Entry=NewRankRows.FindByPredicate([&](const auto& Row){return Row.ParticipantId==ParticipantId;});
    if(!Entry||Entry->AvatarUrl.IsEmpty()){NewRankBoard->SetParticipantAvatar(ParticipantId,nullptr);return;}
    const FString Key=RebuildAvatarKey(LeaderboardScope,*Entry);
    if(const auto Texture=NewRankAvatarCache.FindRef(Key)){NewRankBoard->SetParticipantAvatar(ParticipantId,Texture);return;}
    if(NewRankRequests.Contains(Key))return;
    if(NewRankAvatarRetryAt.FindRef(Key)>FPlatformTime::Seconds())
    {NewRankBoard->SetParticipantAvatar(ParticipantId,nullptr);return;}
    if(NewRankRequests.Num()>=2){NewRankBoard->SetParticipantAvatar(ParticipantId,nullptr);return;}
    auto* Request=NewObject<UInkLeaderboardAvatarRequest>(this);NewRankRequests.Add(Key,Request);NewRankPendingIds.Add(Key,ParticipantId);
    Request->Start(this,Key,Entry->AvatarUrl);
}
void UInkRebuildFrontendWidget::CompleteLeaderboardAvatar(const FString& Key,UTexture2D* Texture)
{
    const FString Participant=NewRankPendingIds.FindRef(Key);NewRankRequests.Remove(Key);NewRankPendingIds.Remove(Key);
    if(Participant.IsEmpty()){Super::CompleteLeaderboardAvatar(Key,Texture);return;}
    if(NewRankAvatarSubjects.FindRef(Participant)!=Key)return;
    if(Texture){if(NewRankAvatarCache.Num()>=90)NewRankAvatarCache.Reset();NewRankAvatarCache.Add(Key,Texture);NewRankAvatarRetryAt.Remove(Key);}
    else NewRankAvatarRetryAt.Add(Key,FPlatformTime::Seconds()+60.);
    if(NewRankBoard)NewRankBoard->SetParticipantAvatar(Participant,Texture);
}
void UInkRebuildFrontendWidget::PollNewRankAvatars()
{
    TArray<FString> Expired;for(const auto& Pair:NewRankRequests)if(!Pair.Value||Pair.Value->HasExpired())Expired.Add(Pair.Key);
    for(const auto& Key:Expired){if(auto* Request=NewRankRequests.FindRef(Key).Get())Request->Cancel();CompleteLeaderboardAvatar(Key,nullptr);}
    if(CurrentPage!=TEXT("Leaderboard")||!NewRankBoard)return;
    for(const auto& Row:NewRankRows)
    {
        const FString Key=RebuildAvatarKey(LeaderboardScope,Row);
        if(NewRankRequests.Num()>=2)break;
        if(!Row.AvatarUrl.IsEmpty()&&Row.AvatarTexture.IsNull()&&!NewRankAvatarCache.Contains(Key)
            &&!NewRankRequests.Contains(Key)&&NewRankAvatarRetryAt.FindRef(Key)<=FPlatformTime::Seconds())
            NewRankBoard->RetryParticipantAvatarAfterHostBackoff(Row.ParticipantId);
    }
}
