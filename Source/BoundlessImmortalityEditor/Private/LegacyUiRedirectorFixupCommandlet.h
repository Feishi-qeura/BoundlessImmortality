#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "LegacyUiRedirectorFixupCommandlet.generated.h"

UCLASS()
class BOUNDLESSIMMORTALITYEDITOR_API ULegacyUiRedirectorFixupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	ULegacyUiRedirectorFixupCommandlet();
	virtual int32 Main(const FString& Params) override;
	/** Accept one mode plus only the engine switches needed for a headless invocation. */
	static bool ParseMode(const FString& Params, bool& bPreflightOnly);
};
