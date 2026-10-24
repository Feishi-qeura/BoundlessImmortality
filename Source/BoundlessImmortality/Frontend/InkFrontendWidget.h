#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "InkFrontendTypes.h"
#include "InkLeaderboardPresentation.h"
#include "UI/InkWheelPresentation.h"
#include "InkFrontendWidget.generated.h"

class AInkMenuDirector;
class UWidgetTree;
class UFont;
class UTexture2D;
class UMaterialInterface;
class UTextBlock;
class UVerticalBox;
class UBorder;
class UWidgetSwitcher;
class UInkFrontendWidget;
class UInkInventoryModel;
class UCanvasPanel;
class UInkExtensionWidget;
class UInkAnchorStatusWidget;
class UInkLeaderboardAvatarRequest;
class UInkRewardReceiptPanel;

/** Action id is saved in the WBP tree, so designer changes do not discard behavior. */
UCLASS(BlueprintType)
class BOUNDLESSIMMORTALITY_API UInkFrontendActionButton : public UButton
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ink UI") FName Action;
    void BindFrontend(UInkFrontendWidget* InFrontend);
private:
    TWeakObjectPtr<UInkFrontendWidget> Frontend;
    UFUNCTION() void HandleClicked();
    UFUNCTION() void HandleHovered();
    UFUNCTION() void HandleUnhovered();
    UFUNCTION() void HandlePressed();
    UFUNCTION() void HandleReleased();
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInkFrontendActionEvent,FName,Action);

