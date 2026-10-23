#include "InkFeaturePanel.h"
#include "InkFrontendWidget.h"
#include "WulingFrontendData.h"
#include "InkBufferedProgressWidget.h"
#include "InkSignInChestWidget.h"
#include "InkSignInPresentation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"

namespace
{
    TArray<TSharedPtr<FJsonObject>> Chests(UUserWidget* Widget,bool Monthly)
    {
        TArray<TSharedPtr<FJsonObject>> Result;const auto A=WulingFrontendData::Activity(Widget);
        const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;
        if(A&&A->TryGetArrayField(Monthly?TEXT("monthly_chests"):TEXT("weekly_chests"),Values))for(const auto& V:*Values)Result.Add(V->AsObject());
        return Result;
    }
    bool Flag(const TSharedPtr<FJsonObject>& A,const TCHAR* Key){bool B=false;if(A)A->TryGetBoolField(Key,B);return B;}
}

void UInkFeaturePanel::BuildSignInChestStrip(bool Monthly,float Y,int32 SelectedChest)
{
    const FString Prefix=Monthly?TEXT("MonthlyChest"):TEXT("WeeklyChest");const auto Records=Chests(this,Monthly);
    const bool Wide=!bPortraitLayout&&BodyWidth>=800;
    const float X=Monthly&&Wide?BodyWidth*.52f+24:0;
    const float W=Monthly?BodyWidth-X:Wide?BodyWidth*.60f:BodyWidth;
    const int32 N=Monthly?4:7;TArray<int32> Days=Monthly?TArray<int32>{3,7,14,21}:TArray<int32>{1,2,3,4,5,6,7};
    if(Monthly)for(int32 I=0;I<N;++I)if(Records.IsValidIndex(I)&&Records[I]){const int32 Milestone=WulingFrontendData::Number(Records[I],TEXT("milestone"));if(Milestone>0)Days[I]=Milestone;}
    const TCHAR* Week[]={TEXT("周一"),TEXT("周二"),TEXT("周三"),TEXT("周四"),TEXT("周五"),TEXT("周六"),TEXT("周日")};
    const TCHAR* Preview[]={TEXT("灵石（占位）"),TEXT("灵草（占位）"),TEXT("玄铁（占位）"),TEXT("丹药（占位）"),TEXT("残卷（占位）"),TEXT("玉简（占位）"),TEXT("锦囊（占位）")};
    Label(BodyCanvas,Monthly?TEXT("CalendarMilestones"):TEXT("SignInMilestones"),Monthly?TEXT("本月累计奖励"):TEXT("七日相伴 · 点击查看"),X,Y,W,64,28);
    const float Cell=Monthly?W/4:(W-36)/4,Size=88,CenterY=Y+132;
    if(Monthly)
    {
        const auto A=WulingFrontendData::Activity(this);auto* Progress=CreateWidget<UInkBufferedProgressWidget>(GetOwningPlayer());Progress->Rename(*(Prefix+TEXT("Progress")),WidgetTree);
        Progress->SetFraction(InkSignIn::MilestoneFraction(WulingFrontendData::Number(A,TEXT("month_sign_count")),Days),true);Progress->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* P=BodyCanvas->AddChildToCanvas(Progress);P->SetOffsets(FMargin(X+Cell*.5f,CenterY-4,W-Cell,8));
    }
    int32 ClaimedCount=0;
    for(int32 I=0;I<N;++I)
    {
        const auto R=Records.IsValidIndex(I)?Records[I]:nullptr;const bool Open=Flag(R,TEXT("claimed")),Eligible=Flag(R,TEXT("eligible"));if(Open)++ClaimedCount;
        const float CardX=X+(Monthly?I*Cell:(I<4?I*(Cell+12):(W-3*Cell-24)*.5f+(I-4)*(Cell+12)));
        const float CardY=Monthly?CenterY-Size*.5f:Y+88+(I/4)*240;
        if(!Monthly||SelectedChest==I)
        {
            auto* Card=Surface(BodyCanvas,NAME_None,InkPresentationStyle::ESurface::Card,CardX,CardY,Cell,Monthly?88:224);
            if(SelectedChest==I||Eligible){auto B=Card->GetBrush();B.SetResourceObject(nullptr);B.DrawAs=ESlateBrushDrawType::RoundedBox;B.TintColor=FLinearColor(.72f,.54f,.22f,.12f);B.OutlineSettings.Color=InkPresentationStyle::Gold();B.OutlineSettings.Width=2;B.OutlineSettings.RoundingType=ESlateBrushRoundingType::FixedRadius;B.OutlineSettings.CornerRadii=FVector4(6,6,6,6);Card->SetBrush(B);}
        }
        auto* Chest=CreateWidget<UInkSignInChestWidget>(GetOwningPlayer());Chest->Rename(*(Prefix+TEXT("Art_")+FString::FromInt(I+1)),WidgetTree);
        Chest->Configure(FrontendOwner,Monthly?TEXT("MonthlySignIn"):TEXT("SignIn"),Open,Eligible);Chest->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* C=BodyCanvas->AddChildToCanvas(Chest);C->SetOffsets(FMargin(CardX+(Cell-Size)*.5f,Monthly?CardY:CardY+36,Size,Size));C->SetZOrder(1);
        auto* Button=Action(BodyCanvas,TEXT("ChestPreview_")+FString::FromInt(I),TEXT(""),CardX,CardY,Cell,Monthly?88:224);
        FButtonStyle Transparent;FSlateBrush Empty;Empty.DrawAs=ESlateBrushDrawType::NoDrawType;FSlateBrush Hover;Hover.DrawAs=ESlateBrushDrawType::RoundedBox;Hover.TintColor=FLinearColor(.12f,.25f,.32f,.10f);Hover.OutlineSettings.Color=FLinearColor(.24f,.38f,.47f,1);Hover.OutlineSettings.Width=1.5f;
        Transparent.SetNormal(Empty);Transparent.SetHovered(Hover);Transparent.SetPressed(Hover);Button->SetStyle(Transparent);CastChecked<UCanvasPanelSlot>(Button->Slot)->SetZOrder(2);
        Label(BodyCanvas,*(Prefix+TEXT("Day_")+FString::FromInt(I+1)),Monthly?FString::Printf(TEXT("%d日"),Days[I]):FString(Week[I]),CardX,Monthly?CenterY+Size*.5f+2:CardY+2,Cell,32,24,true)->SetAutoWrapText(false);
        if(!Monthly)
        {
            const FString RewardName=(R?WulingFrontendData::String(R,TEXT("reward")):FString(Preview[I])).Replace(TEXT("（占位）"),TEXT(""));
            Label(BodyCanvas,*(TEXT("WeeklyRewardName_")+FString::FromInt(I+1)),RewardName,CardX+4,CardY+124,Cell-8,32,26,true)->SetAutoWrapText(false);
            Label(BodyCanvas,NAME_None,TEXT("奖励占位"),CardX+4,CardY+156,Cell-8,28,24,true)->SetAutoWrapText(false);
        }
        Label(BodyCanvas,*(Prefix+TEXT("State_")+FString::FromInt(I+1)),Open?(Monthly?TEXT("已领取"):TEXT("已签到")):Eligible?(Monthly?TEXT("待领取"):TEXT("今日可签")):R?TEXT("未签到"):TEXT("待同步"),CardX,Monthly?CenterY+Size*.5f+32:CardY+190,Cell,32,24,true)->SetAutoWrapText(false);
    }
    if(!Monthly)
    {
        Label(BodyCanvas,TEXT("WeeklyConfirmedCount"),Records.Num()?FString::Printf(TEXT("本周已确认 %d / 7 日"),ClaimedCount):TEXT("本周进度待同步"),X,Y+572,W,48,26);
        auto* Progress=CreateWidget<UInkBufferedProgressWidget>(GetOwningPlayer());Progress->Rename(TEXT("WeeklyConfirmedProgress"),WidgetTree);Progress->SetFraction(ClaimedCount/7.f,true);Progress->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* P=BodyCanvas->AddChildToCanvas(Progress);P->SetOffsets(FMargin(X,Y+636,W,12));
    }
    const float DetailX=!Monthly&&Wide?W+28:X,DetailW=BodyWidth-DetailX;
    const float Top=Monthly?Y+272:Wide?Y:Y+680;
    if(SelectedChest<0||SelectedChest>=N)
    {
        Label(BodyCanvas,TEXT("ChestSelectionHint"),Monthly?TEXT("点击宝箱查看纪念外观与领取条件。\n奖励状态以本月服务器记录为准。"):TEXT("选中任意一天，查看签到记录与奖励占位。\n签到不发放修为或物品。"),DetailX,Top,DetailW,164,26);return;
    }
    const auto R=Records.IsValidIndex(SelectedChest)?Records[SelectedChest]:nullptr;const bool Open=Flag(R,TEXT("claimed")),Eligible=Flag(R,TEXT("eligible"));
    auto* Detail=Surface(BodyCanvas,TEXT("ChestRewardPanel"),InkPresentationStyle::ESurface::Card,DetailX,Top,DetailW,440);
    Label(BodyCanvas,TEXT("ChestRewardTitle"),Monthly?FString::Printf(TEXT("月签 · 第%d日"),Days[SelectedChest]):FString(Week[SelectedChest])+TEXT(" · 占位"),DetailX+16,Top+20,DetailW-124,64,28);
    Action(BodyCanvas,TEXT("ChestPreviewClose"),TEXT("关闭"),DetailX+DetailW-100,Top,88,88);
    const FString Reward=R?WulingFrontendData::String(R,TEXT("reward")):Monthly?TEXT("奖励明细待服务同步"):FString(Preview[SelectedChest]);
    FString Details=Reward+TEXT("\n")+(Open?(Monthly?TEXT("已领取 · 宝箱已空"):TEXT("已签到 · 占位已记录")):Eligible?(Monthly?TEXT("已达到领取条件"):TEXT("今日可签到")):TEXT("尚未达到领取条件"));
    if(!Monthly)Details+=TEXT("\n仅展示奖励占位，不发放修为或物品。");
    Label(BodyCanvas,TEXT("ChestRewardDetails"),Details,DetailX+16,Top+112,DetailW-32,208,26);
    auto* Claim=Action(BodyCanvas,TEXT("ChestRewardClaim"),Open?(Monthly?TEXT("已领取"):TEXT("已签到")):Monthly?TEXT("领取奖励"):TEXT("签到 · 记录占位"),DetailX+16,Top+336,DetailW-32,88,true);
    auto* Service=WulingFrontendData::Service(this);Claim->SetIsEnabled(Eligible&&!Open&&Service&&!Service->IsBusy());
    if(!Wide)if(auto* Scroll=Cast<UScrollBox>(WidgetTree->FindWidget(TEXT("SignInContentScroll"))))Scroll->ScrollWidgetIntoView(Detail,true,EDescendantScrollDestination::IntoView);
}


