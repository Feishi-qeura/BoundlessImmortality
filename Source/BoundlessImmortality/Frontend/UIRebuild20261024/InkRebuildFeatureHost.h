#pragma once
#include "CoreMinimal.h"
#include "Frontend/InkExtensionWidget.h"
#include "Activities/RebuildActivityWidgets.h"
#include "Player/InkPlayerMethodsEquipmentAppearanceWidgets.h"
#include "Player/InkPlayerCraftSourcesWidgets.h"
#include "Player/InkPlayerBreakthroughTribulationWidgets.h"
#include "InkRebuildFeatureHost.generated.h"

class FJsonObject;
class UCanvasPanel;
class UTextBlock;
class UScrollBox;
class UWulingPrototypeSubsystem;

/** Actual feature host. Business state is retained; every visible tree is newUI. */
UCLASS()
class BOUNDLESSIMMORTALITY_API UInkRebuildFeatureHost : public UInkExtensionWidget
{
    GENERATED_BODY()
public:
    static bool SupportsRoute(FName Route);
    static FName CanonicalRoute(FName Route,const FString& Context);
    virtual void Configure(UInkFrontendWidget* InFrontend,FName InPage)override;
    virtual void SetContext(const FString& Entry)override;
    virtual void HandleAction(FName Action)override;
    FName GetLayoutVersion()const;
    FString GetCompositionName()const{return CompositionName;}
    void RefreshAuthority(bool Force=false);
protected:
    virtual void NativeOnInitialized()override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds)override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)override;
private:
    UPROPERTY(Transient) TObjectPtr<UInkFrontendWidget> FrontendOwner;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> RootCanvas;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> BodyCanvas;
    UPROPERTY(Transient) TObjectPtr<UUserWidget> OwnerPage;
    UPROPERTY(Transient) TObjectPtr<UWidget> PurePage;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> FeedbackText;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> PureScroll;
    TSharedPtr<FJsonObject> DisplaySnapshot;
    FVector2D LastSize=FVector2D(720,1280);
    FString SemanticKey,CompositionName,SecondaryEntry,RequestedCatalog,ActorSubject;
    FString ConfirmedStageFingerprint;
    FString PendingGuildOperation,CapturedGuildActor,CapturedGuildId,CapturedGuildVersion;
    FName DraftPreviewFrame,DraftPreviewBar;
    FString DraftQuantity=TEXT("1"),DraftPrice,StageFingerprint,ConfirmedDay,ConfirmedMonth;
    FName DraftMainMethod,PreviewFrame,PreviewBar,AppearanceTab=TEXT("Frame");
    TArray<FName> DraftSupportMethods;
    FGuid SelectedInventoryEntry;
    TMap<FString,int32> MaterialSelection,ConfirmedMaterialSelection;
    uint64 SelectionRevision=0,StageSubjectRevision=0,ConfirmedSelectionRevision=0,ConfirmedStageRevision=0;
    int32 QueryOffset=0,ActionQuantity=1;
    bool bOwnedOnly=false,bRework=false,bConfirmOpen=false,bConfirmedRework=false,bRefreshing=false;
    float RefreshElapsed=0;
    void BuildShell();
    void LayoutShell();
    void EnsureOwnerPage();
    void MountPure(UWidget* Page,const FString& Name);
    void RetirePurePage();
    void RefreshJourney();
    void RefreshActivities();
    void RefreshRankFamily();
    void RefreshPlayer();
    void RefreshEconomy();
    void Navigate(FName Route,const FString& Context=FString());
    void Feedback(const FString& Message);
    bool IsActivePage()const;
    bool CanDispatch()const;
    UWulingPrototypeSubsystem* Service()const;
    bool Submit(const FString& Operation,const TSharedRef<FJsonObject>& Args);
    void QueryCatalog(const FString& Kind,int32 Offset,bool OwnedOnly);
    void HandlePlayerC(FName Action,const InkPlayerMethodsEquipmentAppearanceWidgets::FActionPayload& Payload);
    void HandlePlayerD(FName Action,const InkPlayerCraftSourcesWidgets::FActionPayload& Payload);
    void HandlePlayerE(FName Action,const InkPlayerBreakthroughTribulationWidgets::FActionPayload& Payload);
    UFUNCTION() void HandleActivity(FRebuildActivityActionRequest Request);
    UFUNCTION() void HandleBack();
    UFUNCTION() void HandleMonthlyNavigation();
    UFUNCTION() void HandleJourneySelected(const FString& EntryId);
    UFUNCTION() void HandleJourneyTravel(const FString& EntryId);
    UFUNCTION() void HandleSignIn(const FString& Day);
    UFUNCTION() void HandleMilestone(const FString& Day,const FString& Month,int32 Milestone);
    UFUNCTION() void HandleCancelQueue();
    UFUNCTION() void HandleRaceAction(const FString& RaceId,const FString& Kind);
    UFUNCTION() void HandleHubNavigation(const FString& Action);
    UFUNCTION() void HandleWulingAction(FName Action);
    UFUNCTION() void HandleMemberSelected(FString UserId);
    UFUNCTION() void HandleMemberPage(int32 MemberCursor);
    UFUNCTION() void HandleMemberRemove(FString UserId);
    UFUNCTION() void HandleBuildQuery(int32 Offset,bool OwnedOnly);
    UFUNCTION() void HandleBuild(FString BuildingId,int32 Level,FString Version);
    UFUNCTION() void HandleRetry();
    UFUNCTION() void HandleGiftNavigation();
};
