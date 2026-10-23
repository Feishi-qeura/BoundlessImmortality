#include "InkPresentationStyle.h"
#include "Frontend/UI/InkUiAssetPaths.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"

namespace InkPresentationStyle
{
    namespace
    {
        FSlateBrush Atlas(const TCHAR* Name,const FBox2f& UV,FVector2D Size)
        {
            FSlateBrush B;B.DrawAs=ESlateBrushDrawType::Image;B.ImageSize=Size;
            const FString Path=FString(TEXT("/Game/UI/Ink/NiagaraRevision20261022/Textures/"))+Name+TEXT(".")+Name;
            B.SetResourceObject(InkUiAssets::Load<UTexture2D>(nullptr,*Path,nullptr,LOAD_NoWarn|LOAD_Quiet));B.SetUVRegion(UV);return B;
        }
        FSlateBrush ButtonArt(int32 State)
        {
            const TCHAR* Names[]={TEXT("Normal"),TEXT("Selected"),TEXT("Hover"),TEXT("Pressed")};
            const FString Name=FString(TEXT("M_XianxiaButton"))+Names[FMath::Clamp(State,0,3)]+TEXT("Slice");
            // The material expands only the center of the original atlas crop.
            // Its end ornaments retain their aspect at every control size.
            FSlateBrush B;B.ImageSize=FVector2D(240,68);B.DrawAs=ESlateBrushDrawType::Image;
            B.SetResourceObject(InkUiAssets::Load<UMaterialInterface>(nullptr,*(TEXT("/Game/UI/Ink/NiagaraRevision20261022/")+Name+TEXT(".")+Name),nullptr,LOAD_NoWarn|LOAD_Quiet));return B;
        }
    }
    FSlateBrush IdentityBackground()
    {
        FSlateBrush B;B.ImageSize=FVector2D(368,164);B.DrawAs=ESlateBrushDrawType::RoundedBox;B.TintColor=Paper();
        B.OutlineSettings.CornerRadii=FVector4(4,4,4,4);B.OutlineSettings.RoundingType=ESlateBrushRoundingType::FixedRadius;B.OutlineSettings.Color=Gold();B.OutlineSettings.Width=1;return B;
    }
    FSlateBrush InkWash()
    {
        FSlateBrush B;B.DrawAs=ESlateBrushDrawType::Image;B.SetResourceObject(InkUiAssets::Load<UMaterialInterface>(nullptr,TEXT("/Game/UI/Ink/Materials/Shared/M_InkWash.M_InkWash")));return B;
    }
    FSlateBrush NamecardBackground()
    {
        FSlateBrush B;B.DrawAs=ESlateBrushDrawType::Image;B.ImageSize=FVector2D(368,184);
        B.SetResourceObject(InkUiAssets::Load<UMaterialInterface>(nullptr,TEXT("/Game/UI/Ink/Materials/HUD/M_InkGoldNamecard.M_InkGoldNamecard")));return B;
    }
    FSlateBrush LeaderboardBackground(FVector2D Size)
    {
        const TCHAR* Name=Size.X<400?TEXT("M_InkGoldRankCompact"):Size.X<Size.Y?TEXT("M_InkGoldRankPortrait"):TEXT("M_InkGoldRankLandscape");
        const FString Path=FString(TEXT("/Game/UI/Ink/NiagaraRevision20261022/"))+Name+TEXT(".")+Name;
        FSlateBrush B;B.DrawAs=ESlateBrushDrawType::Image;B.ImageSize=Size;B.SetResourceObject(InkUiAssets::Load<UMaterialInterface>(nullptr,*Path));return B;
    }
    FSlateBrush NavigationFade()
    {
        FSlateBrush B;B.DrawAs=ESlateBrushDrawType::Image;B.SetResourceObject(InkUiAssets::Load<UMaterialInterface>(nullptr,TEXT("/Game/UI/Ink/Materials/HUD/M_InkNavigationFade.M_InkNavigationFade")));return B;
    }
    FSlateBrush Meter(int32 Kind,bool bFill)
    {
        Kind=FMath::Clamp(Kind,0,2);
        // Use the interior of the generated energy strips; tracks retain both jade endcaps.
        const FVector4 Tracks[]={FVector4(65,8,1710,160),FVector4(65,162,1710,318),FVector4(65,322,1710,474)};
        const FVector4 Fills[]={FVector4(212,512,1562,560),FVector4(212,647,1562,694),FVector4(212,780,1562,827)};
        const auto R=bFill?Fills[Kind]:Tracks[Kind];
        return Atlas(TEXT("T_XianxiaMeters"),FBox2f(FVector2f(R.X/1774.f,R.Y/887.f),FVector2f(R.Z/1774.f,R.W/887.f)),FVector2D(252,bFill?12:28));
    }
    FSlateBrush Surface(ESurface Part,FVector2D Size)
    {
        FSlateBrush Brush;Brush.ImageSize=Size;
        if(Part==ESurface::BarTrack||Part==ESurface::BarFill)
        {
            return Meter(1,Part==ESurface::BarFill);
        }
        if(Part==ESurface::PaperButton||Part==ESurface::GoldButton||Part==ESurface::BlueButton||Part==ESurface::SelectedTab||Part==ESurface::Heading)
            return ButtonArt(Part==ESurface::SelectedTab||Part==ESurface::GoldButton||Part==ESurface::BlueButton?1:0);
        Brush.DrawAs=ESlateBrushDrawType::RoundedBox;
        const bool Selected=Part==ESurface::SelectedTab,Blue=Part==ESurface::BlueButton;
        Brush.TintColor=Selected?Ink():Blue?FLinearColor::FromSRGBColor(FColor(61,88,97)):Part==ESurface::GoldButton?Gold():Paper();
        Brush.OutlineSettings.CornerRadii=FVector4(6,6,6,6);Brush.OutlineSettings.RoundingType=ESlateBrushRoundingType::FixedRadius;
        Brush.OutlineSettings.Color=Gold();Brush.OutlineSettings.Width=Selected?1:Part==ESurface::Card?0:.6f;
        if(Part==ESurface::Card&&Size.Y>300)
        {
            return IdentityBackground();
        }
        return Brush;
    }
    FSlateBrush Icon(EIcon Part,FVector2D Size)
    {
        if(Part==EIcon::AvatarFrame)return NavigationIcon(0,Size);
        const int32 Tile=Part==EIcon::Activities?7:Part==EIcon::Cultivation?5:Part==EIcon::Crystal?8:Part==EIcon::Jade?9:Part==EIcon::Chest?12:Part==EIcon::Manual?9:Part==EIcon::Sword?6:INDEX_NONE;
        if(Tile!=INDEX_NONE)return CohesionIcon(Tile,Size);
        FSlateBrush Brush;Brush.DrawAs=ESlateBrushDrawType::Image;Brush.ImageSize=Size;
        auto* Atlas=InkUiAssets::Load<UTexture2D>(nullptr,TEXT("/Game/UI/Ink/Textures/Shared/T_PolishOrnaments.T_PolishOrnaments"),nullptr,LOAD_NoWarn|LOAD_Quiet);Brush.SetResourceObject(Atlas);
        const int32 I=static_cast<uint8>(Part);
        Brush.SetUVRegion(FBox2f(FVector2f((I%4)*.25f,(I/4)*.25f),FVector2f((I%4+1)*.25f,(I/4+1)*.25f)));
        return Brush;
    }
    FSlateBrush CohesionIcon(int32 Tile,FVector2D Size)
    {
        Tile=FMath::Clamp(Tile,0,15);FSlateBrush B;B.DrawAs=ESlateBrushDrawType::Image;B.ImageSize=Size;
        B.SetResourceObject(InkUiAssets::Load<UTexture2D>(nullptr,TEXT("/Game/UI/Ink/Textures/Shared/T_CohesionIcons.T_CohesionIcons"),nullptr,LOAD_NoWarn|LOAD_Quiet));
        B.SetUVRegion(FBox2f(FVector2f((Tile%4)*.25f,(Tile/4)*.25f),FVector2f((Tile%4+1)*.25f,(Tile/4+1)*.25f)));return B;
    }
    FSlateBrush NavigationIcon(int32 Tile,FVector2D Size)
    {
        if(Tile==1||Tile==2||Tile==3)
        {
            const int32 I=Tile-1;auto B=Atlas(TEXT("T_NavigationComplete"),FBox2f(FVector2f(I/3.f,0),FVector2f((I+1)/3.f,1)),Size);if(B.GetResourceObject())return B;
        }
        if(Tile==4)
        {
            auto B=Atlas(TEXT("T_CultivationPadded"),FBox2f(FVector2f(0),FVector2f(1)),Size);
            if(B.GetResourceObject())return B;
        }
        auto* Texture=InkUiAssets::Load<UTexture2D>(nullptr,TEXT("/Game/UI/Ink/Textures/HUD/T_NavigationAtlas.T_NavigationAtlas"),nullptr,LOAD_NoWarn|LOAD_Quiet);
        if(!Texture)return CohesionIcon(Tile,Size);
        Tile=FMath::Clamp(Tile,0,7);FSlateBrush B;B.DrawAs=ESlateBrushDrawType::Image;B.ImageSize=Size;B.SetResourceObject(Texture);
        const float U=1.f/Texture->GetSizeX(),V=1.f/Texture->GetSizeY();
        B.SetUVRegion(FBox2f(FVector2f((Tile%4)*.25f+U,(Tile/4)*.5f+V),FVector2f((Tile%4+1)*.25f-U,(Tile/4+1)*.5f-V)));return B;
    }
    FButtonStyle Button(bool bSelected,bool bPrimary,bool bBlue)
    {
        FButtonStyle Style;auto Normal=Surface(bSelected?ESurface::SelectedTab:bBlue?ESurface::BlueButton:bPrimary?ESurface::GoldButton:ESurface::PaperButton);
        auto Hover=ButtonArt(bSelected||bPrimary||bBlue?1:2);Hover.TintColor=FLinearColor(1.05f,1.05f,1.f,1);
        auto Pressed=ButtonArt(3);
        auto Disabled=Surface(ESurface::PaperButton);Disabled.TintColor=FLinearColor(.65f,.66f,.64f,.65f);
        Style.SetNormal(Normal).SetHovered(Hover).SetPressed(Pressed).SetDisabled(Disabled);
        Style.SetNormalPadding(FMargin(15,7)).SetPressedPadding(FMargin(15,8,15,6));
        return Style;
    }
    FButtonStyle CloseButton()
    {
        auto Skin=[](FLinearColor Tint,FLinearColor Outline)
        {
            FSlateBrush B;B.DrawAs=ESlateBrushDrawType::RoundedBox;B.ImageSize=FVector2D(64);
            B.TintColor=Tint;B.OutlineSettings.CornerRadii=FVector4(5,5,5,5);
            B.OutlineSettings.RoundingType=ESlateBrushRoundingType::FixedRadius;
            B.OutlineSettings.Color=Outline;B.OutlineSettings.Width=1.5f;return B;
        };
        FButtonStyle S;S.SetNormal(Skin(Paper(),Gold())).SetHovered(Skin(FLinearColor::FromSRGBColor(FColor(250,238,203)),Gold()));
        S.SetPressed(Skin(FLinearColor::FromSRGBColor(FColor(223,205,165)),Ink()));
        S.SetDisabled(Skin(Paper(),Gold()));S.SetNormalPadding(FMargin(0));S.SetPressedPadding(FMargin(0,1,0,0));return S;
    }
    FSlateFontInfo ButtonFont(FSlateFontInfo Font,bool bLight)
    {
        // Light captions must remain legible on the pale gold hover/pressed art.
        Font.OutlineSettings.OutlineSize=bLight?1:0;
        Font.OutlineSettings.OutlineColor=Ink();return Font;
    }
    FButtonStyle MainMenuButton(FName Action)
    {
        const TCHAR* Key=Action==TEXT("StartGame")?TEXT("Start"):Action==TEXT("OpenSaves")?TEXT("Continue"):Action==TEXT("OpenSettings")?TEXT("Settings"):TEXT("Exit");
        auto Art=[&](const TCHAR* State)
        {
            const FString Name=FString(TEXT("M_ProvidedMenu"))+Key+State+((Action==TEXT("OpenSettings")||Action==TEXT("RequestQuit"))?TEXT("Clean"):TEXT(""));
            FSlateBrush B;B.ImageSize=FVector2D(208,94);B.DrawAs=ESlateBrushDrawType::Image;
            B.SetResourceObject(InkUiAssets::Load<UMaterialInterface>(nullptr,*(TEXT("/Game/UI/Ink/NiagaraRevision20261022/")+Name+TEXT(".")+Name),nullptr,LOAD_NoWarn|LOAD_Quiet));return B;
        };
        FButtonStyle S;auto Hover=Art(TEXT("Hover")),Pressed=Hover;Pressed.TintColor=FLinearColor(.9f,.86f,.74f,1);
        S.SetNormal(Art(TEXT("Normal"))).SetHovered(Hover).SetPressed(Pressed).SetDisabled(Art(TEXT("Disabled")));
        S.SetNormalPadding(FMargin(0));S.SetPressedPadding(FMargin(0));return S;
    }
    FProgressBarStyle Progress()
    {
        FProgressBarStyle Style;Style.SetBackgroundImage(Surface(ESurface::BarTrack,FVector2D(240,18)));
        Style.SetFillImage(Surface(ESurface::BarFill,FVector2D(240,18)));
        Style.SetMarqueeImage(Surface(ESurface::BarFill,FVector2D(80,18)));
        return Style;
    }
}
