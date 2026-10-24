"""One-time exact source merge; no Unreal operations. Preserves original line endings."""
from pathlib import Path

ROOT=Path(__file__).resolve().parents[5]
path=ROOT/'Source/BoundlessImmortality/Frontend/InkSignInSample.cpp'
data=path.read_bytes()
newline=b'\r\n' if b'\r\n' in data else b'\n'
text=data.decode('utf-8').replace('\r\n','\n')
assert 'RolloutCalendarCell_' not in text, 'Calendar merge already applied'
text=text.replace('#include "InkFeaturePanel.h"', '#include "InkFeaturePanel.h"\n#include "Frontend/UiRollout20261024/Journey/JourneyRolloutWidgets.h"\n#include "Frontend/UiRollout20261024/LogicJourney/InkJourneyAdapter.h"',1)
start=text.index('        auto* DayCard=Surface(BodyCanvas,NAME_None,InkPresentationStyle::ESurface::Card,X+2,Y,Cell-4,Row-8);')
end=text.index('\n    }\n    const float CalendarBottom=',start)
text=text[:start]+'''        auto* DayCell=CreateWidget<UJourneyCalendarDayWidget>(GetOwningPlayer());
        DayCell->Rename(*FString::Printf(TEXT("RolloutCalendarCell_%d"),Day),WidgetTree);
        InkJourneyAdapter::FRequest TodayRequest;TodayRequest.ExpectedDay=Date;
        const bool TodayEligible=Today&&InkJourneyAdapter::Evaluate(InkJourneyAdapter::Capture(WulingFrontendData::Service(this),IsRolloutPageActive()),TodayRequest).bAllowed;
        const auto CellState=Confirmed?EJourneyCalendarState::Signed:TodayEligible?EJourneyCalendarState::TodayClaimable:NotYetReached?EJourneyCalendarState::Future:EJourneyCalendarState::Unknown;
        DayCell->SetDayState(Day,CellState,Day==SelectedCalendarDay);
        DayCell->OnDayPressed.AddUniqueDynamic(this,&UInkMonthlySignInPanel::HandleRolloutCalendarDay);
        auto* CellSlot=BodyCanvas->AddChildToCanvas(DayCell);CellSlot->SetOffsets(FMargin(X+2,Y,Cell-4,Row-8));
''' .rstrip()+text[end:]
text=text.replace('    const float Cell=CalendarW/7,Row=72;', '    const float Cell=CalendarW/7,Row=88;',1)
text=text.replace('    BuildSignInChestStrip(true,Wide?0:', '    if(SelectedCalendarDay>0)Label(BodyCanvas,TEXT("CalendarSelectedDetail"),FString::Printf(TEXT("已选择 %d 日 · 仅查看服务记录，未知状态不代表漏签"),SelectedCalendarDay),0,CalendarBottom+106,CalendarW,72,22);\n    BuildSignInChestStrip(true,Wide?0:',1)
text=text.replace('CalendarBottom+(CalendarMonthOffset?244:132)', 'CalendarBottom+(CalendarMonthOffset?300:212)',1)
text=text.replace('void UInkMonthlySignInPanel::HandlePanelAction', '''void UInkMonthlySignInPanel::HandleRolloutCalendarDay(int32 Day)
{
    if(!IsRolloutPageActive())return;
    SelectedCalendarDay=Day;RebuildPanel();
}

void UInkMonthlySignInPanel::HandlePanelAction''',1)
text=text.replace('{CalendarMonthOffset=Id==TEXT("CalendarCurrent")?', '{SelectedCalendarDay=0;CalendarMonthOffset=Id==TEXT("CalendarCurrent")?',1)
text=text.replace('if(Id.StartsWith(TEXT("ChestPreview_"))){SelectedChest=', 'if(Id.StartsWith(TEXT("ChestPreview_"))){RolloutChestDay=WulingFrontendData::String(WulingFrontendData::Activity(this),TEXT("day"));SelectedChest=',1)
old='    const bool Submitted=Monthly?Service->SubmitPrototypeCommand(TEXT("ClaimSignInChest"),FString::Printf(TEXT("{\\"milestone\\":%d}"),static_cast<int32>(Milestone))):Service->SignIn();\n    FeedbackMessage(Submitted?(Monthly?TEXT("月签领取请求已提交，等待服务确认"):TEXT("签到请求已提交，等待服务确认")):Service->GetStatus());return true;'
assert old in text, 'Chest submission anchor changed'
text=text.replace(old,'''    InkJourneyAdapter::FRequest Request;Request.Action=Monthly?InkJourneyAdapter::EAction::MonthlyChest:InkJourneyAdapter::EAction::SignIn;
    Request.ExpectedDay=Monthly?RolloutChestDay:WulingFrontendData::String(Records[SelectedChest],TEXT("day"));
    Request.Milestone=static_cast<int32>(Milestone);Request.bConfirmed=true;
    FString Feedback;InkJourneyAdapter::Submit(Service,IsRolloutPageActive(),Request,Feedback);FeedbackMessage(Feedback);return true;''',1)
path.write_bytes(text.encode('utf-8').replace(b'\n',newline))
print('Merged calendar cells and chest action gates:',path)
