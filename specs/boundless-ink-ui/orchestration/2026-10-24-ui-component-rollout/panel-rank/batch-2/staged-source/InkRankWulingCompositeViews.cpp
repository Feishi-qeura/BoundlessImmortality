#include "InkRankWulingCompositeViews.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "Frontend/InkFrontendWidget.h"
#include "Frontend/InkLeaderboardTheme.h"
#include "Frontend/InkPresentationStyle.h"
#include "Frontend/UiRollout20261024/RankWuling/InkRankWulingComponents.h"

namespace InkRankWulingCompositeViews
{
    using InkRankWulingComponents::FPlacement;

    namespace
    {
        constexpr float PageWidth = 720.f;
        constexpr int32 RosterPageSize = 8;

        FPlacement Place(float X, float Y, float W, float H, int32 Z = 0)
        {
            FPlacement Result;
            Result.X = X; Result.Y = Y; Result.Width = W; Result.Height = H; Result.ZOrder = Z;
            return Result;
        }

        UTextBlock* Text(UWidgetTree* Tree, UCanvasPanel* Parent, const TCHAR* Name,
            const FText& Value, float X, float Y, float W, float H, int32 Size,
            ETextJustify::Type Justification = ETextJustify::Left,
            FLinearColor Color = FLinearColor(0.12f, 0.16f, 0.14f, 1.f))
        {
            return InkRankWulingComponents::AddDynamicText(Tree, Parent, FName(Name), Value,
                Size, Place(X, Y, W, H, 4), Color, Justification);
        }

        void Card(UWidgetTree* Tree, UCanvasPanel* Parent, const TCHAR* Name,
            UTexture2D* Paper, float X, float Y, float W, float H)
        {
            if (Paper)
                InkRankWulingComponents::AddPaperPanel(Tree, Parent, FName(Name), Paper,
                    Place(X, Y, W, H), FVector4f(136.f, 136.f, 136.f, 136.f));
            else
                InkRankWulingComponents::AddNativeSurface(Tree, Parent, FName(Name),
                    InkPresentationStyle::ESurface::FeatureCard, Place(X, Y, W, H));
        }

        TSharedPtr<FJsonObject> Object(const TSharedPtr<FJsonObject>& Value, const TCHAR* Key)
        {
            const TSharedPtr<FJsonObject>* Found = nullptr;
            return Value && Value->TryGetObjectField(Key, Found) ? *Found : nullptr;
        }

        FString String(const TSharedPtr<FJsonObject>& Value, const TCHAR* Key)
        {
            FString Result;
            if (Value) Value->TryGetStringField(Key, Result);
            return Result;
        }

        FString Number(const TSharedPtr<FJsonObject>& Value, const TCHAR* Key)
        {
            double Result = 0.0;
            return Value && Value->TryGetNumberField(Key, Result)
                ? FString::Printf(TEXT("%.0f"), Result) : FString(TEXT("—"));
        }

        void Attach(UCanvasPanel* Parent, UWidget* Child, float X, float Y, float W, float H)
        {
            if (!Parent || !Child) return;
            Child->RemoveFromParent();
            auto* Slot = Parent->AddChildToCanvas(Child);
            Slot->SetPosition(FVector2D(X, Y));
            Slot->SetSize(FVector2D(W, H));
            Slot->SetZOrder(3);
        }

        FText BoardStateText(const InkCore::FBoardView& Board)
        {
            switch (Board.State)
            {
            case InkCorePolicy::EState::Loading: return FText::FromString(TEXT("榜单同步中"));
            case InkCorePolicy::EState::Offline: return FText::FromString(Board.Status);
            case InkCorePolicy::EState::Empty: return FText::FromString(TEXT("本期暂无上榜修士"));
            case InkCorePolicy::EState::Error: return FText::FromString(Board.Status);
            case InkCorePolicy::EState::Unavailable: return FText::FromString(
                Board.Status.IsEmpty() ? FString(TEXT("榜单服务未启用")) : Board.Status);
            case InkCorePolicy::EState::Data: return FText::FromString(Board.Status);
            }
            return FText::FromString(TEXT("榜单状态待同步"));
        }
    }

