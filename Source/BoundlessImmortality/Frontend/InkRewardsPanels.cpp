#include "InkFeaturePanel.h"
#include "WulingFrontendData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "InkFrontendWidget.h"

using ESurface=InkPresentationStyle::ESurface;

namespace
{
TArray<TSharedPtr<FJsonObject>> ActivityTemplatesForCycle(const TSharedPtr<FJsonObject>& Activities,const TCHAR* Cycle)
{
    TArray<TSharedPtr<FJsonObject>> Matches;
    const TArray<TSharedPtr<FJsonValue>>* Templates=nullptr;
    if(!Activities||!Activities->TryGetArrayField(TEXT("templates"),Templates)||!Templates)return Matches;
    for(const auto& Value:*Templates)
    {
        const auto Row=Value.IsValid()?Value->AsObject():nullptr;
        if(Row&&WulingFrontendData::String(Row,TEXT("cycle"))==Cycle)Matches.Add(Row);
    }
    return Matches;
}

FString ActivityTemplateField(const TSharedPtr<FJsonObject>& Row,const TCHAR* Key)
{
    FString Value;
    return Row&&Row->TryGetStringField(Key,Value)&&!Value.IsEmpty()?Value:TEXT("待同步");
}

FString ActivityTemplateEnabled(const TSharedPtr<FJsonObject>& Row)
{
    bool Enabled=false;
    if(!Row||!Row->TryGetBoolField(TEXT("enabled"),Enabled))return TEXT("启用状态待同步");
    return Enabled?TEXT("已启用"):TEXT("当前未启用");
}

FString ActivityCycleDisplay(const TSharedPtr<FJsonObject>& Row)
{
    const FString Cycle=WulingFrontendData::String(Row,TEXT("cycle"));
    if(Cycle==TEXT("daily"))return TEXT("日常");
    if(Cycle==TEXT("limited"))return TEXT("限时");
    if(Cycle==TEXT("permanent"))return TEXT("常驻");
    return TEXT("待同步");
}

FString ActivityAuthorityState(const UWulingPrototypeSubsystem* Service,const TSharedPtr<FJsonObject>& Activities)
{
    if(!Service||!Service->IsEnabled())return TEXT("活动配置尚未同步，请连接活动服务后查看。");
    if(!Service->IsConnected())return TEXT("活动服务未连接，活动配置尚未同步。");
    if(!Activities)return TEXT("活动资料正在同步。");
    const TArray<TSharedPtr<FJsonValue>>* Templates=nullptr;
    if(!Activities->TryGetArrayField(TEXT("templates"),Templates)||!Templates)return TEXT("服务暂未提供活动配置。");
    return FString();
}

bool HasActivityTemplateList(const TSharedPtr<FJsonObject>& Activities)
{
    const TArray<TSharedPtr<FJsonValue>>* Templates=nullptr;
    return Activities&&Activities->TryGetArrayField(TEXT("templates"),Templates)&&Templates;
}
}

