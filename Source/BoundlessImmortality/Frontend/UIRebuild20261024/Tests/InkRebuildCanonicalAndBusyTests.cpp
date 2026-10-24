#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Frontend/UI/InkUiAssetPaths.h"
#include "Frontend/UIRebuild20261024/InkRebuildFrontendWidget.h"
#include "Frontend/UIRebuild20261024/Player/InkPlayerCharacterInventoryWidgets.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkRebuildCanonicalPathsTest,
    "BoundlessImmortality.Frontend.NewUI.Assets.ExactCanonicalPaths",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkRebuildCanonicalPathsTest::RunTest(const FString&)
{
    TestEqual(TEXT("Historical rare chest preserves Rewards destination"),
        InkUiAssets::Resolve(TEXT("/Game/UI/Ink/Gifts/T_Gift_RareChest.T_Gift_RareChest")),
        FString(TEXT("/Game/newUI/Ink/Textures/Rewards/T_Gift_RareChest.T_Gift_RareChest")));
    TestEqual(TEXT("Former physical body font resolves new root"),
        InkUiAssets::Resolve(TEXT("/Game/UI/Ink/Fonts/Shared/F_InkBody.F_InkBody")),
        FString(TEXT("/Game/newUI/Ink/Fonts/Shared/F_InkBody.F_InkBody")));
    TestEqual(TEXT("Dynamic atlas uses finite canonical index"),InkUiAssets::ResolveNamed(TEXT("T_InkMeditation")),
        FString(TEXT("/Game/newUI/Ink/Textures/Cultivation/T_InkMeditation.T_InkMeditation")));
    TestTrue(TEXT("Unknown dynamic name cannot synthesize an asset"),InkUiAssets::ResolveNamed(TEXT("UnknownAssetForRegression")).IsEmpty());
    TestTrue(TEXT("Ambiguous particle name requires exact family"),InkUiAssets::ResolveNamed(TEXT("NS_HonorParticles")).IsEmpty());
    for(const TCHAR* Kind:{TEXT("Honor"),TEXT("Realm"),TEXT("RealmIcon"),TEXT("AvatarFrame")})
    {
        const FString Name=FString(TEXT("NS_"))+Kind+TEXT("Particles");
        const FString Exact=InkUiAssets::Resolve(*(TEXT("/Game/UI/Ink/NiagaraRevision20261022/Flowing20261022/")+Name+TEXT(".")+Name));
        TestEqual(TEXT("Flowing title particle family preserves original identity"),InkUiAssets::TitleParticlesPath(Name),Exact);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkRebuildInventoryBusyTest,
    "BoundlessImmortality.Frontend.NewUI.Inventory.BusyRetainsConfirmedPresentation",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkRebuildInventoryBusyTest::RunTest(const FString&)
{
    auto* Frontend=NewObject<UInkRebuildFrontendWidget>();auto* Model=Frontend->GetInventoryModel();
    FInkInventoryItem Item;Item.ItemId=TEXT("BUSY_CONFIRMED");Item.Name=FText::FromString(TEXT("Confirmed inventory row"));
    Item.Category=TEXT("Consumable");Item.Count=3;Item.bUsable=true;Item.bSellable=true;
    Model->AddItem(Item,3);const FGuid Selected=Model->FindEntryAtSlot(0)->EntryId;
    for(const bool Busy:{false,true})
    {
        auto* Tree=NewObject<UWidgetTree>(Frontend);
        Tree->RootWidget=InkPlayerCharacterInventoryWidgets::BuildInventoryPage(Tree,Frontend,Model,Selected,
            TEXT("All"),FString(),0,1,true,20,nullptr,InkPlayerCharacterInventoryWidgets::EProviderState::Data,
            Busy?FText::FromString(TEXT("Request pending")):FText(),672,Busy);
        TArray<UWidget*> Widgets;Tree->GetAllWidgets(Widgets);bool HasConfirmedLabel=false,HasFilter=false,HasMutation=false;
        for(auto* Widget:Widgets)
        {
            if(auto* Label=Cast<UTextBlock>(Widget))HasConfirmedLabel|=Label->GetText().EqualTo(Item.Name);
            if(auto* Button=Cast<UInkFrontendActionButton>(Widget))
            {
                if(Button->Action==TEXT("FilterAll")){HasFilter=true;TestTrue(TEXT("Browsing remains available during pending request"),Button->GetIsEnabled());}
                if(Button->Action==TEXT("InventorySell")){HasMutation=true;TestEqual(TEXT("Only mutation follows busy gate"),Button->GetIsEnabled(),!Busy);}
            }
        }
        TestTrue(TEXT("Confirmed selected item detail stays rendered"),HasConfirmedLabel);
        TestTrue(TEXT("Filter controls remain present"),HasFilter);TestTrue(TEXT("Selected mutation remains visible"),HasMutation);
        TestTrue(TEXT("Confirmed model identity and count are unchanged"),Model->FindEntry(Selected)&&Model->FindEntry(Selected)->Item.Count==3);
    }
    return true;
}
#endif
