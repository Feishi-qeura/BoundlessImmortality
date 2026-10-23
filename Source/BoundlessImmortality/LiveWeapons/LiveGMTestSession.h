#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LiveInteractionSubsystem.h"
#include "LiveGMTestSession.generated.h"
class ALiveGMOverlay;

USTRUCT(BlueprintType)
struct BOUNDLESSIMMORTALITY_API FLiveGMCheckResult
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString Name;
    UPROPERTY(BlueprintReadOnly) FString Expected;
    UPROPERTY(BlueprintReadOnly) FString Actual;
    UPROPERTY(BlueprintReadOnly) FString Status;
    UPROPERTY(BlueprintReadOnly) FString Source;
    UPROPERTY(BlueprintReadOnly) FString Time;
};

UCLASS(ClassGroup=(Testing),meta=(BlueprintSpawnableComponent))
class BOUNDLESSIMMORTALITY_API ULiveGMTestSession : public UActorComponent
{
    GENERATED_BODY()
public:
    ULiveGMTestSession();
    UPROPERTY(Transient,BlueprintReadOnly,Category="GM|Presentation") int32 PreviewRealm=0;
    UPROPERTY(Transient,BlueprintReadOnly,Category="GM|Presentation") FString PreviewTitle;
    UPROPERTY(Transient,BlueprintReadOnly,Category="GM|Presentation") double PreviewUntil=0;
    UPROPERTY(Transient,BlueprintReadOnly,Category="GM|Presentation") double AdvanceUntil=0;
    virtual void TickComponent(float Delta,ELevelTick TickType,FActorComponentTickFunction* Function)override;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="GM|Audience") FString ViewerId=TEXT("viewer_qa");
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="GM|Audience") FString ViewerName=TEXT("试剑修士");
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="GM|Audience") FString ViewerAvatarUrl;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="GM|Events") FString Comment=TEXT("加入");
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="GM|Results") TArray<FLiveGMCheckResult> Results;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="GM|Results") int32 ReceivedEvents=0;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="GM|Results") FString LastEvent;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="GM|Results") FString ReportPath;
    static bool IsIsolatedCommandLine(const FString& CommandLine);
    UFUNCTION(BlueprintPure,Category="GM|Session") bool CanMutate()const;
    UFUNCTION(BlueprintPure,Category="GM|Session") FString DescribeSession()const;
    UFUNCTION(BlueprintCallable,Category="GM|Session") bool RunAction(FName Action);
    UFUNCTION(BlueprintCallable,Category="GM|Session") void RunLocalChecks();
    UFUNCTION(BlueprintCallable,Category="GM|Results") bool ExportResults();
    UFUNCTION(BlueprintCallable,Category="GM|Results") bool LoadLatestUIResults();
    virtual void BeginPlay()override;
    virtual void EndPlay(const EEndPlayReason::Type Reason)override;
private:
    TWeakObjectPtr<ULiveInteractionSubsystem> Host;
    int64 Sequence=0;
    int32 ContinuousRemaining=0;float ContinuousWait=0;
    FString NextId();
    bool Join(bool Enter,bool Duplicate=false);
    void Record(const FString& Name,const FString& Expected,const FString& Actual,bool Passed);
    UFUNCTION() void CommentReceived(const FLiveComment& Event);
    UFUNCTION() void PresenceReceived(const FLivePresence& Event);
    UFUNCTION() void LikeReceived(const FLiveLike& Event);
    UFUNCTION() void GiftReceived(const FLiveGift& Event);
    UFUNCTION() void FollowReceived(const FLiveFollow& Event);
    UFUNCTION() void SessionChanged();
};
