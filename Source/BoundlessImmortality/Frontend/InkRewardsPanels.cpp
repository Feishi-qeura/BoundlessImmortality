#include "InkFeaturePanel.h"
#include "WulingFrontendData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "InkFrontendWidget.h"

using ESurface=InkPresentationStyle::ESurface;

void UInkDailyFortunePanel::BuildPanelContent()
{
    const auto A=WulingFrontendData::Activity(this);
    const bool Wide=!bPortraitLayout&&BodyWidth>=800;
    const float ArtW=Wide?BodyWidth*.38f:BodyWidth,ArtH=368.f;
    Scene(BodyCanvas,1,0,0,ArtW,ArtH);
    const FString Verse=A?WulingFrontendData::String(A,TEXT("verse")):(bFortuneDrawn?TEXT("云开月明\n静心自有前程"):TEXT("一念清明\n问道今日仙缘"));
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
    Label(BodyCanvas,TEXT("FortuneHeading"),A?WulingFrontendData::String(A,TEXT("day"))+TEXT(" · 今日剑诀"):TEXT("每日剑诀 · 开发预览"),X,Y,W,64,32);
    Label(BodyCanvas,TEXT("FortuneVerse"),Verse,X+24,Y+88,W-48,180,40);
    Label(BodyCanvas,TEXT("FortuneDescription"),A?TEXT("今日剑诀已由服务确认\n静心读诀，无数值奖励。"):TEXT("固定剑诀预览\n日期和正式内容等待服务同步，无数值奖励。"),X,Y+300,W,112,26);
    if(!A)Action(BodyCanvas,TEXT("Demo_Fortune"),bFortuneDrawn?TEXT("再读剑诀"):TEXT("查看剑诀"),X,Y+436,W,88,true);
}
void UInkDailyFortunePanel::HandlePanelAction(const FString& Id)
{if(Id==TEXT("Demo_Fortune")){bFortuneDrawn=true;RebuildPanel();FeedbackMessage(TEXT("固定剑诀预览 · 无数值奖励"));return;}Super::HandlePanelAction(Id);}

