#pragma once
#include "CoreMinimal.h"
#include "InkProgressState.h"
#include "InkExtensionWidget.h"
#include "InkFrontendWidget.h"
#include "InkPresentationStyle.h"
#include "InkFeaturePanel.generated.h"

class UCanvasPanel;
class USizeBox;
class UTextBlock;
class UImage;
class UScrollBox;
class UEditableTextBox;
class UInkCharacterCultivationWidget;
class UTexture2D;

/** A feature owns its widget tree and local state; the frontend owns navigation. */
UCLASS(Abstract,Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkFeaturePanel : public UInkExtensionWidget
{
    GENERATED_BODY()
public:
    virtual void Configure(UInkFrontendWidget* InFrontend,FName InPage) override;
    virtual void HandleAction(FName Action) override;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Layout") bool bPortraitLayout=true;
    static TSubclassOf<UInkFeaturePanel> ClassForRoute(FName Route);
    void SetContext(const FString& Entry);
    void ConfigureEmbedded(UInkFrontendWidget* Owner,FName Route,bool Portrait,FVector2D Size);
    virtual void RefreshLiveData();
    /** Includes the selected child of a retained SignIn/Activities group. */
    bool IsRolloutPageActive() const;
protected:
    UFUNCTION() void HandleRolloutSignInRequested();
    FString RolloutSignInDay;
    FString RolloutChestDay;
    virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
    virtual FText PanelTitle() const PURE_VIRTUAL(UInkFeaturePanel::PanelTitle,return FText::GetEmpty(););
    virtual void BuildPanelContent() PURE_VIRTUAL(UInkFeaturePanel::BuildPanelContent,);
    virtual void HandlePanelAction(const FString& Action);
    void RebuildPanel();
    bool UsesResponsiveShell()const;
    void RebuildResponsivePanel();
    void ExpandCompactActionRows();
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> ResponsiveRoot;
    UPROPERTY(Transient) TObjectPtr<USizeBox> EmbeddedContentSize;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> ResponsiveScroll;
    UPROPERTY(Transient) TObjectPtr<USizeBox> ResponsiveContentSize;
    float PageOpenElapsed=0;
    FVector2D ResponsiveSafeSize=FVector2D(720,1280);
    UTextBlock* Label(UCanvasPanel* Parent,FName Name,const FString& Value,float X,float Y,float W,float H,int32 FontSize=24,bool bCenter=false);
    UInkFrontendActionButton* Action(UCanvasPanel* Parent,const FString& Id,const FString& Caption,float X,float Y,float W,float H,bool bPrimary=false,bool bBlue=false,bool bSelected=false);
    UImage* Surface(UCanvasPanel* Parent,FName Name,InkPresentationStyle::ESurface Part,float X,float Y,float W,float H);
    UImage* Icon(UCanvasPanel* Parent,FName Name,InkPresentationStyle::EIcon Part,float X,float Y,float Size);
    UImage* AtlasIcon(UCanvasPanel* Parent,FName Name,int32 Tile,float X,float Y,float Size);
    void Reward(UCanvasPanel* Parent,FName Name,int32 Item,const FString& Caption,float X,float Y,float Size,bool bSelected=false);
    void Scene(UCanvasPanel* Parent,int32 Tile,float X,float Y,float W,float H);
    void Link(UCanvasPanel* Parent,const FString& Route,const FString& Title,const FString& Caption,int32 Tile,float X,float Y,float W,float H);
    void FeedbackMessage(const FString& Text);
    void Open(const FString& Route,const FString& Context=FString());
    void BuildCharacterSection(const FString& Section);
    void BuildSignInChestStrip(bool Monthly,float Y,int32 SelectedChest);
    bool HandleSignInChestAction(const FString& Id,bool Monthly,int32& SelectedChest);
    UPROPERTY(Transient) TObjectPtr<UInkFrontendWidget> FrontendOwner;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> BodyCanvas;
    UPROPERTY(Transient) TObjectPtr<UCanvasPanel> ShellCanvas;
    UPROPERTY(Transient) TObjectPtr<USizeBox> PanelSize;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
    float BodyWidth=620,BodyHeight=870;
    bool bSurfaceInitialized=false,bHasLayout=false,bEmbedded=false;
    FString AuthorityContentKey;
};

UCLASS(Abstract) class BOUNDLESSIMMORTALITY_API UInkFeatureGroupPanel : public UInkFeaturePanel
{
    GENERATED_BODY()
public:
    void SelectRoute(FName Route);
    virtual void RefreshLiveData()override;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI") FName SelectedRoute;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI") TMap<FName,TObjectPtr<UInkFeaturePanel>> ContentPanels;
protected:
    virtual void BuildPanelContent()override;
    virtual void HandlePanelAction(const FString& Action)override;
    virtual bool IsActivities()const{return false;}
    FString AuthorityStateKey;
};
UCLASS() class BOUNDLESSIMMORTALITY_API UInkSignInGroupPanel : public UInkFeatureGroupPanel
{GENERATED_BODY() protected: virtual FText PanelTitle()const override{return FText::FromString(TEXT("签到与福缘"));}};
UCLASS() class BOUNDLESSIMMORTALITY_API UInkActivitiesGroupPanel : public UInkFeatureGroupPanel
{GENERATED_BODY() protected: virtual bool IsActivities()const override{return true;} virtual FText PanelTitle()const override{return FText::FromString(TEXT("仙途活动"));}};