void UInkDailyFortunePanel::BuildPanelContent()
{
    const auto A=WulingFrontendData::Activity(this);
    const bool Wide=!bPortraitLayout&&BodyWidth>=800;
    const float ArtW=Wide?BodyWidth*.38f:BodyWidth,ArtH=368.f;
    Scene(BodyCanvas,1,0,0,ArtW,ArtH);
    const FString AwaitingAuthority=TEXT("服务未提供/待同步");
    const FString Verse=WulingFrontendData::String(A,TEXT("verse"));
    bool Signed=false;const bool HasSigned=A&&A->TryGetBoolField(TEXT("signed"),Signed);
    const FString SignStatus=HasSigned?(Signed?TEXT("今日已签到"):TEXT("今日未签到")):FString(TEXT("签到状态："))+AwaitingAuthority;
    TArray<FString> Lines;Verse.Replace(TEXT("，"),TEXT("\n")).ParseIntoArrayLines(Lines,true);
    const float SealW=220,SealX=(ArtW-SealW)*.5f;
    auto* VerseSlip=Surface(BodyCanvas,TEXT("FortuneVerseSlip"),ESurface::Card,SealX,0,SealW,352);
    CastChecked<UCanvasPanelSlot>(VerseSlip->Slot)->SetZOrder(2);
    if(Lines.Num()==2&&Lines[0].Len()<=7&&Lines[1].Len()<=7)
    {
        for(int32 I=0;I<2;++I){FString Vertical;for(int32 C=0;C<Lines[I].Len();++C){if(C)Vertical+=TEXT("\n");Vertical.AppendChar(Lines[I][C]);}Label(BodyCanvas,FName(*FString::Printf(TEXT("FortuneVerticalVerse_%d"),I)),Vertical,SealX+24+(1-I)*96,16,76,320,40,true);}
    }
    else Label(BodyCanvas,TEXT("FortuneVerticalVerse"),TEXT("每\n日\n剑\n诀"),SealX+60,32,100,288,40,true);
    const float X=Wide?ArtW+32:0,Y=Wide?0:ArtH+16,W=BodyWidth-X;
    const FString Day=WulingFrontendData::String(A,TEXT("day"));
    Label(BodyCanvas,TEXT("FortuneHeading"),FString(TEXT("服务日期："))+(Day.IsEmpty()?AwaitingAuthority:Day)+TEXT(" · 每日剑诀"),X,Y,W,64,30);
    Label(BodyCanvas,TEXT("FortuneVerse"),Verse.IsEmpty()?FString(TEXT("剑诀内容："))+AwaitingAuthority:Verse,X+24,Y+88,W-48,180,38);
    Label(BodyCanvas,TEXT("FortuneResult"),FString(TEXT("每日一签/吉凶结果："))+AwaitingAuthority,X,Y+280,W,72,24);
    Label(BodyCanvas,TEXT("FortuneDescription"),SignStatus+TEXT("\n奖励：")+AwaitingAuthority,X,Y+364,W,104,24);
}
void UInkDailyFortunePanel::HandlePanelAction(const FString& Id)
{Super::HandlePanelAction(Id);}

