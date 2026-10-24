#pragma once
#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"

/** Shared ink-and-gold artwork. Text and progress are live UMG/Slate content. */
namespace InkPresentationStyle
{
    enum class ESurface : uint8 { PaperButton, GoldButton, BlueButton, SelectedTab, Card, Heading, BarTrack, BarFill, FeaturePaper, FeatureHeading, FeatureCard, FeatureTab };
    enum class EIcon : uint8 { AvatarFrame, AnchorPortrait, Activities, Cultivation, Health, Shield, Experience, Action, Crystal, Jade, Chest, Manual, Sword, Armor, Critical, Movement };
    BOUNDLESSIMMORTALITY_API FSlateBrush Surface(ESurface Part,FVector2D Size=FVector2D(240,56));
    BOUNDLESSIMMORTALITY_API FSlateBrush Icon(EIcon Part,FVector2D Size=FVector2D(40,40));
    BOUNDLESSIMMORTALITY_API FButtonStyle Button(bool bSelected=false,bool bPrimary=false,bool bBlue=false);
    /** Native, asset-independent style reserved for shared feature pages. */
    BOUNDLESSIMMORTALITY_API FButtonStyle FeatureButton(bool bSelected=false,bool bPrimary=false,bool bBlue=false);
    BOUNDLESSIMMORTALITY_API FButtonStyle CloseButton();
    BOUNDLESSIMMORTALITY_API FSlateFontInfo ButtonFont(FSlateFontInfo Font,bool bLight);
    BOUNDLESSIMMORTALITY_API FProgressBarStyle Progress();
    BOUNDLESSIMMORTALITY_API FSlateBrush CohesionIcon(int32 Tile,FVector2D Size=FVector2D(64));
    BOUNDLESSIMMORTALITY_API FSlateBrush NavigationIcon(int32 Tile,FVector2D Size=FVector2D(80));
    BOUNDLESSIMMORTALITY_API FSlateBrush IdentityBackground();
    BOUNDLESSIMMORTALITY_API FSlateBrush NamecardBackground();
    BOUNDLESSIMMORTALITY_API FSlateBrush LeaderboardBackground(FVector2D Size);
    BOUNDLESSIMMORTALITY_API FSlateBrush InkWash();
    BOUNDLESSIMMORTALITY_API FSlateBrush NavigationFade();
    /** 0 health, 1 cultivation, 2 shield. Fill strips contain no frame. */
    BOUNDLESSIMMORTALITY_API FSlateBrush Meter(int32 Kind,bool bFill);
    inline FLinearColor Ink(){return FLinearColor::FromSRGBColor(FColor(32,40,35));}
    inline FLinearColor Gold(){return FLinearColor::FromSRGBColor(FColor(184,139,69));}
    inline FLinearColor Paper(){return FLinearColor::FromSRGBColor(FColor(243,235,221));}
    inline FLinearColor PaperLight(){return FLinearColor::FromSRGBColor(FColor(251,247,238));}
    inline FLinearColor Jade(){return FLinearColor::FromSRGBColor(FColor(58,141,131));}
}