/** Reusable portrait frontend. Static layout can be authored and saved as a Widget Blueprint. */
UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkFrontendWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UInkFrontendWidget(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Art") TSoftObjectPtr<UTexture2D> PanelTexture;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Art") TSoftObjectPtr<UTexture2D> ButtonTexture;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Art") TSoftObjectPtr<UTexture2D> TitleTexture;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Art") TSoftObjectPtr<UTexture2D> MeditationTexture;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Art") TSoftObjectPtr<UTexture2D> AttributeIconAtlas;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Provided Art") TSoftObjectPtr<UTexture2D> ProvidedButtonAtlas;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Provided Art") TSoftObjectPtr<UTexture2D> ProvidedRankAtlas;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Provided Art") TSoftObjectPtr<UTexture2D> ProvidedInventoryAtlas;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Provided Art") TSoftObjectPtr<UMaterialInterface> TitleFeatherMaterial;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Provided Art") TSoftObjectPtr<UMaterialInterface> ScrollEdgeMaterial;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Art") TSoftObjectPtr<UFont> TextFont;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ink UI|Layout",meta=(ClampMin="0",ClampMax="500")) float ExtraStreamBottomInset=0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ink UI|Layout",meta=(ClampMin="88",ClampMax="128")) float CultivateVisualSize=112;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ink UI|Motion") bool bReducedMotion=false;
    FInkWheelPresentation DrawPreviewWheel;
    TArray<int32> DrawPreviewResults;
    FRandomStream DrawPreviewRandom{61021};
    /** Presentation selection only; never a sign-in record or authority date. */
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Sign In Preview") int32 SignInSelectedDay=1;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Sign In Preview") FName SignInSelectedRoute=TEXT("SignIn");
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Art") FLinearColor PaperColor;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Art") FLinearColor InkColor;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Art") FLinearColor AccentColor;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Art") FLinearColor CinnabarColor;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Art", meta=(ClampMin="20",ClampMax="60")) int32 BodyFontSize=28;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Art", meta=(ClampMin="64",ClampMax="128")) float ButtonHeight=80;
    /** Runtime landscape adaptation only. Disable to preserve a fully authored main-menu layout. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ink UI|Layout") bool bAdaptiveMainLayout=true;
    /** Constrain page widths and navigation sizes, with compact landscape headers. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ink UI|Layout") bool bAdaptiveSurfaceLayout=true;
    /** Applies paper-safe padding to the five subpages, including previously saved WBP trees. Disable for custom Designer padding. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ink UI|StyleLayout") bool bUsePageSafePadding=true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ink UI|StyleLayout", meta=(EditCondition="bUsePageSafePadding")) FMargin PageSafePadding=FMargin(84,108,76,96);

    /** Main / HUD / Saves / Settings / Character / Inventory / Leaderboard. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ink UI") FName CurrentPage=TEXT("Main");
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Preview") FName DesignerPreviewPage=TEXT("Main");
    UPROPERTY(BlueprintAssignable, Category="Ink UI") FInkFrontendActionEvent OnActionTriggered;
#if WITH_EDITOR
    // Safe PIE route acceptance uses this pair to distinguish a handled Slate mouse-down from an actual button callback.
    FName LastDispatchedAction;
    uint32 ActionDispatchSerial=0;
#endif
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI") TArray<TObjectPtr<UInkFrontendActionButton>> ActionButtons;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ink UI") FName PendingConfirmationAction;

    UFUNCTION(BlueprintCallable, Category="Ink UI") void Configure(AInkMenuDirector* InDirector);
    UFUNCTION(BlueprintCallable, Category="Ink UI") void ShowPage(FName Page);
    UFUNCTION(BlueprintCallable, Category="Ink UI") void TriggerAction(FName Action);
    UFUNCTION(BlueprintCallable, Category="Ink UI|Data") void SetCharacterData(const FText& Name,const TArray<FInkCharacterAttribute>& Attributes,const TArray<FInkDisciplineEntry>& Disciplines);
    UFUNCTION(BlueprintCallable, Category="Ink UI|Data") void SetInventoryItems(const TArray<FInkInventoryItem>& Items);
    UFUNCTION(BlueprintCallable, Category="Ink UI|Inventory") UInkInventoryModel* GetInventoryModel();
    UFUNCTION(BlueprintCallable, Category="Ink UI|Inventory") int32 AddInventoryItem(const FInkInventoryItem& Item,int32 Quantity=1);
    UFUNCTION(BlueprintCallable, Category="Ink UI|Inventory") void RefreshInventoryFromModel();
    UFUNCTION(BlueprintCallable, Category="Ink UI|Inventory") void HoverInventorySlot(int32 SlotIndex,bool bHovered);
    UFUNCTION(BlueprintCallable, Category="Ink UI|Inventory") void SetInventoryUnlockedSlots(int32 Count);
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Inventory") TObjectPtr<UInkInventoryModel> InventoryModel;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Inventory",meta=(ClampMin="0",ClampMax="200")) int32 DefaultUnlockedInventorySlots=20;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Inventory") FGuid SelectedInventoryEntry;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Inventory") FGuid HoveredInventoryEntry;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Inventory") FName InventoryCategory=TEXT("All");
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Inventory") FString InventorySearchText;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Inventory") int32 InventoryPageIndex=0;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Inventory") int32 InventoryActionQuantity=1;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Inventory") bool bInventoryListView=false;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Inventory") bool bInventorySortAscending=true;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Inventory") bool bInventoryFilterUsable=false;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Inventory") bool bInventoryFilterStackable=false;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Inventory") bool bInventoryMultiSelect=false;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Inventory") TSet<FGuid> InventoryBatchSelection;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Inventory") FName InventoryQuantityPreset=TEXT("1");
    UFUNCTION(BlueprintCallable, Category="Ink UI|Data") void SetLeaderboardEntries(const TArray<FInkLeaderboardEntry>& Entries);
    /** Global data is supplied explicitly; room rows never masquerade as a global board. */
    UFUNCTION(BlueprintCallable, Category="Ink UI|Data") void SetLeaderboardEntriesForScope(const TArray<FInkLeaderboardEntry>& Entries,FName Scope);
    UFUNCTION(BlueprintCallable, Category="Ink UI|Data") void SetGlobalLeaderboardConnection(bool bConnected,const FText& Status);
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Leaderboard") FName LeaderboardScope=TEXT("Local");
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Leaderboard") bool bGlobalLeaderboardConnected=false;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Leaderboard") FText GlobalLeaderboardStatus;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Extensions") TMap<FName,TObjectPtr<UInkExtensionWidget>> ExtensionPages;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Anchor Status") TObjectPtr<UInkAnchorStatusWidget> AnchorStatusWidget;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Rewards") TObjectPtr<UInkRewardReceiptPanel> RewardReceiptPanel;
    UFUNCTION(BlueprintCallable, Category="Ink UI|Extensions") void RefreshExtensionActions();
    UFUNCTION(BlueprintCallable, Category="Ink UI|Panels") bool OpenFeaturePanel(FName Page,const FString& Context);
    UFUNCTION(BlueprintCallable, Category="Ink UI|Panels") void CloseFeaturePanel(bool bReturnToGame=false);
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Panels") TArray<FName> FeaturePanelStack;
    UFUNCTION(BlueprintCallable, Category="Ink UI") bool RevealFrontendAction(FName Action);
    virtual void CompleteLeaderboardAvatar(const FString& Key,UTexture2D* Texture);
    const TArray<FInkCharacterAttribute>& GetPresentedCharacterAttributes()const{return CharacterAttributes;}
    const FText& GetPresentedCharacterName()const{return CharacterName;}
    const TArray<FInkLeaderboardEntry>& GetPresentedRoomLeaderboard()const{return LeaderboardEntries;}
    bool HasPresentedRoomLeaderboard()const{return bRolloutRoomLeaderboardProvided;}
    const TArray<FInkDisciplineEntry>& GetPresentedCharacterDisciplines()const{return CharacterDisciplines;}
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Leaderboard",meta=(ClampMin="0.05",ClampMax="3")) float LeaderboardFadeInSeconds=.4f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Leaderboard",meta=(ClampMin="0.05",ClampMax="3")) float LeaderboardFadeOutSeconds=.5f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Leaderboard",meta=(ClampMin="0",ClampMax="10")) float DamageLeaderboardHoldSeconds=2.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Leaderboard",meta=(ClampMin="240",ClampMax="400")) float CompactLeaderboardWidth=320.f;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ink UI|Leaderboard",meta=(ClampMin="0",ClampMax="100")) float LeaderboardRightInset=20.f;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Leaderboard") FName ActiveGameplayLeaderboard;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Leaderboard") float GameplayLeaderboardOpacity=0;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Leaderboard") bool bGameplayLeaderboardRequested=false;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Leaderboard") FName RequestedGameplayLeaderboard=TEXT("CultivationSpeed");
    UFUNCTION(BlueprintCallable, Category="Ink UI|Data") void SetFeedback(const FText& Message);
    UFUNCTION(BlueprintPure, Category="Ink UI") bool IsQuitConfirmationVisible() const;

    /** Intended for the editor factory: creates a serializable tree only when it is empty. */
    virtual void BuildDefaultTree(UWidgetTree* Tree);
    /** New presentation controllers reuse business state without rebuilding the legacy tree. */
    virtual bool UsesRebuiltPresentation()const{return false;}
    UFUNCTION(BlueprintPure,Category="newUI|Verification") virtual FName GetRuntimeLayoutVersion()const{return NAME_None;}
    UFUNCTION(BlueprintPure,Category="newUI|Verification") virtual FName GetRuntimePageLayoutVersion(FName Page)const{return NAME_None;}
    AInkMenuDirector* GetPresentationDirector()const{return Director.Get();}
    FName GetPresentedLeaderboardCategory()const{return LeaderboardCategory;}
    const TArray<FInkLeaderboardEntry>& GetPresentedGlobalLeaderboard()const{return GlobalLeaderboardEntries;}
    /** Replaces only the inventory surface; used by the backed-up editor migration. */
    void RebuildInventoryPage(UWidgetTree* Tree);
    /** Replaces only the leaderboard page and its HUD overlay. */
    void RebuildLeaderboardSurfaces(UWidgetTree* Tree);

