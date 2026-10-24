#include "InkHomeArt.h"
#include "Frontend/UI/InkUiAssetPaths.h"
#include "Engine/Texture2D.h"
#include "Rendering/DrawElements.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

namespace InkHomeArt
{
    const FSprite* Find(FName Id)
    {
        static const FSprite Sprites[]={
#include "InkHomeSpriteTable.inl"
        };
        for(const auto& S:Sprites)if(Id==S.Id)return &S;
        return nullptr;
    }
    FSlateBrush Brush(FName Id)
    {
        FSlateBrush B;const auto* S=Find(Id);if(!S)return B;
        B.DrawAs=ESlateBrushDrawType::Image;B.ImageSize=S->Size;
        const FString Path=InkUiAssets::ResolveNamed(S->Name);
        B.SetResourceObject(InkUiAssets::Load<UTexture2D>(nullptr,*Path));return B;
    }
    FSlateRect Fit(FVector2D Source,FSlateRect Bounds)
    {
        const auto Box=Bounds.GetSize();
        if(Source.X<=0||Source.Y<=0||Box.X<=0||Box.Y<=0)return FSlateRect(Bounds.GetTopLeft(),Bounds.GetTopLeft());
        const auto Size=Source*FMath::Min(Box.X/Source.X,Box.Y/Source.Y);
        const auto P=Bounds.GetTopLeft()+(Box-Size)*.5;
        return FSlateRect(P,P+Size);
    }
    bool HorizontalSlice(FName Id,FVector2D D)
    {
        const auto* S=Find(Id);if(!S||D.X<=0||D.Y<=0)return false;
        // Compare in source-height space, including at the native aspect boundary.
        if(Id==TEXT("home-rank-paper"))return D.X*S->Size.Y>=D.Y*S->Size.X-1.e-3;
        if(Id==TEXT("home-bar-track"))return D.X*S->Size.Y/D.Y>S->MarginsLRTB.X+S->MarginsLRTB.Y;
        return false;
    }
    FSlateRect ArtBounds(FName Id,FSlateRect Container)
    {
        const auto* S=Find(Id);if(!S)return FSlateRect(0,0,0,0);
        return HorizontalSlice(Id,Container.GetSize())?Container:Fit(S->Size,Container);
    }
    FMargin NormalizedMargin(FName Id)
    {
        const auto* S=Find(Id);if(!S)return FMargin(0);
        const auto M=S->MarginsLRTB;
        return FMargin(M.X/S->Size.X,M.Z/S->Size.Y,M.Y/S->Size.X,M.W/S->Size.Y);
    }
    void Paint(FName Id,const FSlateBrush& Brush,FSlateRect Bounds,const FGeometry& G,FSlateWindowElementList& E,int32 L,FLinearColor Tint,bool ForceFit)
    {
        const auto* S=Find(Id);if(!S||!Brush.GetResourceObject())return;
        const auto Rect=ForceFit?Fit(S->Size,Bounds):ArtBounds(Id,Bounds);const auto Size=Rect.GetSize();if(Size.X<=0||Size.Y<=0)return;
        auto Draw=[&](FSlateBrush B,FVector2D P,FVector2D Extent)
        {
            FSlateDrawElement::MakeBox(E,L,G.ToPaintGeometry(FVector2f(Extent),FSlateLayoutTransform(FVector2f(P))),&B,ESlateDrawEffect::None,Tint);
        };
        if(ForceFit||!HorizontalSlice(Id,Bounds.GetSize())){Draw(Brush,Rect.GetTopLeft(),Size);return;}
        // With destination height uniformly scaled and locked, the three Y bands
        // have identical scale. Full-height columns are exactly equivalent to
        // nine patches, without Slate Box's resource-size-dependent corner scale.
        const double Scale=Size.Y/S->Size.Y;
        const double Left=S->MarginsLRTB.X*Scale,Right=S->MarginsLRTB.Y*Scale;
        const double Xs[]={0,Left,Size.X-Right,Size.X};
        const double Us[]={0,S->MarginsLRTB.X/S->Size.X,1-S->MarginsLRTB.Y/S->Size.X,1};
        for(int32 I=0;I<3;++I)
        {
            auto B=Brush;B.SetUVRegion(FBox2f(FVector2f(Us[I],0),FVector2f(Us[I+1],1)));
            Draw(B,Rect.GetTopLeft()+FVector2D(Xs[I],0),FVector2D(Xs[I+1]-Xs[I],Size.Y));
        }
    }
}

void UInkHomeArtWidget::Configure(FName Id)
{
    Component=Id;Sprite=InkHomeArt::Brush(Id);SetVisibility(ESlateVisibility::HitTestInvisible);ForceVolatile(true);
}
int32 UInkHomeArtWidget::NativePaint(const FPaintArgs& A,const FGeometry& G,const FSlateRect& R,FSlateWindowElementList& E,int32 L,const FWidgetStyle& S,bool B)const
{
    const bool ForceFit=PolicyContainerSize.X>0&&!InkHomeArt::HorizontalSlice(Component,PolicyContainerSize);
    InkHomeArt::Paint(Component,Sprite,FSlateRect(FVector2D(0),G.GetLocalSize()),G,E,L,S.GetColorAndOpacityTint(),ForceFit);
    return Super::NativePaint(A,G,R,E,L+1,S,B);
}
FString UInkHomeArtWidget::DescribeArt()const
{
    const auto Size=GetCachedGeometry().GetLocalSize();const auto Rect=InkHomeArt::ArtBounds(Component,FSlateRect(FVector2D(0),Size));
    auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("component"),Component.ToString());J->SetStringField(TEXT("object"),GetPathName());
    const auto PolicySize=PolicyContainerSize.X>0?PolicyContainerSize:Size;
    J->SetStringField(TEXT("mode"),InkHomeArt::HorizontalSlice(Component,PolicySize)?TEXT("height-locked-X-nine-slice"):TEXT("full-Fit"));
    J->SetNumberField(TEXT("container_width"),PolicySize.X);J->SetNumberField(TEXT("container_height"),PolicySize.Y);
    J->SetNumberField(TEXT("width"),Size.X);J->SetNumberField(TEXT("height"),Size.Y);J->SetNumberField(TEXT("art_x"),Rect.Left);J->SetNumberField(TEXT("art_y"),Rect.Top);
    J->SetNumberField(TEXT("art_width"),Rect.GetSize().X);J->SetNumberField(TEXT("art_height"),Rect.GetSize().Y);
    auto Contained=[&](const UWidget* Widget)
    {
        if(!Widget)return false;
        const auto& G=Widget->GetCachedGeometry();const auto& Local=GetCachedGeometry();
        const auto A=Local.AbsoluteToLocal(G.LocalToAbsolute(FVector2D(0)));
        const auto B=Local.AbsoluteToLocal(G.LocalToAbsolute(G.GetLocalSize()));
        return A.X>=Rect.Left-.1&&A.Y>=Rect.Top-.1&&B.X<=Rect.Right+.1&&B.Y<=Rect.Bottom+.1;
    };
    const auto* Parent=GetParent();const UTextBlock* Caption=nullptr;
    if(Parent)for(auto* Child:Parent->GetAllChildren())if(Child->GetFName()==TEXT("HUDRankCaption"))Caption=Cast<UTextBlock>(Child);
    J->SetBoolField(TEXT("hit_inside_art"),Parent&&Contained(Parent->GetParent()));
    J->SetBoolField(TEXT("caption_inside_art"),Contained(Caption));
    FString Out;FJsonSerializer::Serialize(J,TJsonWriterFactory<>::Create(&Out));return Out;
}
