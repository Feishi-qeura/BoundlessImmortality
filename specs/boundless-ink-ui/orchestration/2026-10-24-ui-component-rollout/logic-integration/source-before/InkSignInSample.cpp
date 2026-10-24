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
    const FString AwaitingAuthority=TEXT("服务未提供/待同步");
    constexpr int32 SupportedMonthlyChestMilestones[]={3,7,14,21};

    bool IsSupportedMonthlyChestMilestone(double Value)
    {
        if(!FMath::IsFinite(Value)||FMath::FloorToDouble(Value)!=Value)return false;
        for(const int32 Milestone:SupportedMonthlyChestMilestones)if(Value==static_cast<double>(Milestone))return true;
        return false;
    }

    TArray<TSharedPtr<FJsonObject>> Chests(UUserWidget* Widget,bool Monthly)
    {
        TArray<TSharedPtr<FJsonObject>> Result;const auto A=WulingFrontendData::Activity(Widget);
        const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;
        if(A&&A->TryGetArrayField(Monthly?TEXT("monthly_chests"):TEXT("weekly_chests"),Values))for(const auto& V:*Values)Result.Add(V->AsObject());
        return Result;
    }
    bool Flag(const TSharedPtr<FJsonObject>& A,const TCHAR* Key){bool B=false;if(A)A->TryGetBoolField(Key,B);return B;}
    bool HasFlag(const TSharedPtr<FJsonObject>& A,const TCHAR* Key,bool& Value){Value=false;return A&&A->TryGetBoolField(Key,Value);}
    FString AuthorityString(const TSharedPtr<FJsonObject>& A,const TCHAR* Key){const FString Value=WulingFrontendData::String(A,Key);return Value.IsEmpty()?AwaitingAuthority:Value;}
    FString SignState(const TSharedPtr<FJsonObject>& A)
    {
        bool Signed=false;if(!HasFlag(A,TEXT("signed"),Signed))return TEXT("签到状态尚未同步");
        return Signed?TEXT("今日已签到"):TEXT("今日未签到");
    }
}