void UInkActivitiesPanel::BuildPanelContent()
{
    Link(BodyCanvas,TEXT("DailyActivities"),TEXT("日常活动"),TEXT("历练与奖励 · 查看当前可用行动"),2,0,0,BodyWidth,176);
    Link(BodyCanvas,TEXT("TimedActivities"),TEXT("限时活动"),TEXT("周期与开放条件以实际配置为准"),7,0,192,BodyWidth,176);
}
void UInkDailyActivitiesPanel::BuildPanelContent()
{
    const bool Wide=!bPortraitLayout&&BodyWidth>=800;
    auto* Service=WulingFrontendData::Service(this);
    if(Service&&Service->IsEnabled())
    {
        const float SummaryW=Wide?BodyWidth*.36f:BodyWidth,X=Wide?SummaryW+28:0,Y=Wide?0:260,W=BodyWidth-X;
        Label(BodyCanvas,TEXT("DailyHeading"),TEXT("今日共修"),0,0,SummaryW,60,40);
        Label(BodyCanvas,TEXT("DailyAuthorityStatus"),Service->GetStatus()+TEXT("\n共修只结算实际在线修为，不另发额外数值。"),0,76,SummaryW,156,26);
        Link(BodyCanvas,TEXT("Wuling"),TEXT("世界历练"),TEXT("三阶段产出 · 实际剑灵参与"),4,X,Y,W,176);
        const float ButtonW=(W-16)*.5f;
        Action(BodyCanvas,TEXT("BeginShared"),TEXT("开始共修"),X,Y+196,ButtonW,88,true)->SetIsEnabled(!Service->IsBusy());
        Action(BodyCanvas,TEXT("EndShared"),TEXT("结束共修"),X+ButtonW+16,Y+196,ButtonW,88)->SetIsEnabled(!Service->IsBusy());
        Link(BodyCanvas,TEXT("WulingActivity"),TEXT("青云初启"),TEXT("七项任务 · 共同进度与奖励状态"),0,X,Y+312,W,176);return;
    }
    Label(BodyCanvas,TEXT("DailyPreviewStatus"),TEXT("日常活动 · 开发预览\n目标、进度和奖励等待服务接入。"),0,0,BodyWidth,104,26);
    const TCHAR* Names[]={TEXT("静心修炼"),TEXT("秘境历练"),TEXT("丹道研习")};
    const int32 Columns=Wide?2:1;const float W=(BodyWidth-(Columns-1)*24)/Columns,H=250;
    for(int32 I=0;I<3;++I)
    {
        const float X=(I%Columns)*(W+24),Y=128+(I/Columns)*(H+24);
        Surface(BodyCanvas,NAME_None,ESurface::Card,X,Y,W,H);
        AtlasIcon(BodyCanvas,NAME_None,8+I,X+16,Y+20,72);
        Label(BodyCanvas,NAME_None,Names[I],X+108,Y+20,W-128,48,32);
        Label(BodyCanvas,NAME_None,TEXT("任务与奖励尚未接入"),X+108,Y+82,W-128,64,24);
        Action(BodyCanvas,TEXT("Open_DailyActivityDetail|")+FString(Names[I]),TEXT("查看目标与条件"),X+16,Y+146,W-32,88,true);
    }
}
void UInkTimedActivitiesPanel::BuildPanelContent()
{
    const bool Wide=!bPortraitLayout&&BodyWidth>=800;
    auto* Service=WulingFrontendData::Service(this);
    if(Service&&Service->IsEnabled())
    {
        const float SummaryW=Wide?BodyWidth*.36f:BodyWidth,X=Wide?SummaryW+28:0,Y=Wide?0:260,W=BodyWidth-X;
        Label(BodyCanvas,TEXT("TimedHeading"),TEXT("本期历练"),0,0,SummaryW,60,40);
        Label(BodyCanvas,TEXT("TimedAuthorityStatus"),TEXT("封闭原型周活动模板\n沿用世界历练基础奖励。未配置日历排期，由主播显式发起。"),0,76,SummaryW,156,26);
        Action(BodyCanvas,TEXT("WeekGuard"),TEXT("云台护阵 · 开始"),X,Y,W,88,true)->SetIsEnabled(!Service->IsBusy());
        Action(BodyCanvas,TEXT("WeekIron"),TEXT("玄铁采集 · 开始"),X,Y+108,W,88,true)->SetIsEnabled(!Service->IsBusy());
        Link(BodyCanvas,TEXT("WulingRace"),TEXT("灵脉竞采"),TEXT("与实际武灵排队匹配 · 查看资格"),5,X,Y+228,W,176);return;
    }
    Label(BodyCanvas,TEXT("TimedPreviewStatus"),TEXT("限时活动 · 开发预览\n尚未配置开放周期，不显示虚构倒计时。"),0,0,BodyWidth,104,26);
    const int32 Columns=Wide?2:1;const float W=(BodyWidth-(Columns-1)*24)/Columns,H=360;
    const TCHAR* Names[]={TEXT("仙缘寻宝"),TEXT("云山问道")};
    for(int32 I=0;I<2;++I)
    {
        const float X=(I%Columns)*(W+24),Y=128+(I/Columns)*(H+24);
        Surface(BodyCanvas,NAME_None,ESurface::Card,X,Y,W,H);Scene(BodyCanvas,I?3:7,X+16,Y+12,W-32,112);
        Label(BodyCanvas,NAME_None,Names[I],X+20,Y+136,W-40,48,32);
        Label(BodyCanvas,NAME_None,TEXT("开放时间与奖励待配置"),X+20,Y+194,W-40,48,24);
        Action(BodyCanvas,TEXT("Open_TimedActivityDetail|")+FString(Names[I]),TEXT("查看活动"),X+16,Y+256,W-32,88,true);
    }
}
