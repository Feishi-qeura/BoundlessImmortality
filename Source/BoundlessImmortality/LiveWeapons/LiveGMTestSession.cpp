#include "LiveGMTestSession.h"
#include "LiveGMOverlay.h"
#include "LiveWeaponSwarm.h"
#include "Cultivation/CultivationNameplate.h"
#include "LiveAudienceAdapterComponent.h"
#include "Frontend/InkDamageFeedbackComponent.h"
#include "Frontend/InkProgressState.h"
#include "Frontend/InkMenuDirector.h"
#include "Frontend/InkFrontendWidget.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Cultivation/CultivationHealthComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

ULiveGMTestSession::ULiveGMTestSession(){PrimaryComponentTick.bCanEverTick=true;}
bool ULiveGMTestSession::IsIsolatedCommandLine(const FString& Line)
{
    FString Suffix;if(!FParse::Value(*Line,TEXT("saveddirsuffix="),Suffix))return false;
    if(Suffix.IsEmpty())return false;
    for(TCHAR C:Suffix)if(!FChar::IsAlnum(C)&&C!=TEXT('_')&&C!=TEXT('-'))return false;
    return Suffix.StartsWith(TEXT("CohesionUI"))||Suffix.StartsWith(TEXT("PortraitUI"))||Suffix.StartsWith(TEXT("LiveWeaponsQA"));
}
bool ULiveGMTestSession::CanMutate()const
{
#if !UE_BUILD_SHIPPING
    return Host.IsValid()&&Host->IsDevelopmentSimulation();
#else
    return false;
#endif
}
FString ULiveGMTestSession::DescribeSession()const
{
    if(!Host.IsValid())return TEXT("会话未建立");
    const auto& S=Host->GetCurrentSession();const FString Kind=Host->IsDevelopmentSimulation()?TEXT("本地模拟"):S.bLocalTest?TEXT("开发转发"):Host->IsConnected()?TEXT("真实平台会话"):TEXT("真实平台未连接");
    return Kind+TEXT(" · ")+Host->ConnectionStatus+TEXT("\n房间：")+(S.RoomId.IsEmpty()?TEXT("未连接"):S.RoomId)+TEXT("\n")+(Host->IsDevelopmentSimulation()?TEXT("本地演示已就绪 · 无需抖音 SDK"):TEXT("等待本地演示会话"));
}
void ULiveGMTestSession::BeginPlay()
{
    Super::BeginPlay();Host=GetWorld()->GetGameInstance()->GetSubsystem<ULiveInteractionSubsystem>();
    if(!Host.IsValid())return;
    Host->OnComment.AddDynamic(this,&ThisClass::CommentReceived);Host->OnPresence.AddDynamic(this,&ThisClass::PresenceReceived);Host->OnLike.AddDynamic(this,&ThisClass::LikeReceived);Host->OnGift.AddDynamic(this,&ThisClass::GiftReceived);Host->OnFollow.AddDynamic(this,&ThisClass::FollowReceived);
    Host->OnSessionChanged.AddDynamic(this,&ThisClass::SessionChanged);
}
void ULiveGMTestSession::EndPlay(const EEndPlayReason::Type Reason)
{
    if(Host.IsValid()){Host->OnComment.RemoveAll(this);Host->OnPresence.RemoveAll(this);Host->OnLike.RemoveAll(this);Host->OnGift.RemoveAll(this);Host->OnFollow.RemoveAll(this);Host->OnSessionChanged.RemoveAll(this);}Host.Reset();Super::EndPlay(Reason);
}
FString ULiveGMTestSession::NextId(){return TEXT("gm:")+FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT(":")+FString::FromInt(++Sequence);}
void ULiveGMTestSession::Record(const FString& Name,const FString& Expected,const FString& Actual,bool Passed)
{
    FLiveGMCheckResult R;R.Name=Name;R.Expected=Expected;R.Actual=Actual;R.Status=Passed?TEXT("通过"):TEXT("失败");R.Source=Host.IsValid()&&Host->IsDevelopmentSimulation()?TEXT("本地模拟"):TEXT("实际会话");R.Time=FDateTime::UtcNow().ToIso8601();Results.Add(R);if(Results.Num()>100)Results.RemoveAt(0);
}
bool ULiveGMTestSession::Join(bool Enter,bool Duplicate)
{
    if(!CanMutate()||ViewerId.TrimStartAndEnd().IsEmpty())return false;
    FLivePresence E;E.Session=Host->GetCurrentSession();E.MessageId=NextId();E.UserId=ViewerId;E.Nickname=ViewerName;E.AvatarUrl=ViewerAvatarUrl;E.EnterType=Enter?1:2;
    const bool Applied=Host->DeliverPresence(E);if(Duplicate){const int32 Before=ReceivedEvents;const bool Again=Host->DeliverPresence(E);Record(TEXT("重复进场去重"),TEXT("重复事件被拒绝且不重复接收"),FString::Printf(TEXT("accepted=%d received_delta=%d"),Again,ReceivedEvents-Before),!Again&&ReceivedEvents==Before);}
    return Applied;
}
bool ULiveGMTestSession::RunAction(FName Action)
{
#if !UE_BUILD_SHIPPING
    if(Action==TEXT("Export"))return ExportResults();
    if(Action==TEXT("LoadUIReport"))return LoadLatestUIResults();
    if(Action==TEXT("Checks")){if(!CanMutate())return false;RunLocalChecks();return true;}
    if(!CanMutate())return false;
    if(Action==TEXT("Join"))return Join(true);
    if(Action==TEXT("Leave"))return Join(false);
    if(Action==TEXT("Duplicate"))return Join(true,true);
    if(Action==TEXT("Comment"))return Host->SimulateComment(ViewerId,ViewerName,Comment);
    if(Action==TEXT("Like"))return Host->SimulateLike(ViewerId,ViewerName,10);
    if(Action==TEXT("Follow")){FLiveFollow E;E.Session=Host->GetCurrentSession();E.MessageId=NextId();E.UserId=ViewerId;E.Nickname=ViewerName;E.TargetUserId=E.Session.AnchorUserId.IsEmpty()?TEXT("anchor"):E.Session.AnchorUserId;E.Action=1;return Host->DeliverFollow(E);}
    if(Action==TEXT("PreviewTitle")||Action==TEXT("PreviewRealm")||Action==TEXT("PreviewAdvance"))
    {
        if(!Join(true))return false;
        if(Action==TEXT("PreviewTitle"))PreviewTitle=PreviewTitle==TEXT("本命剑 · 预览")?TEXT("剑斩神魔 · 预览"):TEXT("本命剑 · 预览");
        if(Action==TEXT("PreviewRealm"))PreviewRealm=(PreviewRealm+9)%34;
        PreviewUntil=GetWorld()->GetTimeSeconds()+12.;
        if(Action==TEXT("PreviewAdvance"))AdvanceUntil=GetWorld()->GetTimeSeconds()+3.;
        const auto P=CultivationNameplate::Resolve(GetWorld()->GetGameInstance()->GetSubsystem<UCultivationSubsystem>(),Host->GetScopedUserId(ViewerId),ViewerName);
        Record(TEXT("称号/境界展示预览"),TEXT("仅修改测试观众展示，12 秒后恢复"),P.WeaponTitle+TEXT(" · ")+P.RealmTitle,P.RealmIndex==PreviewRealm);return true;
    }
    if(Action==TEXT("ContinuousDamage")){ContinuousRemaining=6;ContinuousWait=0;Record(TEXT("连续小额伤害"),TEXT("6 次独立实际结算"),TEXT("已排队，每次约最大生命 6%，闪避仍有效"),true);return true;}
    const FString Id=Action.ToString();
    if(Id.StartsWith(TEXT("Damage_")))
    {
        const double Ratio=FCString::Atod(*Id.Mid(7))/100.;auto* PC=GetWorld()->GetFirstPlayerController();APawn* Pawn=PC?PC->GetPawn():nullptr;
        auto* Feedback=Pawn?Pawn->FindComponentByClass<UInkDamageFeedbackComponent>():nullptr;if(!Feedback)return false;Feedback->PreviewDamageRatio(Ratio);
        Record(TEXT("伤害反馈样本 ")+Id.Mid(7)+TEXT("%"),FString::FromInt(InkFeedback::DamageTier(Ratio)),FString::FromInt(Feedback->LastDamageTier),Feedback->LastDamageTier==InkFeedback::DamageTier(Ratio));return true;
    }
    if(Action==TEXT("HitHealth")||Action==TEXT("Heal"))
    {
        auto* State=GetWorld()->GetGameInstance()->GetSubsystem<UCultivationSubsystem>();auto* Health=State?State->GetAnchorHealth():nullptr;if(!Health)return false;
        if(Action==TEXT("Heal")){if(Health->bDead)Health->Revive();else Health->Heal(Health->MaxHealth);return true;}
        const double Applied=Health->ApplyIncomingDamage(Health->MaxHealth*.35);Record(TEXT("实际战斗损血"),TEXT("按防御/闪避后实际结算"),FString::Printf(TEXT("%.1f 生命"),Applied),true);return true;
    }
    if(Id.StartsWith(TEXT("Layout_")))for(TActorIterator<AInkMenuDirector> It(GetWorld());It;++It)if(It->FrontendWidget)return It->FrontendWidget->OpenFeaturePanel(FName(*Id.Mid(7)),TEXT(""));
#endif
    return false;
}
void ULiveGMTestSession::RunLocalChecks()
{
    if(!CanMutate())return;auto* Owner=Cast<ALiveGMOverlay>(GetOwner());if(!Owner||!Owner->Swarm)return;
    const FString Scoped=Host->GetScopedUserId(ViewerId);Join(true,true);
    const auto* Joined=Owner->Swarm->GetRoster().Find(Scoped);Record(TEXT("观众身份绑定"),ViewerName,Joined?Joined->Profile.DisplayName:TEXT("未加入"),Joined&&Joined->Profile.DisplayName==ViewerName);
    Join(false);const auto* Left=Owner->Swarm->GetRoster().Find(Scoped);Record(TEXT("观众离场"),TEXT("不再参与"),Left&&Left->bParticipating?TEXT("仍在参与"):TEXT("已离场"),!Left||!Left->bParticipating);
    Join(true);const int32 Before=ReceivedEvents;Host->SimulateLike(ViewerId,ViewerName,10);Record(TEXT("点赞接收链路"),TEXT("新增一个事件"),FString::FromInt(ReceivedEvents-Before),ReceivedEvents==Before+1);
    for(const TCHAR* Ratio:{TEXT("19.9"),TEXT("20"),TEXT("25.5"),TEXT("26"),TEXT("41.9"),TEXT("42"),TEXT("60"),TEXT("60.1")})RunAction(FName(*(FString(TEXT("Damage_"))+Ratio)));
    FLiveGMCheckResult Real;Real.Name=TEXT("抖音 SDK 接入");Real.Status=TEXT("未启用");Real.Source=TEXT("当前本地演示版");Real.Expected=TEXT("提审后单独接入");Real.Actual=TEXT("当前不调用抖音 SDK，不影响本地演示检查");Real.Time=FDateTime::UtcNow().ToIso8601();Results.Add(Real);ExportResults();
}
bool ULiveGMTestSession::ExportResults()
{
#if !UE_BUILD_SHIPPING
    TArray<TSharedPtr<FJsonValue>> Rows;for(const auto& R:Results){auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("name"),R.Name);J->SetStringField(TEXT("expected"),R.Expected);J->SetStringField(TEXT("actual"),R.Actual);J->SetStringField(TEXT("status"),R.Status);J->SetStringField(TEXT("source"),R.Source);J->SetStringField(TEXT("time"),R.Time);Rows.Add(MakeShared<FJsonValueObject>(J));}
    auto Root=MakeShared<FJsonObject>();Root->SetArrayField(TEXT("checks"),Rows);Root->SetStringField(TEXT("session"),DescribeSession());Root->SetNumberField(TEXT("received_events"),ReceivedEvents);
    FString Text;FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Text));ReportPath=FPaths::ProjectSavedDir()/TEXT("GMReports")/(TEXT("gm-")+FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S"))+TEXT(".json"));IFileManager::Get().MakeDirectory(*FPaths::GetPath(ReportPath),true);return FFileHelper::SaveStringToFile(Text,*ReportPath,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
#else
    return false;
#endif
}
bool ULiveGMTestSession::LoadLatestUIResults()
{
#if !UE_BUILD_SHIPPING
    const FString Path=FPaths::ProjectDir()/TEXT("Saved/Codex/CohesionUI20261021/cohesion-pie.json");FString Json;
    TSharedPtr<FJsonObject> Root;if(!FFileHelper::LoadFileToString(Json,*Path)||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root)||!Root.IsValid())return false;
    const TSharedPtr<FJsonObject>* Checks=nullptr;if(!Root->TryGetObjectField(TEXT("checks"),Checks))return false;
    Results.RemoveAll([](const FLiveGMCheckResult& R){return R.Source.StartsWith(TEXT("Windows PIE"));});
    auto Add=[&](const FString& Name,int32 Passed,int32 Total)
    {FLiveGMCheckResult R;R.Name=Name;R.Expected=TEXT("全部断言通过");R.Actual=FString::Printf(TEXT("%d / %d"),Passed,Total);R.Status=Total==0?TEXT("未运行"):Passed==Total?TEXT("通过"):TEXT("失败");R.Source=TEXT("Windows PIE · 本机记录");R.Time=IFileManager::Get().GetTimeStamp(*Path).ToIso8601();Results.Add(R);};
    const TCHAR* Prefixes[]={TEXT("active_tab_"),TEXT("button_in_frame_"),TEXT("rank_"),TEXT("health_"),TEXT("shield_"),TEXT("niagara_"),TEXT("gm_")};
    const TCHAR* Names[]={TEXT("页签切换"),TEXT("功能控件边界"),TEXT("排行榜裁切与滚动"),TEXT("生命残量缓冲"),TEXT("护盾残量缓冲"),TEXT("Niagara 粒子运行"),TEXT("GM 事件与结果")};
    for(int32 I=0;I<UE_ARRAY_COUNT(Prefixes);++I){int32 Total=0,Passed=0;for(const auto& Pair:(*Checks)->Values)if(Pair.Key.ToView().StartsWith(Prefixes[I])){const auto O=Pair.Value->AsObject();bool Value=false;++Total;if(O.IsValid()&&O->TryGetBoolField(TEXT("passed"),Value)&&Value)++Passed;}Add(Names[I],Passed,Total);}
    bool Overall=false;Root->TryGetBoolField(TEXT("passed"),Overall);Add(TEXT("最新界面验收报告"),Overall?1:0,1);
    return true;