void UInkFeaturePanel::BuildSignInChestStrip(bool Monthly,float Y,int32 SelectedChest)
{
    const FString Prefix=Monthly?TEXT("MonthlyChest"):TEXT("WeeklyChest");const auto Records=Chests(this,Monthly);
    const bool Wide=!bPortraitLayout&&BodyWidth>=800;
    const float X=Monthly&&Wide?BodyWidth*.52f+24:0;
    const float W=Monthly?BodyWidth-X:Wide?BodyWidth*.60f:BodyWidth;
    const int32 N=Records.Num();
    if(N==0)
    {
        const float CardH=Monthly?112.f:132.f;
        Surface(BodyCanvas,Monthly?TEXT("MonthlySignInEmptyPaper"):TEXT("WeeklySignInEmptyPaper"),InkPresentationStyle::ESurface::FeatureCard,X,Y,W,CardH);
        Surface(BodyCanvas,Monthly?TEXT("MonthlySignInEmptyHeading"):TEXT("WeeklySignInEmptyHeading"),InkPresentationStyle::ESurface::FeatureHeading,X+8,Y+8,W-16,48);
        Label(BodyCanvas,Monthly?TEXT("MonthlySignInEmptyTitle"):TEXT("WeeklySignInEmptyTitle"),Monthly?TEXT("月签记录尚未同步"):TEXT("本周签到记录尚未同步"),X+20,Y+14,W-40,38,25);
        Label(BodyCanvas,Monthly?TEXT("MonthlySignInEmptyState"):TEXT("WeeklySignInEmptyState"),Monthly?TEXT("同步后显示服务日期与奖励节点，未同步前不展示日期或奖励。"):TEXT("同步后显示签到日期与服务确认的奖励记录。"),X+20,Y+68,W-40,56,21);
        return;
    }
    TArray<int32> Milestones;for(const auto& Record:Records){double Value=0;if(Record&&Record->TryGetNumberField(TEXT("milestone"),Value))Milestones.Add(int32(Value));}
    Label(BodyCanvas,Monthly?TEXT("CalendarMilestones"):TEXT("SignInMilestones"),Monthly?TEXT("本月累计奖励"):TEXT("签到记录 · 点击查看"),X,Y,W,64,28);
    const float Cell=Monthly?W/4:(W-36)/4,Size=88,CenterY=Y+132;
    if(Monthly)
    {
        const auto A=WulingFrontendData::Activity(this);
        double MonthSignCount=0;
        if(Milestones.Num()==Records.Num()&&Milestones.Num()>1&&A&&A->TryGetNumberField(TEXT("month_sign_count"),MonthSignCount))
        {
            auto* Progress=CreateWidget<UInkBufferedProgressWidget>(GetOwningPlayer());Progress->Rename(*(Prefix+TEXT("Progress")),WidgetTree);
            Progress->SetFraction(InkSignIn::MilestoneFraction(MonthSignCount,Milestones),true);Progress->SetVisibility(ESlateVisibility::HitTestInvisible);
            auto* P=BodyCanvas->AddChildToCanvas(Progress);P->SetOffsets(FMargin(X+Cell*.5f,CenterY-4,W-Cell,8));
        }
    }
    int32 ClaimedCount=0;
    for(int32 I=0;I<N;++I)
    {
        const auto R=Records.IsValidIndex(I)?Records[I]:nullptr;bool Open=false,Eligible=false;const bool HasClaimed=HasFlag(R,TEXT("claimed"),Open),HasEligible=HasFlag(R,TEXT("eligible"),Eligible);if(HasClaimed&&Open)++ClaimedCount;
        const float CardX=X+(Monthly?I*Cell:(I<4?I*(Cell+12):(W-3*Cell-24)*.5f+(I-4)*(Cell+12)));
        const float CardY=Monthly?CenterY-Size*.5f:Y+88+(I/4)*240;
        if(!Monthly||SelectedChest==I)
        {
            auto* Card=Surface(BodyCanvas,NAME_None,InkPresentationStyle::ESurface::Card,CardX,CardY,Cell,Monthly?88:224);
            if(SelectedChest==I||Eligible){auto B=Card->GetBrush();B.SetResourceObject(nullptr);B.DrawAs=ESlateBrushDrawType::RoundedBox;B.TintColor=FLinearColor(.72f,.54f,.22f,.12f);B.OutlineSettings.Color=InkPresentationStyle::Gold();B.OutlineSettings.Width=2;B.OutlineSettings.RoundingType=ESlateBrushRoundingType::FixedRadius;B.OutlineSettings.CornerRadii=FVector4(6,6,6,6);Card->SetBrush(B);}
        }
        if(HasClaimed&&HasEligible)
        {
            auto* Chest=CreateWidget<UInkSignInChestWidget>(GetOwningPlayer());Chest->Rename(*(Prefix+TEXT("Art_")+FString::FromInt(I+1)),WidgetTree);
            Chest->Configure(FrontendOwner,Monthly?TEXT("MonthlySignIn"):TEXT("SignIn"),Open,Eligible);Chest->SetVisibility(ESlateVisibility::HitTestInvisible);
            auto* C=BodyCanvas->AddChildToCanvas(Chest);C->SetOffsets(FMargin(CardX+(Cell-Size)*.5f,Monthly?CardY:CardY+36,Size,Size));C->SetZOrder(1);
        }
        auto* Button=Action(BodyCanvas,TEXT("ChestPreview_")+FString::FromInt(I),TEXT(""),CardX,CardY,Cell,Monthly?88:224);
        FButtonStyle Transparent;FSlateBrush Empty;Empty.DrawAs=ESlateBrushDrawType::NoDrawType;FSlateBrush Hover;Hover.DrawAs=ESlateBrushDrawType::RoundedBox;Hover.TintColor=FLinearColor(.12f,.25f,.32f,.10f);Hover.OutlineSettings.Color=FLinearColor(.24f,.38f,.47f,1);Hover.OutlineSettings.Width=1.5f;
        Transparent.SetNormal(Empty);Transparent.SetHovered(Hover);Transparent.SetPressed(Hover);Button->SetStyle(Transparent);CastChecked<UCanvasPanelSlot>(Button->Slot)->SetZOrder(2);
        if(!Monthly)
        {
            Label(BodyCanvas,*(TEXT("WeeklyRewardName_")+FString::FromInt(I+1)),AuthorityString(R,TEXT("reward")),CardX+4,CardY+124,Cell-8,32,24,true)->SetAutoWrapText(false);
            bool Placeholder=false;const bool HasPlaceholder=HasFlag(R,TEXT("placeholder"),Placeholder);
            Label(BodyCanvas,NAME_None,HasPlaceholder?(Placeholder?TEXT("服务标注：预览奖励"):TEXT("服务标注：非预览")):FString(TEXT("奖励类别"))+AwaitingAuthority,CardX+4,CardY+156,Cell-8,28,20,true)->SetAutoWrapText(false);
        }
        FString DayLabel=Monthly?AwaitingAuthority:AuthorityString(R,TEXT("day"));
        double MilestoneValue=0;if(Monthly&&R&&R->TryGetNumberField(TEXT("milestone"),MilestoneValue))DayLabel=FString::Printf(TEXT("累计 %d 日"),int32(MilestoneValue));
        Label(BodyCanvas,*(Prefix+TEXT("Day_")+FString::FromInt(I+1)),DayLabel,CardX,Monthly?CenterY+Size*.5f+2:CardY+2,Cell,32,22,true)->SetAutoWrapText(false);
        const FString RecordState=!HasClaimed?FString(TEXT("领取状态"))+AwaitingAuthority:Open?(Monthly?TEXT("已领取"):TEXT("已签到")):!HasEligible?FString(TEXT("领取条件"))+AwaitingAuthority:Eligible?(Monthly?TEXT("待领取"):TEXT("今日可签")):(Monthly?TEXT("未达领取条件"):TEXT("当前不可签到"));
        Label(BodyCanvas,*(Prefix+TEXT("State_")+FString::FromInt(I+1)),RecordState,CardX,Monthly?CenterY+Size*.5f+32:CardY+190,Cell,32,22,true)->SetAutoWrapText(false);
    }
    if(!Monthly)
    {
        Label(BodyCanvas,TEXT("WeeklyConfirmedCount"),Records.Num()?FString::Printf(TEXT("已确认签到 %d / %d 条服务记录"),ClaimedCount,Records.Num()):FString(TEXT("本周签到记录"))+AwaitingAuthority,X,Y+572,W,48,24);
        if(Records.Num()>0){auto* Progress=CreateWidget<UInkBufferedProgressWidget>(GetOwningPlayer());Progress->Rename(TEXT("WeeklyConfirmedProgress"),WidgetTree);Progress->SetFraction(ClaimedCount/float(Records.Num()),true);Progress->SetVisibility(ESlateVisibility::HitTestInvisible);auto* P=BodyCanvas->AddChildToCanvas(Progress);P->SetOffsets(FMargin(X,Y+636,W,12));}
    }
    const float DetailX=!Monthly&&Wide?W+28:X,DetailW=BodyWidth-DetailX;
    const float Top=Monthly?Y+272:Wide?Y:Y+680;
    if(SelectedChest<0||SelectedChest>=N)
    {
        Label(BodyCanvas,TEXT("ChestSelectionHint"),Records.Num()?TEXT("选择一条服务记录查看奖励与领取状态。\n内容仅显示服务快照提供的字段。"):FString(TEXT("签到奖励与领取记录"))+AwaitingAuthority,DetailX,Top,DetailW,164,26);return;
    }
    const auto R=Records.IsValidIndex(SelectedChest)?Records[SelectedChest]:nullptr;bool Open=Flag(R,TEXT("claimed")),Eligible=Flag(R,TEXT("eligible"));
    auto* Detail=Surface(BodyCanvas,TEXT("ChestRewardPanel"),InkPresentationStyle::ESurface::Card,DetailX,Top,DetailW,440);
    double ChestMilestone=0;const FString ChestTitle=Monthly?(R&&R->TryGetNumberField(TEXT("milestone"),ChestMilestone)?FString::Printf(TEXT("月签 · 累计 %d 日"),int32(ChestMilestone)):FString(TEXT("月签 · "))+AwaitingAuthority):AuthorityString(R,TEXT("day"));
    Label(BodyCanvas,TEXT("ChestRewardTitle"),ChestTitle,DetailX+16,Top+20,DetailW-124,64,28);
    Action(BodyCanvas,TEXT("ChestPreviewClose"),TEXT("关闭"),DetailX+DetailW-100,Top,88,88);
    const FString Reward=AuthorityString(R,TEXT("reward"));
    bool ClaimedValue=false,EligibleValue=false;const bool HasClaimedDetail=HasFlag(R,TEXT("claimed"),ClaimedValue),HasEligibleDetail=HasFlag(R,TEXT("eligible"),EligibleValue);
    const FString ClaimStatus=!HasClaimedDetail?FString(TEXT("领取状态："))+AwaitingAuthority:ClaimedValue?(Monthly?TEXT("领取状态：已领取"):TEXT("签到状态：已签到")):!HasEligibleDetail?FString(TEXT("领取条件："))+AwaitingAuthority:EligibleValue?(Monthly?TEXT("领取状态：待领取"):TEXT("签到状态：今日可签")):(Monthly?TEXT("领取状态：未达条件"):TEXT("签到状态：当前不可签"));
    bool HasPlaceholder=false;const bool HasPlaceholderField=HasFlag(R,TEXT("placeholder"),HasPlaceholder);
    const FString RewardStatus=!HasPlaceholderField?FString(TEXT("奖励类别："))+AwaitingAuthority:HasPlaceholder?TEXT("服务标注：预览项目，不代表已发放"):TEXT("服务标注：非预览项目");
    const FString Details=FString(TEXT("奖励："))+Reward+TEXT("\n")+RewardStatus+TEXT("\n")+ClaimStatus;
    Label(BodyCanvas,TEXT("ChestRewardDetails"),Details,DetailX+16,Top+112,DetailW-32,176,24);
    const bool HasClaimed=HasFlag(R,TEXT("claimed"),Open),HasEligible=HasFlag(R,TEXT("eligible"),Eligible);
    double Milestone=0;const bool HasMilestone=!Monthly||(R&&R->TryGetNumberField(TEXT("milestone"),Milestone)&&IsSupportedMonthlyChestMilestone(Milestone));
    auto* Service=WulingFrontendData::Service(this);
    const auto Activities=WulingFrontendData::Activity(this);
    const bool Connected=Service&&Service->IsEnabled()&&Service->IsConnected()&&Service->Snapshot().IsValid()&&Activities.IsValid();
    const bool CanClaim=HasClaimed&&HasEligible&&HasMilestone&&Eligible&&!Open&&Connected&&!Service->IsBusy();
    const FString ActionStatus=!HasClaimed||!HasEligible?TEXT("authority 未提供完整的已领取/领取资格状态，操作已禁用。"):
        Open?(Monthly?TEXT("本节点已领取，不可重复领取。"):TEXT("今日已签到，不可重复提交签到。")):
        !HasMilestone?TEXT("authority 未提供有效里程碑编号，无法提交领取请求。"):
        !Eligible?(Monthly?TEXT("尚未达到该月签里程碑，暂不可领取。"):TEXT("当前不是可签到日期，暂不可提交。")):
        !Connected?TEXT("签到服务或活动快照未连接，操作已禁用。"):
        Service->IsBusy()?TEXT("服务请求处理中，请等待确认后再操作。"):
        Monthly?TEXT("可领取服务已解锁的外观；领取结果以服务确认为准。"):
        TEXT("可提交签到记录；该周奖励为服务标注的占位项目，不代表实际发放。");
    Label(BodyCanvas,TEXT("ChestRewardActionStatus"),ActionStatus,DetailX+16,Top+296,DetailW-32,32,19);
    auto* Claim=Action(BodyCanvas,TEXT("ChestRewardClaim"),Open?(Monthly?TEXT("已领取"):TEXT("已签到")):Monthly?TEXT("领取奖励"):TEXT("签到"),DetailX+16,Top+336,DetailW-32,88,true);
    Claim->SetIsEnabled(CanClaim);
    if(!Wide)if(auto* Scroll=Cast<UScrollBox>(WidgetTree->FindWidget(TEXT("SignInContentScroll"))))Scroll->ScrollWidgetIntoView(Detail,true,EDescendantScrollDestination::IntoView);
}


