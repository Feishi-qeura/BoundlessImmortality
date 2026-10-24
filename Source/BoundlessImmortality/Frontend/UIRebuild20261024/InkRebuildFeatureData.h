#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

namespace InkNewFeatureData
{
using FObject=TSharedPtr<FJsonObject>;
inline FObject Object(const FObject& O,const TCHAR* Key)
{const FObject* V=nullptr;return O&&O->TryGetObjectField(Key,V)?*V:nullptr;}
inline FString String(const FObject& O,const TCHAR* Key)
{FString V;if(O)O->TryGetStringField(Key,V);return V;}
inline bool Number(const FObject& O,const TCHAR* Key,double& Out)
{return O&&O->TryGetNumberField(Key,Out)&&FMath::IsFinite(Out);}
inline bool Integer(const FObject& O,const TCHAR* Key,int32& Out)
{double V=0;if(!Number(O,Key,V)||V<MIN_int32||V>MAX_int32||FMath::FloorToDouble(V)!=V)return false;Out=int32(V);return true;}
inline bool Boolean(const FObject& O,const TCHAR* Key,bool Default=false)
{bool V=Default;if(O)O->TryGetBoolField(Key,V);return V;}
inline bool Has(const FObject& O,const TCHAR* Key)
{const auto V=O?O->TryGetField(Key):nullptr;return V&&V->Type!=EJson::Null;}
inline FString Json(const FObject& O)
{FString Out;if(O)FJsonSerializer::Serialize(O.ToSharedRef(),TJsonWriterFactory<>::Create(&Out));return Out;}
inline FText Text(const FObject& O,const TCHAR* Key)
{
    FString S;if(O&&O->TryGetStringField(Key,S))return FText::FromString(S);
    double N=0;if(Number(O,Key,N))return FText::AsNumber(N);
    bool B=false;if(O&&O->TryGetBoolField(Key,B))return FText::FromString(B?TEXT("是"):TEXT("否"));
    return FText();
}
inline FText NumericText(const FObject& O,const TCHAR* Key)
{double N=0;return Number(O,Key,N)?FText::AsNumber(N):FText();}
inline FString Version(const FObject& O,const TCHAR* Key=TEXT("version"))
{double N=0;return Number(O,Key,N)?FString::Printf(TEXT("%.0f"),N):String(O,Key);}
inline FText DateText(const FObject& O,const TCHAR* Key)
{double N=0;return Number(O,Key,N)&&N>0?FText::AsDateTime(FDateTime::FromUnixTimestamp(int64(N)),EDateTimeStyle::Short,EDateTimeStyle::Short):Text(O,Key);}
inline TArray<FObject> Rows(const FObject& O,const TCHAR* Key,bool* Provided=nullptr)
{
    TArray<FObject> Out;const TArray<TSharedPtr<FJsonValue>>* Array=nullptr;
    const bool Known=O&&O->TryGetArrayField(Key,Array);if(Provided)*Provided=Known;
    if(Known)for(const auto& Value:*Array)if(Value&&Value->Type==EJson::Object&&Value->AsObject())Out.Add(Value->AsObject());
    return Out;
}
inline TArray<FName> Names(const FObject& O,const TCHAR* Key,bool* Provided=nullptr)
{
    TArray<FName> Out;const TArray<TSharedPtr<FJsonValue>>* Array=nullptr;
    const bool Known=O&&O->TryGetArrayField(Key,Array);if(Provided)*Provided=Known;
    if(Known)for(const auto& Value:*Array){FString S;if(Value&&Value->TryGetString(S)&&!S.IsEmpty())Out.Add(FName(*S));}
    return Out;
}
inline FString StageSubjectIdentity(const FObject& State)
{const auto Stage=Object(Object(State,TEXT("breakthrough_quote")),TEXT("stage"));return String(Stage,TEXT("stage_id"))+TEXT("|")+Version(Stage,TEXT("from_realm_index"))+TEXT("|")+Version(Stage,TEXT("to_realm_index"));}
inline bool ConfirmedStageMatches(const FObject& State,const FString& Captured)
{return Captured==StageSubjectIdentity(State);}
inline FObject Catalog(const FObject& State,const FString& Kind,int32 Offset=-1)
{
    const auto C=Object(State,TEXT("catalog"));int32 Actual=0;
    return String(C,TEXT("kind"))==Kind&&(Offset<0||(Integer(C,TEXT("offset"),Actual)&&Actual==Offset))?C:nullptr;
}
inline FString CatalogId(const FObject& Row,const FString& Kind)
{
    return String(Row,Kind==TEXT("recipes")?TEXT("recipe_id"):Kind==TEXT("sources")?TEXT("source_id"):
        Kind==TEXT("buildings")?TEXT("building_id"):Kind==TEXT("shop")?TEXT("sku"):
        Kind==TEXT("market")?TEXT("id"):TEXT("item_id"));
}
inline FObject FindCatalog(const FObject& State,const FString& Kind,const FString& Id)
{for(const auto& Row:Rows(Catalog(State,Kind),TEXT("rows")))if(CatalogId(Row,Kind)==Id)return Row;return nullptr;}
inline FText Named(const FObject& O,const TCHAR* NameKey,const FString& ActualId=FString())
{const auto Name=Text(O,NameKey);return !Name.IsEmpty()?Name:FText::FromString(ActualId);}
inline FString Identity(const FObject& State)
{const auto I=Object(State,TEXT("identity"));return String(I,TEXT("user"))+TEXT("|")+String(I,TEXT("anchor"))+TEXT("|")+String(I,TEXT("role"))+TEXT("|")+String(Object(State,TEXT("guild")),TEXT("id"));}
inline FString CatalogKindForRoute(FName Route)
{
    if(Route==TEXT("CultivationMethods"))return TEXT("methods");
    if(Route==TEXT("EquipmentFabao"))return TEXT("equipment");
    if(Route==TEXT("WulingCraft"))return TEXT("recipes");
    if(Route==TEXT("ItemSources"))return TEXT("sources");
    if(Route==TEXT("BreakthroughPrepare"))return TEXT("items");
    if(Route==TEXT("GuildConstruction"))return TEXT("buildings");
    if(Route==TEXT("Shop"))return TEXT("shop");
    if(Route==TEXT("Market"))return TEXT("market");
    return FString();
}
}
