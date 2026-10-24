#include "InkCoreMaterialBinding.h"
#include "InkCoreAdapter.h"
#include "Frontend/InkFrontendWidget.h"
#include "Frontend/UiRollout20261024/Player/InkPlayerMaterialSlotWidget.h"
#include "Wuling/WulingPrototypeSubsystem.h"
#include "Dom/JsonObject.h"

namespace
{
TSharedPtr<FJsonObject> CoreBindingObject(const TSharedPtr<FJsonObject>& O,const TCHAR* Key)
{const TSharedPtr<FJsonObject>* Value=nullptr;return O&&O->TryGetObjectField(Key,Value)?*Value:nullptr;}
bool CoreBindingOwned(const TSharedPtr<FJsonObject>& S,FName ItemId,int32& Owned)
{
    const auto Inventory=CoreBindingObject(S,TEXT("inventory"));double Count=0;
    if(!Inventory||!Inventory->TryGetNumberField(ItemId.ToString(),Count)||!FMath::IsFinite(Count)||Count<0||Count>MAX_int32||FMath::FloorToDouble(Count)!=Count)return false;
    Owned=int32(Count);return true;
}
FString CoreBindingName(const TSharedPtr<FJsonObject>& S,FName ItemId)
{
    const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
    if(S&&S->TryGetArrayField(TEXT("item_instances"),Rows))for(const auto& Value:*Rows)
    {
        const auto O=Value&&Value->Type==EJson::Object?Value->AsObject():nullptr;FString Item;
        if(!O||!O->TryGetStringField(TEXT("item"),Item)||Item!=ItemId.ToString())continue;
        const auto Definition=CoreBindingObject(O,TEXT("definition"));FString Name;if(Definition&&Definition->TryGetStringField(TEXT("name"),Name))return Name;
    }
    // The actual identifier is preferable to an invented material name.
    return ItemId.ToString();
}
}

void UInkCoreMaterialBinding::Configure(UInkFrontendWidget* Owner,FName Route,bool Rework)
{
    for(const auto& Slot:Slots)if(Slot)Slot->OnSlotActivated.RemoveDynamic(this,&UInkCoreMaterialBinding::HandleSlotActivated);
    Slots.Reset();Icons.Reset();Frames.Reset();Selection.Reset();Frontend=Owner;Page=Route;bRework=Rework;
}
bool UInkCoreMaterialBinding::IsActive()const{return Frontend.IsValid()&&Frontend->CurrentPage==Page;}
void UInkCoreMaterialBinding::BindSlot(UInkPlayerMaterialSlotWidget* Slot,FName SlotId,FName ItemId,UTexture2D* Icon,UTexture2D* Frame)
{
    if(!Slot||SlotId.IsNone()||ItemId.IsNone())return;
    // One stable slot id owns one delegate and art binding.
    for(int32 I=Slots.Num()-1;I>=0;--I)if(Slots[I]&&(Slots[I]==Slot||Slots[I]->SlotId==SlotId))
    {Slots[I]->OnSlotActivated.RemoveDynamic(this,&UInkCoreMaterialBinding::HandleSlotActivated);Slots.RemoveAt(I);}
    Slots.Add(Slot);Icons.Add(SlotId,Icon);Frames.Add(SlotId,Frame);
    Slot->ConfigureSlot(SlotId,ItemId,FText::FromName(ItemId),FText::FromString(TEXT("—")),FText::FromString(TEXT("已选 0")),Icon,Frame,false);
    Slot->OnSlotActivated.AddUniqueDynamic(this,&UInkCoreMaterialBinding::HandleSlotActivated);Refresh();
}
void UInkCoreMaterialBinding::Refresh()
{
    auto* W=InkCore::Service(Frontend.Get());const auto S=W&&W->IsEnabled()&&W->IsConnected()?W->Snapshot():nullptr;
    bool Anchor=false;const auto Identity=CoreBindingObject(S,TEXT("identity"));FString Role;if(Identity&&Identity->TryGetStringField(TEXT("role"),Role))Anchor=Role==TEXT("anchor");
    const bool Ready=InkCorePolicy::CanDispatch(InkCore::Transport(Frontend.Get(),IsActive()))&&Anchor;
    for(const auto& Slot:Slots)if(Slot)
    {
        int32 Owned=0;const bool HasOwned=CoreBindingOwned(S,Slot->ItemId,Owned);const auto Item=Slot->ItemId.ToString();const int32 Selected=Selection.FindRef(Item);
        Slot->ConfigureSlot(Slot->SlotId,Slot->ItemId,FText::FromString(CoreBindingName(S,Slot->ItemId)),
            FText::FromString(HasOwned?FString::Printf(TEXT("持有 %d"),Owned):FString(TEXT("持有 —"))),
            FText::FromString(FString::Printf(TEXT("已选 %d"),Selected)),Icons.FindRef(Slot->SlotId),Frames.FindRef(Slot->SlotId),
            Ready&&HasOwned&&Item.StartsWith(TEXT("MAT_BR"))&&Selected<FMath::Min(100,Owned));
    }
}
void UInkCoreMaterialBinding::HandleSlotActivated(FName SlotId,FName ItemId)
{
    const bool Bound=Slots.ContainsByPredicate([&](const auto& Slot){return Slot&&Slot->SlotId==SlotId&&Slot->ItemId==ItemId;});
    if(!Bound||!IsActive())return;
    auto* W=InkCore::Service(Frontend.Get());int32 Owned=0;const FString Item=ItemId.ToString();
    const int32 Previous=Selection.FindRef(Item);
    if(!W||!W->IsConnected()||!Item.StartsWith(TEXT("MAT_BR"))||!CoreBindingOwned(W->Snapshot(),ItemId,Owned)||Previous>=FMath::Min(100,Owned)){Refresh();return;}
    auto Draft=Selection;Draft.Add(Item,Previous+1);
    const auto Result=InkCore::QuoteBreakthrough(Frontend.Get(),Draft,bRework,IsActive());
    if(Result.Accepted)Selection=MoveTemp(Draft);
    OnFeedback.Broadcast(Result.Message);Refresh();
}
void UInkCoreMaterialBinding::ClearSelection(){Selection.Reset();Refresh();}
bool UInkCoreMaterialBinding::CanSubmit()const
{
    const auto T=InkCore::Transport(Frontend.Get(),IsActive());auto* W=InkCore::Service(Frontend.Get());
    if(!InkCorePolicy::CanDispatch(T)||!W)return false;
    const auto Identity=CoreBindingObject(W->Snapshot(),TEXT("identity"));FString Role;
    return Identity&&Identity->TryGetStringField(TEXT("role"),Role)&&Role==TEXT("anchor")&&InkCore::EvaluateQuote(W->Snapshot(),Selection,bRework).CanSubmit;
}
void UInkCoreMaterialBinding::Submit()
{const auto Result=InkCore::PrepareBreakthrough(Frontend.Get(),Selection,bRework,IsActive());OnFeedback.Broadcast(Result.Message);Refresh();}
