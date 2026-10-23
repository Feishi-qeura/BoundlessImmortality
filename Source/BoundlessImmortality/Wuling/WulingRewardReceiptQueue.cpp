#include "WulingRewardReceiptQueue.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

void FWulingRewardReceiptQueue::Observe(const TSharedPtr<FJsonObject>& Projection)
{
    double Watermark=0;
    if(!Projection||!Projection->TryGetNumberField(TEXT("high_watermark"),Watermark)||Watermark<0)return;
    const int64 IncomingHigh=int64(Watermark);
    // Opening a game, including reopening an existing save, establishes history.
    if(!bBaselined){bBaselined=true;HighWatermark=IncomingHigh;return;}
    if(IncomingHigh<=HighWatermark)return;
    const TArray<TSharedPtr<FJsonValue>>* Entries=nullptr;
    if(!Projection->TryGetArrayField(TEXT("entries"),Entries))return;
    TArray<TSharedPtr<FJsonObject>> Ready;
    for(const auto& Entry:*Entries)
    {
        if(!Entry.IsValid()||Entry->Type!=EJson::Object)continue;
        auto Receipt=Entry->AsObject();double Sequence=0;FString Id;
        const TArray<TSharedPtr<FJsonValue>>* Items=nullptr;
        if(!Receipt||!Receipt->TryGetStringField(TEXT("receipt_id"),Id)||Id.IsEmpty()||Seen.Contains(Id)||
           !Receipt->TryGetNumberField(TEXT("sequence"),Sequence)||Sequence<=HighWatermark||Sequence>IncomingHigh||
           !Receipt->TryGetArrayField(TEXT("items"),Items)||Items->IsEmpty())continue;
        Ready.Add(Receipt);
    }
    Ready.Sort([](const auto& A,const auto& B){return A->GetNumberField(TEXT("sequence"))<B->GetNumberField(TEXT("sequence"));});
    for(const auto& Receipt:Ready)
    {
        const FString Id=Receipt->GetStringField(TEXT("receipt_id"));if(Seen.Contains(Id))continue;
        FString Json;FJsonSerializer::Serialize(Receipt.ToSharedRef(),TJsonWriterFactory<>::Create(&Json));
        // A long unattended session remains bounded. Consumed IDs never replay,
        // even once evicted, because their sequence is below HighWatermark.
        if(Pending.Num()==128)Pending.RemoveAt(0);
        Pending.Add(MoveTemp(Json));Seen.Add(Id);SeenOrder.Add(Id);
        if(SeenOrder.Num()>256){Seen.Remove(SeenOrder[0]);SeenOrder.RemoveAt(0);}
    }
    HighWatermark=IncomingHigh;
}

FString FWulingRewardReceiptQueue::TakeNext()
{
    if(Pending.IsEmpty())return FString();
    FString Receipt=MoveTemp(Pending[0]);Pending.RemoveAt(0);return Receipt;
}