UCLASS() class BOUNDLESSIMMORTALITY_API UInkFeatureHubPanel : public UInkFeaturePanel
{ GENERATED_BODY() protected: virtual FText PanelTitle() const override; virtual void BuildPanelContent() override; };
UCLASS() class BOUNDLESSIMMORTALITY_API UInkSignInPanel : public UInkFeaturePanel
{ GENERATED_BODY() protected: virtual FText PanelTitle() const override; virtual void BuildPanelContent() override; virtual void HandlePanelAction(const FString& Action)override; private: int32 SelectedDay=1,SelectedChest=-1; };
UCLASS() class BOUNDLESSIMMORTALITY_API UInkMonthlySignInPanel : public UInkFeaturePanel
{ GENERATED_BODY() protected: virtual FText PanelTitle() const override; virtual void BuildPanelContent() override; virtual void HandlePanelAction(const FString& Action) override; private: UFUNCTION() void HandleRolloutCalendarDay(int32 Day); int32 SelectedCalendarDay=0; int32 SelectedChest=-1,CalendarMonthOffset=0; };
UCLASS() class BOUNDLESSIMMORTALITY_API UInkDailyFortunePanel : public UInkFeaturePanel
{ GENERATED_BODY() protected: virtual FText PanelTitle() const override; virtual void BuildPanelContent() override; virtual void HandlePanelAction(const FString& Action)override; private: bool bFortuneDrawn=false; };
UCLASS() class BOUNDLESSIMMORTALITY_API UInkActivitiesPanel : public UInkFeaturePanel
{ GENERATED_BODY() protected: virtual FText PanelTitle() const override; virtual void BuildPanelContent() override; };
UCLASS() class BOUNDLESSIMMORTALITY_API UInkDailyActivitiesPanel : public UInkFeaturePanel
{ GENERATED_BODY() protected: virtual FText PanelTitle() const override; virtual void BuildPanelContent() override; };
UCLASS() class BOUNDLESSIMMORTALITY_API UInkTimedActivitiesPanel : public UInkFeaturePanel
{ GENERATED_BODY() protected: virtual FText PanelTitle() const override; virtual void BuildPanelContent() override; };
UCLASS() class BOUNDLESSIMMORTALITY_API UInkDrawPanel : public UInkFeaturePanel
{ GENERATED_BODY() public: UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI") TArray<int32> DemoResults;
protected: virtual FText PanelTitle() const override; virtual void BuildPanelContent() override; virtual void HandlePanelAction(const FString& Action)override; virtual void NativeTick(const FGeometry& Geometry,float Delta)override;
private: bool bShowRules=false; };
UCLASS() class BOUNDLESSIMMORTALITY_API UInkExpeditionPanel : public UInkFeaturePanel
{ GENERATED_BODY() protected: virtual FText PanelTitle() const override; virtual void BuildPanelContent() override; virtual void HandlePanelAction(const FString& Action) override; };
UCLASS() class BOUNDLESSIMMORTALITY_API UInkWorldCultivationPanel : public UInkFeaturePanel
{ GENERATED_BODY() public: virtual void RefreshLiveData()override; protected: virtual FText PanelTitle() const override; virtual void BuildPanelContent() override; virtual void HandlePanelAction(const FString& Action) override; private: FInkProgressContext ProgressContext; };
UCLASS() class BOUNDLESSIMMORTALITY_API UInkWulingPanel : public UInkFeaturePanel
{
    GENERATED_BODY()
public: virtual void RefreshLiveData()override;
protected:
    virtual FText PanelTitle()const override{return FText::FromString(TEXT("武灵 · 青云历练"));}
    virtual void BuildPanelContent()override;
    virtual void HandlePanelAction(const FString& Action)override;
private:
    UPROPERTY(Transient) TObjectPtr<UEditableTextBox> GuildNameInput;
    FString DraftName=TEXT("青云武灵"),Preset=TEXT("ENC_SOLO"),Route=TEXT("guard");
    int32 Cursor=0;
    bool bGuildView=false;
};
UCLASS() class BOUNDLESSIMMORTALITY_API UInkResourceContestPanel : public UInkFeaturePanel
{ GENERATED_BODY() protected: virtual FText PanelTitle() const override; virtual void BuildPanelContent() override; virtual void HandlePanelAction(const FString& Action) override; };
UCLASS() class BOUNDLESSIMMORTALITY_API UInkCharacterProfilePanel : public UInkFeaturePanel
{
    GENERATED_BODY()
public:
    UInkCharacterProfilePanel(const FObjectInitializer& Init);
    virtual void RefreshLiveData() override;
    UFUNCTION(BlueprintCallable,Category="Ink UI|Character") void RefreshCharacterPresentation(){RebuildPanel();RefreshLiveData();}
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ink UI|Character|Effects",meta=(ClampMin="0.05",ClampMax="3")) float SlotEffectSpeed=.42f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ink UI|Character|Effects",meta=(ClampMin="1",ClampMax="8")) float SlotParticleSize=2.6f;
    /** Dantian location in the square meditation artwork, normalized 0..1. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ink UI|Character|Effects") FVector2D DantianUV=FVector2D(.5,.65);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ink UI|Character|Effects",meta=(ClampMin="8",ClampMax="48")) float DantianRadius=18;
    /** Qi, Foundation, Crystal, Golden Core, Nascent Soul. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ink UI|Character|Effects") TArray<FLinearColor> DantianColors;
    /** Common, green, blue, purple, orange, red, gold. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ink UI|Character|Effects") TArray<FLinearColor> QualityColors;
    /** Explicit preview only. Production quality comes from equipped discipline data. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ink UI|Character|Effects") TArray<int32> NodeQualities;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ink UI|Character|Preview") bool bPreviewQualities=false;
    /** Explicit editor/test presentation input, never changes character progression. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ink UI|Character|Preview") bool bPreviewRealm=false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Ink UI|Character|Preview",meta=(EditCondition="bPreviewRealm",ClampMin="0")) int32 PreviewRealmIndex=0;
    UPROPERTY(Transient,BlueprintReadOnly,Category="Ink UI|Character") TObjectPtr<UInkCharacterCultivationWidget> CultivationVisual;
protected:
    virtual FText PanelTitle()const override;
    virtual void BuildPanelContent()override;
    virtual void HandlePanelAction(const FString& Action)override;
private:
    TArray<FName> PresentedFields;
    UPROPERTY(Transient) TArray<TObjectPtr<UTexture2D>> RatingTextures;
    TArray<FVector2D> NodePositions;
    TArray<FVector2D> NodeSizes;
    FVector2D VisualSize,DantianPosition;
    float AttributeWidth=180,VisualScale=1;
};
UCLASS() class BOUNDLESSIMMORTALITY_API UInkCharacterEquipmentPanel : public UInkFeaturePanel
{ GENERATED_BODY() protected: virtual FText PanelTitle() const override; virtual void BuildPanelContent() override; };
UCLASS() class BOUNDLESSIMMORTALITY_API UInkCharacterSkillsPanel : public UInkFeaturePanel
{ GENERATED_BODY() protected: virtual FText PanelTitle() const override; virtual void BuildPanelContent() override; };
UCLASS() class BOUNDLESSIMMORTALITY_API UInkCharacterArtifactsPanel : public UInkFeaturePanel
{ GENERATED_BODY() protected: virtual FText PanelTitle() const override; virtual void BuildPanelContent() override; };
UCLASS() class BOUNDLESSIMMORTALITY_API UInkCharacterFatePanel : public UInkFeaturePanel
{ GENERATED_BODY() protected: virtual FText PanelTitle() const override; virtual void BuildPanelContent() override; };
UCLASS() class BOUNDLESSIMMORTALITY_API UInkCharacterAppearancePanel : public UInkFeaturePanel
{ GENERATED_BODY() protected: virtual FText PanelTitle() const override; virtual void BuildPanelContent() override; virtual void HandlePanelAction(const FString& Action) override;
private: FString FrameStyle=TEXT("云鹤"),BarStyle=TEXT("普通"); };

/** Separate details keep their parent list alive and unmodified. */
UCLASS(Abstract) class BOUNDLESSIMMORTALITY_API UInkDetailPanel : public UInkFeaturePanel
{ GENERATED_BODY() protected: virtual FText PanelTitle() const override; virtual void BuildPanelContent() override; virtual FString Description() const; virtual int32 SceneTile() const; };
UCLASS() class BOUNDLESSIMMORTALITY_API UInkDailyActivityDetailPanel : public UInkDetailPanel
{ GENERATED_BODY() protected: virtual FString Description() const override; };
UCLASS() class BOUNDLESSIMMORTALITY_API UInkTimedActivityDetailPanel : public UInkDetailPanel
{ GENERATED_BODY() protected: virtual FString Description() const override; };
UCLASS() class BOUNDLESSIMMORTALITY_API UInkExpeditionDetailPanel : public UInkDetailPanel
{ GENERATED_BODY() protected: virtual FString Description() const override; virtual int32 SceneTile() const override; };
UCLASS() class BOUNDLESSIMMORTALITY_API UInkRegionDetailPanel : public UInkDetailPanel
{ GENERATED_BODY() protected: virtual FString Description() const override; virtual int32 SceneTile() const override; };
UCLASS() class BOUNDLESSIMMORTALITY_API UInkResourceDetailPanel : public UInkDetailPanel
{ GENERATED_BODY() protected: virtual FString Description() const override; virtual int32 SceneTile() const override; };
UCLASS() class BOUNDLESSIMMORTALITY_API UInkResourceRankingPanel : public UInkFeaturePanel
{
    GENERATED_BODY()
public:
    virtual void RefreshLiveData() override;
protected:
    virtual FText PanelTitle() const override;
    virtual void BuildPanelContent() override;
private:
    FString AuthorityStateKey;
};
UCLASS() class BOUNDLESSIMMORTALITY_API UInkResourceRewardsPanel : public UInkFeaturePanel
{ GENERATED_BODY() protected: virtual FText PanelTitle() const override; virtual void BuildPanelContent() override; };
