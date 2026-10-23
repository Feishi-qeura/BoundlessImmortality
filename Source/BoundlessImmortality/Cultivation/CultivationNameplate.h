#pragma once
#include "CoreMinimal.h"
#include "CultivationNameplate.generated.h"

/** Shared by Slate and Canvas; distances are measured from the player's pawn. */
USTRUCT(BlueprintType)
struct BOUNDLESSIMMORTALITY_API FCultivationNameplateDistanceSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Nameplate",meta=(ClampMin="0",Units="cm")) float FullSizeDistance=250;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Nameplate",meta=(ClampMin="1",Units="cm")) float MinSizeDistance=2500;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Nameplate",meta=(ClampMin="0.05",ClampMax="1")) float MinimumScale=.25f;
};
class UTexture2D;
class UFont;
class UCanvas;
class UCultivationSubsystem;
struct FGeometry;
class FSlateWindowElementList;

/** Shared geometry for Canvas HUD and Slate overlays; never changes world trajectories. */
namespace CultivationNameplate
{
    struct FLayout
    {
        FBox2D Badge,Avatar,Weapon,Realm,Name;
        FVector2D Size;
    };
    struct FPresentation
    {
        FString DisplayName,WeaponTitle,RealmTitle;
        FName TitleId;
        int32 RealmIndex=0;
        double RealmAdvancePulse=0;
        UTexture2D* AvatarTexture=nullptr;
        int32 AvatarFrame=0;
        /** False while the UMG widget owns the entire nameplate, including avatar and name. */
        bool bDrawPlaques=true;
    };
    BOUNDLESSIMMORTALITY_API FLayout Layout(float Scale=1.f);
    BOUNDLESSIMMORTALITY_API float DistanceScale(float Distance,const FCultivationNameplateDistanceSettings& Settings);
    BOUNDLESSIMMORTALITY_API FBox2D BannerUV(int32 RealmIndex);
    BOUNDLESSIMMORTALITY_API FBox2D BadgeUV(int32 RealmIndex);
    BOUNDLESSIMMORTALITY_API FBox2D WeaponUV(FName TitleId);
    BOUNDLESSIMMORTALITY_API FBox2D OrnamentUV(int32 Tile);
    BOUNDLESSIMMORTALITY_API FPresentation Resolve(UCultivationSubsystem* State,const FString& OwnerId,const FString& FallbackName);
    BOUNDLESSIMMORTALITY_API int32 DrawSlate(const FPresentation& Data,const FLayout& Geometry,const FVector2D& Origin,const FGeometry& Parent,FSlateWindowElementList& Elements,int32 Layer,UFont* Font,UTexture2D* Banners,UTexture2D* Chart,UTexture2D* Titles);
    BOUNDLESSIMMORTALITY_API void DrawCanvas(const FPresentation& Data,const FLayout& Geometry,const FVector2D& Origin,UCanvas* Canvas,UFont* Font,UTexture2D* Banners,UTexture2D* Chart,UTexture2D* Titles);
}
