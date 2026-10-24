#include "InkFrontendWidget.h"
#include "InkExtensionWidget.h"
#include "InkFeaturePanel.h"
#include "InkAnchorStatusWidget.h"
#include "InkPresentationStyle.h"
#include "UI/InkRewardReceiptPanel.h"
#include "Wuling/WulingPrototypeSubsystem.h"
#include "InkMenuDirector.h"
#include "Engine/GameInstance.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WidgetSwitcherSlot.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Engine/Font.h"

void UInkFrontendWidget::RefreshExtensionActions(){BindTree();}

void UInkFrontendWidget::UpdateRewardReceipts()
{
    if(RewardReceiptPanel||CurrentPage==TEXT("Main")||!PendingConfirmationAction.IsNone()||!GetGameInstance())return;
    auto* Authority=GetGameInstance()->GetSubsystem<UWulingPrototypeSubsystem>();
    if(!Authority||!Authority->IsEnabled()||!Authority->IsConnected())return;
    auto* Canvas=Cast<UCanvasPanel>(GetWidgetFromName(TEXT("SafeCanvas")));if(!Canvas)return;
    const FString Receipt=Authority->TakeNextRewardReceiptJson();if(Receipt.IsEmpty())return;
    auto* ReceiptClass=LoadClass<UInkRewardReceiptPanel>(nullptr,TEXT("/Game/newUI/Ink/Widgets/Shared/WBP_RewardReceipt.WBP_RewardReceipt_C"),nullptr,LOAD_NoWarn|LOAD_Quiet);
    RewardReceiptPanel=CreateWidget<UInkRewardReceiptPanel>(GetOwningPlayer(),ReceiptClass?ReceiptClass:UInkRewardReceiptPanel::StaticClass());
    if(!RewardReceiptPanel)return;
    RewardReceiptPanel->SetReceiptJson(Receipt);RewardReceiptPanel->OnClosed.AddDynamic(this,&ThisClass::HandleRewardReceiptClosed);
    auto* S=Canvas->AddChildToCanvas(RewardReceiptPanel);S->SetAnchors(FAnchors(0,0,1,1));S->SetOffsets(FMargin(0));S->SetZOrder(1000);
    if(Director)Director->SetFrontendInputBlocked(true);RewardReceiptPanel->SetKeyboardFocus();
}

void UInkFrontendWidget::HandleRewardReceiptClosed()
{
    if(!RewardReceiptPanel)return;
    RewardReceiptPanel->RemoveFromParent();RewardReceiptPanel=nullptr;
    if(Director)Director->SetFrontendInputBlocked(CurrentPage!=TEXT("HUD"));
    SetKeyboardFocus();UpdateRewardReceipts();
}

bool UInkFrontendWidget::EnsureFeaturePage(FName Page)
{
    if(ExtensionPages.Contains(Page))return true;
    auto* Pages=Cast<UWidgetSwitcher>(GetWidgetFromName(TEXT("PageSwitcher")));if(!Pages)return false;
    UInkExtensionWidget* Panel=nullptr;
    if(UsesRebuiltPresentation()){Panel=CreateRebuiltFeaturePage(Page);if(!Panel)return false;}
    else{const auto Class=UInkFeaturePanel::ClassForRoute(Page);if(!Class)return false;Panel=CreateWidget<UInkFeaturePanel>(GetOwningPlayer(),Class);}
    if(!Panel)return false;
    ExtensionPages.Add(Page,Panel);Panel->Configure(this,Page);
    auto* LayoutSlot=CastChecked<UWidgetSwitcherSlot>(Pages->AddChild(Panel));LayoutSlot->SetHorizontalAlignment(HAlign_Fill);LayoutSlot->SetVerticalAlignment(VAlign_Fill);
    return true;
}