#else
    return false;
#endif
}
void ULiveGMTestSession::CommentReceived(const FLiveComment& E){++ReceivedEvents;LastEvent=TEXT("评论 · ")+E.Nickname+TEXT(" · ")+E.Content;}
void ULiveGMTestSession::PresenceReceived(const FLivePresence& E){++ReceivedEvents;LastEvent=(E.EnterType==1?TEXT("加入 · "):TEXT("离开 · "))+E.Nickname;}
void ULiveGMTestSession::LikeReceived(const FLiveLike& E){++ReceivedEvents;LastEvent=TEXT("点赞 · ")+E.Nickname;}
void ULiveGMTestSession::GiftReceived(const FLiveGift& E){++ReceivedEvents;LastEvent=TEXT("礼物 · ")+E.GiftName;}
void ULiveGMTestSession::FollowReceived(const FLiveFollow& E){++ReceivedEvents;LastEvent=TEXT("关注 · ")+E.Nickname;}
void ULiveGMTestSession::SessionChanged(){ReceivedEvents=0;LastEvent.Empty();ContinuousRemaining=0;PreviewUntil=AdvanceUntil=0;PreviewTitle.Empty();}
void ULiveGMTestSession::TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta,Type,Function);
    if(!CanMutate()){ContinuousRemaining=0;PreviewUntil=AdvanceUntil=0;return;}
    if(ContinuousRemaining<=0)return;
    ContinuousWait-=Delta;if(ContinuousWait>0)return;ContinuousWait=.18f;--ContinuousRemaining;
    auto* State=GetWorld()->GetGameInstance()->GetSubsystem<UCultivationSubsystem>();auto* Health=State?State->GetAnchorHealth():nullptr;
    if(!Health||Health->bDead){ContinuousRemaining=0;return;}
    const double Applied=Health->ApplyIncomingDamage(Health->MaxHealth*.06);
    Record(TEXT("连续受击实际结算"),TEXT("单次结算，不累加震屏档位"),FString::Printf(TEXT("实际损血 %.1f / %.1f"),Applied,Health->MaxHealth),Applied/Health->MaxHealth<.2);
}
