from pathlib import Path
import runpy,re
R=Path.cwd();H=R/'specs/boundless-ink-ui/rebuild-2026-10-24/logic-integration';B='Source/BoundlessImmortality/Frontend/UIRebuild20261024/'
rec=runpy.run_path(str(R/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/record_source_change.py'),run_name='visible_entry_completion')
def edit(p,fn,reason):
 t=R/p;s=t.read_text(encoding='utf-8-sig');n=fn(s);assert s!=n,p;rec['begin'](p,reason);t.write_text(n,encoding='utf-8');rec['finish'](p)
def hud(s):
 s=s.replace('#include "Components/SizeBox.h"','#include "Components/SizeBox.h"\n#include "Components/ScrollBox.h"')
 old='Menu->SetContent(MenuBody);';assert s.count(old)==1
 s=s.replace(old,'''auto* MenuScroll=Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("NewHudFeatureMenuScroll"));MenuScroll->AddChild(MenuBody);
    auto* MenuBounds=Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("NewHudFeatureMenuBounds"));MenuBounds->SetWidthOverride(300);MenuBounds->SetHeightOverride(420);MenuBounds->SetContent(MenuScroll);Menu->SetContent(MenuBounds);''')
 s=s.replace('const FName MenuActions[] = {TEXT("OpenSaves"), TEXT("OpenSettings"), TEXT("ReturnToMainMenu")};','const FName MenuActions[] = {TEXT("OpenExtensions"),TEXT("OpenActivities"),TEXT("OpenSignIn"),TEXT("OpenWuling"),TEXT("OpenSwordSpirit"),TEXT("OpenSaves"), TEXT("OpenSettings"), TEXT("ReturnToMainMenu")};')
 s=s.replace('const FText MenuLabels[] = {','const FText MenuLabels[] = {FText::FromString(TEXT("仙途")),FText::FromString(TEXT("活动")),FText::FromString(TEXT("签到")),FText::FromString(TEXT("武灵")),FText::FromString(TEXT("剑灵")),')
 return s
edit(B+'Player/InkPlayerVisibleEntryWidgets.cpp',hud,'Expose actual new feature pages from default HUD menu in a bounded scroll surface')
def host(s):
 s=s.replace('#include "Components/TextBlock.h"','#include "Components/TextBlock.h"\n#include "Components/SizeBox.h"\n#include "Components/ScrollBox.h"')
 marker='const FName RebuiltFeatureRoutes[]='
 helper='''TArray<TPair<FName,FText>> RelatedRoutes(FName Page)
{
    TArray<FName> Routes;
    const FString Id=Page.ToString();
    if(Id.Contains(TEXT("Race")))Routes={TEXT("RaceLobby"),TEXT("RaceLive"),TEXT("RaceSettlement"),TEXT("ResourceLease")};
    else if(Id.Contains(TEXT("Resource")))Routes={TEXT("ResourceContest"),TEXT("ResourceDetail"),TEXT("RaceLobby"),TEXT("ResourceLease"),TEXT("ResourceRanking"),TEXT("ResourceRewards")};
    else if(Page==TEXT("WorldCultivation")||Page==TEXT("RegionDetail")||Page==TEXT("EventSettlement"))Routes={TEXT("WorldCultivation"),TEXT("RegionDetail"),TEXT("EventSettlement")};
    else if(Page==TEXT("SignIn")||Page==TEXT("MonthlySignIn")||Page==TEXT("DailyFortune"))Routes={TEXT("SignIn"),TEXT("MonthlySignIn"),TEXT("DailyFortune")};
    else if(Id.Contains(TEXT("Activit"))||Id.Contains(TEXT("Launch")))Routes={TEXT("Activities"),TEXT("DailyActivities"),TEXT("TimedActivities"),TEXT("LaunchEvent"),TEXT("LaunchRank")};
    else if(Page==TEXT("BreakthroughPrepare")||Page==TEXT("Tribulation"))Routes={TEXT("BreakthroughPrepare"),TEXT("Tribulation")};
    else if(Page==TEXT("EquipmentFabao")||Page==TEXT("CultivationMethods")||Page==TEXT("CharacterAppearance"))Routes={TEXT("EquipmentFabao"),TEXT("CultivationMethods"),TEXT("CharacterAppearance")};
    else if(Page==TEXT("WulingCraft")||Page==TEXT("ItemSources"))Routes={TEXT("WulingCraft"),TEXT("ItemSources")};
    else if(Page==TEXT("Shop")||Page==TEXT("Market")||Page==TEXT("TradeOrders"))Routes={TEXT("Shop"),TEXT("Market"),TEXT("TradeOrders"),TEXT("QueryItems")};
    else if(Page==TEXT("GiftRules")||Page==TEXT("SwordSpirit"))Routes={TEXT("SwordSpirit"),TEXT("GiftRules")};
    static const TMap<FName,FString> Names={{TEXT("RaceLobby"),TEXT("竞采大厅")},{TEXT("RaceLive"),TEXT("竞采现场")},{TEXT("RaceSettlement"),TEXT("竞采结算")},{TEXT("ResourceLease"),TEXT("开采权")},{TEXT("ResourceContest"),TEXT("资源地图")},{TEXT("ResourceDetail"),TEXT("资源详情")},{TEXT("ResourceRanking"),TEXT("资源排名")},{TEXT("ResourceRewards"),TEXT("资源奖励")},{TEXT("WorldCultivation"),TEXT("世界地图")},{TEXT("RegionDetail"),TEXT("地域详情")},{TEXT("EventSettlement"),TEXT("事件结算")},{TEXT("SignIn"),TEXT("每日签到")},{TEXT("MonthlySignIn"),TEXT("月签奖励")},{TEXT("DailyFortune"),TEXT("每日签文")},{TEXT("Activities"),TEXT("活动中心")},{TEXT("DailyActivities"),TEXT("每日活动")},{TEXT("TimedActivities"),TEXT("限时活动")},{TEXT("LaunchEvent"),TEXT("青云初启")},{TEXT("LaunchRank"),TEXT("活动贡献榜")},{TEXT("BreakthroughPrepare"),TEXT("突破准备")},{TEXT("Tribulation"),TEXT("天劫进程")},{TEXT("EquipmentFabao"),TEXT("装备法宝")},{TEXT("CultivationMethods"),TEXT("功法")},{TEXT("CharacterAppearance"),TEXT("外观")},{TEXT("WulingCraft"),TEXT("物品炼制")},{TEXT("ItemSources"),TEXT("物品来源")},{TEXT("Shop"),TEXT("坊市商店")},{TEXT("Market"),TEXT("寄售")},{TEXT("TradeOrders"),TEXT("交易订单")},{TEXT("QueryItems"),TEXT("读取可寄售物品")},{TEXT("SwordSpirit"),TEXT("剑灵")},{TEXT("GiftRules"),TEXT("礼物规则")}};
    TArray<TPair<FName,FText>> Out;for(FName Route:Routes)if(Route!=Page)Out.Emplace(Route,FText::FromString(Names.FindRef(Route)));return Out;
}
'''
 s=s.replace(marker,helper+marker)
 s=re.sub(r'    if\(PageId==TEXT\("Market"\)&&!WidgetTree->FindWidget\(TEXT\("NewFeatureQuerySell"\)\)\).*?;\n','',s)
 s=s.replace('    Move(TEXT("NewFeatureQuerySell"),FVector4(24,LastSize.Y-88,300,88));','''    Move(TEXT("NewFeatureRelatedToggle"),FVector4(FMath::Max(24.f,LastSize.X-224),LastSize.Y-88,200,88));
    Move(TEXT("NewFeatureRelatedMenu"),FVector4(FMath::Max(24.f,LastSize.X-344),FMath::Max(24.f,LastSize.Y-508),FMath::Min(320.f,LastSize.X-48),400));''')
 old='    if(OwnerPage||PurePage||!FrontendOwner||!GetOwningPlayer())return;'
 assert old in s
 s=s.replace(old,'''    const auto Routes=RelatedRoutes(PageId);
    if(!Routes.IsEmpty()&&FrontendOwner&&!WidgetTree->FindWidget(TEXT("NewFeatureRelatedMenu")))
    {
        InkRebuildUI::Action(WidgetTree,RootCanvas,FrontendOwner,TEXT("NewFeatureRelatedToggle"),TEXT("Ext_ToggleRelated"),FText::FromString(TEXT("相关页面")),FVector4(LastSize.X-224,LastSize.Y-88,200,88));
        auto* Menu=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("NewFeatureRelatedMenu"));InkRebuildUI::Place(RootCanvas,Menu,FVector4(LastSize.X-344,LastSize.Y-508,320,400),50);
        InkRebuildUI::Card(WidgetTree,Menu,TEXT("NewFeatureRelatedPaper"),FVector4(0,0,320,400));
        auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("NewFeatureRelatedScroll"));InkRebuildUI::Place(Menu,Scroll,FVector4(8,8,304,384));
        auto* Extent=WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("NewFeatureRelatedExtent"));Extent->SetWidthOverride(296);Extent->SetHeightOverride(Routes.Num()*100);Scroll->AddChild(Extent);
        auto* Choices=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("NewFeatureRelatedChoices"));Extent->SetContent(Choices);
        for(int32 Index=0;Index<Routes.Num();++Index)InkRebuildUI::Action(WidgetTree,Choices,FrontendOwner,FName(*FString::Printf(TEXT("NewRelated_%d"),Index)),Routes[Index].Key==TEXT("QueryItems")?FName(TEXT("Ext_QueryItems")):FName(*(TEXT("Ext_Open_")+Routes[Index].Key.ToString())),Routes[Index].Value,FVector4(0,Index*100,296,88));
        Menu->SetVisibility(ESlateVisibility::Collapsed);
    }
'''+old)
 s=s.replace('const FString Id=Action.ToString();if(Id==TEXT("QueryItems"))','const FString Id=Action.ToString();if(Id==TEXT("ToggleRelated")){if(auto* Menu=WidgetTree->FindWidget(TEXT("NewFeatureRelatedMenu")))Menu->SetVisibility(Menu->IsVisible()?ESlateVisibility::Collapsed:ESlateVisibility::Visible);return;}if(Id==TEXT("QueryItems"))')
 s=s.replace('Navigate(FName(*Id.Mid(5)))','Navigate(FName(*Id.Mid(5)),SelectedEntry)')
 s=s.replace('{if(FrontendOwner)FrontendOwner->OpenFeaturePanel(Route,Context);}','{if(auto* Menu=WidgetTree->FindWidget(TEXT("NewFeatureRelatedMenu")))Menu->SetVisibility(ESlateVisibility::Collapsed);if(FrontendOwner)FrontendOwner->OpenFeaturePanel(Route,Context);}')
 return s
edit(B+'InkRebuildFeatureHost.cpp',host,'Expose related canonical pages through a bounded native popup; preserve actual selected entry context and back stack')
