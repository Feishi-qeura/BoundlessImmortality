#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Frontend/UIRebuild20261024/InkRebuildFrontendWidget.h"
#include "Frontend/InkInventoryModel.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkRebuildVisibleInventoryIdentityTest,
    "BoundlessImmortality.Frontend.NewUI.Inventory.VisibleIdentitySurvivesProjectionChanges",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkRebuildVisibleInventoryIdentityTest::RunTest(const FString&)
{
    auto* Frontend=NewObject<UInkRebuildFrontendWidget>();
    auto* Model=Frontend->GetInventoryModel();
    FInkInventoryItem A;A.ItemId=TEXT("VISIBLE_A");A.Name=FText::FromString(TEXT("Z"));A.Count=1;A.bStackable=false;
    FInkInventoryItem B=A;B.ItemId=TEXT("OTHER_B");B.Name=FText::FromString(TEXT("A"));
    Model->AddItem(A);Model->AddItem(B);
    const FGuid VisibleA=Model->FindEntryAtSlot(0)->EntryId;
    const FGuid OtherB=Model->FindEntryAtSlot(1)->EntryId;
    const FName VisibleAction(*(TEXT("InventorySelectId_")+VisibleA.ToString(EGuidFormats::Digits)));
    Model->SortItems(true);
    TestTrue(TEXT("Fixture actually replaces the previously displayed slot"),Model->FindEntryAtSlot(0)->EntryId==OtherB);
    Frontend->TriggerAction(VisibleAction);
    TestTrue(TEXT("Click selects the originally displayed identity after reorder"),Frontend->SelectedInventoryEntry==VisibleA);
    Frontend->SelectedInventoryEntry=OtherB;
    Model->RemoveItem(VisibleA,1);
    Frontend->TriggerAction(VisibleAction);
    TestTrue(TEXT("Removed visible identity never falls through to another slot item"),Frontend->SelectedInventoryEntry==OtherB);
    // Slot zero was rendered empty before a later projection filled it.
    Frontend->TriggerAction(TEXT("InventorySelectEmpty_0"));
    TestTrue(TEXT("An empty-slot intent never selects its later replacement"),Frontend->SelectedInventoryEntry==OtherB);
    Frontend->TriggerAction(TEXT("InventorySelectEmpty_5"));
    TestFalse(TEXT("Still-empty physical slot clears detail selection"),Frontend->SelectedInventoryEntry.IsValid());
    return true;
}
#endif
