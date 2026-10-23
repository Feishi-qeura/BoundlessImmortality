#include "WulingInventoryProjection.h"
#include "Frontend/InkInventoryModel.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

namespace {
FString S(const TSharedPtr<FJsonObject>& O,const TCHAR* K){FString V;if(O)O->TryGetStringField(K,V);return V;}
double N(const TSharedPtr<FJsonObject>& O,const TCHAR* K){double V=0;if(O)O->TryGetNumberField(K,V);return V;}
}
void WulingInventoryProjection::Apply(UInkInventoryModel& Model,const TSharedPtr<FJsonObject>& State)
{
    const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;if(!State||!State->TryGetArrayField(TEXT("item_instances"),Rows))return;
    const TSharedPtr<FJsonObject>* Loadout=nullptr;State->TryGetObjectField(TEXT("loadout"),Loadout);
    FString Revision;FJsonSerializer::Serialize(*Rows,TJsonWriterFactory<>::Create(&Revision));
    if(Loadout){FString L;FJsonSerializer::Serialize((*Loadout).ToSharedRef(),TJsonWriterFactory<>::Create(&L));Revision+=L;}
    TArray<FInkInventoryEntry> Entries;
    for(const auto& V:*Rows)
    {
        auto O=V->AsObject();const TSharedPtr<FJsonObject>* Def=nullptr;
        if(!O||!O->TryGetObjectField(TEXT("definition"),Def))continue;
        const FString Item=S(O,TEXT("item")),Id=S(O,TEXT("id"));FInkInventoryEntry E;
        if(!FGuid::Parse(Id,E.EntryId))continue;
        E.AuthorityInstanceId=Id;E.ExpiresUtc=N(O,TEXT("expiry"));E.Item.ItemId=FName(*Item);E.Item.Count=int32(N(O,TEXT("count")));
        E.Item.Name=FText::FromString(S(*Def,TEXT("name")));E.Item.bStackable=true;E.Item.MaxStackSize=FMath::Max(1,int32(N(*Def,TEXT("max_stack"))));
        const bool Reserved=!S(O,TEXT("reserved")).IsEmpty();
        TSharedPtr<FJsonObject> Source;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S(O,TEXT("source"))),Source);FString Origin=S(Source,TEXT("label"));if(Origin.IsEmpty())Origin=TEXT("已确认结算记录，可在物品来源页查看获取方式");
        E.Item.Description=FText::FromString(FString::Printf(TEXT("%s\n%s · 到期UTC：%s\n来源：%s%s"),*S(*Def,TEXT("use")),S(O,TEXT("binding"))==TEXT("unbound")?TEXT("未绑定"):TEXT("已绑定"),*FDateTime::FromUnixTimestamp(int64(E.ExpiresUtc)).ToIso8601(),*Origin,Reserved?TEXT("\n已预留，暂不可操作"):TEXT("\n操作按服务端最早到期实例结算")));
        E.Item.Category=Item.StartsWith(TEXT("EQ_"))?TEXT("Equipment"):(Item.StartsWith(TEXT("FABAO_"))?TEXT("Treasure"):((Item.StartsWith(TEXT("PROP_"))||Item.StartsWith(TEXT("ITEM_"))||Item.StartsWith(TEXT("BOOK_")))?TEXT("Consumable"):TEXT("Material")));
        E.Item.bUsable=!Reserved&&E.Item.Category==TEXT("Consumable");E.Item.bSellable=!Reserved&&(Item==TEXT("MAT_HERB_T")||Item==TEXT("MAT_IRON_T"));E.Item.bDecomposable=false;
        E.Item.EquipmentSlot=FName(*Item); // Actual slot and equip validity are resolved by the authority.
        if(Loadout){const TSharedPtr<FJsonObject>* Equipment=nullptr;if((*Loadout)->TryGetObjectField(TEXT("equipment"),Equipment))for(const auto& P:(*Equipment)->Values)if(P.Value->AsString()==Item)E.bEquipped=true;E.bEquipped|=S(*Loadout,TEXT("fabao"))==Item;}
        Entries.Add(E);
    }
    Model.ApplyAuthoritativeEntries(Entries,Revision);
}
