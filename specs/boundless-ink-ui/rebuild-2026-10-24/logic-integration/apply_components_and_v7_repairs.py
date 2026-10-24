from pathlib import Path
import runpy
R=Path.cwd();H=R/'specs/boundless-ink-ui/rebuild-2026-10-24/logic-integration';B='Source/BoundlessImmortality/Frontend/UIRebuild20261024/'
rec=runpy.run_path(str(R/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/record_source_change.py'),run_name='component_and_v7_repairs')
def edit(p,fn,reason):
 t=R/p;s=t.read_text(encoding='utf-8-sig');n=fn(s);assert s!=n,p;rec['begin'](p,reason);t.write_text(n,encoding='utf-8');rec['finish'](p)
def host(s):
 marker='    if(auto* Activity=Cast<URebuildActivityPageWidget>(OwnerPage))';assert marker in s
 s=s.replace(marker,'''    if(PageId==TEXT("OfflineRewards")&&!WidgetTree->FindWidget(TEXT("NewFeatureBack")))InkRebuildUI::Action(WidgetTree,RootCanvas,FrontendOwner,TEXT("NewFeatureBack"),TEXT("Ext_Back"),FText::FromString(TEXT("返回")),FVector4(LastSize.X-112,24,88,88));
'''+marker)
 return s
edit(B+'InkRebuildFeatureHost.cpp',host,'PIE05 actual OfflineRewards screen has no Back control; add one real host button with existing back stack')
for name in ['Activities/RebuildActivitySecondaryWidgets.cpp','Activities/RebuildEconomyWidgets.cpp']:
 edit(B+name,lambda s:s.replace('FLinearColor Color = FLinearColor::White)','FLinearColor Color = InkRebuildUI::Ink())'),'PIE05 visible contrast: default text ink on paper; preserve explicit colored button labels')
def rank(s):
 marker='FInkWulingOverviewData Vm;Vm.ServiceStatus=Status;';assert marker in s
 return s.replace(marker,'FInkWulingOverviewData Vm;Vm.Insignia=LoadObject<UTexture2D>(nullptr,TEXT("/Game/newUI/Components/RankWuling/T_NUI_Wuling_SectInsignia_Normal_V1.T_NUI_Wuling_SectInsignia_Normal_V1"));Vm.ServiceStatus=Status;').replace('#include "Wuling/WulingPrototypeSubsystem.h"','#include "Wuling/WulingPrototypeSubsystem.h"\n#include "Engine/Texture2D.h"')
edit(B+'InkRebuildFeatureRank.cpp',rank,'Bind current candidate Rank insignia v3 through existing live UMG Insignia field; no superseded or whole-page art')
def data(s):
 marker='inline FObject Catalog(';assert marker in s
 return s.replace(marker,'''inline FString StageSubjectIdentity(const FObject& State)
{const auto Stage=Object(Object(State,TEXT("breakthrough_quote")),TEXT("stage"));return String(Stage,TEXT("stage_id"))+TEXT("|")+Version(Stage,TEXT("from_realm_index"))+TEXT("|")+Version(Stage,TEXT("to_realm_index"));}
inline bool ConfirmedStageMatches(const FObject& State,const FString& Captured)
{return Captured==StageSubjectIdentity(State);}
'''+marker)
edit(B+'InkRebuildFeatureData.h',data,'Use exact native stage identity tuple at confirmation and final dispatch; optional display data is excluded')
edit(B+'InkRebuildFeatureHost.h',lambda s:s.replace('FString PendingGuildOperation,','FString ConfirmedStageFingerprint;\n    FString PendingGuildOperation,'),'Capture exact confirmed stage tuple alongside local selection/mode revisions')
def player(s):
 s=s.replace('#include "Wuling/WulingPrototypeSubsystem.h"','#include "Wuling/WulingPrototypeSubsystem.h"\n#include "Engine/Texture2D.h"')
 marker='C::FPageAtlases Art;';assert marker in s
 s=s.replace(marker,'''C::FPageAtlases Art;
        Art.MethodsGlyphs=LoadObject<UTexture2D>(nullptr,TEXT("/Game/newUI/Components/Player/T_MethodLoadoutGlyphAtlas_v1.T_MethodLoadoutGlyphAtlas_v1"));
        Art.EquipmentGlyphs=LoadObject<UTexture2D>(nullptr,TEXT("/Game/newUI/Components/Player/T_EquipmentFabaoGlyphAtlas_v1.T_EquipmentFabaoGlyphAtlas_v1"));
        Art.AppearanceOrnaments=LoadObject<UTexture2D>(nullptr,TEXT("/Game/newUI/Components/Player/T_AppearancePreviewOrnamentAtlas_v1.T_AppearancePreviewOrnamentAtlas_v1"));
        Art.CharacterAttributeGlyphs=LoadObject<UTexture2D>(nullptr,TEXT("/Game/newUI/Components/Player/T_CharacterAttributeGlyphAtlas_v1.T_CharacterAttributeGlyphAtlas_v1"));''')
 s=s.replace('Vm.FabaoName=D::Text(Equipment,TEXT("fabao"));','Vm.FabaoName=Vm.bFabaoKnown&&D::String(Equipment,TEXT("fabao")).IsEmpty()?FText::FromString(TEXT("未装备法宝")):D::Text(Equipment,TEXT("fabao"));')
 s=s.replace('const FString Fingerprint=D::String(Stage,TEXT("stage_id"))+TEXT("|")+D::Version(Stage,TEXT("from_realm_index"))+TEXT("|")+D::Version(Stage,TEXT("to_realm_index"));','const FString Fingerprint=D::StageSubjectIdentity(DisplaySnapshot);')
 old='if(Id==TEXT("ui.page.back")){HandleBack();return;}RefreshAuthority(true);';assert s.count(old)==1
 s=s.replace(old,'''if(Id==TEXT("ui.page.back")){HandleBack();return;}
    const bool Boundary=Id==TEXT("ui.breakthrough.request-confirmation")||Id==TEXT("prepare-breakthrough")||Id==TEXT("rework-quality");
    if(Boundary&&D::StageSubjectIdentity(Service()?Service()->Snapshot():nullptr)!=StageFingerprint){bConfirmOpen=false;ConfirmedStageFingerprint.Empty();Feedback(TEXT("突破阶段已变化，请重新查看并确认"));RefreshAuthority(true);return;}
    RefreshAuthority(true);''')
 s=s.replace('ConfirmedMaterialSelection=MaterialSelection;ConfirmedSelectionRevision=SelectionRevision;','ConfirmedStageFingerprint=D::StageSubjectIdentity(Service()?Service()->Snapshot():nullptr);ConfirmedMaterialSelection=MaterialSelection;ConfirmedSelectionRevision=SelectionRevision;')
 old='if(!bConfirmOpen||ConfirmedSelectionRevision!=SelectionRevision||P.SelectionRevision!=SelectionRevision';assert old in s
 s=s.replace(old,'if(!D::ConfirmedStageMatches(Service()?Service()->Snapshot():nullptr,ConfirmedStageFingerprint)){bConfirmOpen=false;Feedback(TEXT("确认的突破阶段已失效，请重新确认"));RefreshAuthority(true);return;}\n        if(!bConfirmOpen||ConfirmedSelectionRevision!=SelectionRevision||P.SelectionRevision!=SelectionRevision')
 return s
edit(B+'InkRebuildFeaturePlayer.cpp',player,'Apply all Player-c candidate atlases plus existing attribute atlas; V7-CONFIRM-01 exact freshest stage tuple guards both confirmation and dispatch')
def ecolumns(s):
 s=s.replace('#include "Components/HorizontalBox.h"','#include "Components/HorizontalBox.h"\n#include "Components/HorizontalBoxSlot.h"')
 for name in ['Left','Right','StatusColumn','SettlementColumn']:
  old='Columns->AddChildToHorizontalBox('+name+');';assert old in s
  s=s.replace(old,'if(auto* ColumnLayout=Columns->AddChildToHorizontalBox('+name+')){ColumnLayout->SetSize(FSlateChildSize(ESlateSizeRule::Fill));ColumnLayout->SetPadding(FMargin(6,0));}')
 return s
edit(B+'Player/InkPlayerBreakthroughTribulationWidgets.cpp',ecolumns,'Actual landscape capture leaves half the viewport unused; fill both native columns at available width')
def activityheader(s):
 at=s.index('struct FRebuildActivitiesHubViewModel');before=s[:at];after=s[at:];after=after.replace('    UPROPERTY(BlueprintReadWrite) FString ContentKey;','    UPROPERTY(BlueprintReadWrite) FString ContentKey;\n    UPROPERTY(BlueprintReadWrite) FName RouteCycle;',1);return before+after
edit(B+'Activities/RebuildActivityWidgets.h',activityheader,'Carry explicit route cycle independently of service category lists')
def activityhost(s):
 marker='Vm.bCategoriesProvided=View.Templates.bProvided||D::Has(View.Activities,TEXT("categories"));Vm.Categories=Categories;Vm.bHasFeaturedLaunch='
 assert marker in s
 return s.replace(marker,'Vm.bCategoriesProvided=View.Templates.bProvided||D::Has(View.Activities,TEXT("categories"));Vm.RouteCycle=PageId==TEXT("DailyActivities")?FName(TEXT("daily")):PageId==TEXT("TimedActivities")?FName(TEXT("limited")):NAME_None;Vm.Categories=Categories;if(!Vm.RouteCycle.IsNone())Vm.Categories.RemoveAll([&](const auto& Category){return Category.Cycle!=Vm.RouteCycle;});Vm.bHasFeaturedLaunch=')
edit(B+'InkRebuildFeatureActivities.cpp',activityhost,'V7-CYCLE-01: Daily/Timed hubs project only their exact cycle; Activities retains the full category hub')
def activityowner(s):
 s=s.replace('#include "Components/ProgressBar.h"','#include "Components/ProgressBar.h"\n#include "Components/Image.h"\n#include "Engine/Texture2D.h"')
 marker='FText CategoryName(';assert marker in s
 s=s.replace(marker,'''UWidget* CategoryBadge(UWidgetTree* Tree,const FText& Label,FName Cycle,FLinearColor Color)
{
    const TCHAR* Path=Cycle==TEXT("daily")?TEXT("/Game/newUI/Activities/Icons/T_DailyActivityScroll_v1.T_DailyActivityScroll_v1"):Cycle==TEXT("limited")?TEXT("/Game/newUI/Activities/Icons/T_LimitedActivityCalendar_v1.T_LimitedActivityCalendar_v1"):TEXT("/Game/newUI/Activities/Icons/T_PermanentActivityLotus_v1.T_PermanentActivityLotus_v1");
    auto* Body=Vertical(Tree);if(auto* Texture=LoadObject<UTexture2D>(nullptr,Path)){auto* Icon=Tree->ConstructWidget<UImage>();Icon->SetBrushFromTexture(Texture);Icon->SetDesiredSizeOverride(FVector2D(72));if(auto* Layout=Body->AddChildToVerticalBox(Icon))Layout->SetHorizontalAlignment(HAlign_Center);}Add(Body,Text(Tree,Label,22,Color,ETextJustify::Center),0);return Body;
}
'''+marker)
 at=s.index('void URebuildActivitiesHubWidget::ApplyViewModel');end=s.index('\nvoid ',at+8);part=s[at:end];part=part.replace('    ViewModel = Model;','    ViewModel = Model;if(!Model.RouteCycle.IsNone())SelectedCycle=Model.RouteCycle;');s=s[:at]+part+s[end:]
 s=s.replace('FText::FromString(TEXT("活动")), ViewModel.ServiceStateText','FText::FromString(ViewModel.RouteCycle==TEXT("daily")?TEXT("每日活动"):ViewModel.RouteCycle==TEXT("limited")?TEXT("限时活动"):TEXT("活动")), ViewModel.ServiceStateText')
 old='Card(WidgetTree, Text(WidgetTree, Label, 22, Cycle == SelectedCycle ? White : Ink, ETextJustify::Center),';assert old in s
 s=s.replace(old,'Card(WidgetTree, CategoryBadge(WidgetTree,Label,Cycle,Cycle==SelectedCycle?White:Ink),')
 return s
edit(B+'Activities/RebuildActivityWidgets.cpp',activityowner,'Apply explicit cycle identity and three exact semantic icons to live native category buttons; no concept page backgrounds')
