#pragma once

#include "CoreMinimal.h"
#include "Frontend/UiRollout20261024/LogicCore/InkCoreAdapter.h"

class UCanvasPanel;
class UInkFrontendWidget;
class UImage;
class UScrollBox;
class UTextBlock;
class UTexture2D;
class UWidget;
class UWidgetTree;
class FJsonObject;

namespace InkRankWulingCompositeViews
{
    struct BOUNDLESSIMMORTALITY_API FRankChrome
    {
        TObjectPtr<UTextBlock> Title;
        TObjectPtr<UTextBlock> ProviderState;
        TObjectPtr<UTextBlock> SelfPosition;
        TObjectPtr<UScrollBox> Rows;
    };

    /** Builds rank title, scope/category controls, field headings, stable row viewport,
     *  state surface and self-position strip around the host's persistent row tree.
     *  The host continues to own ReadBoard, subscriptions, avatar requests and refresh.
     */
    BOUNDLESSIMMORTALITY_API FRankChrome BuildLeaderboardComposite(
        UWidgetTree* Tree, UCanvasPanel* Parent, UInkFrontendWidget* Frontend,
        const InkCore::FBoardView& Board, UScrollBox* PersistentRows,
        const FText& AuthoritativeSelfPosition, UTexture2D* PaperTexture = nullptr);

    struct BOUNDLESSIMMORTALITY_API FWulingOverviewWidgets
    {
        TObjectPtr<UTextBlock> Name;
        TObjectPtr<UTextBlock> Status;
        TObjectPtr<UTextBlock> Members;
        TObjectPtr<UTextBlock> Participating;
        TObjectPtr<UTextBlock> Helpers;
        TObjectPtr<UTextBlock> Supply;
        TObjectPtr<UTextBlock> Reputation;
    };

    /** Draw only fields present in the real UWulingPrototypeSubsystem snapshot.
     *  Action widgets are supplied by UInkWulingSectPanel so they retain its Ext_ routing.
     */
    BOUNDLESSIMMORTALITY_API FWulingOverviewWidgets BuildWulingOverviewComposite(
        UWidgetTree* Tree, UCanvasPanel* Parent, const TSharedPtr<FJsonObject>& Snapshot,
        const FString& ServiceStatus, bool bServiceEnabled, bool bConnected, bool bBusy,
        const TArray<UWidget*>& ExistingActionWidgets, UTexture2D* PaperTexture = nullptr);

    struct BOUNDLESSIMMORTALITY_API FWulingRosterWidgets
    {
        TObjectPtr<UTextBlock> State;
        TObjectPtr<UCanvasPanel> Rows;
        int32 RenderedRows = 0;
    };

    /** Reparent the host's real server-paged member row actions and management
     *  controls. The host retains Rows, SelectedMember, handlers, and permissions.
     */
    BOUNDLESSIMMORTALITY_API FWulingRosterWidgets BuildWulingRosterComposite(
        UWidgetTree* Tree, UCanvasPanel* Parent, const TSharedPtr<FJsonObject>& Snapshot,
        const FString& ServiceStatus, bool bServiceEnabled, bool bConnected, bool bBusy,
        int32 RequestedCursor, const TArray<UWidget*>& ExistingMemberRowWidgets,
        const TArray<UWidget*>& PagingActionWidgets, UWidget* RemoveActionWidget,
        UTexture2D* PaperTexture = nullptr);
}
