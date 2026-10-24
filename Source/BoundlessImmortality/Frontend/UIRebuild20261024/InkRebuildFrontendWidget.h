#pragma once
#include "CoreMinimal.h"
#include "Frontend/InkFrontendWidget.h"
#include "InkRebuildFrontendWidget.generated.h"

class UInkRankBoardPage;
class UInkLeaderboardAvatarRequest;
class UCanvasPanel;
class UWidget;
class UWidgetTree;

/** New default presentation: independent tree, retained gameplay/data/action owners. */
UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkRebuildFrontendWidget : public UInkFrontendWidget
{
    GENERATED_BODY()
public:
    virtual void BuildDefaultTree(UWidgetTree* Tree)override;
    virtual bool UsesRebuiltPresentation()const override{return true;}
    virtual FName GetRuntimeLayoutVersion()const override{return TEXT("newui-r1-root-20261024");}
    virtual FName GetRuntimePageLayoutVersion(FName Page)const override;
    virtual void CompleteLeaderboardAvatar(const FString& Key,UTexture2D* Texture)override;
    /** Mount a reviewed owner component into the new shell; no legacy root fallback. */
    void MountPageComponent(FName Page,UWidget* Component,FName Version);
    UCanvasPanel* GetPageContentCanvas(FName Page)const;
protected:
    virtual void RefreshRebuiltPresentation()override;
    virtual void TickRebuiltPresentation(const FGeometry& Geometry,float DeltaSeconds)override;
    virtual bool HandleRebuiltNavigation(FName Action)override;
    virtual void SetRebuiltFeedback(const FText& Message)override;
    virtual UInkExtensionWidget* CreateRebuiltFeaturePage(FName Page)override;
    virtual FName ResolveRebuiltFeatureRoute(FName Page,const FString& Context)const override;
private:
    UPROPERTY(Transient) TMap<FName,TObjectPtr<UCanvasPanel>> PageCanvases;
    UPROPERTY(Transient) TMap<FName,TObjectPtr<UCanvasPanel>> PageBodies;
    UPROPERTY(Transient) TMap<FName,TObjectPtr<UWidget>> PageComponents;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> SafeRoot;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> ConfirmationRoot;
    TMap<FName,FName> PageVersions;
    TMap<FName,FString> ContentKeys;
    FVector2D LastSafeSize=FVector2D::ZeroVector;
    FText LatestFeedback;
    float RefreshElapsed=0;
    float HealthFraction=0,CultivationFraction=0;
    bool bRefreshing=false;
    void BuildConfirmation(UWidgetTree* Tree);
    void LayoutPages(FVector2D Size);
    void RefreshMainAndHud();
    void RefreshLocalPage(FName Page);
    UPROPERTY(Transient) TObjectPtr<UInkRankBoardPage> NewRankBoard;
    UPROPERTY(Transient) TMap<FString,TObjectPtr<UInkLeaderboardAvatarRequest>> NewRankRequests;
    UPROPERTY(Transient) TMap<FString,TObjectPtr<UTexture2D>> NewRankAvatarCache;
    TArray<FInkLeaderboardEntry> NewRankRows;
    TMap<FString,FString> NewRankAvatarSubjects,NewRankPendingIds;
    TMap<FString,double> NewRankAvatarRetryAt;
    FString NewRankContentKey;
    void RefreshNewLeaderboard();
    void PollNewRankAvatars();
    UFUNCTION() void HandleNewRankCategory(FName Category);
    UFUNCTION() void HandleNewRankScope(FName Scope);
    UFUNCTION() void HandleNewRankRetry();
    UFUNCTION() void HandleNewRankAvatar(FString ParticipantId);
    UFUNCTION() void HandleNewInventorySearch(const FText& Text);
};
