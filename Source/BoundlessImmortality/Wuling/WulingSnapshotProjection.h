#pragma once
#include "CoreMinimal.h"
class UCultivationSubsystem;
class FJsonObject;

namespace WulingSnapshotProjection
{
    BOUNDLESSIMMORTALITY_API bool Apply(UCultivationSubsystem& Cultivation,const TSharedPtr<FJsonObject>& Snapshot,const FString& ServiceAnchorId);
}
