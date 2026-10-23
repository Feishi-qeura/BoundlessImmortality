#include "WulingSnapshotProjection.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Cultivation/CultivationHealthComponent.h"
#include "Dom/JsonObject.h"

bool WulingSnapshotProjection::Apply(UCultivationSubsystem& C,const TSharedPtr<FJsonObject>& Root,const FString& Anchor)
{
    const TSharedPtr<FJsonObject>* Manifest=nullptr;
    if(!Root||!Root->TryGetObjectField(TEXT("manifest"),Manifest))return false;
    FString Version,Profile;
    if(!(*Manifest)->TryGetStringField(TEXT("rule_version"),Version)||Version!=TEXT("wuling-prototype-v1.4.0")||
       !(*Manifest)->TryGetStringField(TEXT("profile"),Profile)||Profile!=TEXT("prototype_full"))return false;
    const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;if(!Root->TryGetArrayField(TEXT("records"),Rows)||Rows->Num()>10001)return false;
    struct FEntry{FString Id,Honor;int32 Realm=0;double Qi=0,Threshold=0,Materials=0,Cost=0,Rate=0;FCultivationAttributes Attributes;bool bHasAttributes=false;};
    TArray<FEntry> Entries;TSet<FString> Ids;
    for(const auto& Value:*Rows)
    {
        if(!Value||Value->Type!=EJson::Object)return false;const auto O=Value->AsObject();FEntry E;double Realm=0;FString Role;
        if(!O->TryGetStringField(TEXT("user"),E.Id)||E.Id.IsEmpty()||!O->TryGetStringField(TEXT("role"),Role)||
           !O->TryGetNumberField(TEXT("realm_index"),Realm)||!O->TryGetNumberField(TEXT("qi"),E.Qi)||!O->TryGetNumberField(TEXT("qi_to_next"),E.Threshold))return false;
        if(E.Id==Anchor)E.Id=C.GetAnchorId();
        O->TryGetStringField(TEXT("honor_title"),E.Honor);
        O->TryGetNumberField(TEXT("material_count"),E.Materials);O->TryGetNumberField(TEXT("material_cost"),E.Cost);O->TryGetNumberField(TEXT("rate"),E.Rate);
        const TSharedPtr<FJsonObject>* Attr=nullptr;if(O->TryGetObjectField(TEXT("attributes"),Attr))
        {
            if(!(*Attr)->TryGetNumberField(TEXT("attack"),E.Attributes.Attack)||!(*Attr)->TryGetNumberField(TEXT("max_health"),E.Attributes.MaxHealth)||!(*Attr)->TryGetNumberField(TEXT("defense"),E.Attributes.Defense))return false;
            double Move=0;(*Attr)->TryGetNumberField(TEXT("move_speed_add"),Move);E.Attributes.MoveSpeed=1+FMath::Clamp(Move,0.,.10);E.Attributes.HealthRegeneration=0;
            if(!FMath::IsFinite(E.Attributes.Attack)||E.Attributes.Attack<0||!FMath::IsFinite(E.Attributes.MaxHealth)||E.Attributes.MaxHealth<0||!FMath::IsFinite(E.Attributes.Defense)||E.Attributes.Defense<0||!FMath::IsFinite(Move))return false;
            E.bHasAttributes=true;
        }
        if(Ids.Contains(E.Id)||!FMath::IsFinite(Realm)||Realm<0||Realm>33||Realm!=FMath::FloorToDouble(Realm)||
           !FMath::IsFinite(E.Qi)||E.Qi<0||!FMath::IsFinite(E.Threshold)||E.Threshold<0||!FMath::IsFinite(E.Materials)||E.Materials<0||E.Materials>MAX_int32||
           !FMath::IsFinite(E.Cost)||E.Cost<0||E.Cost>MAX_int32||!FMath::IsFinite(E.Rate)||E.Rate<0)return false;
        E.Realm=int32(Realm);Ids.Add(E.Id);Entries.Add(E);
    }
    for(const auto& E:Entries){C.ApplyWulingGrowth(E.Id,E.Realm,E.Qi,E.Threshold,int32(E.Materials),int32(E.Cost),E.Rate);C.ApplyWulingHonorTitle(E.Id,E.Honor);if(E.bHasAttributes)C.ApplyWulingAttributes(E.Id,E.Attributes);}
    const TSharedPtr<FJsonObject>* Combat=nullptr;if(Root->TryGetObjectField(TEXT("combat"),Combat))if(auto* Health=C.GetAnchorHealth())
    {double Current=0,Maximum=0;if((*Combat)->TryGetNumberField(TEXT("health"),Current)&&(*Combat)->TryGetNumberField(TEXT("max_health"),Maximum))Health->ApplyWulingHealth(Current,Maximum);}
    return true;
}
