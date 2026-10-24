#include "InkLaunchHudCountdownWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "Frontend/InkFrontendWidget.h"
#include "Frontend/UI/InkUiTheme.h"
#include "UObject/ConstructorHelpers.h"
#include "Wuling/WulingPrototypeSubsystem.h"

UInkLaunchHudCountdownWidget::UInkLaunchHudCountdownWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    static ConstructorHelpers::FObjectFinderOptional<UTexture2D> ImportedFrame(
        TEXT("/Game/UI/Ink/ComponentRollout20261024/Activities/T_HudLaunchCountdownFrame_v1.T_HudLaunchCountdownFrame_v1"));
    FrameTexture = ImportedFrame.Get();
}

void UInkLaunchHudCountdownWidget::Configure(UInkFrontendWidget* InFrontend)
{
    Frontend = InFrontend;
    RefreshFromAuthority(FPlatformTime::Seconds());
}

void UInkLaunchHudCountdownWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree) return;

    if (!WidgetTree->RootWidget)
    {
        UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("LaunchCountdownRoot"));
        WidgetTree->RootWidget = Root;
        FrameImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LaunchCountdownFrame"));
        Root->AddChild(FrameImage);
        if (UCanvasPanelSlot* Layout = Cast<UCanvasPanelSlot>(FrameImage->Slot))
        {
            Layout->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
            Layout->SetOffsets(FMargin(0.f));
        }
        if (FrameTexture) FrameImage->SetBrushFromTexture(FrameTexture, true);

        HitTarget = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("LaunchCountdownHitTarget"));
        Root->AddChild(HitTarget);
        if (UCanvasPanelSlot* Layout = Cast<UCanvasPanelSlot>(HitTarget->Slot))
        {
            Layout->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
            Layout->SetOffsets(FMargin(0.f));
        }
        HitTarget->OnClicked.AddDynamic(this, &UInkLaunchHudCountdownWidget::HandleOpenLaunchEvent);

        CountdownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LaunchCountdownText"));
        Root->AddChild(CountdownText);
        if (UCanvasPanelSlot* Layout = Cast<UCanvasPanelSlot>(CountdownText->Slot))
        {
            Layout->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
            Layout->SetOffsets(FMargin(12.f, 8.f, 12.f, 8.f));
        }
        CountdownText->SetJustification(ETextJustify::Center);
        CountdownText->SetFont(InkUiTypography::MakeFont(EInkUiTextRole::Data,20));
        CountdownText->SetColorAndOpacity(FSlateColor(FLinearColor(.13f,.22f,.19f,1.f)));
        CountdownText->SetVisibility(ESlateVisibility::HitTestInvisible);
        CountdownText->SetAutoWrapText(true);
    }
    RefreshFromAuthority(FPlatformTime::Seconds());
}

void UInkLaunchHudCountdownWidget::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
    Super::NativeTick(Geometry, DeltaSeconds);
    const double Now = FPlatformTime::Seconds();
    if (Now >= NextRefreshAt)
    {
        NextRefreshAt = Now + 1.0;
        RefreshFromAuthority(Now);
    }
}

void UInkLaunchHudCountdownWidget::RefreshFromAuthority(double MonotonicNow)
{
    auto* Service = GetGameInstance() ? GetGameInstance()->GetSubsystem<UWulingPrototypeSubsystem>() : nullptr;
    const TSharedPtr<FJsonObject> State = Service ? Service->Snapshot() : nullptr;
    const TSharedPtr<FJsonObject>* EventField = nullptr;
    const TSharedPtr<FJsonObject>* ManifestField = nullptr;
    if(State){State->TryGetObjectField(TEXT("launch"),EventField);State->TryGetObjectField(TEXT("manifest"),ManifestField);}
    const TSharedPtr<FJsonObject> Event = EventField ? *EventField : nullptr;
    const TSharedPtr<FJsonObject> Manifest = ManifestField ? *ManifestField : nullptr;

    FString Status;
    double ServerUtc = 0.0;
    double EndUtc = 0.0;
    if (State) State->TryGetNumberField(TEXT("server_utc"), ServerUtc);
    if (Event)
    {
        Event->TryGetNumberField(TEXT("end"), EndUtc);
        Event->TryGetStringField(TEXT("status"), Status);
    }
    FString Profile;
    if (Manifest) Manifest->TryGetStringField(TEXT("profile"), Profile);

    if (ServerUtc > 0.0 && ServerUtc != LastAuthorityUtc)
    {
        LastAuthorityUtc = ServerUtc;
        LastAuthorityMonotonic = MonotonicNow;
    }
    const double EstimatedServerNow = LastAuthorityUtc > 0.0
        ? LastAuthorityUtc + FMath::Max(0.0, MonotonicNow - LastAuthorityMonotonic)
        : 0.0;
    const bool bCanShow = Service && Service->IsEnabled() && Event && Profile == TEXT("prototype_full") &&
        Status == TEXT("Active") && EndUtc > 0.0 && EstimatedServerNow > 0.0 && EndUtc > EstimatedServerNow;
    SetVisibility(bCanShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (!bCanShow || !CountdownText) return;

    const int64 Seconds = FMath::Max<int64>(0, FMath::CeilToInt64(EndUtc - EstimatedServerNow));
    const int64 Days = Seconds / 86400;
    const int64 Hours = (Seconds % 86400) / 3600;
    const int64 Minutes = (Seconds % 3600) / 60;
    const int64 Remainder = Seconds % 60;
    FString Remaining = Days > 0
        ? FString::Printf(TEXT("%lld天 %02lld:%02lld:%02lld"), Days, Hours, Minutes, Remainder)
        : FString::Printf(TEXT("%02lld:%02lld:%02lld"), Hours, Minutes, Remainder);
    if (!Service->IsConnected()) Remaining = FString(TEXT("上次确认 · ")) + Remaining;
    CountdownText->SetText(FText::FromString(FString(TEXT("开服\n")) + Remaining));
}

void UInkLaunchHudCountdownWidget::HandleOpenLaunchEvent()
{
    if (UInkFrontendWidget* Owner = Frontend.Get()) Owner->OpenFeaturePanel(TEXT("LaunchEvent"), FString());
}

