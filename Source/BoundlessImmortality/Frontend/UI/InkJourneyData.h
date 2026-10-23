#pragma once
#include "Frontend/WulingFrontendData.h"
#include "Serialization/JsonSerializer.h"

namespace InkJourneyData
{
inline TArray<TSharedPtr<FJsonObject>> Rows(const UUserWidget* Widget,bool Resources)
{
    const auto* Service=WulingFrontendData::Service(Widget);
    const auto State=Service&&Service->IsEnabled()?Service->Snapshot():nullptr;
    const auto Journey=WulingFrontendData::Object(State,TEXT("journey"));
    const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;
    TArray<TSharedPtr<FJsonObject>> Result;
    if(Journey&&Journey->TryGetArrayField(Resources?TEXT("resources"):TEXT("worlds"),Values))
    {for(const auto& Value:*Values)if(Value.IsValid()&&Value->Type==EJson::Object)Result.Add(Value->AsObject());return Result;}
    // Offline atlas is geography only. It cannot authorize travel or show rewards/owners.
    const TCHAR* WorldIds[]={TEXT("qingyun"),TEXT("luoxia"),TEXT("xuantie"),TEXT("xuanbing"),TEXT("yaoshou"),TEXT("chiyan"),TEXT("xingyun"),TEXT("taixu")};
    const TCHAR* WorldNames[]={TEXT("青云山"),TEXT("落霞谷"),TEXT("玄铁岭"),TEXT("玄冰原"),TEXT("妖兽林"),TEXT("赤炎洲"),TEXT("星陨台"),TEXT("太虚境")};
    const FVector2D WorldPos[]={{.22,.65},{.50,.48},{.32,.22},{.76,.18},{.84,.72},{.72,.43},{.90,.40},{.67,.88}};
    if(!Resources)for(int32 I=0;I<8;++I)
    {
        auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("id"),WorldIds[I]);O->SetStringField(TEXT("name"),WorldNames[I]);
        O->SetNumberField(TEXT("x"),WorldPos[I].X);O->SetNumberField(TEXT("y"),WorldPos[I].Y);O->SetBoolField(TEXT("open"),I<3);
        O->SetStringField(TEXT("difficulty"),I<3?TEXT("待同步"):TEXT("未开放"));O->SetStringField(TEXT("description"),I<3?TEXT("山川蕴灵。连接后可查看区域机制、难度与三阶段奖励。"):TEXT("此地墨云未散，后续章节尚未开放。"));Result.Add(O);
    }
    return Result;
}
inline TSharedPtr<FJsonObject> Find(const UUserWidget* Widget,bool Resources,const FString& Id)
{
    for(const auto& Row:Rows(Widget,Resources))if(WulingFrontendData::String(Row,TEXT("id"))==Id||WulingFrontendData::String(Row,TEXT("name"))==Id)return Row;
    return nullptr;
}
inline bool Open(const TSharedPtr<FJsonObject>& Row){bool V=false;if(Row)Row->TryGetBoolField(TEXT("open"),V);return V;}
inline bool Ready(const UUserWidget* W){const auto* S=WulingFrontendData::Service(W);return S&&S->IsEnabled()&&S->IsConnected()&&S->Snapshot()&&!S->IsBusy();}
inline double Number(const TSharedPtr<FJsonObject>& Row,const TCHAR* Key){double V=0;if(Row)Row->TryGetNumberField(Key,V);return V;}
inline FString Owners(const TSharedPtr<FJsonObject>& Row)
{
    const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;TArray<FString> Names;
    if(Row&&Row->TryGetArrayField(TEXT("owners"),Values))for(const auto& V:*Values)if(V.IsValid()&&V->Type==EJson::Object)Names.AddUnique(WulingFrontendData::String(V->AsObject(),TEXT("name")));
    return Names.IsEmpty()?TEXT("暂无主播持有"):FString::Join(Names,TEXT("、"));
}
inline FString Yield(const TSharedPtr<FJsonObject>& Row)
{
    const double Qi=Number(Row,TEXT("qi_additive"));
    if(Qi>0)return FString::Printf(TEXT("修炼倍率加成 +%.0f%%"),Qi*100);
    return FString::Printf(TEXT("%s × %.0f / %.0f分钟"),*WulingFrontendData::String(Row,TEXT("item_name")),Number(Row,TEXT("quantity")),Number(Row,TEXT("interval_seconds"))/60);
}
inline FString Key(const UUserWidget* Widget,bool Resources,const FString& Id=FString())
{
    FString Result;const auto Rows_=Id.IsEmpty()?Rows(Widget,Resources):TArray<TSharedPtr<FJsonObject>>{Find(Widget,Resources,Id)};
    for(const auto& Row:Rows_)if(Row){FString Part;FJsonSerializer::Serialize(Row.ToSharedRef(),TJsonWriterFactory<>::Create(&Part));Result+=Part;}
    const auto* S=WulingFrontendData::Service(Widget);return Result+(Ready(Widget)?TEXT("ready"):TEXT("blocked"))+(S?S->GetStatus():TEXT("offline"));
}
}