bool UInkFeaturePanel::HandleSignInChestAction(const FString& Id,bool Monthly,int32& SelectedChest)
{
    if(Id.StartsWith(TEXT("ChestPreview_"))){SelectedChest=FCString::Atoi(*Id.Mid(13));RebuildPanel();return true;}
    if(Id==TEXT("ChestPreviewClose")){SelectedChest=-1;RebuildPanel();return true;}
    if(Id!=TEXT("ChestRewardClaim"))return false;
    const auto Records=Chests(this,Monthly);auto* Service=WulingFrontendData::Service(this);
    double Milestone=0;const bool HasMilestone=!Monthly||(Records.IsValidIndex(SelectedChest)&&Records[SelectedChest]&&Records[SelectedChest]->TryGetNumberField(TEXT("milestone"),Milestone)&&IsSupportedMonthlyChestMilestone(Milestone));
    bool Claimed=false,Eligible=false;
    const bool HasClaimed=Records.IsValidIndex(SelectedChest)&&HasFlag(Records[SelectedChest],TEXT("claimed"),Claimed);
    const bool HasEligible=Records.IsValidIndex(SelectedChest)&&HasFlag(Records[SelectedChest],TEXT("eligible"),Eligible);
    const auto Activities=WulingFrontendData::Activity(this);
    if(!Records.IsValidIndex(SelectedChest)||!HasMilestone||!HasClaimed||!HasEligible||!Service||!Service->IsEnabled()||!Service->IsConnected()||!Service->Snapshot()||!Activities||Service->IsBusy()||!Eligible||Claimed)
    {
        FeedbackMessage(!HasClaimed||!HasEligible?TEXT("authority 未提供完整领取状态，操作未提交。"):
            Claimed?TEXT("该节点已完成，不能重复领取或签到。"):
            !HasMilestone?TEXT("authority 未提供有效里程碑编号，操作未提交。"):
            !Eligible?TEXT("尚未满足服务端的领取/签到条件，操作未提交。"):
            Service&&Service->IsBusy()?TEXT("服务请求处理中，操作未提交。"):
            TEXT("签到服务或活动快照未连接，操作未提交。"));return true;
    }
    const bool Submitted=Monthly?Service->SubmitPrototypeCommand(TEXT("ClaimSignInChest"),FString::Printf(TEXT("{\"milestone\":%d}"),static_cast<int32>(Milestone))):Service->SignIn();
    FeedbackMessage(Submitted?(Monthly?TEXT("月签领取请求已提交，等待服务确认"):TEXT("签到请求已提交，等待服务确认")):Service->GetStatus());return true;
}

