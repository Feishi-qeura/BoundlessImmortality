#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "WulingPackageAcceptance.generated.h"

class UInkFrontendWidget;
class UWulingPrototypeSubsystem;
class AInkMenuDirector;
class UInkSignInChestWidget;
class FJsonValue;

/** Opt-in rendered acceptance for an isolated local Development package. Never runs in Shipping. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UWulingPackageAcceptance : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;
    virtual UWorld* GetTickableGameObjectWorld() const override;
private:
    TWeakObjectPtr<UInkFrontendWidget> Frontend;
    TWeakObjectPtr<UWulingPrototypeSubsystem> Authority;
    TWeakObjectPtr<AInkMenuDirector> Director;
    TWeakObjectPtr<UInkSignInChestWidget> LifecycleChest;
    TArray<TSharedPtr<FJsonValue>> Checks;
    TArray<FString> Captures;
    FString Output;
    double Started=0,Next=0;
    int32 Step=0,Page=0,LifecyclePass=0;
    bool bRunning=false,bPassed=true;
    void Check(const FString& Name,bool Passed);
    void Capture(const FString& Name);
    bool Click(FName Action);
    void Finish();
};
