#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JourneyRolloutWidgets.generated.h"

class UButton;
class UBorder;
class UImage;
class UOverlay;
class UTextBlock;

UENUM(BlueprintType)
enum class EJourneyGlyphFamily : uint8
{
    WorldNode,
    ResourcePin,
    FortuneSeal
};

UENUM(BlueprintType)
enum class EJourneyCalendarState : uint8
{
    Unknown,
    Signed,
    TodayClaimable,
    Future,
    Missed
};

UENUM(BlueprintType)
enum class EDailySignInClaimState : uint8
{
    Unknown,
    Claimable,
    Pending,
    Claimed,
    Disconnected,
    Unavailable,
    Error
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJourneyGlyphPressed, FName, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJourneyCalendarDayPressed, int32, Day);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FJourneyClaimRequested);

/** A single transparent status glyph. Caption remains runtime TextBlock content. */
UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UJourneyStateBadgeWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Ink UI|Journey")
    void SetGlyphState(EJourneyGlyphFamily Family, FName State);

    UFUNCTION(BlueprintCallable, Category="Ink UI|Journey")
    void SetCaption(const FText& Caption);

    UFUNCTION(BlueprintCallable, Category="Ink UI|Journey")
    void SetBadgeEnabled(bool bEnabled);

    UPROPERTY(BlueprintAssignable, Category="Ink UI|Journey")
    FJourneyGlyphPressed OnGlyphPressed;

    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Journey")
    EJourneyGlyphFamily GlyphFamily = EJourneyGlyphFamily::WorldNode;

    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Journey")
    FName GlyphState = TEXT("available");

protected:
    virtual void NativeOnInitialized() override;

private:
    UFUNCTION() void HandlePressed();
    void RefreshGlyph();
    UPROPERTY(Transient) TObjectPtr<UButton> HitTarget;
    UPROPERTY(Transient) TObjectPtr<UImage> GlyphImage;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> CaptionText;
};

/** A server-state calendar cell. Day number is data, never part of the texture. */
UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UJourneyCalendarDayWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Ink UI|Journey")
    void SetDayState(int32 Day, EJourneyCalendarState State, bool bSelected = false);

    UFUNCTION(BlueprintCallable, Category="Ink UI|Journey")
    void SetDayEnabled(bool bEnabled);

    UPROPERTY(BlueprintAssignable, Category="Ink UI|Journey")
    FJourneyCalendarDayPressed OnDayPressed;

    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Journey")
    int32 ServerDayNumber = 0;

    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Journey")
    EJourneyCalendarState ServerState = EJourneyCalendarState::Unknown;

protected:
    virtual void NativeOnInitialized() override;

private:
    UFUNCTION() void HandlePressed();
    void RefreshCell();
    UPROPERTY(Transient) TObjectPtr<UButton> HitTarget;
    UPROPERTY(Transient) TObjectPtr<UOverlay> CellOverlay;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> DayText;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> StateText;
    bool bSelected = false;
};

/** Mutually exclusive daily check-in action state; it never submits authority itself. */
UCLASS(Blueprintable)
class BOUNDLESSIMMORTALITY_API UJourneyClaimStateWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Ink UI|Journey")
    void SetClaimState(EDailySignInClaimState State, const FText& Reason = FText::GetEmpty());

    UPROPERTY(BlueprintAssignable, Category="Ink UI|Journey")
    FJourneyClaimRequested OnClaimRequested;

    UPROPERTY(Transient, BlueprintReadOnly, Category="Ink UI|Journey")
    EDailySignInClaimState ClaimState = EDailySignInClaimState::Unknown;

protected:
    virtual void NativeOnInitialized() override;

private:
    UFUNCTION() void HandlePressed();
    void RefreshClaimState();
    UPROPERTY(Transient) TObjectPtr<UButton> ClaimButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> ClaimLabel;
    FText ClaimReason;
    bool bClaimEventSent = false;
};
