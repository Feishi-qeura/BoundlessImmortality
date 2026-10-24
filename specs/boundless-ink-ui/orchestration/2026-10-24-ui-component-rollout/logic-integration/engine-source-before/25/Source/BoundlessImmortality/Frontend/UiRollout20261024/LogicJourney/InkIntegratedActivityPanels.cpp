#include "InkIntegratedActivityPanels.h"
#include "InkJourneyAdapter.h"
#include "Frontend/InkFrontendWidget.h"
#include "Frontend/WulingFrontendData.h"
#include "Serialization/JsonSerializer.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

namespace
{
FString IntegratedActivityValue(const InkJourneyAdapter::FObject& Row,const TCHAR* Key)
{
    const auto* V=Row?Row->Values.Find(Key):nullptr;
    if(!V||!V->IsValid())return TEXT("待同步");
    if((*V)->Type==EJson::String)return (*V)->AsString().IsEmpty()?FString(TEXT("待同步")):(*V)->AsString();
    if((*V)->Type==EJson::Boolean)return (*V)->AsBool()?TEXT("是"):TEXT("否");
    if((*V)->Type==EJson::Number&&FMath::IsFinite((*V)->AsNumber()))return FString::SanitizeFloat((*V)->AsNumber());
    return TEXT("待同步");
}
FString IntegratedActivityKey(const UUserWidget* Widget)
{
    auto* S=WulingFrontendData::Service(Widget);FString Key;
    if(S&&S->Snapshot())
    {
        const auto A=WulingFrontendData::Object(S->Snapshot(),TEXT("activities"));
        if(A)FJsonSerializer::Serialize(A.ToSharedRef(),TJsonWriterFactory<>::Create(&Key));
    }
    return Key+(S?FString::Printf(TEXT("|%d|%d|%d|%s"),S->IsEnabled(),S->IsConnected(),S->IsBusy(),*S->GetStatus()):FString(TEXT("disabled")));
}
}

