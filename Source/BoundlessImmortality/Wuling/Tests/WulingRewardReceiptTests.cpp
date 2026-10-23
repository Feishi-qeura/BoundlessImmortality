#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Wuling/WulingRewardReceiptQueue.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    FString ReceiptId(const FString& Json)
    {TSharedPtr<FJsonObject> Object;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Object);return Object?Object->GetStringField(TEXT("receipt_id")):FString();}
    TSharedPtr<FJsonObject> ReceiptProjection(int32 High,std::initializer_list<int32> Sequences)
    {
        auto Projection=MakeShared<FJsonObject>();Projection->SetNumberField(TEXT("high_watermark"),High);
        TArray<TSharedPtr<FJsonValue>> Entries;
        for(int32 Sequence:Sequences)
        {
            auto Entry=MakeShared<FJsonObject>();Entry->SetStringField(TEXT("receipt_id"),FString::FromInt(Sequence));Entry->SetNumberField(TEXT("sequence"),Sequence);
            auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("name"),TEXT("灵草"));Item->SetNumberField(TEXT("quantity"),2);
            Entry->SetArrayField(TEXT("items"),{MakeShared<FJsonValueObject>(Item)});Entries.Add(MakeShared<FJsonValueObject>(Entry));
        }
        Projection->SetArrayField(TEXT("entries"),Entries);return Projection;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWulingRewardReceiptQueueTest,"BoundlessImmortality.Wuling.RewardReceiptQueue",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWulingRewardReceiptQueueTest::RunTest(const FString&)
{
    FWulingRewardReceiptQueue Inbox;
    Inbox.Observe(nullptr);Inbox.Observe(ReceiptProjection(3,{1,2,3}));
    TestEqual(TEXT("first confirmed snapshot baselines history"),Inbox.Num(),0);
    Inbox.Observe(ReceiptProjection(5,{5,4,3}));
    TestEqual(TEXT("only new confirmed receipts enqueue"),Inbox.Num(),2);
    TestEqual(TEXT("out of order entry array is sorted chronologically"),ReceiptId(Inbox.TakeNext()),FString(TEXT("4")));
    Inbox.Observe(ReceiptProjection(5,{3,4,5}));Inbox.Observe(ReceiptProjection(2,{1,2}));
    TestEqual(TEXT("retry poll and old command snapshots never replay"),Inbox.Num(),1);
    TestEqual(TEXT("next close drains the second receipt"),ReceiptId(Inbox.TakeNext()),FString(TEXT("5")));
    TestTrue(TEXT("empty queue stays empty after close and reopen"),Inbox.TakeNext().IsEmpty());
    Inbox.Observe(ReceiptProjection(5,{4,5}));TestEqual(TEXT("closed receipts stay consumed"),Inbox.Num(),0);
    Inbox.Observe(ReceiptProjection(6,{6,6}));TestEqual(TEXT("duplicate IDs in same projection enqueue once"),Inbox.Num(),1);
    Inbox.TakeNext();Inbox.Observe(ReceiptProjection(7,{}));Inbox.Observe(ReceiptProjection(6,{6}));
    TestEqual(TEXT("empty later projection cannot rewind consumption"),Inbox.Num(),0);
    FWulingRewardReceiptQueue Reopened;Reopened.Observe(ReceiptProjection(6,{4,5,6}));
    TestEqual(TEXT("new game instance baselines saved history"),Reopened.Num(),0);
    return true;
}
#endif
