#include "CultivationNameplate.h"
#include "CultivationSubsystem.h"
#include "Frontend/InkNiagaraTitleWidget.h"
#include "Materials/MaterialInterface.h"
#include "LiveWeapons/LiveGMOverlay.h"
#include "LiveWeapons/LiveGMTestSession.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Engine/Texture2D.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "CanvasItem.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Styling/CoreStyle.h"

namespace CultivationNameplate
{
float DistanceScale(float Distance,const FCultivationNameplateDistanceSettings& Settings)
{
    const float Near=FMath::IsFinite(Settings.FullSizeDistance)?FMath::Max(0.f,Settings.FullSizeDistance):250.f;
    const float Far=FMath::IsFinite(Settings.MinSizeDistance)?FMath::Max(Near+1.f,Settings.MinSizeDistance):Near+2250.f;
    const float Minimum=FMath::IsFinite(Settings.MinimumScale)?FMath::Clamp(Settings.MinimumScale,.05f,1.f):.25f;
    const float T=FMath::IsFinite(Distance)?FMath::Clamp((Distance-Near)/(Far-Near),0.f,1.f):0.f;
    return FMath::Lerp(1.f,Minimum,T*T*(3.f-2.f*T));
}
FLayout Layout(float Scale)
{
    Scale=FMath::Max(.01f,Scale);FLayout L;
    L.Badge=FBox2D(FVector2D(0,9)*Scale,FVector2D(34,43)*Scale);
    L.Avatar=FBox2D(FVector2D(1,47)*Scale,FVector2D(33,79)*Scale);
    L.Weapon=FBox2D(FVector2D(38,0)*Scale,FVector2D(168,26)*Scale);
    L.Realm=FBox2D(FVector2D(38,26)*Scale,FVector2D(168,52)*Scale);
    L.Name=FBox2D(FVector2D(38,54)*Scale,FVector2D(168,79)*Scale);L.Size=FVector2D(168,80)*Scale;return L;
}
FBox2D OrnamentUV(int32 Tile)
{Tile=FMath::Clamp(Tile,0,15);return FBox2D(FVector2D((Tile%4)*.25,(Tile/4)*.25),FVector2D((Tile%4+1)*.25,(Tile/4+1)*.25));}
FBox2D BannerUV(int32 RealmIndex)
{
    const int32 I=FMath::Clamp(RealmIndex,0,33),Row=I%17;
    static const float Y[]={0,90,170,250,330,410,490,570,650,730,810,890,970,1050,1130,1210,1290,1402};
    const float Left=I<17?0.f:560.f,Right=I<17?556.f:1122.f;
    return FBox2D(FVector2D(Left/1122.f,Y[Row]/1402.f),FVector2D(Right/1122.f,Y[Row+1]/1402.f));
}
FBox2D BadgeUV(int32 RealmIndex)
{
    // Individually inspected stage illustrations, excluding the chart's vertical captions.
    static const FVector4 Regions[]={
        FVector4(153,5,277,111),FVector4(290,5,412,111),FVector4(420,5,541,111),FVector4(557,5,680,111),FVector4(694,5,817,111),FVector4(837,5,963,111),FVector4(990,5,1121,111),FVector4(1145,5,1276,111),FVector4(1296,5,1435,111),
        FVector4(145,176,295,280),FVector4(307,176,457,280),FVector4(475,176,635,280),FVector4(661,176,841,280),
        FVector4(157,348,289,446),FVector4(316,348,450,446),FVector4(476,348,614,446),
        FVector4(156,509,296,606),FVector4(319,509,459,606),FVector4(479,509,619,606),FVector4(633,509,773,606),FVector4(790,509,930,606),FVector4(943,509,1083,606),
        FVector4(146,674,312,780),FVector4(500,674,674,780),FVector4(826,674,994,780),FVector4(999,674,1163,780),
        FVector4(144,837,263,946),FVector4(269,837,391,946),FVector4(403,837,525,946),FVector4(536,837,659,946),FVector4(676,837,807,946),
        FVector4(922,837,1090,1007),FVector4(1148,837,1289,1007),FVector4(1367,824,1533,1007)};
    const auto R=Regions[FMath::Clamp(RealmIndex,0,33)];return FBox2D(FVector2D(R.X/1536.f,R.Y/1024.f),FVector2D(R.Z/1536.f,R.W/1024.f));
}
FBox2D WeaponUV(FName TitleId)
{
    FVector4 R(0,0,0,0);
    if(TitleId==TEXT("LeftGuardian"))R=FVector4(5,0,626,183);else if(TitleId==TEXT("RightGuardian"))R=FVector4(632,0,1254,178);
    else if(TitleId==TEXT("Novice"))R=FVector4(7,182,620,329);else if(TitleId==TEXT("Godslayer"))R=FVector4(634,174,1254,347);
    else if(TitleId==TEXT("SoulEater"))R=FVector4(4,329,627,515);else if(TitleId==TEXT("SevenStar"))R=FVector4(632,348,1254,522);
    else if(TitleId==TEXT("DivineBlade"))R=FVector4(0,513,627,706);
    // The supplied Cyberspace image spells a different word; use exact runtime text.
    return FBox2D(FVector2D(R.X/1254.f,R.Y/1254.f),FVector2D(R.Z/1254.f,R.W/1254.f));
}
FPresentation Resolve(UCultivationSubsystem* State,const FString& OwnerId,const FString& FallbackName)
{
    FPresentation P;P.DisplayName=FallbackName.IsEmpty()?TEXT("无名剑灵"):FallbackName;
    if(State)
    {
        const auto S=State->GetParticipantSnapshot(OwnerId);P.RealmIndex=S.RealmIndex;P.RealmTitle=S.RealmName.ToString();P.TitleId=S.TitleId;P.WeaponTitle=S.TitleName.ToString();P.RealmAdvancePulse=S.RealmAdvancePulse;
        if(P.WeaponTitle.IsEmpty())P.WeaponTitle=S.bIsAnchor?TEXT("本命剑"):TEXT("剑灵");
        if(!S.DisplayName.IsEmpty())P.DisplayName=S.DisplayName;
    }
#if !UE_BUILD_SHIPPING
    if(State&&State->GetWorld())for(TActorIterator<ALiveGMOverlay> It(State->GetWorld());It;++It)
    {
        const auto* Session=It->TestSession.Get();
        if(!Session||!Session->CanMutate()||Session->PreviewUntil<=State->GetWorld()->GetTimeSeconds())continue;
        auto* Host=State->GetWorld()->GetGameInstance()->GetSubsystem<ULiveInteractionSubsystem>();
        if(!Host||Host->GetScopedUserId(Session->ViewerId)!=OwnerId)continue;
        P.RealmIndex=Session->PreviewRealm;P.RealmTitle=TEXT("境界试映 ")+FString::FromInt(P.RealmIndex+1);
        if(!Session->PreviewTitle.IsEmpty())P.WeaponTitle=Session->PreviewTitle;
        P.RealmAdvancePulse=FMath::Clamp((Session->AdvanceUntil-State->GetWorld()->GetTimeSeconds())/3.,0.,1.);
    }
#endif
    if(P.RealmTitle.IsEmpty())P.RealmTitle=TEXT("练气一阶");if(P.DisplayName.Len()>10)P.DisplayName=P.DisplayName.Left(9)+TEXT("…");return P;
}
int32 DrawSlate(const FPresentation& Data,const FLayout& L,const FVector2D& Origin,const FGeometry& Parent,FSlateWindowElementList& Elements,int32 Layer,UFont* Font,UTexture2D* Banners,UTexture2D* Chart,UTexture2D* Titles)
{
    // An active projected UMG nameplate also owns identity; never split one participant across renderers.
    if(!Data.bDrawPlaques)return Layer;
    const auto FontMeasure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    FSlateFontInfo TextFont=FCoreStyle::GetDefaultFontStyle("Regular",FMath::Max(1,FMath::RoundToInt(13.f*L.Size.X/168.f)));if(Font)TextFont.FontObject=Font;
    auto Box=[&](const FBox2D& R,UTexture2D* Texture,const FBox2D& UV,FLinearColor Color)
    {
        FSlateBrush Brush;Brush.DrawAs=ESlateBrushDrawType::Image;Brush.SetResourceObject(Texture);
        if(Texture)Brush.SetUVRegion(FBox2f(FVector2f(UV.Min),FVector2f(UV.Max)));
        else Brush=*FCoreStyle::Get().GetBrush("WhiteBrush");
        FSlateDrawElement::MakeBox(Elements,Layer,Parent.ToPaintGeometry(FVector2f(R.GetSize()),FSlateLayoutTransform(FVector2f(Origin+R.Min))),&Brush,ESlateDrawEffect::None,Color);
    };
    auto Caption=[&](const FBox2D& R,const FString& Text,FLinearColor Color)
    {
        const auto Extent=FontMeasure->Measure(Text,TextFont);const FVector2D Pos=Origin+R.GetCenter()-FVector2D(Extent)*.5;
        FSlateDrawElement::MakeText(Elements,Layer,Parent.ToPaintGeometry(FVector2f(R.GetSize()),FSlateLayoutTransform(FVector2f(Pos+FVector2D(.75,.75)))),Text,TextFont,ESlateDrawEffect::None,FLinearColor(0,0,0,.9f));
        FSlateDrawElement::MakeText(Elements,Layer+1,Parent.ToPaintGeometry(FVector2f(R.GetSize()),FSlateLayoutTransform(FVector2f(Pos))),Text,TextFont,ESlateDrawEffect::None,Color);
    };
    if(Data.bDrawPlaques)
    {
        auto Art=[&](const FBox2D& Rect,const FSlateBrush& Brush)
        {
            if(!Brush.GetResourceObject())return false;
            FBox2D Fit=Rect;const FVector2D Source=Brush.ImageSize;
            if(Source.X>0&&Source.Y>0){const double Ratio=FMath::Min(Rect.GetSize().X/Source.X,Rect.GetSize().Y/Source.Y);const auto Half=Source*Ratio*.5;Fit=FBox2D(Rect.GetCenter()-Half,Rect.GetCenter()+Half);}
            FSlateDrawElement::MakeBox(Elements,Layer,Parent.ToPaintGeometry(FVector2f(Fit.GetSize()),FSlateLayoutTransform(FVector2f(Origin+Fit.Min))),&Brush,ESlateDrawEffect::None,FLinearColor::White);return true;
        };
        if(!Art(L.Weapon,InkNiagaraTitlePresentation::HonorBrush(Data.TitleId)))Caption(L.Weapon,Data.WeaponTitle,FLinearColor(.98f,.85f,.55f,1));
        if(!Art(L.Realm,InkNiagaraTitlePresentation::RealmBrush(Data.RealmIndex)))Caption(L.Realm,Data.RealmTitle,FLinearColor(.90f,.94f,.90f,1));
        Art(L.Badge,InkNiagaraTitlePresentation::RealmIconBrush(Data.RealmIndex));
    }
    if(Data.AvatarTexture)
    {
        FSlateBrush Avatar;Avatar.DrawAs=ESlateBrushDrawType::RoundedBox;Avatar.ImageType=ESlateBrushImageType::FullColor;Avatar.OutlineSettings=FSlateBrushOutlineSettings(float(L.Avatar.GetSize().X*.5));Avatar.SetResourceObject(Data.AvatarTexture);
        FSlateDrawElement::MakeBox(Elements,Layer,Parent.ToPaintGeometry(FVector2f(L.Avatar.GetSize()),FSlateLayoutTransform(FVector2f(Origin+L.Avatar.Min))),&Avatar,ESlateDrawEffect::None,FLinearColor::White);
    }
    else if(Chart)Box(L.Avatar,Chart,OrnamentUV(4),FLinearColor(.75f,.81f,.79f,1));
    if(Chart)Box(L.Avatar,Chart,OrnamentUV(Data.AvatarFrame),FLinearColor::White);
    Caption(L.Name,Data.DisplayName,FLinearColor(.94f,.97f,.93f,1));return Layer+2;
}
void DrawCanvas(const FPresentation& Data,const FLayout& L,const FVector2D& Origin,UCanvas* Canvas,UFont* Font,UTexture2D* Banners,UTexture2D* Chart,UTexture2D* Titles)
{
    if(!Data.bDrawPlaques||!Canvas||!Font)return;
    auto Box=[&](const FBox2D& R,UTexture2D* Texture,const FBox2D& UV,FLinearColor Color)
    {
        if(Texture&&Texture->GetResource())
        {FCanvasTileItem Tile(Origin+R.Min,Texture->GetResource(),R.GetSize(),UV.Min,UV.Max,Color);Tile.BlendMode=SE_BLEND_Translucent;Canvas->DrawItem(Tile);}
        else{FCanvasTileItem Plain(Origin+R.Min,R.GetSize(),Color);Plain.BlendMode=SE_BLEND_Translucent;Canvas->DrawItem(Plain);}
    };
    auto Caption=[&](const FBox2D& R,const FString& Text,FLinearColor Color)
    {
        float W=0,H=0;Canvas->StrLen(Font,Text,W,H);const float S=FMath::Min(float(R.GetSize().Y/FMath::Max(1.f,H)),float(R.GetSize().X/FMath::Max(1.f,W)));
        FCanvasTextItem Item(Origin+R.GetCenter()-FVector2D(W,H)*S*.5,FText::FromString(Text),Font,Color);Item.Scale=FVector2D(S);Item.EnableShadow(FLinearColor::Black);Canvas->DrawItem(Item);
    };
    if(Data.bDrawPlaques)
    {
        auto Art=[&](const FBox2D& Rect,const FSlateBrush& Brush)
        {
            const FVector2D Source=Brush.ImageSize;FBox2D Fit=Rect;
            if(Source.X>0&&Source.Y>0){const double Ratio=FMath::Min(Rect.GetSize().X/Source.X,Rect.GetSize().Y/Source.Y);const auto Half=Source*Ratio*.5;Fit=FBox2D(Rect.GetCenter()-Half,Rect.GetCenter()+Half);}
            if(auto* M=Cast<UMaterialInterface>(Brush.GetResourceObject())){FCanvasTileItem Tile(Origin+Fit.Min,M->GetRenderProxy(),Fit.GetSize());Canvas->DrawItem(Tile);return true;}
            if(auto* T=Cast<UTexture2D>(Brush.GetResourceObject())){const FBox2f UV=Brush.GetUVRegion();Box(Fit,T,UV.bIsValid?FBox2D(FVector2D(UV.Min),FVector2D(UV.Max)):FBox2D(FVector2D::ZeroVector,FVector2D(1)),FLinearColor::White);return true;}return false;
        };
        if(!Art(L.Weapon,InkNiagaraTitlePresentation::HonorBrush(Data.TitleId)))Caption(L.Weapon,Data.WeaponTitle,FLinearColor(.98f,.85f,.55f,1));
        if(!Art(L.Realm,InkNiagaraTitlePresentation::RealmBrush(Data.RealmIndex)))Caption(L.Realm,Data.RealmTitle,FLinearColor(.90f,.94f,.9f,1));
        Art(L.Badge,InkNiagaraTitlePresentation::RealmIconBrush(Data.RealmIndex));
    }
    if(Data.AvatarTexture)Box(L.Avatar,Data.AvatarTexture,FBox2D(FVector2D::ZeroVector,FVector2D(1)),FLinearColor::White);
    else if(Chart)Box(L.Avatar,Chart,OrnamentUV(4),FLinearColor(.75f,.81f,.79f,1));
    if(Chart)Box(L.Avatar,Chart,OrnamentUV(Data.AvatarFrame),FLinearColor::White);
    Caption(L.Name,Data.DisplayName,FLinearColor(.88f,.95f,.92f,1));
}
}
