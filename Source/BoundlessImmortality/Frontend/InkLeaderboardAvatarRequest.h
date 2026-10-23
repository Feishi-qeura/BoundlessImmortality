#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InkLeaderboardAvatarRequest.generated.h"
class UInkFrontendWidget;
class UTexture2D;
class ULiveInteractionSubsystem;

/** Owns one HTTP image task until its identity-scoped result is delivered. */
UCLASS()
class UInkLeaderboardAvatarRequest : public UObject
{
    GENERATED_BODY()
public:
    void Start(UInkFrontendWidget* InOwner,const FString& InKey,const FString& Url);
    void Cancel();
    bool HasExpired()const{return FPlatformTime::Seconds()-StartedAt>12.;}
    virtual void BeginDestroy()override;
private:
    TWeakObjectPtr<UInkFrontendWidget> Owner;
    FString Key;
    TWeakObjectPtr<ULiveInteractionSubsystem> Host;
    double StartedAt=0;
    UFUNCTION() void Succeeded(const FString& UserId,UTexture2D* Texture);
};
