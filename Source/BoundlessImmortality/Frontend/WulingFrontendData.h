#pragma once
#include "Wuling/WulingPrototypeSubsystem.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Blueprint/UserWidget.h"
namespace WulingFrontendData
{
inline UWulingPrototypeSubsystem* Service(const UUserWidget* Widget){return Widget->GetGameInstance()?Widget->GetGameInstance()->GetSubsystem<UWulingPrototypeSubsystem>():nullptr;}
inline TSharedPtr<FJsonObject> Activity(const UUserWidget* Widget)
{
    auto* W=Service(Widget);const TSharedPtr<FJsonObject>* A=nullptr;
    return W&&W->IsEnabled()&&W->IsConnected()&&W->Snapshot()&&W->Snapshot()->TryGetObjectField(TEXT("activities"),A)?*A:nullptr;
}
inline FString String(const TSharedPtr<FJsonObject>& O,const TCHAR* Key){FString V;if(O)O->TryGetStringField(Key,V);return V;}
inline int32 Number(const TSharedPtr<FJsonObject>& O,const TCHAR* Key){double V=0;if(O)O->TryGetNumberField(Key,V);return int32(V);}
inline bool Signed(const TSharedPtr<FJsonObject>& A){bool V=false;if(A)A->TryGetBoolField(TEXT("signed"),V);return V;}
inline TSharedPtr<FJsonObject> Object(const TSharedPtr<FJsonObject>& O,const TCHAR* Key){const TSharedPtr<FJsonObject>* V=nullptr;return O&&O->TryGetObjectField(Key,V)?*V:nullptr;}
inline FString OwnedName(const TSharedPtr<FJsonObject>& State,const FString& Item)
{
    const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;if(State&&State->TryGetArrayField(TEXT("item_instances"),Rows))for(const auto& V:*Rows){auto O=V->AsObject();if(String(O,TEXT("item"))==Item)return String(Object(O,TEXT("definition")),TEXT("name"));}
    return Item.IsEmpty()?FString(TEXT("空槽")):Item;
}
}
