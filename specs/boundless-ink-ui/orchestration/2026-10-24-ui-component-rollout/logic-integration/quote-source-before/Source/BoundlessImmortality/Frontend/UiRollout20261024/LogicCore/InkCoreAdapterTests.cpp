#if WITH_DEV_AUTOMATION_TESTS
#include "InkCoreAdapter.h"
#include "InkCoreMaterialBinding.h"
#include "Frontend/UiRollout20261024/Player/InkPlayerMaterialSlotWidget.h"
#include "Frontend/InkFrontendWidget.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/AutomationTest.h"

namespace
{
TSharedPtr<FJsonObject> ParseCoreTestJson(const FString& Text)
{TSharedPtr<FJsonObject> O;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),O);return O;}
// Synthetic complete future-contract fixture. Never displayed in a live UI.
TSharedPtr<FJsonObject> CoreQuoteFixture()
{
    return ParseCoreTestJson(TEXT(R"JSON({"server_utc":100,"growth":{"name":"fixture-current"},"breakthrough_quote":{"target_stage":"fixture-target","q":20,"q_normalized":0.2,"quality_band":"fixture-quality","growth_min":1,"growth_max":2,"readiness":{"ready":true},"sources":[{"source_id":"fixture-source"}],"probability_basis_points":5000,"selection":[{"item_id":"fixture-core","count":1},{"item_id":"fixture-guard","count":2}],"slots":[{"item_id":"fixture-core","category":"core","count":1},{"item_id":"fixture-guard","category":"guard","quantity":2}]}})JSON"));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkCoreQuoteContractTest,"BoundlessImmortality.Frontend.Rollout20261024.Core.QuoteContract",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkCoreQuoteContractTest::RunTest(const FString&)
{
    const TMap<FString,int32> Selection={{TEXT("fixture-guard"),2},{TEXT("fixture-core"),1}};
    auto S=CoreQuoteFixture();TestTrue(TEXT("complete quote matches independent selection order"),InkCore::EvaluateQuote(S,Selection).CanSubmit);
    auto Q=S->GetObjectField(TEXT("breakthrough_quote"));
    TestFalse(TEXT("a normal quote cannot confirm rework"),InkCore::EvaluateQuote(S,Selection,true).CanSubmit);
    auto Changed=Selection;Changed[TEXT("fixture-guard")]=1;
    TestFalse(TEXT("changed local selection invalidates quote"),InkCore::EvaluateQuote(S,Changed).CanSubmit);
    Q->SetArrayField(TEXT("selection"),{MakeShared<FJsonValueNull>()});
    TestFalse(TEXT("malformed selection is inert"),InkCore::EvaluateQuote(S,Selection).CanSubmit);
    S=CoreQuoteFixture();Q=S->GetObjectField(TEXT("breakthrough_quote"));
    const auto Echo=Q->GetArrayField(TEXT("selection"));auto Duplicated=Echo;Duplicated.Add(Echo[0]);Q->SetArrayField(TEXT("selection"),Duplicated);
    TestFalse(TEXT("duplicated echoed item rejected"),InkCore::EvaluateQuote(S,Selection).CanSubmit);
    S=CoreQuoteFixture();Q=S->GetObjectField(TEXT("breakthrough_quote"));Q->SetArrayField(TEXT("slots"),{MakeShared<FJsonValueNull>()});
    TestFalse(TEXT("malformed slot is inert"),InkCore::EvaluateQuote(S,Selection).CanSubmit);
    S=CoreQuoteFixture();Q=S->GetObjectField(TEXT("breakthrough_quote"));Q->RemoveField(TEXT("q_normalized"));
    TestFalse(TEXT("missing normalized Q never becomes zero"),InkCore::EvaluateQuote(S,Selection).CanSubmit);
    S=CoreQuoteFixture();auto Attempt=ParseCoreTestJson(TEXT("{\"id\":\"attempt-fixture\",\"status\":\"Prepare\",\"trial_start\":101}"));S->SetObjectField(TEXT("tribulation"),Attempt);
    auto Decision=InkCore::EvaluateQuote(S,Selection);TestTrue(TEXT("server permits pre-trial cancel"),Decision.CanCancel);TestFalse(TEXT("active attempt blocks repeat prepare"),Decision.CanSubmit);
    S->SetNumberField(TEXT("server_utc"),101);TestFalse(TEXT("trial boundary refuses cancellation"),InkCore::EvaluateQuote(S,Selection).CanCancel);
    Attempt->SetStringField(TEXT("status"),TEXT("Reconcile"));TestTrue(TEXT("valid reconcile id permitted"),InkCore::EvaluateQuote(S,{}).CanReconcile);
    Attempt->RemoveField(TEXT("id"));TestFalse(TEXT("missing attempt id cannot reconcile"),InkCore::EvaluateQuote(S,{}).CanReconcile);
    TestFalse(TEXT("missing real service envelope cannot submit"),InkCore::EvaluateQuote(nullptr,Selection).CanSubmit);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkCoreBoardContractTest,"BoundlessImmortality.Frontend.Rollout20261024.Core.Boards",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkCoreBoardContractTest::RunTest(const FString&)
{
    using InkCorePolicy::EState;
    TestTrue(TEXT("unknown room response is loading"),InkCore::ReadBoard(nullptr,{},TEXT("Local"),TEXT("Anchor")).State==EState::Loading);
    TestTrue(TEXT("confirmed empty room response is empty"),InkCore::ReadBoard(nullptr,{},TEXT("Local"),TEXT("Anchor"),true).State==EState::Empty);
    FInkLeaderboardEntry A;A.ParticipantId=TEXT("fixture-a");A.Rank=7;A.NumericMetric=3;A.Category=TEXT("Damage");
    auto B=A;B.ParticipantId=TEXT("fixture-b");B.Rank=3;B.NumericMetric=1;
    auto Board=InkCore::ReadBoard(nullptr,{A,B},TEXT("Local"),TEXT("Damage"),true);
    TestEqual(TEXT("populated board preserved"),Board.Entries.Num(),2);
    if(Board.Entries.Num()==2){TestEqual(TEXT("authority rank is not numeric metric sort"),Board.Entries[0].Rank,3);TestEqual(TEXT("rank gap preserved"),Board.Entries[1].Rank,7);}
    TestTrue(TEXT("other category remains empty"),InkCore::ReadBoard(nullptr,{A,B},TEXT("Local"),TEXT("Anchor"),true).State==EState::Empty);
    B.Rank=7;TestTrue(TEXT("duplicate rank produces error"),InkCore::ReadBoard(nullptr,{A,B},TEXT("Local"),TEXT("Damage"),true).State==EState::Error);
    TestTrue(TEXT("global cannot borrow local data"),InkCore::ReadBoard(nullptr,{A},TEXT("Global"),TEXT("Damage"),true).Entries.IsEmpty());
    TestTrue(TEXT("invalid category refused"),InkCore::ReadBoard(nullptr,{A},TEXT("Local"),TEXT("MadeUp"),true).State==EState::Error);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkCoreLocalInventoryContractTest,"BoundlessImmortality.Frontend.Rollout20261024.Core.Inventory",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkCoreLocalInventoryContractTest::RunTest(const FString&)
{
    using InkCorePolicy::EInventoryAction;
    auto* Frontend=NewObject<UInkFrontendWidget>();auto* Model=Frontend->GetInventoryModel();
    FInkInventoryItem Pill;Pill.ItemId=TEXT("fixture-pill");Pill.Category=TEXT("Consumable");Pill.Count=3;Model->AddItem(Pill,3);
    TestEqual(TEXT("fixture populated actual model"),Model->GetEntriesView().Num(),1);
    if(Model->GetEntriesView().IsEmpty())return false;
    const auto Id=Model->GetEntriesView()[0].EntryId;
    TestFalse(TEXT("inactive page refuses use"),InkCore::InventoryAction(Frontend,Id,EInventoryAction::Use,1,false).Accepted);
    TestFalse(TEXT("quantity overdraw refused"),InkCore::InventoryAction(Frontend,Id,EInventoryAction::Use,4,true).Accepted);
    auto R=InkCore::InventoryAction(Frontend,Id,EInventoryAction::Use,2,true);
    TestTrue(TEXT("legacy action delegates to existing real model"),R.Accepted);TestFalse(TEXT("local result never claims remote confirmation pending"),R.PendingAuthority);
    const auto* Remaining=Model->FindEntry(Id);TestNotNull(TEXT("identity retained after partial consume"),Remaining);if(Remaining)TestEqual(TEXT("actual remaining quantity"),Remaining->Item.Count,1);
    TestFalse(TEXT("removed identity cannot mutate another slot"),InkCore::InventoryAction(Frontend,FGuid::NewGuid(),EInventoryAction::Use,1,true).Accepted);
    TestFalse(TEXT("unavailable service cannot prepare"),InkCore::PrepareBreakthrough(nullptr,{},false,true).Accepted);
    TestFalse(TEXT("unavailable service cannot create organization"),InkCore::CreateGuild(nullptr,TEXT("fixture-guild"),true).Accepted);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkCoreMaterialBindingContractTest,"BoundlessImmortality.Frontend.Rollout20261024.Core.MaterialBinding",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkCoreMaterialBindingContractTest::RunTest(const FString&)
{
    auto* Frontend=NewObject<UInkFrontendWidget>();Frontend->CurrentPage=TEXT("WulingBreakthrough");
    auto* Binding=NewObject<UInkCoreMaterialBinding>(Frontend);Binding->Configure(Frontend,TEXT("WulingBreakthrough"));
    auto* Old=NewObject<UInkPlayerMaterialSlotWidget>(Frontend);auto* New=NewObject<UInkPlayerMaterialSlotWidget>(Frontend);
    Binding->BindSlot(Old,TEXT("core"),TEXT("MAT_BR04_C01_G1"));TestTrue(TEXT("delivered atom delegate bound"),Old->OnSlotActivated.IsBound());
    Binding->BindSlot(New,TEXT("core"),TEXT("MAT_BR04_C01_G1"));TestFalse(TEXT("replaced atom delegate detached"),Old->OnSlotActivated.IsBound());
    TestTrue(TEXT("replacement atom delegate bound"),New->OnSlotActivated.IsBound());
    New->OnSlotActivated.Broadcast(TEXT("core"),TEXT("MAT_BR04_C01_G1"));
    TestTrue(TEXT("no authority cannot create selected materials"),Binding->GetSelection().IsEmpty());
    TestFalse(TEXT("no authority cannot enable submit"),Binding->CanSubmit());
    Binding->Configure(Frontend,TEXT("WulingBreakthrough"),true);
    TestFalse(TEXT("mode change detaches old delegates"),New->OnSlotActivated.IsBound());
    return true;
}
#endif
