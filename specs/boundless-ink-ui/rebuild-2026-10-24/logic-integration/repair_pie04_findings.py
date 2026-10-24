from pathlib import Path
import runpy,re
R=Path.cwd();H=R/'specs/boundless-ink-ui/rebuild-2026-10-24/logic-integration';B='Source/BoundlessImmortality/Frontend/UIRebuild20261024/'
rec=runpy.run_path(str(R/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/record_source_change.py'),run_name='actual_pie04_findings')
def edit(p,fn,reason):
 t=R/p;s=t.read_text(encoding='utf-8-sig');n=fn(s);assert s!=n,p;rec['begin'](p,reason);t.write_text(n,encoding='utf-8');rec['finish'](p)
def journey(s):
 for typ in ['UTextBlock','UBorder','UButton','UVerticalBox','UScrollBox','UWrapBox']:
  old='T->ConstructWidget<'+typ+'>('+typ+'::StaticClass(),Name)';new='T->ConstructWidget<'+typ+'>('+typ+'::StaticClass(),MakeUniqueObjectName(T,'+typ+'::StaticClass(),FName(Name)))';s=s.replace(old,new)
 for typ in ['UInkJourneyMapPinButton','UInkJourneyMilestoneButton','UInkJourneyRaceActionButton','USizeBox']:
  pattern=r'WidgetTree->ConstructWidget<'+typ+r'>\('+typ+r'::StaticClass\(\),\*FString::Printf\(TEXT\("[^"\n]*"\),[^\n]*?\)\)'
  s=re.sub(pattern,'WidgetTree->ConstructWidget<'+typ+'>()',s)
 old='void UInkJourneyMapPageWidget::NativeTick(const FGeometry& G,float Delta){ Super::NativeTick(G,Delta); const FVector2D S=G.GetLocalSize(); if(S.X>0&&S.Y>0) LayoutResponsive(S); }';assert old in s
 s=s.replace(old,'''void UInkJourneyMapPageWidget::NativeTick(const FGeometry& G,float Delta)
{
 Super::NativeTick(G,Delta);const FVector2D S=G.GetLocalSize();if(S.X<=0||S.Y<=0)return;LayoutResponsive(S);const bool Wide=S.X>S.Y;
 const float MapWidth=Wide?FMath::Max(200.f,S.X*.60f):FMath::Min(435.f,S.X-24),MapHeight=Wide?FMath::Max(160.f,S.Y-130):FMath::Min(650.f,FMath::Max(160.f,(S.Y-220)*.52f));
 auto* Box=Cast<USizeBox>(GetWidgetFromName(Wide?TEXT("JourneyLandscapeMapBox"):TEXT("JourneyPortraitMapBox")));
 if(Box&&(!FMath::IsNearlyEqual(Box->GetWidthOverride(),MapWidth,1.f)||!FMath::IsNearlyEqual(Box->GetHeightOverride(),MapHeight,1.f)))
 {Box->SetWidthOverride(MapWidth);Box->SetHeightOverride(MapHeight);if(Wide){if(auto* Detail=Cast<USizeBox>(GetWidgetFromName(TEXT("JourneyLandscapeDetailBox")))){Detail->SetWidthOverride(FMath::Max(180.f,S.X-MapWidth));Detail->SetHeightOverride(MapHeight);}BuildMapPins(LandscapeMapCanvas,LandscapePins,MapWidth,MapHeight,true);}else BuildMapPins(PortraitMapCanvas,PortraitPins,MapWidth,MapHeight,false);}
 if(PortraitActionButton)PortraitActionButton->SetVisibility(Wide?ESlateVisibility::Collapsed:ESlateVisibility::Visible);
}''')
 s=s.replace('Root->AddChild(LayoutSwitcher);','if(auto* Layout=Cast<UVerticalBoxSlot>(Root->AddChild(LayoutSwitcher)))Layout->SetSize(FSlateChildSize(ESlateSizeRule::Fill));')
 s=s.replace('PortraitLayout->AddChild(InkJourneyNew::Card(WidgetTree,TEXT("JourneyPortraitDetailFrame"),PortraitDetails));','if(auto* Layout=Cast<UVerticalBoxSlot>(PortraitLayout->AddChild(InkJourneyNew::Card(WidgetTree,TEXT("JourneyPortraitDetailFrame"),PortraitDetails))))Layout->SetSize(FSlateChildSize(ESlateSizeRule::Fill));')
 s=s.replace('PortraitDetails->AddChild(PortraitDetailRows);','if(auto* Layout=Cast<UVerticalBoxSlot>(PortraitDetails->AddChild(PortraitDetailRows)))Layout->SetSize(FSlateChildSize(ESlateSizeRule::Fill));')
 s=s.replace('LandscapeDetails->AddChild(LandscapeDetailRows);','if(auto* Layout=Cast<UVerticalBoxSlot>(LandscapeDetails->AddChild(LandscapeDetailRows)))Layout->SetSize(FSlateChildSize(ESlateSizeRule::Fill));')
 s=s.replace('LandscapeLayout->AddChild(LMapBox);','if(auto* Layout=Cast<UHorizontalBoxSlot>(LandscapeLayout->AddChild(LMapBox)))Layout->SetVerticalAlignment(VAlign_Top);')
 return s
edit(B+'Journey/InkJourneyNewWidgets.cpp',journey,'PIE04 observed repeated map rows: unique transient row/pin identities, bounded map/details, and one active portrait/landscape travel control')
p=H/'verify_all44_default_pie.py';s=p.read_text(encoding='utf-8-sig');marker='def click(action):';at=s.index(marker)
s=s[:at]+'''def scroll_action(action):
    snapshot=describe();matches=[x for x in snapshot.get('buttons',[]) if x.get('action')==action and x.get('visible')]
    check('one real action target before scroll '+action,len(matches)==1,matches)
    check('real scroll to action '+action,LIB.scroll_to_frontend_widget(FRONTEND,matches[0]['name']))

'''+s[at:]
s=s.replace("('Character native feature entry '+route,lambda r=route:click('Open'+r),1),","('scroll Character feature entry '+route,lambda r=route:scroll_action('Open'+r),1),\n        ('Character native feature entry '+route,lambda r=route:click('Open'+r),1),")
p.write_text(s,encoding='utf-8')
