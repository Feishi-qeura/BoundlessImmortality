#pragma once
#include "CoreMinimal.h"
#include "UObject/UObjectGlobals.h"

// Legacy dynamically assembled UI paths resolve to the canonical typed folders.
namespace InkUiAssets
{
    BOUNDLESSIMMORTALITY_API FString Resolve(const TCHAR* Path);
    template<class T,class... Args> T* Load(UObject* Outer,const TCHAR* Path,Args&&... Arguments)
    {const FString Canonical=Resolve(Path);return LoadObject<T>(Outer,*Canonical,Forward<Args>(Arguments)...);}
}
