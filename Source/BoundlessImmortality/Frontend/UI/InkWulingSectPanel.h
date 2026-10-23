#pragma once

#include "CoreMinimal.h"
#include "Frontend/InkFeaturePanel.h"
#include "InkWulingSectPanel.generated.h"

/** The named Wuling organization overview. World-round preparation has its own route. */
UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UInkWulingSectPanel : public UInkFeaturePanel
{
    GENERATED_BODY()
public:
    virtual void RefreshLiveData() override;
protected:
    virtual FText PanelTitle() const override;
    virtual void BuildPanelContent() override;
    virtual void HandlePanelAction(const FString& Action) override;
private:
    UPROPERTY(Transient) TObjectPtr<UEditableTextBox> GuildNameInput;
    UPROPERTY(Transient) TObjectPtr<UInkFrontendActionButton> CreateButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> GuildTitle;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ConnectionText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> MemberCount;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ParticipatingCount;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> HelperCount;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> SupplyText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ReputationText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CreationTitle;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CreationHint;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RosterText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> RosterStatus;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> WorldStatus;
    FString DraftName;
    FString SectStateKey;
    bool bBuiltWithGuild = false;
    bool bAwaitingCreationConfirmation = false;
};