    FRankChrome BuildLeaderboardComposite(UWidgetTree* Tree, UCanvasPanel* Parent,
        UInkFrontendWidget* Frontend, const InkCore::FBoardView& Board,
        UScrollBox* PersistentRows, const FText& SelfPosition, UTexture2D* PaperTexture)
    {
        FRankChrome Result;
        if (!Tree || !Parent || !Frontend) return Result;

        Card(Tree, Parent, TEXT("RankCompositeHeaderCard"), PaperTexture, 0, 0, PageWidth, 216);
        Result.Title = Text(Tree, Parent, TEXT("RankCompositeTitle"),
            FText::FromString(InkLeaderboardTheme::CategoryTitle(Board.Category)),
            24, 12, PageWidth - 48, 52, 38, ETextJustify::Center,
            InkLeaderboardTheme::Color(Board.Category));

        const FName Categories[] = {TEXT("RankDamage"), TEXT("RankCultivation"), TEXT("RankAnchor")};
        const FName Ids[] = {TEXT("Damage"), TEXT("CultivationSpeed"), TEXT("Anchor")};
        const FText Captions[] = {FText::FromString(TEXT("伤害榜")), FText::FromString(TEXT("修炼速度")), FText::FromString(TEXT("主播剑灵"))};
        for (int32 I = 0; I < 3; ++I)
        {
            InkRankWulingComponents::AddActionButton(Tree, Parent, Frontend,
                FName(*FString::Printf(TEXT("RankCompositeCategory%d"), I)), Categories[I], Captions[I],
                Board.Category == Ids[I], false, Place(18.f + I * 230.f, 72.f, 220.f, 64.f, 5));
        }

        const FName ScopeIds[] = {TEXT("RankLocal"), TEXT("RankGlobal")};
        const FText ScopeCaptions[] = {FText::FromString(TEXT("本房")), FText::FromString(TEXT("全服"))};
        for (int32 I = 0; I < 2; ++I)
        {
            InkRankWulingComponents::AddActionButton(Tree, Parent, Frontend,
                FName(*FString::Printf(TEXT("RankCompositeScope%d"), I)), ScopeIds[I], ScopeCaptions[I],
                Board.Scope == (I == 0 ? FName(TEXT("Local")) : FName(TEXT("Global"))), false,
                Place(180.f + I * 184.f, 146.f, 176.f, 56.f, 5));
        }

        Card(Tree, Parent, TEXT("RankCompositeColumns"), nullptr, 0, 224, PageWidth, 58);
        Text(Tree, Parent, TEXT("RankCompositeColRank"), FText::FromString(TEXT("名次")),
            20, 238, 96, 34, 24, ETextJustify::Center);
        Text(Tree, Parent, TEXT("RankCompositeColIdentity"), FText::FromString(TEXT("身份")),
            132, 238, 260, 34, 24, ETextJustify::Center);
        Text(Tree, Parent, TEXT("RankCompositeColMetric"),
            FText::FromString(InkLeaderboardTheme::MetricLabel(Board.Category)),
            412, 238, 280, 34, 24, ETextJustify::Right,
            InkLeaderboardTheme::Color(Board.Category));

        Card(Tree, Parent, TEXT("RankCompositeStateCard"), nullptr, 0, 292, PageWidth, 52);
        Result.ProviderState = Text(Tree, Parent, TEXT("RankCompositeProviderState"),
            BoardStateText(Board), 16, 300, PageWidth - 32, 40, 22, ETextJustify::Center);

        if (PersistentRows)
        {
            Attach(Parent, PersistentRows, 0, 352, PageWidth, 640);
            PersistentRows->SetVisibility(Board.State == InkCorePolicy::EState::Data
                ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
            Result.Rows = PersistentRows;
        }

        Card(Tree, Parent, TEXT("RankCompositeSelfCard"), PaperTexture, 0, 1000, PageWidth, 112);
        Result.SelfPosition = Text(Tree, Parent, TEXT("RankCompositeSelfPosition"),
            SelfPosition.IsEmpty() ? FText::FromString(TEXT("本人名次未包含在当前榜单响应中")) : SelfPosition,
            24, 1020, PageWidth - 48, 72, 24, ETextJustify::Center);
        return Result;
    }

    FWulingOverviewWidgets BuildWulingOverviewComposite(UWidgetTree* Tree, UCanvasPanel* Parent,
        const TSharedPtr<FJsonObject>& Snapshot, const FString& ServiceStatus,
        bool bServiceEnabled, bool bConnected, bool bBusy,
        const TArray<UWidget*>& ExistingActionWidgets, UTexture2D* PaperTexture)
    {
        FWulingOverviewWidgets Result;
        if (!Tree || !Parent) return Result;
        const auto Guild = Object(Snapshot, TEXT("guild"));
        const auto Counts = Object(Snapshot, TEXT("counts"));
        const FString GuildName = Guild ? String(Guild, TEXT("name"))
            : Snapshot ? FString(TEXT("尚未创建武灵")) : FString(TEXT("武灵数据待同步"));

        Card(Tree, Parent, TEXT("WulingCompositeIdentity"), PaperTexture, 0, 0, 720, 184);
        Result.Name = Text(Tree, Parent, TEXT("WulingCompositeName"),
            FText::FromString(GuildName),
            24, 20, 672, 56, 34, ETextJustify::Center, InkPresentationStyle::Gold());
        FString State = !bServiceEnabled ? TEXT("武灵服务未启用") : !bConnected ? TEXT("服务未连接 · 显示已有同步快照")
            : bBusy ? TEXT("操作确认中") : !ServiceStatus.IsEmpty() ? ServiceStatus
            : Snapshot ? FString(TEXT("组织快照已同步")) : FString(TEXT("组织数据待同步"));
        Result.Status = Text(Tree, Parent, TEXT("WulingCompositeStatus"), FText::FromString(State),
            24, 88, 672, 72, 22, ETextJustify::Center);

        Card(Tree, Parent, TEXT("WulingCompositeSummary"), nullptr, 0, 204, 720, 280);
        const FText Captions[] = {FText::FromString(TEXT("正式成员")), FText::FromString(TEXT("参与成员")), FText::FromString(TEXT("临时助战"))};
        TObjectPtr<UTextBlock>* Outputs[] = {&Result.Members, &Result.Participating, &Result.Helpers};
        const TCHAR* Fields[] = {TEXT("members"), TEXT("participating_members"), TEXT("helpers")};
        for (int32 I = 0; I < 3; ++I)
        {
            const float X = 20.f + I * 228.f;
            Text(Tree, Parent, *FString::Printf(TEXT("WulingCompositeCountLabel%d"), I), Captions[I],
                X, 232, 220, 42, 22, ETextJustify::Center);
            *Outputs[I] = Text(Tree, Parent, *FString::Printf(TEXT("WulingCompositeCountValue%d"), I),
                FText::FromString(Number(Counts, Fields[I])), X, 282, 220, 60, 34,
                ETextJustify::Center, InkPresentationStyle::Gold());
        }
        Result.Supply = Text(Tree, Parent, TEXT("WulingCompositeSupply"),
            FText::FromString(TEXT("公共资材 ") + Number(Guild, TEXT("supply"))), 28, 368, 320, 56, 24);
        Result.Reputation = Text(Tree, Parent, TEXT("WulingCompositeReputation"),
            FText::FromString(TEXT("武灵声望 ") + Number(Guild, TEXT("reputation"))), 372, 368, 320, 56, 24);

        Card(Tree, Parent, TEXT("WulingCompositeActions"), nullptr, 0, 504, 720, 180);
        int32 ActionIndex = 0;
        for (UWidget* Action : ExistingActionWidgets)
        {
            if (!Action) continue;
            const int32 Column = ActionIndex % 2;
            const int32 Row = ActionIndex / 2;
            Attach(Parent, Action, 16.f + Column * 356.f, 520.f + Row * 84.f, 332.f, 72.f);
            ++ActionIndex;
        }
        return Result;
    }

    FWulingRosterWidgets BuildWulingRosterComposite(UWidgetTree* Tree, UCanvasPanel* Parent,
        const TSharedPtr<FJsonObject>& Snapshot, const FString& ServiceStatus,
        bool bServiceEnabled, bool bConnected, bool bBusy, int32 RequestedCursor,
        const TArray<UWidget*>& ExistingMemberRowWidgets,
        const TArray<UWidget*>& PagingActionWidgets, UWidget* RemoveActionWidget,
        UTexture2D* PaperTexture)
    {
        FWulingRosterWidgets Result;
        if (!Tree || !Parent) return Result;
        const TArray<TSharedPtr<FJsonValue>>* Members = nullptr;
        const bool bHasMembers = Snapshot && Snapshot->TryGetArrayField(TEXT("members"), Members);
        double AuthoritativeCursor = -1.0;
        const bool bHasCursor = Snapshot && Snapshot->TryGetNumberField(TEXT("member_cursor"), AuthoritativeCursor);
        const bool bPageMatches = bHasCursor && FMath::IsNearlyEqual(AuthoritativeCursor, double(RequestedCursor));
        const bool bReady = bServiceEnabled && bConnected && !bBusy && bHasMembers && bPageMatches;

        Card(Tree, Parent, TEXT("WulingRosterCompositeHeader"), PaperTexture, 0, 0, 720, 104);
        const FString State = !bServiceEnabled ? TEXT("武灵服务未启用") : !bConnected ? ServiceStatus : bBusy ? TEXT("成员同步中") :
            !bHasMembers || !bPageMatches ? TEXT("等待当前页成员同步") : Members->IsEmpty() ? TEXT("暂无成员记录") : ServiceStatus;
        Result.State = Text(Tree, Parent, TEXT("WulingRosterCompositeState"), FText::FromString(State),
            20, 16, 680, 72, 22, ETextJustify::Center);

        Card(Tree, Parent, TEXT("WulingRosterCompositeList"), nullptr, 0, 120, 720, 832);
        Result.Rows = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("WulingRosterCompositeRows"));
        auto* RowsSlot = Parent->AddChildToCanvas(Result.Rows);
        RowsSlot->SetPosition(FVector2D(12, 132));
        RowsSlot->SetSize(FVector2D(696, 808));
        RowsSlot->SetZOrder(3);

        if (bReady)
        {
            const int32 Count = FMath::Min(FMath::Min(Members->Num(), ExistingMemberRowWidgets.Num()), RosterPageSize);
            for (int32 I = 0; I < Count; ++I)
            {
                UWidget* ExistingRow = ExistingMemberRowWidgets[I];
                if (!ExistingRow) continue;
                const float Y = I * 96.f;
                Card(Tree, Result.Rows, *FString::Printf(TEXT("WulingRosterRowCard%d"), I), nullptr,
                    0, Y, 696, 84);
                Attach(Result.Rows, ExistingRow, 8.f, Y + 4.f, 680.f, 76.f);
                ++Result.RenderedRows;
            }
        }

        float X = 16.f;
        for (UWidget* Action : PagingActionWidgets)
        {
            if (!Action) continue;
            Attach(Parent, Action, X, 968.f, 332.f, 72.f);
            X += 356.f;
        }
        // Preserve the existing selection-dependent enabled state and the original
        // RecordRemove handler by moving that exact widget into the management area.
        if (RemoveActionWidget)
            Attach(Parent, RemoveActionWidget, 372.f, 968.f, 332.f, 72.f);
        return Result;
    }
}