protected:
    void RebindPresentationActions(){BindTree();}
    virtual void RefreshRebuiltPresentation(){}
    virtual UInkExtensionWidget* CreateRebuiltFeaturePage(FName Page){return nullptr;}
    virtual FName ResolveRebuiltFeatureRoute(FName Page,const FString& Context)const{return Page;}
    virtual void TickRebuiltPresentation(const FGeometry& Geometry,float DeltaSeconds){}
    virtual bool HandleRebuiltNavigation(FName Action){return false;}
    virtual void SetRebuiltFeedback(const FText& Message){}
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeOnInitialized() override;
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& KeyEvent) override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& KeyEvent) override;

private:
    UPROPERTY(Transient) TObjectPtr<AInkMenuDirector> Director;
    UPROPERTY(Transient) TObjectPtr<UBorder> RuntimeHUDFeedbackPanel;
    UPROPERTY(Transient) TArray<FInkCharacterAttribute> CharacterAttributes;
    UPROPERTY(Transient) TArray<FInkDisciplineEntry> CharacterDisciplines;
    bool bRolloutRoomLeaderboardProvided=false;
    UPROPERTY(Transient) TArray<FInkLeaderboardEntry> LeaderboardEntries;
    UPROPERTY(Transient) TArray<FInkLeaderboardEntry> GlobalLeaderboardEntries;
    FText CharacterName;
    FName LeaderboardCategory=TEXT("Anchor");
    float StatusRefreshTime=0;
    FAnchorData AuthoredMainMenuLayout;
    bool bMainMenuLayoutCached=false;
    bool bMainMenuLayoutAdapted=false;
    float LastMainLayoutRatio=-1;
    TMap<FName,FIntPoint> LastPageFrameSizes;
    bool bInventoryModelDirty=false;
    bool bInventoryReadingHover=false;
    bool bInventoryPortrait=false;
    bool bInventoryLayoutInitialized=false;
    bool bLeaderboardPortrait=false,bLeaderboardLayoutInitialized=false;
    void UpdateLeaderboardAdaptiveLayout(bool bForce=false);
    int32 InventoryGridColumns=10;
    float LeaderboardAvatarPoll=0;
    TSet<FSoftObjectPath> RequestedLeaderboardAvatars;
    UPROPERTY(Transient) TMap<FString,TObjectPtr<UTexture2D>> LeaderboardAvatarCache;
    UPROPERTY(Transient) TMap<FString,TObjectPtr<UInkLeaderboardAvatarRequest>> LeaderboardAvatarRequests;
    TMap<FString,double> LeaderboardAvatarRetryAt;
    TMap<FString,FInkLeaderboardEntry> PresentedLeaderboardRows;
    InkLeaderboard::FFadeState LeaderboardFade;
    float LeaderboardActivityPoll=0;
    bool bLeaderboardCultivating=false;
    bool bLeaderboardAttacking=false;
    TWeakObjectPtr<APawn> LeaderboardObservedPawn;

    void BindTree();
    void EnsureExtensionPages();
    bool EnsureFeaturePage(FName Page);
    void UpdateInventoryAdaptiveLayout();
    void EnsureLeaderboardControls();
    void UpdateLeaderboardAvatars();
    void RefreshPage();
    void RefreshLiveStatus();
    void UpdateAdaptiveMainLayout();
    void ApplyMainMenuPresentation();
    void UpdateAdaptiveSurfaceLayout();
    void UpdateAdaptiveHudLayout();
    FVector2D LastHudSafeSize=FVector2D::ZeroVector;
    FVector2D LastHudLayoutTokens=FVector2D::ZeroVector;
    float LastHudBottomInset=-1;
    bool bRuntimeHudBuilt=false;
    void UpdateConfirmationLayout();
    void ApplyPageSafePadding();
    void ApplyProvidedArt();
    void RefreshCharacter(UWidgetTree* SourceTree=nullptr);
    void RefreshInventory(UWidgetTree* SourceTree=nullptr);
    void BuildInventorySurface(UWidgetTree* Tree,UCanvasPanel* Page);
    void EnsureInventoryDetailWindow();
    void EnsureInventoryTooltipWindow();
    void RemoveInventoryWindow(FName Name);
    void RefreshInventoryDetails(UWidgetTree* Tree);
    void UpdateInventoryTooltip();
    bool ExecuteInventoryAction(FName Action);
    UFUNCTION() void HandleInventorySearchChanged(const FText& Text);
    UFUNCTION() void HandleInventoryModelChanged();
    void RefreshLeaderboard(UWidgetTree* SourceTree=nullptr);
    void UpdateGameplayLeaderboard(float DeltaSeconds);
    void RefreshLeaderboardRows(UWidgetTree* Tree,FName Category,bool bCompact);
    void RefreshSaves();
    void RefreshSettings();
    void UpdateRewardReceipts();
    UFUNCTION() void HandleRewardReceiptClosed();
    void ExecuteAction(FName Action,bool bConfirmed=false);
    void SetQuitConfirmation(bool bVisible);
    void ShowConfirmation(const FText& Title,const FText& Message,FName ConfirmAction);
    void SetPageFeedback(const FText& Message);
    UTextBlock* EnsureHUDFeedback();
    UTextBlock* AddText(UWidgetTree* Tree,UVerticalBox* Box,const FText& Text,int32 Size=0,FName Name=NAME_None);
    UInkFrontendActionButton* MakeButton(UWidgetTree* Tree,FName Name,FName Action,const FText& Text);
    UBorder* MakeInkCard(UWidgetTree* Tree,UWidget* Content,FMargin Padding=FMargin(12),bool bSelected=false);
    UBorder* MakeLeaderboardRow(UWidgetTree* Tree,const FText& Rank,const FText& Name,const FText& Metric,bool bHeader=false);
};
