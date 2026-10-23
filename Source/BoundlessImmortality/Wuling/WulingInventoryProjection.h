#pragma once
#include "CoreMinimal.h"
class FJsonObject;
class UInkInventoryModel;
namespace WulingInventoryProjection
{
    void Apply(UInkInventoryModel& Model,const TSharedPtr<FJsonObject>& State);
}