bool UInkFeaturePanel::HandleSignInChestAction(const FString& Id,bool Monthly,int32& SelectedChest)
{
    if(Id.StartsWith(TEXT("ChestPreview_"))){SelectedChest=FCString::Atoi(*Id.Mid(13));RebuildPanel();return true;}
    if(Id==TEXT("ChestPreviewClose")){SelectedChest=-1;RebuildPanel();return true;}
    if(Id!=TEXT("ChestRewardClaim"))return false;
    const auto Records=Chests(this,Monthly);auto* Service=WulingFrontendData::Service(this);
    if(!Records.IsValidIndex(SelectedChest)||!Service||Service->IsBusy()||!Flag(Records[SelectedChest],TEXT("eligible"))||Flag(Records[SelectedChest],TEXT("claimed"))){FeedbackMessage(TEXT("奖励当前不可领取，请等待服务同步。"));return true;}
    const bool Submitted=Monthly?Service->SubmitPrototypeCommand(TEXT("ClaimSignInChest"),FString::Printf(TEXT("{\"milestone\":%d}"),WulingFrontendData::Number(Records[SelectedChest],TEXT("milestone")))):Service->SignIn();
    FeedbackMessage(Submitted?TEXT("领取请求已记录，等待服务确认"):Service->GetStatus());return true;
}

