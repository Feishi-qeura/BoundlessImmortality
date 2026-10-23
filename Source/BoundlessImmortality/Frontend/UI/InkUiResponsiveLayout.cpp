#include "InkUiResponsiveLayout.h"
#include "InkUiTheme.h"

namespace InkUiLayout
{
    FInkHudLayoutRects ResolveHud(FVector2D Size, float BottomInset)
    {
        FInkHudLayoutRects Out;
        const float W = FMath::Max(320., Size.X), H = FMath::Max(320., Size.Y);
        const auto* Theme=InkUiTypography::Theme();
        const float Pad=FMath::IsFinite(Theme->OuterPadding)?FMath::Clamp(Theme->OuterPadding,0.f,64.f):24.f;
        const float Hit=FMath::IsFinite(Theme->MinHitSize)?FMath::Clamp(Theme->MinHitSize,88.f,128.f):88.f;
        const bool Landscape = IsLandscape({W,H});
        const float NavW = FMath::Min(W - 2*Pad, FMath::Max(5*Hit,Landscape ? 560.f : 672.f));
        const float NavH = FMath::Max(104.f,Hit);
        BottomInset = FMath::IsFinite(BottomInset) ? FMath::Clamp(BottomInset, 0.f, H - NavH - 200.f) : 0;
        const float X = (W - NavW) * .5f, Y = H - Pad - BottomInset - NavH;
        Out.Navigation = FSlateRect(X, Y, X + NavW, Y + NavH);
        for (int32 I = 0; I < 5; ++I)
            Out.BottomActions.Emplace(X + I * NavW / 5, Y, X + (I + 1) * NavW / 5, Y + NavH);
        Out.Identity = FSlateRect(Pad, Pad, FMath::Min(Pad+440, W-Pad), Pad+128);
        const bool WrapShortcuts=Landscape&&W<1000;
        const float TopW=WrapShortcuts?3*Hit+16:5*Hit+32;
        // Portrait shortcuts follow the right edge, leaving the central standing character clear.
        Out.bTopActionsOnSecondRow = !Landscape;
        for (int32 I = 0; I < 5; ++I)
        {
            const int32 PortraitColumn=I<3?0:1,PortraitRow=I<3?I:I-3;
            const float PortraitTop=FMath::Max(Pad+Hit,FMath::Min(float(Out.Identity.Bottom+16),float(Out.Navigation.Top-(3*Hit+16))));
            const float TopX=Landscape?W-Pad-TopW+(WrapShortcuts?I%3:I)*(Hit+8):W-Pad-Hit-PortraitColumn*(Hit+8);
            const float TopY=Landscape?Pad+(WrapShortcuts?I/3:0)*(Hit+8):PortraitTop+PortraitRow*(Hit+8);
            Out.TopActions.Emplace(TopX,TopY,TopX+Hit,TopY+Hit);
        }
        Out.RuntimeMenu = FSlateRect(W-Pad-264, Y-296, W-Pad, Y-8);
        float ShortcutLeft=W-Pad;for(const auto& Rect:Out.TopActions)ShortcutLeft=FMath::Min(ShortcutLeft,Rect.Left);
        const float AnnouncementRight=Landscape?W-Pad:ShortcutLeft-12.f;
        const float AnnouncementWidth=FMath::Max(0.f,FMath::Min(560.f,AnnouncementRight-Pad));
        const float AnnouncementY=FMath::Max(Out.Identity.Bottom,Landscape?Out.TopActions.Last().Bottom:Out.Identity.Bottom)+16.f;
        Out.Announcement=FSlateRect(Pad,AnnouncementY,Pad+AnnouncementWidth,AnnouncementY+40.f);
        return Out;
    }

    FSlateRect ResolveFeatureFrame(FVector2D Size, bool bDense)
    {
        const float W = FMath::Max(320., Size.X), H = FMath::Max(320., Size.Y);
        const float RawPadding=InkUiTypography::Theme()->OuterPadding;
        const float Pad=FMath::IsFinite(RawPadding)?FMath::Clamp(RawPadding,0.f,64.f):24.f;
        const bool Landscape = IsLandscape({W,H});
        // Feature lists/details need real reading width; a narrow right drawer
        // forced landscape content back into portrait and hid actions below it.
        const float Width = FMath::Min(W-2*Pad,Landscape?1180.f:900.f);
        const float X = (W-Width)*.5f;
        return FSlateRect(X,Pad,X+Width,H-Pad);
    }
}
