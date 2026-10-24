#include "InkRebuildFeatureHost.h"
#include "InkRebuildFeatureData.h"
#include "Rank/InkRankWulingExtendedPages.h"
#include "Frontend/UiRollout20261024/LogicCore/InkCoreAdapter.h"
#include "Wuling/WulingPrototypeSubsystem.h"
#include "Engine/Texture2D.h"
namespace D=InkNewFeatureData;
void UInkRebuildFeatureHost::RefreshRankFamily()
{
    if(!OwnerPage)return;auto* W=Service();const auto Guild=D::Object(DisplaySnapshot,TEXT("guild")),Counts=D::Object(DisplaySnapshot,TEXT("counts"));const FText Status=FText::FromString(W?W->GetStatus():TEXT("服务未启用"));
    const auto State=!W||!W->IsEnabled()?EInkRankPageState::Unavailable:!W->IsConnected()?EInkRankPageState::Offline:!DisplaySnapshot?EInkRankPageState::Loading:EInkRankPageState::Data;
    if(auto* Overview=Cast<UInkWulingOverviewPage>(OwnerPage))
    {
        FInkWulingOverviewData Vm;Vm.Insignia=LoadObject<UTexture2D>(nullptr,TEXT("/Game/newUI/Components/RankWuling/T_NUI_Wuling_SectInsignia_Normal_V1.T_NUI_Wuling_SectInsignia_Normal_V1"));Vm.ServiceStatus=Status;Vm.SectName=D::Text(Guild,TEXT("name"));Vm.MemberCount=D::NumericText(Counts,TEXT("members"));Vm.ParticipatingCount=D::NumericText(Counts,TEXT("participating_members"));Vm.HelperCount=D::NumericText(Counts,TEXT("helpers"));Vm.PublicSupply=D::NumericText(Guild,TEXT("supply"));Vm.Reputation=D::NumericText(Guild,TEXT("reputation"));
        const TPair<const TCHAR*,const TCHAR*> Actions[]={{TEXT("WulingMembers"),TEXT("成员")},{TEXT("GuildConstruction"),TEXT("武灵建设")},{TEXT("CultivationMethods"),TEXT("功法")},{TEXT("EquipmentFabao"),TEXT("装备与法宝")},{TEXT("WulingCraft"),TEXT("物品炼制")},{TEXT("WorldCultivation"),TEXT("世界历练")},{TEXT("ResourceContest"),TEXT("灵脉竞采")},{TEXT("BreakthroughPrepare"),TEXT("准备突破")},{TEXT("OfflineRewards"),TEXT("离线收益")},{TEXT("Shop"),TEXT("坊市商店")},{TEXT("Market"),TEXT("寄售")},{TEXT("TradeOrders"),TEXT("交易订单")}};
        const auto Identity=D::Object(DisplaySnapshot,TEXT("identity"));const bool Manage=CanDispatch()&&!D::String(Guild,TEXT("id")).IsEmpty()&&D::String(Guild,TEXT("anchor"))==D::String(Identity,TEXT("user"));
        const TPair<const TCHAR*,const TCHAR*> Governance[]={{TEXT("GuildCreate"),TEXT("创建武灵")},{TEXT("GuildRename"),TEXT("武灵改名")},{TEXT("GuildDissolve"),TEXT("解散武灵")}};
        for(const auto& Choice:Governance){FInkWulingActionView Entry;Entry.ActionId=Choice.Key;Entry.Label=FText::FromString(Choice.Value);Entry.bEnabled=Entry.ActionId==TEXT("GuildCreate")?CanDispatch()&&DisplaySnapshot&&DisplaySnapshot->HasField(TEXT("guild"))&&!Guild:Manage;Vm.ExistingActions.Add(Entry);}
        FInkWulingActionView Shared;Shared.ActionId=TEXT("GuildShared");Shared.Label=FText::FromString(TEXT("武灵共修"));Shared.bEnabled=true;Vm.ExistingActions.Add(Shared);
        for(const auto& Action:Actions){FInkWulingActionView Entry;Entry.ActionId=Action.Key;Entry.Label=FText::FromString(Action.Value);Entry.bEnabled=true;Vm.ExistingActions.Add(Entry);}Overview->SetOverviewData(Vm);
    }
    if(auto* Members=Cast<UInkWulingMembersPage>(OwnerPage))
    {
        FInkWulingMembersData Vm;Vm.PageSize=20;Vm.State=State;Vm.ServiceStatus=Status;const auto Page=DisplaySnapshot;bool Provided=false;const auto Rows=D::Rows(Page,TEXT("members"),&Provided);if(W&&W->IsEnabled()&&W->IsConnected())Vm.State=!Provided?EInkRankPageState::Loading:Rows.IsEmpty()?EInkRankPageState::Empty:EInkRankPageState::Data;
        for(const auto& Row:Rows){FInkWulingMemberRow Member;Member.UserId=D::String(Row,TEXT("user"));Member.DisplayName=D::Named(Row,TEXT("name"),Member.UserId);Vm.Rows.Add(Member);}
        D::Integer(Page,TEXT("member_cursor"),Vm.Cursor);D::Integer(Page,TEXT("next_cursor"),Vm.NextCursor);if(Vm.Cursor>0)Vm.PreviousCursor=FMath::Max(0,Vm.Cursor-Vm.PageSize);Vm.SelectedUserId=SelectedEntry;Vm.bRemovePending=W&&W->IsBusy();
        Vm.bCanRemoveSelected=CanDispatch()&&!SelectedEntry.IsEmpty()&&SelectedEntry!=D::String(Guild,TEXT("anchor"))&&Vm.Rows.ContainsByPredicate([&](const auto& Row){return Row.UserId==SelectedEntry;});Vm.RemoveDisabledReason=FText::FromString(TEXT("请选择当前成员页中的正式成员，并等待服务就绪"));Members->SetMembersData(Vm);
    }
    if(auto* Building=Cast<UInkGuildBuildPage>(OwnerPage))
    {
        FInkGuildBuildPageData Vm;Vm.State=State;Vm.ServiceStatus=Status;Vm.Offset=QueryOffset;Vm.bOwnedOnly=bOwnedOnly;Vm.bCanQuery=CanDispatch();
        Vm.bBuildPending=W&&W->IsBusy();Vm.SnapshotVersion=D::Version(DisplaySnapshot);const auto Catalog=D::Catalog(DisplaySnapshot,TEXT("buildings"),QueryOffset);bool Provided=false;const auto Rows=D::Rows(Catalog,TEXT("rows"),&Provided);
        if(W&&W->IsEnabled()&&W->IsConnected())Vm.State=!Provided?EInkRankPageState::Loading:Rows.IsEmpty()?EInkRankPageState::Empty:EInkRankPageState::Data;
        Vm.PreviousOffset=QueryOffset>0?FMath::Max(0,QueryOffset-8):INDEX_NONE;D::Integer(Catalog,TEXT("next_offset"),Vm.NextOffset);
        for(const auto& Row:Rows){FInkGuildBuildingCardView Entry;Entry.BuildingId=D::String(Row,TEXT("building_id"));Entry.DisplayName=D::Named(Row,TEXT("name"),Entry.BuildingId);Entry.bHasCurrentLevel=D::Integer(Row,TEXT("current_level"),Entry.CurrentLevel);if(!Entry.bHasCurrentLevel)for(const auto& Current:D::Rows(DisplaySnapshot,TEXT("buildings")))if(D::String(Current,TEXT("building_id"))==Entry.BuildingId){Entry.bHasCurrentLevel=D::Integer(Current,TEXT("level"),Entry.CurrentLevel);break;}Entry.bHasTargetLevel=D::Integer(Row,TEXT("level"),Entry.TargetLevel);Entry.AuthoritativeCosts=D::NumericText(Row,TEXT("supply_cost"));Entry.StateText=D::Text(Row,TEXT("status"));Entry.bCanBuild=CanDispatch()&&!Entry.BuildingId.IsEmpty()&&Entry.bHasTargetLevel&&Entry.TargetLevel>0&&!Vm.SnapshotVersion.IsEmpty();Vm.Buildings.Add(Entry);}Building->SetBuildPageData(Vm);
    }
    if(auto* Sword=Cast<UInkSwordSpiritPage>(OwnerPage))
    {
        FInkSwordSpiritPageData Vm;Vm.State=State;Vm.ServiceStatus=Status;const auto Core=InkCore::Read(FrontendOwner,IsActivePage());Vm.DisplayName=Core.CharacterName;Vm.ParticipantId=D::String(D::Object(DisplaySnapshot,TEXT("identity")),TEXT("user"));Vm.Realm=D::Text(D::Object(DisplaySnapshot,TEXT("growth")),TEXT("name"));
        for(const auto& Attribute:Core.Attributes){FInkSwordSpiritAttributeView Row;Row.Label=Attribute.Label;Row.Value=Attribute.Value;Vm.Attributes.Add(Row);}Sword->SetSwordSpiritData(Vm);
    }
    if(auto* Gifts=Cast<UInkGiftRulesPage>(OwnerPage))
    {
        FInkGiftEffectPageData Vm;Vm.State=State;Vm.ServiceStatus=Status;const auto Rules=D::Rows(DisplaySnapshot,TEXT("gift_rules"),&Vm.bRulesProvided);
        for(const auto& Row:Rules){FInkGiftRuleView Rule;Rule.RuleId=D::String(Row,TEXT("id"));Rule.Name=D::Text(Row,TEXT("name"));Rule.ActualDescription=D::Text(Row,TEXT("description"));Vm.Rules.Add(Rule);}
        for(const auto& Row:D::Rows(DisplaySnapshot,TEXT("gift_effects"))){FInkGiftRuleView Effect;Effect.RuleId=D::String(Row,TEXT("id"));Effect.Name=D::Text(Row,TEXT("name"));Effect.ActualDescription=D::Text(Row,TEXT("description"));Vm.ActiveEffects.Add(Effect);}Gifts->SetGiftEffectData(Vm);
    }
}
void UInkRebuildFeatureHost::HandleMemberSelected(FString UserId){SelectedEntry=UserId;RefreshAuthority(true);}
void UInkRebuildFeatureHost::HandleMemberPage(int32 MemberCursor){Feedback(InkCore::RequestMembers(this,MemberCursor,IsActivePage()).Message);RefreshAuthority(true);}
void UInkRebuildFeatureHost::HandleMemberRemove(FString UserId)
{
    const auto State=Service()?Service()->Snapshot():nullptr;const auto Guild=D::Object(State,TEXT("guild"));bool Found=false;
    for(const auto& Row:D::Rows(State,TEXT("members")))Found|=D::String(Row,TEXT("user"))==UserId;
    if(UserId.IsEmpty()||UserId!=SelectedEntry||UserId==D::String(Guild,TEXT("anchor"))||!Found)return;
    auto Args=MakeShared<FJsonObject>();Args->SetStringField(TEXT("user_id"),UserId);Submit(TEXT("RemoveMember"),Args);RefreshAuthority(true);
}
void UInkRebuildFeatureHost::HandleBuildQuery(int32 Offset,bool OwnedOnly){QueryCatalog(TEXT("buildings"),Offset,OwnedOnly);}
void UInkRebuildFeatureHost::HandleBuild(FString BuildingId,int32 Level,FString Version)
{
    const auto State=Service()?Service()->Snapshot():nullptr;int32 CurrentTarget=0;bool Found=false;
    for(const auto& Row:D::Rows(D::Catalog(State,TEXT("buildings")),TEXT("rows")))if(D::String(Row,TEXT("building_id"))==BuildingId&&D::Integer(Row,TEXT("level"),CurrentTarget)&&CurrentTarget==Level){Found=true;break;}
    if(!Found||BuildingId.IsEmpty()||Level<=0||Version.IsEmpty()||D::Version(State)!=Version){Feedback(TEXT("建设目标或快照已变化，请重新确认"));return;}
    auto Args=MakeShared<FJsonObject>();Args->SetStringField(TEXT("building_id"),BuildingId);Args->SetNumberField(TEXT("target_level"),Level);Submit(TEXT("Build"),Args);RefreshAuthority(true);
}