void UInkSignInPanel::BuildPanelContent()
{
    const auto A=WulingFrontendData::Activity(this);
    Label(BodyCanvas,TEXT("SignInState"),A?WulingFrontendData::String(A,TEXT("day"))+TEXT(" · 本周签到"):TEXT("七日签到 · 服务未连接"),0,0,BodyWidth,56,32);
    Label(BodyCanvas,TEXT("SignInTodayDetails"),A?(WulingFrontendData::Signed(A)?TEXT("今日已签到 · 奖励占位已记录"):TEXT("今日未签到 · 奖励使用占位，不发放修为或物品。")):TEXT("开发预览 · 日期、进度与领取记录待同步"),0,72,BodyWidth,80,26);
    BuildSignInChestStrip(false,176,SelectedChest);
}

void UInkSignInPanel::HandlePanelAction(const FString& Id)
{if(!HandleSignInChestAction(Id,false,SelectedChest))Super::HandlePanelAction(Id);}

void UInkMonthlySignInPanel::BuildPanelContent()
{
    const auto A=WulingFrontendData::Activity(this);
    const FString Date=WulingFrontendData::String(A,TEXT("day"));const int ServerYear=FCString::Atoi(*Date.Left(4)),ServerMonth=FCString::Atoi(*Date.Mid(5,2));
    const bool Valid=A&&ServerYear>=1&&ServerYear<=9999&&ServerMonth>=1&&ServerMonth<=12;
    int Year=ServerYear,Month=ServerMonth;
    if(Valid){const int Absolute=FMath::Clamp((ServerYear-1)*12+ServerMonth-1+CalendarMonthOffset,0,9999*12-1);Year=Absolute/12+1;Month=Absolute%12+1;}
    const bool Wide=!bPortraitLayout&&BodyWidth>=800;const float CalendarW=Wide?BodyWidth*.52f:BodyWidth;
    Label(BodyCanvas,TEXT("CalendarMonth"),Valid?FString::Printf(TEXT("%d 年 %d 月"),Year,Month):TEXT("月签日历 · 待同步"),96,12,CalendarW-192,64,32,true);
    Action(BodyCanvas,TEXT("CalendarPrevious"),TEXT("‹"),0,0,88,88)->SetIsEnabled(Valid&&CalendarMonthOffset>-12);
    Action(BodyCanvas,TEXT("CalendarNext"),TEXT("›"),CalendarW-88,0,88,88)->SetIsEnabled(Valid&&CalendarMonthOffset<0);
    Label(BodyCanvas,TEXT("CalendarStatus"),Valid?(CalendarMonthOffset==0?FString::Printf(TEXT("本月累计 %d 日 · 已确认签到记录"),WulingFrontendData::Number(A,TEXT("month_sign_count"))):TEXT("历史月份 · 仅标记服务提供的已签记录")):TEXT("尚未取得服务器日期和签到记录。"),0,108,CalendarW,84,24);
    const TArray<TSharedPtr<FJsonValue>>* Signed=nullptr;if(A)A->TryGetArrayField(TEXT("signed_days"),Signed);TSet<FString> Dates;if(Signed)for(const auto& V:*Signed)Dates.Add(V->AsString());
    const float Cell=CalendarW/7,Row=72;const TCHAR* Week[]={TEXT("一"),TEXT("二"),TEXT("三"),TEXT("四"),TEXT("五"),TEXT("六"),TEXT("日")};
    for(int I=0;I<7;++I)Label(BodyCanvas,NAME_None,Week[I],I*Cell,212,Cell,36,24,true);
    const int Offset=Valid?int(FDateTime(Year,Month,1).GetDayOfWeek()):0,Days=Valid?FDateTime::DaysInMonth(Year,Month):35;
    for(int Day=1;Day<=Days;++Day)
    {
        const int I=Offset+Day-1;const FString Key=FString::Printf(TEXT("%04d-%02d-%02d"),Year,Month,Day);const bool Confirmed=Valid&&Dates.Contains(Key),Today=Valid&&Key==Date;
        const float X=I%7*Cell,Y=264+I/7*Row;
        if(Confirmed||Today)
        {
            auto* DayCard=Surface(BodyCanvas,NAME_None,InkPresentationStyle::ESurface::Card,X+2,Y,Cell-4,Row-8);
            if(Today){auto B=DayCard->GetBrush();B.SetResourceObject(nullptr);B.DrawAs=ESlateBrushDrawType::RoundedBox;B.TintColor=FLinearColor(.72f,.54f,.22f,.20f);B.OutlineSettings.Color=InkPresentationStyle::Gold();B.OutlineSettings.Width=1;DayCard->SetBrush(B);}
        }
        Label(BodyCanvas,FName(*FString::Printf(TEXT("CalendarDay_%d"),Day)),Valid?FString::Printf(TEXT("%d"),Day):TEXT("—"),X,Y+4,Cell,36,26,true);
        if(Confirmed)Label(BodyCanvas,NAME_None,TEXT("·"),X,Y+34,Cell,28,24,true);
    }
    const float CalendarBottom=264+FMath::DivideAndRoundUp(Offset+Days,7)*Row;
    Label(BodyCanvas,TEXT("CalendarLegend"),TEXT("金色为今日 · 圆点为已签\n未标记日期仅表示当前记录未提供确认。"),0,CalendarBottom+16,CalendarW,88,24);
    if(CalendarMonthOffset!=0)Action(BodyCanvas,TEXT("CalendarCurrent"),TEXT("返回本月"),0,CalendarBottom+124,CalendarW,88);
    BuildSignInChestStrip(true,Wide?0:CalendarBottom+(CalendarMonthOffset?244:132),SelectedChest);
}

void UInkMonthlySignInPanel::HandlePanelAction(const FString& Id)
{
    if(Id==TEXT("CalendarPrevious")||Id==TEXT("CalendarNext")||Id==TEXT("CalendarCurrent"))
    {CalendarMonthOffset=Id==TEXT("CalendarCurrent")?0:FMath::Clamp(CalendarMonthOffset+(Id==TEXT("CalendarPrevious")?-1:1),-12,0);RebuildPanel();return;}
    if(!HandleSignInChestAction(Id,true,SelectedChest))Super::HandlePanelAction(Id);
}