void UInkIntegratedActivitiesPanel::HandlePanelAction(const FString& Id)
{
    if(Id==TEXT("ActivityCycle_daily")){Open(TEXT("DailyActivities"));return;}
    if(Id==TEXT("ActivityCycle_limited")){Open(TEXT("TimedActivities"));return;}
    if(Id==TEXT("ActivityCycle_permanent")){Open(TEXT("PermanentActivities"));return;}
    Super::HandlePanelAction(Id);
}
void UInkIntegratedActivitiesPanel::RefreshLiveData()
{
    if(!IsRolloutPageActive())return;
    const FString Key=IntegratedActivityKey(this);
    if(Key!=RolloutActivityKey){RolloutActivityKey=Key;RebuildPanel();}
}
void UInkIntegratedActivitiesPanel::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{Super::NativeTick(Geometry,DeltaSeconds);RolloutPollElapsed+=DeltaSeconds;if(RolloutPollElapsed>=.25f){RolloutPollElapsed=0;RefreshLiveData();}}
void UInkIntegratedCyclePanel::NativeTick(const FGeometry& Geometry,float DeltaSeconds)
{Super::NativeTick(Geometry,DeltaSeconds);RolloutPollElapsed+=DeltaSeconds;if(RolloutPollElapsed>=.25f){RolloutPollElapsed=0;RefreshLiveData();}}
void UInkIntegratedCyclePanel::RefreshLiveData()
{
    if(!IsRolloutPageActive())return;
    const FString Key=IntegratedActivityKey(this);
    if(Key!=RolloutActivityKey){RolloutActivityKey=Key;RebuildPanel();}
}
void UInkIntegratedCyclePanel::BuildPanelContent()
{
    auto* S=WulingFrontendData::Service(this);
    const auto Context=InkJourneyAdapter::Capture(S,IsRolloutPageActive());
    const auto View=InkJourneyAdapter::Read(Context);
    const auto Rows=InkJourneyAdapter::ActivitiesForCycle(View,ActivityCycle());
    const FString State=View.State==InkJourneyAdapter::EState::Disabled?TEXT("活动服务未启用"):
        View.State==InkJourneyAdapter::EState::Offline?TEXT("服务未连接 · 以下为上次确认数据"):
        View.State==InkJourneyAdapter::EState::Loading?TEXT("活动数据待同步"):
        View.State==InkJourneyAdapter::EState::Pending?TEXT("请求处理中 · 等待服务确认"):
        !Rows.bProvided?TEXT("活动分类数据待同步或格式无效"):Rows.Items.IsEmpty()?TEXT("本分类暂无活动"):TEXT("服务已提供活动配置");
    Surface(BodyCanvas,TEXT("IntegratedCycleHeading"),InkPresentationStyle::ESurface::FeatureHeading,0,0,BodyWidth,64);
    Label(BodyCanvas,TEXT("IntegratedCycleState"),State,16,10,BodyWidth-32,48,25);
    Label(BodyCanvas,TEXT("IntegratedCycleServiceStatus"),S?S->GetStatus():FString(TEXT("服务未启用")),0,80,BodyWidth,72,22);
    float Y=176;
    for(int32 Index=0;Index<Rows.Items.Num();++Index)
    {
        const auto& Row=Rows.Items[Index];
        const FString Prefix=FString::Printf(TEXT("IntegratedActivity_%d_"),Index);
        Surface(BodyCanvas,NAME_None,InkPresentationStyle::ESurface::FeatureCard,0,Y,BodyWidth,640);
        Label(BodyCanvas,FName(*(Prefix+TEXT("Name"))),IntegratedActivityValue(Row,TEXT("name")),16,Y+12,BodyWidth-32,56,30);
        const TCHAR* Keys[]={TEXT("enabled"),TEXT("status"),TEXT("participation_eligible"),TEXT("claim_eligible"),TEXT("objective"),TEXT("progress"),TEXT("target"),TEXT("reward_summary"),TEXT("source"),TEXT("start_at"),TEXT("end_at")};
        const TCHAR* Names[]={TEXT("已启用"),TEXT("状态"),TEXT("参与资格"),TEXT("领取资格"),TEXT("目标"),TEXT("当前进度"),TEXT("目标进度"),TEXT("奖励摘要"),TEXT("来源"),TEXT("开始时间"),TEXT("结束时间")};
        for(int32 Field=0;Field<UE_ARRAY_COUNT(Keys);++Field)
            Label(BodyCanvas,FName(*(Prefix+Keys[Field])),FString(Names[Field])+TEXT("：")+IntegratedActivityValue(Row,Keys[Field]),16,Y+80+Field*38,BodyWidth-32,36,20);
        const FString Id=WulingFrontendData::String(Row,TEXT("activity_id"));
        const FString Cycle(ActivityCycle());
        const FString Route=Id==TEXT("ACT_SIGN")?TEXT("SignIn"):Id==TEXT("ACT_VERSE")?TEXT("DailyFortune"):
            Cycle==TEXT("daily")?TEXT("DailyActivityDetail"):Cycle==TEXT("limited")?TEXT("TimedActivityDetail"):TEXT("");
        auto* Button=Action(BodyCanvas,Route.IsEmpty()?TEXT("PermanentUnavailable"):TEXT("Open_")+Route+TEXT("|")+Id,
            Route.IsEmpty()?TEXT("暂无已确认的业务操作"):TEXT("查看详情"),16,Y+528,BodyWidth-32,88);
        Button->SetIsEnabled(!Route.IsEmpty()&&!Id.IsEmpty());
        Y+=660;
    }
    RolloutActivityKey=IntegratedActivityKey(this);
}

void UInkIntegratedLaunchCountdown::NativeConstruct()
{
    Super::NativeConstruct();
    if(auto* Button=Cast<UButton>(GetWidgetFromName(TEXT("LaunchCountdownHitTarget"))))
    {
        FSlateBrush Clear;Clear.DrawAs=ESlateBrushDrawType::NoDrawType;
        FButtonStyle Style;Style.SetNormal(Clear).SetHovered(Clear).SetPressed(Clear).SetDisabled(Clear);Button->SetStyle(Style);
    }
    if(auto* Image=Cast<UImage>(GetWidgetFromName(TEXT("LaunchCountdownFrame"))))
    {
        auto Brush=Image->GetBrush();
        if(auto* Texture=Cast<UTexture2D>(Brush.GetResourceObject()))
        {
            Brush.DrawAs=ESlateBrushDrawType::Box;
            Brush.Margin=FMargin(140.f/Texture->GetSizeX(),90.f/Texture->GetSizeY(),140.f/Texture->GetSizeX(),90.f/Texture->GetSizeY());
            Image->SetBrush(Brush);
        }
    }
}
