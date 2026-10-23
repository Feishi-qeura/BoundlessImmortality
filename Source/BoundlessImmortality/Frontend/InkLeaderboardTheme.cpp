#include "InkLeaderboardTheme.h"
#include "Frontend/UI/InkUiAssetPaths.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture2D.h"

namespace InkLeaderboardTheme
{
    static FString Key(FName Category){return Category==TEXT("Damage")?TEXT("Damage"):Category==TEXT("CultivationSpeed")?TEXT("Speed"):TEXT("Sword");}
    static FSlateBrush Material(const FString& Name,FVector2D Size)
    {
        FSlateBrush B;B.DrawAs=ESlateBrushDrawType::Image;B.ImageSize=Size;
        B.SetResourceObject(InkUiAssets::Load<UMaterialInterface>(nullptr,*(TEXT("/Game/UI/Ink/NiagaraRevision20261022/")+Name+TEXT(".")+Name)));return B;
    }
    FSlateBrush Header(FName Category)
    {
        const FString K=Key(Category);
        return Material(TEXT("M_Rank")+K+TEXT("Header"),K==TEXT("Sword")?FVector2D(2070,490):K==TEXT("Speed")?FVector2D(2065,465):FVector2D(2062,476));
    }
    FSlateBrush Row(FName Category,int32 Rank)
    {
        if(Rank<1||Rank>10)return FSlateBrush();
        auto Brush=Material(FString::Printf(TEXT("M_Rank%sTier%02d"),*Key(Category),Rank),FVector2D(600,88));
        // Each tier has a different crop height. Retain its actual source aspect.
        if(const auto* Asset=Cast<UMaterialInterface>(Brush.GetResourceObject()))
        {
            FLinearColor Crop;UTexture* Atlas=nullptr;
            if(Asset->GetVectorParameterValue(FMaterialParameterInfo(TEXT("CropSize")),Crop)&&
                Asset->GetTextureParameterValue(FMaterialParameterInfo(TEXT("TierTexture")),Atlas)&&Atlas)
            {
                // Platform mips may resize a non-square atlas independently on each axis.
                // Crop UVs describe the imported image, so preserve that image's aspect.
                const auto* Texture=Cast<UTexture2D>(Atlas);
                const FVector2D AtlasSize=Texture?FVector2D(Texture->GetImportedSize()):FVector2D(Atlas->GetSurfaceWidth(),Atlas->GetSurfaceHeight());
                const FVector2D Source(Crop.R*AtlasSize.X,Crop.G*AtlasSize.Y);
                if(Source.X>0&&Source.Y>0)Brush.ImageSize=Source;
            }
        }
        return Brush;
    }
    FVector4 FitRowArt(FVector2D /*SourceSize*/,FVector2D RowSize)
    {
        // The slice material preserves the end ornaments and expands the readable center.
        // Every row has the same small outer gap instead of bottom-aligned transparent space.
        return FVector4(0,2,RowSize.X,FMath::Max(0.,RowSize.Y-4));
    }
    FLinearColor Color(FName Category)
    {
        return Category==TEXT("Damage")?FLinearColor(.8f,.23f,.10f,.85f):Category==TEXT("CultivationSpeed")?FLinearColor(.13f,.66f,.49f,.85f):FLinearColor(.88f,.60f,.16f,.85f);
    }
    int32 Quality(int32 Rank){return FMath::Clamp((10-Rank)*6/9,0,6);}
    float Strength(int32 Rank){return Rank>=1&&Rank<=10?.22f+(11-Rank)*.09f:0.f;}
}