bool UInkFrontendWidget::OpenFeaturePanel(FName Page,const FString& Context)
{
    // Screen-contract names that share an authority-backed Wuling feature route.
    FString ResolvedContext=Context;
    if(UsesRebuiltPresentation())Page=ResolveRebuiltFeatureRoute(Page,Context);
    else
    {
    if(Page==TEXT("CultivationMethods")){Page=TEXT("WulingEquipment");if(ResolvedContext.IsEmpty())ResolvedContext=TEXT("methods");}
    else if(Page==TEXT("EquipmentFabao"))Page=TEXT("CharacterEquipment");
    else if(Page==TEXT("ItemSources"))Page=TEXT("WulingSources");
    else if(Page==TEXT("GuildConstruction"))Page=TEXT("WulingConstruction");
    else if(Page==TEXT("BreakthroughPrepare")||Page==TEXT("Tribulation"))Page=TEXT("WulingBreakthrough");
    else if(Page==TEXT("OfflineRewards"))Page=TEXT("WulingOffline");
    else if(Page==TEXT("SwordSpirit"))Page=TEXT("WulingRights");
    else if(Page==TEXT("Shop"))Page=TEXT("WulingShop");
    else if(Page==TEXT("Market"))Page=TEXT("WulingMarket");
    else if(Page==TEXT("TradeOrders"))Page=TEXT("WulingOrders");
    else if(Page==TEXT("EventSettlement"))Page=TEXT("WulingWorld");
    else if(Page==TEXT("RaceLobby")||Page==TEXT("RaceLive")||Page==TEXT("RaceSettlement")||Page==TEXT("ResourceLease"))Page=TEXT("WulingRace");
    }
    if(UsesRebuiltPresentation())for(const TCHAR* Core:{TEXT("Main"),TEXT("HUD"),TEXT("Saves"),TEXT("Settings"),TEXT("Character"),TEXT("Inventory"),TEXT("Leaderboard")})
        if(Page==Core){ShowPage(Page);return CurrentPage==Page;}
    const FName Requested=Page==TEXT("SignIn")&&CurrentPage!=TEXT("SignIn")?SignInSelectedRoute:Page;
    if(!UsesRebuiltPresentation()&&(Page==TEXT("MonthlySignIn")||Page==TEXT("DailyFortune")))Page=TEXT("SignIn");
    // Rollout cycle pages are real retained children, with a detail stack of their own.
    if(!UsesRebuiltPresentation()&&!UInkFeaturePanel::ClassForRoute(Page))return false;
    if(CurrentPage==Page){if(auto* Group=Cast<UInkFeatureGroupPanel>(ExtensionPages.FindRef(Page)))Group->SelectRoute(Requested);if(auto* Panel=ExtensionPages.FindRef(Page).Get())Panel->SetContext(ResolvedContext);BindTree();return true;}
    // Returning to an ancestor retires its descendants instead of creating a navigation cycle.
    if(FeaturePanelStack.Contains(Page))
    {
        while(CurrentPage!=Page&&!FeaturePanelStack.IsEmpty())CloseFeaturePanel(false);
        if(auto* Group=Cast<UInkFeatureGroupPanel>(ExtensionPages.FindRef(Page)))Group->SelectRoute(Requested);
        if(auto* Panel=ExtensionPages.FindRef(Page).Get())Panel->SetContext(ResolvedContext);
        BindTree();return CurrentPage==Page;
    }
    if(!EnsureFeaturePage(Page))return false;
    if(auto* Group=Cast<UInkFeatureGroupPanel>(ExtensionPages.FindRef(Page)))Group->SelectRoute(Requested);
    if(ExtensionPages.Contains(CurrentPage))FeaturePanelStack.Add(CurrentPage);
    else{FeaturePanelStack.Reset();if(UsesRebuiltPresentation()&&CurrentPage!=TEXT("HUD"))FeaturePanelStack.Add(CurrentPage);}
    if(auto* Panel=ExtensionPages.FindRef(Page).Get())Panel->SetContext(ResolvedContext);
    ShowPage(Page);return CurrentPage==Page;
}

void UInkFrontendWidget::CloseFeaturePanel(bool bReturnToGame)
{
    const FName Closing=CurrentPage;
    if(!ExtensionPages.Contains(Closing))return;
    const FName Target=bReturnToGame||FeaturePanelStack.IsEmpty()?FName(TEXT("HUD")):FeaturePanelStack.Pop();
    ShowPage(Target);
    auto Retire=[&](FName Id){if(auto* Panel=ExtensionPages.FindRef(Id).Get())Panel->RemoveFromParent();ExtensionPages.Remove(Id);};
    Retire(Closing);
    if(bReturnToGame){const auto Keys=FeaturePanelStack;for(FName Id:Keys)Retire(Id);FeaturePanelStack.Reset();}
    BindTree();
}
