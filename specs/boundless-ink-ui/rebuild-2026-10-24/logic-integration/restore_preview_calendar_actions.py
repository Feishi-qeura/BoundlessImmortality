from pathlib import Path
import runpy
R=Path.cwd();H=R/'specs/boundless-ink-ui/rebuild-2026-10-24/logic-integration';B='Source/BoundlessImmortality/Frontend/UIRebuild20261024/'
rec=runpy.run_path(str(R/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/record_source_change.py'),run_name='local_preview_calendar')
def edit(p,fn,reason):
 t=R/p;s=t.read_text(encoding='utf-8-sig');n=fn(s);assert s!=n,p;rec['begin'](p,reason);t.write_text(n,encoding='utf-8');rec['finish'](p)
def player(s):
 s=s.replace('#include "Frontend/InkFrontendWidget.h"','#include "Frontend/InkFrontendWidget.h"\n#include "Frontend/InkAnchorStatusWidget.h"')
 old='''            // Preview options require an actual provider; no fabricated unlocks or persistence.
            Vm.PreviewDisabledReason=FText::FromString(TEXT("外观预览选项尚未提供"));''';assert old in s
 s=s.replace(old,'''            // Existing local presentation samples are not unlocks, ownership or saves.
            Vm.AppearanceState.State=C::EProviderState::Data;Vm.AppearanceState.Message=FText::FromString(TEXT("仅本地试戴，不改变持有或装备状态"));
            Vm.bFrameOptionsKnown=true;Vm.bBarOptionsKnown=true;
            for(const TCHAR* Name:{TEXT("素墨头像框"),TEXT("青金头像框"),TEXT("山水头像框")}){C::FPreviewOption Option;Option.StyleId=Name;Option.Label=FText::FromString(Name);Vm.FrameOptions.Add(Option);}
            for(const TCHAR* Name:{TEXT("普通"),TEXT("渐变"),TEXT("护盾"),TEXT("华丽")}){C::FPreviewOption Option;Option.StyleId=Name;Option.Label=FText::FromString(Name);Vm.BarOptions.Add(Option);}
            Vm.DraftPreviewFrame=DraftPreviewFrame;Vm.DraftPreviewBar=DraftPreviewBar;Vm.PreviewFrame=PreviewFrame;Vm.PreviewBar=PreviewBar;Vm.bPreviewFrameKnown=!PreviewFrame.IsNone();Vm.bPreviewBarKnown=!PreviewBar.IsNone();Vm.SelectedPreviewStyleId=AppearanceTab==TEXT("Bar")?DraftPreviewBar:DraftPreviewFrame;Vm.bPreviewActionAvailable=IsActivePage()&&!Vm.SelectedPreviewStyleId.IsNone();
            if(const auto* IdentityWidget=FrontendOwner->AnchorStatusWidget.Get()){Vm.Portrait=IdentityWidget->AvatarTexture;Vm.bPortraitKnown=Vm.Portrait!=nullptr;}
            Vm.PreviewDisabledReason=FText::FromString(TEXT("请选择本地试戴样式"));''')
 old='else if(Id==TEXT("preview-appearance"))Feedback(TEXT("当前外观仅供本地预览，未改变持有状态"));';assert old in s
 s=s.replace(old,'''else if(Id==TEXT("ui.appearance.select-frame")){if(P.PreviewStyleId==TEXT("素墨头像框")||P.PreviewStyleId==TEXT("青金头像框")||P.PreviewStyleId==TEXT("山水头像框"))DraftPreviewFrame=P.PreviewStyleId;}
    else if(Id==TEXT("ui.appearance.select-bar")){if(P.PreviewStyleId==TEXT("普通")||P.PreviewStyleId==TEXT("渐变")||P.PreviewStyleId==TEXT("护盾")||P.PreviewStyleId==TEXT("华丽"))DraftPreviewBar=P.PreviewStyleId;}
    else if(Id==TEXT("preview-appearance")){if(AppearanceTab==TEXT("Bar")&&P.PreviewStyleId==DraftPreviewBar)PreviewBar=DraftPreviewBar;else if(AppearanceTab!=TEXT("Bar")&&P.PreviewStyleId==DraftPreviewFrame)PreviewFrame=DraftPreviewFrame;else return;Feedback(TEXT("本地试戴已更新；未解锁、未装备、未保存"));}''')
 return s
edit(B+'InkRebuildFeaturePlayer.cpp',player,'Restore existing local frame/bar sample selection as presentation-only preview; use actual portrait, no ownership or invented progress')
def owner(s):
 old='PreviewColumn->AddChildToVerticalBox(Card(Tree, Style, IdentityContent))->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));';assert old in s
 s=s.replace(old,'''auto* IdentityPreview=Card(Tree,Style,IdentityContent);FStyleTokens PreviewTokens=Style;
    if(ViewModel.bPreviewFrameKnown)PreviewTokens.Accent=ViewModel.PreviewFrame==TEXT("青金头像框")?FLinearColor(.2f,.44f,.52f,1):ViewModel.PreviewFrame==TEXT("山水头像框")?FLinearColor(.35f,.42f,.32f,1):Style.Gold;
    IdentityPreview->SetBrush(SurfaceBrush(Style.Paper,PreviewTokens,ViewModel.bPreviewFrameKnown?3.f:1.f));PreviewColumn->AddChildToVerticalBox(IdentityPreview)->SetPadding(FMargin(0.f,0.f,0.f,12.f));
    if(ViewModel.bPreviewBarKnown){auto* Swatch=Tree->ConstructWidget<UBorder>();Swatch->SetBrushColor(ViewModel.PreviewBar==TEXT("护盾")?FLinearColor(.25f,.48f,.8f,1):ViewModel.PreviewBar==TEXT("华丽")?FLinearColor(.68f,.46f,.15f,1):ViewModel.PreviewBar==TEXT("渐变")?FLinearColor(.55f,.3f,.6f,1):FLinearColor(.25f,.46f,.45f,1));auto* Bounds=Tree->ConstructWidget<USizeBox>();Bounds->SetHeightOverride(16);Bounds->SetContent(Swatch);PreviewColumn->AddChild(Bounds);PreviewColumn->AddChild(Text(Tree,NSLOCTEXT("InkPlayerC","BarColorOnly","状态条配色预览，不表示气血或进度"),Style.SecondaryFont,Style.Muted));}''')
 s=s.replace(':Style.Gold;',':Style.Accent;')
 return s
edit(B+'Player/InkPlayerMethodsEquipmentAppearanceWidgets.cpp',owner,'Render local frame border and bar color swatch; never fabricate health/progress or alter gameplay appearance state')
def journey(s):
 marker='TArray<FInkJourneyNewDisplayRow> JourneyFacts('
 s=s.replace(marker,'''FString ServiceMonth(const J::FView& View)
{const auto Explicit=D::String(View.Activities,TEXT("month"));if(!Explicit.IsEmpty())return Explicit;FDateTime Day;return FDateTime::ParseIso8601(*(View.ServiceDay+TEXT("T00:00:00Z")),Day)?View.ServiceDay.Left(7):FString();}
'''+marker)
 s=s.replace('M.bEligible=CanDispatch()&&D::Boolean(Row,TEXT("eligible"));','M.bEligible=CalendarOffset==0&&CanDispatch()&&D::Boolean(Row,TEXT("eligible"));')
 s=s.replace('M.Rewards=JourneyFacts(D::Object(Row,TEXT("reward")));Milestones.Add(M);','M.Rewards=JourneyFacts(D::Object(Row,TEXT("reward")));const auto RewardText=D::Text(Row,TEXT("reward"));if(!RewardText.IsEmpty()){FInkJourneyNewDisplayRow Reward;Reward.Label=FText::FromString(TEXT("奖励说明"));Reward.Value=RewardText;M.Rewards.Add(Reward);}if(CalendarOffset==0)Milestones.Add(M);')
 old='''        int32 CalendarDays=0,ServerDay=0;D::Integer(View.Activities,TEXT("calendar_days"),CalendarDays);D::Integer(View.Activities,TEXT("calendar_day"),ServerDay);
        Monthly->SetPresentation(View.ServiceDay,D::String(View.Activities,TEXT("month")),CalendarDays,ServerDay,View.MonthSignCount.IsSet(),View.MonthSignCount.IsSet()?int32(View.MonthSignCount.GetValue()):0,Milestones,Status);''';assert old in s
 s=s.replace(old,'''        int32 CalendarDays=0,ServerDay=0;FString Month=ServiceMonth(View);FDateTime Date;
        if(FDateTime::ParseIso8601(*(Month+TEXT("-01T00:00:00Z")),Date)){const int32 Index=Date.GetYear()*12+Date.GetMonth()-1+FMath::Clamp(CalendarOffset,-12,12);const int32 Year=Index/12,MonthIndex=Index%12+1;CalendarDays=FDateTime::DaysInMonth(Year,MonthIndex);Month=FString::Printf(TEXT("%04d-%02d"),Year,MonthIndex);FDateTime Today;if(CalendarOffset==0&&FDateTime::ParseIso8601(*(View.ServiceDay+TEXT("T00:00:00Z")),Today))ServerDay=Today.GetDay();}
        Monthly->SetPresentation(View.ServiceDay,Month,CalendarDays,ServerDay,CalendarOffset==0&&View.MonthSignCount.IsSet(),View.MonthSignCount.IsSet()?int32(View.MonthSignCount.GetValue()):0,Milestones,CalendarOffset==0?Status:FText::FromString(TEXT("仅浏览日期，历史签到与奖励状态未提供")));''')
 s=s.replace('const auto CurrentMonth=D::String(Current.Activities,TEXT("month"));if(Month!=CurrentMonth)','const auto CurrentMonth=ServiceMonth(Current);if(CalendarOffset!=0||Month!=CurrentMonth)')
 return s
edit(B+'InkRebuildFeatureJourney.cpp',journey,'Derive calendar metadata only from actual service date; preserve missing history and optional month, show actual reward text, gate claims to current subject')
def host(s):
 s=s.replace('Routes={TEXT("SignIn"),TEXT("MonthlySignIn"),TEXT("DailyFortune")};','Routes={TEXT("SignIn"),TEXT("MonthlySignIn"),TEXT("DailyFortune"),TEXT("MonthPrevious"),TEXT("MonthCurrent"),TEXT("MonthNext")};')
 s=s.replace('static const TMap<FName,FString> Names={{','static const TMap<FName,FString> Names={{TEXT("MonthPrevious"),TEXT("浏览上月")},{TEXT("MonthCurrent"),TEXT("返回本月")},{TEXT("MonthNext"),TEXT("浏览下月")},')
 s=s.replace('(Route!=TEXT("QueryItems")||Page==TEXT("Market"))','(Route!=TEXT("QueryItems")||Page==TEXT("Market"))&&(!Route.ToString().StartsWith(TEXT("MonthP"))&&!Route.ToString().StartsWith(TEXT("MonthC"))&&!Route.ToString().StartsWith(TEXT("MonthN"))||Page==TEXT("MonthlySignIn"))')
 # Local calendar intents stay on the same canonical route.
 old='if(Id.StartsWith(TEXT("Open_")))Navigate(FName(*Id.Mid(5)),SelectedEntry);'
 new='if(Id==TEXT("Open_MonthPrevious")||Id==TEXT("Open_MonthNext")||Id==TEXT("Open_MonthCurrent")){CalendarOffset=Id==TEXT("Open_MonthCurrent")?0:FMath::Clamp(CalendarOffset+(Id==TEXT("Open_MonthNext")?1:-1),-12,12);if(auto* Menu=WidgetTree->FindWidget(TEXT("NewFeatureRelatedMenu")))Menu->SetVisibility(ESlateVisibility::Collapsed);RefreshAuthority(true);return;}if(Id.StartsWith(TEXT("Open_")))Navigate(FName(*Id.Mid(5)),SelectedEntry);';assert old in s;s=s.replace(old,new)
 return s
edit(B+'InkRebuildFeatureHost.cpp',host,'Provide local month browsing without synthesizing historical sign-ins or claimable rewards')
