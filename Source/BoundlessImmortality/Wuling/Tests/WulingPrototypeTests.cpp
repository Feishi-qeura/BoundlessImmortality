#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Wuling/WulingSnapshotProjection.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Frontend/InkFeaturePanel.h"
#include "Frontend/InkInventoryModel.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWulingProjectionTest,"BoundlessImmortality.Wuling.AuthorityProjection",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWulingProjectionTest::RunTest(const FString&)
{
    auto* GI=NewObject<UGameInstance>();auto* C=NewObject<UCultivationSubsystem>(GI);C->bPersistLocalRecords=false;
    C->RegisterParticipant(TEXT("anchor"),TEXT("主播"),TEXT(""),true);
    C->SetWulingAuthorityEnabled(true);
    auto Root=MakeShared<FJsonObject>();auto Manifest=MakeShared<FJsonObject>();
    Manifest->SetStringField(TEXT("rule_version"),TEXT("wuling-prototype-v1.4.0"));Manifest->SetStringField(TEXT("profile"),TEXT("prototype_full"));
    Root->SetObjectField(TEXT("manifest"),Manifest);Root->SetNumberField(TEXT("version"),1);
    auto P=MakeShared<FJsonObject>();P->SetStringField(TEXT("user"),TEXT("prototype-anchor"));P->SetStringField(TEXT("role"),TEXT("anchor"));
    P->SetNumberField(TEXT("realm_index"),2);P->SetNumberField(TEXT("qi"),88);P->SetNumberField(TEXT("qi_to_next"),120);P->SetNumberField(TEXT("material_count"),4);P->SetNumberField(TEXT("material_cost"),20);
    auto A=MakeShared<FJsonObject>();A->SetNumberField(TEXT("attack"),77);A->SetNumberField(TEXT("max_health"),1234);A->SetNumberField(TEXT("defense"),33);P->SetObjectField(TEXT("attributes"),A);
    Root->SetArrayField(TEXT("records"),{MakeShared<FJsonValueObject>(P)});
    TestTrue(TEXT("validated authority record applies"),WulingSnapshotProjection::Apply(*C,Root,TEXT("prototype-anchor")));
    TestEqual(TEXT("anchor realm mirrors authority"),C->GetParticipantSnapshot(TEXT("anchor")).RealmIndex,2);
    TestEqual(TEXT("anchor threshold uses service half cost"),C->GetParticipantSnapshot(TEXT("anchor")).RequiredCultivation,120.0);
    TestEqual(TEXT("displayed attack mirrors actual authority damage attributes"),C->GetParticipantSnapshot(TEXT("anchor")).Attributes.Attack,77.0);
    TestFalse(TEXT("local xp cannot double settle"),C->AddCultivation(TEXT("anchor"),500));
    TestFalse(TEXT("local breakthrough cannot spend server materials"),C->TryAnchorBreakthrough());
    TestEqual(TEXT("mirror remains authoritative"),C->GetParticipantSnapshot(TEXT("anchor")).Cultivation,88.0);
    Manifest->SetStringField(TEXT("profile"),TEXT("douyin_release_candidate"));
    TestFalse(TEXT("wrong profile cannot project"),WulingSnapshotProjection::Apply(*C,Root,TEXT("prototype-anchor")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWulingFunctionalRoutesTest,"BoundlessImmortality.Wuling.FunctionalRoutes",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWulingFunctionalRoutesTest::RunTest(const FString&)
{
    for(const TCHAR* Route:{TEXT("WulingCraft"),TEXT("WulingSources"),TEXT("WulingBreakthrough"),TEXT("WulingEquipment"),TEXT("WulingConstruction"),TEXT("WulingActivity"),TEXT("WulingRace"),TEXT("WulingShop"),TEXT("WulingMarket"),TEXT("WulingGuild"),TEXT("WulingOrders"),TEXT("WulingOffline"),TEXT("WulingRights")})
        TestNotNull(*FString::Printf(TEXT("functional route %s exists"),Route),UInkFeaturePanel::ClassForRoute(FName(Route)).Get());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWulingInventoryTest,"BoundlessImmortality.Wuling.AuthorityInventory",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWulingInventoryTest::RunTest(const FString&)
{
    auto* Model=NewObject<UInkInventoryModel>();TArray<FInkInventoryEntry> Entries;
    for(int32 I=0;I<201;++I){auto& E=Entries.AddDefaulted_GetRef();E.EntryId=FGuid::NewGuid();E.Item.ItemId=TEXT("PROP_01");E.Item.Category=TEXT("Consumable");E.Item.Count=2;E.Item.bUsable=true;}
    const FGuid Id=Entries[0].EntryId;Model->ApplyAuthoritativeEntries(Entries,TEXT("r1"));
    TestEqual(TEXT("all authority instances fit view without deleting overflow"),Model->GetEntriesView().Num(),201);
    TestEqual(TEXT("remote count cannot be locally consumed"),Model->UseItem(Id,1),0);
    Model->SortItems();Model->MarkSeen(Id);Model->ApplyAuthoritativeEntries(Entries,TEXT("r1"));
    TestNotNull(TEXT("selection identity survives refresh and sorting"),Model->FindEntry(Id));
    TestFalse(TEXT("same revision does not restore new marker"),Model->FindEntry(Id)->bIsNew);
    TestEqual(TEXT("authority count intact"),Model->FindEntry(Id)->Item.Count,2);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWulingHonorTest,"BoundlessImmortality.Wuling.HonorKeepsLegacyGift",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWulingHonorTest::RunTest(const FString&)
{
    auto* C=NewObject<UCultivationSubsystem>(NewObject<UGameInstance>());C->bPersistLocalRecords=false;C->SetClockForTests([](){return 1000.;});
    C->RegisterParticipant(TEXT("viewer"),TEXT("剑灵"),TEXT(""));C->SetWulingAuthorityEnabled(true);
    FCultivationGiftEvent E;E.ParticipantId=TEXT("viewer");E.MessageId=TEXT("gift-one");E.GiftId=ECultivationGift::RareChest;
    TestTrue(TEXT("legacy gift remains acknowledged"),C->ProcessGift(E));const FString Before=C->ExportRecords();const FName Original=C->GetParticipantSnapshot(TEXT("viewer")).TitleId;
    C->ApplyWulingHonorTitle(TEXT("viewer"),TEXT("青云踏飞雁"));
    TestEqual(TEXT("honor is only a display caption"),C->GetParticipantSnapshot(TEXT("viewer")).TitleName.ToString(),FString(TEXT("青云踏飞雁")));
    TestEqual(TEXT("gift passive identity survives display choice"),C->GetParticipantSnapshot(TEXT("viewer")).TitleId,Original);
    TestEqual(TEXT("gift receipts, duration and unlocked passives remain intact"),C->ExportRecords(),Before);
    C->ApplyWulingHonorTitle(TEXT("viewer"),TEXT(""));
    TestTrue(TEXT("clearing honor restores original caption"),C->GetParticipantSnapshot(TEXT("viewer")).TitleName.ToString()!=TEXT("青云踏飞雁"));return true;
}
#endif