void UInkActivitiesPanel::BuildPanelContent()
{
	const auto Activities=WulingFrontendData::Activity(this);
	const auto* Service=WulingFrontendData::Service(this);
	const FString State=ActivityAuthorityState(Service,Activities);
	Surface(BodyCanvas,TEXT("ActivityCenterHeadingPaper"),ESurface::FeatureHeading,0,0,BodyWidth,62);
	Label(BodyCanvas,TEXT("ActivityCenterHeading"),TEXT("活动分类"),16,8,BodyWidth-32,44,32);
	Surface(BodyCanvas,TEXT("ActivityCenterAuthorityPaper"),ESurface::FeatureCard,0,72,BodyWidth,84);
	Label(BodyCanvas,TEXT("ActivityCenterAuthority"),State.IsEmpty()?TEXT("活动状态由服务端快照确认；未提供的进度、奖励和领取状态保持空态。"):State,16,80,BodyWidth-32,68,20);
	const float CardH=136.f,Gap=12.f;
	const float CardW=(BodyWidth-Gap)*.5f;
	const struct FActivityEntry { const TCHAR* Route; const TCHAR* Title; const TCHAR* Caption; } Entries[]={
		{TEXT("DailyActivities"),TEXT("日常活动"),TEXT("活动目标和进度待同步")},
		{TEXT("TimedActivities"),TEXT("限时活动"),TEXT("开放时间与进度待同步")},
		{TEXT("PermanentActivities"),TEXT("常驻活动"),TEXT("仅显示明确标为 permanent 的条目")},
		{TEXT("LaunchEvent"),TEXT("开服活动"),TEXT("状态、任务与截止时间以服务记录为准")}};
	for(int32 I=0;I<UE_ARRAY_COUNT(Entries);++I)
	{
		const float X=(I%2)*(CardW+Gap),Y=172.f+(I/2)*(CardH+Gap);
		Surface(BodyCanvas,NAME_None,ESurface::FeatureCard,X,Y,CardW,CardH);
		Surface(BodyCanvas,NAME_None,ESurface::FeatureHeading,X+8,Y+8,CardW-16,48);
		Label(BodyCanvas,NAME_None,Entries[I].Title,X+16,Y+10,CardW-32,42,27);
		Label(BodyCanvas,NAME_None,Entries[I].Caption,X+16,Y+60,CardW-32,64,19);
		Action(BodyCanvas,FString(TEXT("Open_"))+Entries[I].Route,TEXT("查看"),X+CardW-112,Y+88,96,40,true);
	}
}
void UInkDailyActivitiesPanel::BuildPanelContent()
{
    const bool Wide=!bPortraitLayout&&BodyWidth>=800;
    auto* Service=WulingFrontendData::Service(this);
    const auto Activities=WulingFrontendData::Activity(this);
    const FString AuthorityState=ActivityAuthorityState(Service,Activities);
    const auto Daily=ActivityTemplatesForCycle(Activities,TEXT("daily"));
    Surface(BodyCanvas,TEXT("DailyHeadingPaper"),ESurface::FeatureHeading,0,0,BodyWidth,54);
    Label(BodyCanvas,TEXT("DailyHeading"),TEXT("日常活动"),12,4,BodyWidth-24,46,32);
    Surface(BodyCanvas,TEXT("DailyAuthorityPaper"),ESurface::FeatureCard,0,58,BodyWidth,92);
    if(!AuthorityState.IsEmpty())
        Label(BodyCanvas,TEXT("DailyAuthorityStatus"),AuthorityState,16,68,BodyWidth-32,72,21);
    else
        Label(BodyCanvas,TEXT("DailyClassificationNote"),TEXT("仅显示 cycle=daily；event 与 relative_guild_creation 不归入日常或限时。"),16,68,BodyWidth-32,72,18);
    float Y=166;
    if(HasActivityTemplateList(Activities))
    {
        if(Daily.IsEmpty())
        {
            Surface(BodyCanvas,TEXT("DailyTemplateEmptyPaper"),ESurface::FeatureCard,0,Y,BodyWidth,76);
            Label(BodyCanvas,TEXT("DailyTemplateEmpty"),TEXT("暂无日常活动配置。"),16,Y+16,BodyWidth-32,44,23);
        }
        else
        {
            const int32 Columns=Wide?2:1;const float CardW=(BodyWidth-(Columns-1)*20)/Columns,CardH=216;
            for(int32 I=0;I<Daily.Num();++I)
            {
                const auto& Row=Daily[I];const FString Id=WulingFrontendData::String(Row,TEXT("activity_id"));
                const float X=(I%Columns)*(CardW+20),CardY=Y+(I/Columns)*(CardH+16);
                Surface(BodyCanvas,NAME_None,ESurface::FeatureCard,X,CardY,CardW,CardH);
                Surface(BodyCanvas,NAME_None,ESurface::FeatureHeading,X+8,CardY+8,CardW-16,48);
                Label(BodyCanvas,NAME_None,ActivityTemplateField(Row,TEXT("name")),X+16,CardY+12,CardW-32,40,27);
                Label(BodyCanvas,NAME_None,FString(TEXT("编号："))+Id+TEXT(" · ")+ActivityTemplateEnabled(Row),X+16,CardY+64,CardW-32,34,19);
                Label(BodyCanvas,NAME_None,FString(TEXT("周期："))+ActivityCycleDisplay(Row)+TEXT(" · 开放：")+ActivityTemplateField(Row,TEXT("start_at")),X+16,CardY+102,CardW-32,32,18);
                Action(BodyCanvas,TEXT("Open_DailyActivityDetail|")+Id,TEXT("查看配置详情"),X+16,CardY+148,CardW-32,64,true);
            }
            Y+=FMath::DivideAndRoundUp(Daily.Num(),Columns)*(CardH+16);
        }
    }
    if(Service&&Service->IsEnabled())
    {
        const float OpsY=Y+72;
        Surface(BodyCanvas,TEXT("DailySharedOperationPaper"),ESurface::FeatureCard,0,OpsY,BodyWidth,224);
        Label(BodyCanvas,TEXT("SharedOperationHeading"),TEXT("武灵共修"),0,OpsY,BodyWidth,48,25);
        Label(BodyCanvas,TEXT("SharedOperationStatus"),TEXT("服务进度以实际记录为准。"),0,OpsY+52,BodyWidth,62,19);
        const float ButtonW=(BodyWidth-16)*.5f;
        Action(BodyCanvas,TEXT("BeginShared"),TEXT("开始共修"),0,OpsY+126,ButtonW,72,true)->SetIsEnabled(Service->IsConnected()&&!Service->IsBusy());
        Action(BodyCanvas,TEXT("EndShared"),TEXT("结束共修"),ButtonW+16,OpsY+126,ButtonW,72)->SetIsEnabled(Service->IsConnected()&&!Service->IsBusy());
    }
}
void UInkTimedActivitiesPanel::BuildPanelContent()
{
    const bool Wide=!bPortraitLayout&&BodyWidth>=800;
    auto* Service=WulingFrontendData::Service(this);
    const auto Activities=WulingFrontendData::Activity(this);
    const FString AuthorityState=ActivityAuthorityState(Service,Activities);
    const auto Limited=ActivityTemplatesForCycle(Activities,TEXT("limited"));
    Surface(BodyCanvas,TEXT("TimedHeadingPaper"),ESurface::FeatureHeading,0,0,BodyWidth,54);
    Label(BodyCanvas,TEXT("TimedHeading"),TEXT("限时活动"),12,4,BodyWidth-24,46,32);
    Surface(BodyCanvas,TEXT("TimedAuthorityPaper"),ESurface::FeatureCard,0,58,BodyWidth,92);
    if(!AuthorityState.IsEmpty())Label(BodyCanvas,TEXT("TimedAuthorityStatus"),AuthorityState,16,68,BodyWidth-32,72,21);
    else Label(BodyCanvas,TEXT("TimedClassificationNote"),TEXT("仅显示 cycle=limited；event 与 relative_guild_creation 不归入限时。"),16,68,BodyWidth-32,72,18);
    float Y=166;
    if(HasActivityTemplateList(Activities))
    {
        if(Limited.IsEmpty())
        {
            Surface(BodyCanvas,TEXT("TimedTemplateEmptyPaper"),ESurface::FeatureCard,0,Y,BodyWidth,76);
            Label(BodyCanvas,TEXT("TimedTemplateEmpty"),TEXT("暂无限时活动配置。"),16,Y+16,BodyWidth-32,44,23);
        }
        else
        {
            const int32 Columns=Wide?2:1;const float CardW=(BodyWidth-(Columns-1)*20)/Columns,CardH=216;
            for(int32 I=0;I<Limited.Num();++I)
            {
                const auto& Row=Limited[I];const FString Id=WulingFrontendData::String(Row,TEXT("activity_id"));
                const float X=(I%Columns)*(CardW+20),CardY=Y+(I/Columns)*(CardH+16);
                Surface(BodyCanvas,NAME_None,ESurface::FeatureCard,X,CardY,CardW,CardH);
                Surface(BodyCanvas,NAME_None,ESurface::FeatureHeading,X+8,CardY+8,CardW-16,48);
                Label(BodyCanvas,NAME_None,ActivityTemplateField(Row,TEXT("name")),X+16,CardY+12,CardW-32,40,27);
                Label(BodyCanvas,NAME_None,FString(TEXT("编号："))+Id+TEXT(" · ")+ActivityTemplateEnabled(Row),X+16,CardY+64,CardW-32,34,19);
                Label(BodyCanvas,NAME_None,FString(TEXT("开始："))+ActivityTemplateField(Row,TEXT("start_at"))+TEXT(" · 截止：")+ActivityTemplateField(Row,TEXT("end_at")),X+16,CardY+102,CardW-32,32,18);
                Action(BodyCanvas,TEXT("Open_TimedActivityDetail|")+Id,TEXT("查看配置详情"),X+16,CardY+148,CardW-32,64,true);
            }
            Y+=FMath::DivideAndRoundUp(Limited.Num(),Columns)*(CardH+16);
        }
    }
    if(Service&&Service->IsEnabled())
    {
        const float OpsY=Y+72;
        Surface(BodyCanvas,TEXT("TimedPrototypeOperationPaper"),ESurface::FeatureCard,0,OpsY,BodyWidth,224);
        Label(BodyCanvas,TEXT("PrototypeOperationHeading"),TEXT("武灵共修"),0,OpsY,BodyWidth,48,25);
        Label(BodyCanvas,TEXT("PrototypeOperationStatus"),TEXT("共修状态以服务记录为准。"),0,OpsY+52,BodyWidth,62,19);
        const float ButtonW=(BodyWidth-16)*.5f;
        Action(BodyCanvas,TEXT("WeekGuard"),TEXT("云台护阵 · 开始"),0,OpsY+126,ButtonW,72,true)->SetIsEnabled(Service->IsConnected()&&!Service->IsBusy());
        Action(BodyCanvas,TEXT("WeekIron"),TEXT("玄铁采集 · 开始"),ButtonW+16,OpsY+126,ButtonW,72,true)->SetIsEnabled(Service->IsConnected()&&!Service->IsBusy());
    }
}