void UInkSignInPanel::BuildPanelContent()
{
    const auto A=WulingFrontendData::Activity(this);
    const FString Day=WulingFrontendData::String(A,TEXT("day"));
    Label(BodyCanvas,TEXT("SignInState"),Day.IsEmpty()?TEXT("服务日期尚未同步"):FString(TEXT("服务日期："))+Day,0,0,BodyWidth,56,30);
    Label(BodyCanvas,TEXT("SignInTodayDetails"),SignState(A)+TEXT(" · 签到结果由服务端确认。"),0,72,BodyWidth,56,24);
    BuildSignInChestStrip(false,176,SelectedChest);
}

void UInkSignInPanel::HandlePanelAction(const FString& Id)
{if(!HandleSignInChestAction(Id,false,SelectedChest))Super::HandlePanelAction(Id);}

void UInkMonthlySignInPanel::BuildPanelContent()
{
    const auto A=WulingFrontendData::Activity(this);
    const FString Date=WulingFrontendData::String(A,TEXT("day"));const int ServerYear=FCString::Atoi(*Date.Left(4)),ServerMonth=FCString::Atoi(*Date.Mid(5,2)),ServerDay=FCString::Atoi(*Date.Mid(8,2));
    const bool Valid=A&&Date.Len()>=10&&Date[4]==TEXT('-')&&Date[7]==TEXT('-')&&ServerYear>=1&&ServerYear<=9999&&ServerMonth>=1&&ServerMonth<=12&&ServerDay>=1&&ServerDay<=FDateTime::DaysInMonth(ServerYear,ServerMonth);
    if(!Valid)
    {
        Surface(BodyCanvas,TEXT("CalendarUnavailablePaper"),InkPresentationStyle::ESurface::FeatureCard,0,12,BodyWidth,148);
        Surface(BodyCanvas,TEXT("CalendarUnavailableHeading"),InkPresentationStyle::ESurface::FeatureHeading,8,20,BodyWidth-16,52);
        Label(BodyCanvas,TEXT("CalendarUnavailableTitle"),TEXT("月签日期尚未同步"),20,28,BodyWidth-40,40,27);
        Label(BodyCanvas,TEXT("CalendarUnavailableState"),TEXT("取得有效服务日期和签到记录后显示日历。"),20,88,BodyWidth-40,56,22);
        return;
    }
    int Year=ServerYear,Month=ServerMonth;
    if(Valid){const int Absolute=FMath::Clamp((ServerYear-1)*12+ServerMonth-1+CalendarMonthOffset,0,9999*12-1);Year=Absolute/12+1;Month=Absolute%12+1;}
    const bool Wide=!bPortraitLayout&&BodyWidth>=800;const float CalendarW=Wide?BodyWidth*.52f:BodyWidth;
    Label(BodyCanvas,TEXT("CalendarMonth"),Valid?FString::Printf(TEXT("%d 年 %d 月"),Year,Month):TEXT("月签日历 · 待同步"),96,12,CalendarW-192,64,32,true);
    Action(BodyCanvas,TEXT("CalendarPrevious"),TEXT("‹"),0,0,88,88)->SetIsEnabled(Valid&&CalendarMonthOffset>-12);
    Action(BodyCanvas,TEXT("CalendarNext"),TEXT("›"),CalendarW-88,0,88,88)->SetIsEnabled(Valid&&CalendarMonthOffset<0);
    double MonthSignCount=0;const bool HasMonthSignCount=A&&A->TryGetNumberField(TEXT("month_sign_count"),MonthSignCount);
    Label(BodyCanvas,TEXT("CalendarStatus"),CalendarMonthOffset==0?(HasMonthSignCount?FString::Printf(TEXT("本月累计 %d 日 · 已确认签到记录"),int32(MonthSignCount)):TEXT("本月累计签到记录尚未同步")):TEXT("历史月份 · 仅标记服务提供的已签记录"),0,108,CalendarW,84,24);
    const TArray<TSharedPtr<FJsonValue>>* Signed=nullptr;if(A)A->TryGetArrayField(TEXT("signed_days"),Signed);TSet<FString> Dates;if(Signed)for(const auto& V:*Signed)Dates.Add(V->AsString());
    const float Cell=CalendarW/7,Row=72;const TCHAR* Week[]={TEXT("一"),TEXT("二"),TEXT("三"),TEXT("四"),TEXT("五"),TEXT("六"),TEXT("日")};
    for(int I=0;I<7;++I)Label(BodyCanvas,NAME_None,Week[I],I*Cell,212,Cell,36,24,true);
    // FDateTime uses Monday=1..Sunday=0; the calendar grid begins on Monday.
    const int Offset=(int(FDateTime(Year,Month,1).GetDayOfWeek())+6)%7,Days=FDateTime::DaysInMonth(Year,Month);
    for(int Day=1;Day<=Days;++Day)
    {
        const int I=Offset+Day-1;const FString Key=FString::Printf(TEXT("%04d-%02d-%02d"),Year,Month,Day);const bool Confirmed=Dates.Contains(Key),Today=Key==Date;
        const bool NotYetReached=CalendarMonthOffset==0&&Day>ServerDay;
        const float X=I%7*Cell,Y=264+I/7*Row;
        auto* DayCard=Surface(BodyCanvas,NAME_None,InkPresentationStyle::ESurface::Card,X+2,Y,Cell-4,Row-8);
        auto DayBrush=DayCard->GetBrush();DayBrush.SetResourceObject(nullptr);DayBrush.DrawAs=ESlateBrushDrawType::RoundedBox;
        DayBrush.OutlineSettings.RoundingType=ESlateBrushRoundingType::FixedRadius;DayBrush.OutlineSettings.CornerRadii=FVector4(5,5,5,5);
        if(Confirmed)
        {
            DayBrush.TintColor=FLinearColor(.23f,.55f,.49f,.16f);DayBrush.OutlineSettings.Color=Today?InkPresentationStyle::Gold():InkPresentationStyle::Jade();DayBrush.OutlineSettings.Width=Today?2.f:1.5f;
        }
        else if(Today)
        {
            DayBrush.TintColor=FLinearColor(.72f,.54f,.22f,.19f);DayBrush.OutlineSettings.Color=InkPresentationStyle::Gold();DayBrush.OutlineSettings.Width=2;
        }
        else if(NotYetReached)
        {
            DayBrush.TintColor=FLinearColor(.48f,.49f,.45f,.07f);DayBrush.OutlineSettings.Color=FLinearColor(.48f,.49f,.45f,.42f);DayBrush.OutlineSettings.Width=.75f;
        }
        else
        {
            DayBrush.TintColor=FLinearColor(.96f,.94f,.88f,.48f);DayBrush.OutlineSettings.Color=FLinearColor(.35f,.39f,.34f,.18f);DayBrush.OutlineSettings.Width=.5f;
        }
        DayCard->SetBrush(DayBrush);
        auto* DayNumber=Label(BodyCanvas,FName(*FString::Printf(TEXT("CalendarDay_%d"),Day)),FString::Printf(TEXT("%d"),Day),X,Y+3,Cell,34,25,true);
        DayNumber->SetColorAndOpacity(NotYetReached?FLinearColor(.52f,.53f,.49f,1):InkPresentationStyle::Ink());
        const FString DayState=Confirmed?TEXT("已签"):Today?TEXT("今日"):NotYetReached?TEXT("未到"):TEXT("未确认");
        auto* State=Label(BodyCanvas,FName(*FString::Printf(TEXT("CalendarState_%d"),Day)),DayState,X,Y+37,Cell,24,17,true);
        State->SetColorAndOpacity(Today?InkPresentationStyle::Gold():Confirmed?InkPresentationStyle::Jade():FLinearColor(.47f,.49f,.45f,1));
    }
    const float CalendarBottom=264+FMath::DivideAndRoundUp(Offset+Days,7)*Row;
    Label(BodyCanvas,TEXT("CalendarLegend"),TEXT("金框为今日 · 青玉为已签到\n“未到”为未来日期；“未确认”不代表漏签。"),0,CalendarBottom+16,CalendarW,88,22);
    if(CalendarMonthOffset!=0)Action(BodyCanvas,TEXT("CalendarCurrent"),TEXT("返回本月"),0,CalendarBottom+124,CalendarW,88);
    BuildSignInChestStrip(true,Wide?0:CalendarBottom+(CalendarMonthOffset?244:132),SelectedChest);
}

void UInkMonthlySignInPanel::HandlePanelAction(const FString& Id)
{
    if(Id==TEXT("CalendarPrevious")||Id==TEXT("CalendarNext")||Id==TEXT("CalendarCurrent"))
    {CalendarMonthOffset=Id==TEXT("CalendarCurrent")?0:FMath::Clamp(CalendarMonthOffset+(Id==TEXT("CalendarPrevious")?-1:1),-12,0);RebuildPanel();return;}
    if(!HandleSignInChestAction(Id,true,SelectedChest))Super::HandlePanelAction(Id);
}

