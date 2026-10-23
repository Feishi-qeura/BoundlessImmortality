#include "UI/InkUiResponsiveLayout.h"
#include "UI/InkUiTheme.h"
#include "Frontend/UI/InkUiAssetPaths.h"
#include "InkFrontendWidget.h"
#include "InkPresentationStyle.h"
#include "InkHomeArt.h"
#include "InkNiagaraTitleWidget.h"
#include "InkLeaderboardTheme.h"
#include "Cultivation/CultivationRules.h"
#include "InkInventoryScrollBar.h"
#include "InkLeaderboardAvatarRequest.h"
#include "Cultivation/CultivationNameplate.h"
#include "CultivationGameplayComponent.h"
#include "LiveWeapons/LiveWeaponSwarm.h"
#include "LiveWeapons/LiveGMOverlay.h"
#include "LiveWeapons/LiveWeaponGameMode.h"
#include "LiveWeapons/LiveAudienceAdapterComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/ButtonSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"

namespace InkRankUI
{
    constexpr float CompactWidth=360.f;
    const float CompactRowsHeight=[](){float H=0;for(int32 Rank=1;Rank<=InkLeaderboard::VisibleRows;++Rank)H+=InkLeaderboard::CompactRowLayout(Rank).Height;return H;}();
    const float CompactHeight=140.f+CompactRowsHeight;
    template<typename T> T* Find(UWidgetTree* Tree,FName Name){return Tree?Cast<T>(Tree->FindWidget(Name)):nullptr;}
    FSlateBrush Art(const FString& Name)
    {
        FSlateBrush B;B.DrawAs=ESlateBrushDrawType::Image;B.ImageSize=FVector2D(64);
        const FString Asset=TEXT("/Game/UI/Ink/Materials/Leaderboard/MI_Rank_")+Name;
        B.SetResourceObject(InkUiAssets::Load<UMaterialInterface>(nullptr,*(Asset+TEXT(".MI_Rank_")+Name)));
        return B;
    }
    UCanvasPanelSlot* Place(UCanvasPanel* Parent,UWidget* Child,float X,float Y,float W,float H)
    {
        auto* S=Parent->AddChildToCanvas(Child);S->SetPosition(FVector2D(X,Y));S->SetSize(FVector2D(W,H));S->SetZOrder(2);return S;
    }
    void Header(UImage* Image,FName Category,float Width,float Y,float Height)
    {
        if(!Image)return;auto Brush=InkLeaderboardTheme::Header(Category);Image->SetBrush(Brush);Image->SetVisibility(ESlateVisibility::HitTestInvisible);
        const float W=FMath::Min(Width-56,float(Height*Brush.ImageSize.X/Brush.ImageSize.Y));
        const float H=W*Brush.ImageSize.Y/FMath::Max(1.f,Brush.ImageSize.X);
        if(auto* S=Cast<UCanvasPanelSlot>(Image->Slot))S->SetOffsets(FMargin((Width-W)*.5f,Y+(Height-H)*.5f,W,H));
    }
    void RowBackground(UImage* Image,FName Category,int32 Rank,FVector2D Size,bool Empty)
    {
        if(!Image)return;
        if(!Empty&&Rank>=1&&Rank<=10)
        {
            const auto Brush=InkLeaderboardTheme::Row(Category,Rank);auto* Dynamic=Cast<UMaterialInstanceDynamic>(Image->GetBrush().GetResourceObject());
            if(!Dynamic||Dynamic->Parent!=Brush.GetResourceObject()){Image->SetBrush(Brush);Dynamic=Image->GetDynamicMaterial();}
            if(Dynamic)
            {
                // Let the existing slice expand the white center while retaining the end caps.
                Dynamic->SetVectorParameterValue(TEXT("PanelSize"),FLinearColor(Size.X,Size.Y-2,0,0));
                Dynamic->SetVectorParameterValue(TEXT("SourceSize"),FLinearColor(Brush.ImageSize.X,Brush.ImageSize.Y,0,0));
            }
            const auto Rect=InkLeaderboardTheme::FitRowArt(Brush.ImageSize,FVector2D(Size.X,Size.Y+2));
            if(auto* Slot=Cast<UCanvasPanelSlot>(Image->Slot))Slot->SetOffsets(FMargin(Rect.X,Rect.Y,Rect.Z,Rect.W));
        }
        else
        {
            auto Brush=InkPresentationStyle::Surface(InkPresentationStyle::ESurface::Card,Size);
            auto Tint=InkPresentationStyle::Paper();Tint.A=.78f;Brush.TintColor=Tint;
            Brush.OutlineSettings.Color=FLinearColor::FromSRGBColor(FColor(157,132,88,190));
            Brush.OutlineSettings.Width=1.f;Image->SetBrush(Brush);
            if(auto* Slot=Cast<UCanvasPanelSlot>(Image->Slot))Slot->SetOffsets(FMargin(0,1,Size.X,Size.Y));
        }
    }
    UImage* Picture(UWidgetTree* Tree,UCanvasPanel* Parent,FName Name,const FString& ArtName,float X,float Y,float W,float H)
    {
        auto* I=Tree->ConstructWidget<UImage>(UImage::StaticClass(),Name);
        I->SetBrush(Art(ArtName));I->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Parent,I,X,Y,W,H);return I;
    }
    void Clear(UCanvasPanel* Parent)
    {
        TArray<UWidget*> Old;
        TFunction<void(UWidget*)> Collect=[&](UWidget* W)
        {if(auto* P=Cast<UPanelWidget>(W))for(auto* C:P->GetAllChildren()){Collect(C);Old.Add(C);}};
        Collect(Parent);Parent->ClearChildren();
        for(auto* W:Old)W->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);
    }
    void FitAvatar(UImage* Image,const FVector4& Rect)
    {
        if(!Image)return;
        auto Source=Image->GetBrush().ImageSize;
        if(const auto* Texture=Cast<UTexture>(Image->GetBrush().GetResourceObject()))
        {
            Source=FVector2D(Texture->GetSurfaceWidth(),Texture->GetSurfaceHeight());
            const FBox2f UV=Image->GetBrush().GetUVRegion();
            if(UV.bIsValid)Source*=FVector2D(UV.GetSize());
        }
        if(Source.X<=0||Source.Y<=0)return;
        const double Scale=FMath::Min(Rect.Z/Source.X,Rect.W/Source.Y);
        const auto Size=Source*Scale;
        if(auto* Slot=Cast<UCanvasPanelSlot>(Image->Slot))Slot->SetOffsets(FMargin(Rect.X+(Rect.Z-Size.X)*.5,Rect.Y+(Rect.W-Size.Y)*.5,Size.X,Size.Y));
    }
    void LayoutRow(UWidgetTree* Tree,const FString& Prefix,int32 Rank,const InkLeaderboard::FRowLayout& Initial,bool Compact,FName Category,bool Decorated)
    {
        auto R=Initial;if(Decorated)InkLeaderboard::AlignTierContent(R,Rank,Category,Compact);
        auto Named=[&](const TCHAR* Part){return FName(*(Prefix+Part+TEXT("_")+FString::FromInt(Rank)));};
        auto Move=[&](const TCHAR* Part,const FVector4& Rect)
        {
            if(auto* Widget=Tree->FindWidget(Named(Part)))if(auto* Slot=Cast<UCanvasPanelSlot>(Widget->Slot))
                Slot->SetOffsets(FMargin(Rect.X,Rect.Y,Rect.Z,Rect.W));
        };
        if(auto* Sizing=Find<USizeBox>(Tree,Named(TEXT("Row")))){Sizing->SetWidthOverride(R.Width);Sizing->SetHeightOverride(R.Height);}
        const FVector4 Background(0,1,R.Width,R.Height-2);
        Move(TEXT("PrestigeEffect"),Background);
        Move(TEXT("Name"),R.Name);Move(TEXT("Metric"),R.Metric);Move(TEXT("Number"),R.Number);
        Move(TEXT("Avatar"),R.Avatar);Move(TEXT("AvatarPlaceholder"),R.Avatar);Move(TEXT("Medal"),R.Medal);
        FitAvatar(Find<UImage>(Tree,Named(TEXT("Avatar"))),R.Avatar);
        for(const TCHAR* Part:{TEXT("Title"),TEXT("TitleText"),TEXT("TitleEffect")})Move(Part,R.Title);
        Move(TEXT("Realm"),R.Realm);Move(TEXT("RealmEffect"),R.Realm);
        // Canvas slots allocate geometry but do not constrain a child's paint by default.
        for(const TCHAR* Part:{TEXT("Name"),TEXT("Number"),TEXT("Metric"),TEXT("AvatarPlaceholder"),TEXT("Avatar"),TEXT("Medal"),TEXT("Title"),TEXT("TitleText"),TEXT("Realm")})
            if(auto* Widget=Tree->FindWidget(Named(Part)))Widget->SetClipping(EWidgetClipping::ClipToBoundsAlways);
        for(const TCHAR* Part:{TEXT("Name"),TEXT("Number"),TEXT("Metric"),TEXT("AvatarPlaceholder")})
        {
            if(auto* Text=Find<UTextBlock>(Tree,Named(Part)))
            {
                auto Font=Text->GetFont();Font.Size=Compact?(FString(Part)==TEXT("Number")?12:10):18;
                if(Compact||FString(Part)==TEXT("Number")){Font.OutlineSettings.OutlineSize=1;Font.OutlineSettings.OutlineColor=FLinearColor(.97f,.95f,.88f,.95f);}
                const bool Metric=FString(Part)==TEXT("Metric");
                const auto Rect=Metric?R.Metric:FString(Part)==TEXT("Name")?R.Name:FString(Part)==TEXT("Number")?R.Number:R.Avatar;
                if(FSlateApplication::IsInitialized())
                {
                    const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
                    while(Font.Size>8)
                    {
                        const auto Measured=Measure->Measure(Text->GetText(),Font);
                        if(Measured.Y<=Rect.W-2&&(!Metric||Measured.X<=Rect.Z-4))break;
                        --Font.Size;
                    }
                }
                Text->SetFont(Font);Text->SetAutoWrapText(false);Text->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
                if(Metric)Text->SetJustification(ETextJustify::Right);
                else if(FString(Part)==TEXT("Name"))Text->SetJustification(ETextJustify::Left);
            }
        }
        for(const TCHAR* Part:{TEXT("TitleEffect"),TEXT("RealmEffect"),TEXT("PrestigeEffect")})
            if(auto* FX=Tree->FindWidget(Named(Part)))FX->SetClipping(EWidgetClipping::ClipToBoundsAlways);
    }
    bool KnownTitle(FName Title)
    {
        for(const TCHAR* Id:{TEXT("LeftGuardian"),TEXT("RightGuardian"),TEXT("Novice"),TEXT("Godslayer"),TEXT("SoulEater"),TEXT("SevenStar"),TEXT("DivineBlade"),TEXT("Cyberspace")})
            if(Title==Id)return true;
        return false;
    }
}

void UInkFrontendWidget::RebuildLeaderboardSurfaces(UWidgetTree* Tree)
{
    using namespace InkRankUI;
    if(!Tree)return;
    auto* Page=Find<UCanvasPanel>(Tree,TEXT("Leaderboard"));
    auto* Safe=Find<UCanvasPanel>(Tree,TEXT("SafeCanvas"));
    if(!Page||!Safe)return;
    Clear(Page);
    if(auto* OldEntry=Tree->FindWidget(TEXT("HUDRankView"))){OldEntry->RemoveFromParent();OldEntry->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);}
    if(auto* Old=Find<UScaleBox>(Tree,TEXT("RankHUDScale")))
    {
        if(auto* Shell=Find<UCanvasPanel>(Tree,TEXT("RankHUDShell")))Clear(Shell);
        // Rename the entire old hierarchy before constructing widgets with the same stable names.
        TArray<UWidget*> Parts;
        TFunction<void(UWidget*)> Collect=[&](UWidget* W){if(auto* P=Cast<UPanelWidget>(W))for(auto* C:P->GetAllChildren()){Collect(C);Parts.Add(C);}};
        Collect(Old);Old->RemoveFromParent();
        for(auto* W:Parts)W->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);
        Old->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);
    }
    auto Build=[&](UCanvasPanel* Parent,bool Compact)
    {
        const FString Prefix=Compact?TEXT("RankHUD"):TEXT("RankAnchor");
        const float Width=Compact?CompactWidth:880.f,Height=Compact?CompactHeight:800.f;
        auto* Scale=Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(),*(Prefix+TEXT("Scale")));
        Scale->SetStretch(EStretch::ScaleToFit);Scale->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        auto* Layout=Parent->AddChildToCanvas(Scale);
        if(Compact)
        {
            Layout->SetAnchors(FAnchors(1,.5f));Layout->SetAlignment(FVector2D(1,.5f));
            Layout->SetOffsets(FMargin(-20,0,Width,Height));Layout->SetZOrder(15);
            Scale->SetVisibility(ESlateVisibility::Collapsed);Scale->SetRenderOpacity(0);
        }
        else{Layout->SetAnchors(FAnchors(0,0,1,1));Layout->SetOffsets(FMargin(40));}
        auto* Size=Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),*(Prefix+TEXT("Size")));
        Size->SetWidthOverride(Width);Size->SetHeightOverride(Height);Scale->SetContent(Size);
        auto* Shell=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),*(Prefix+TEXT("Shell")));
        Shell->SetVisibility(ESlateVisibility::SelfHitTestInvisible);Size->SetContent(Shell);
        auto* Frame=Picture(Tree,Shell,*(Prefix+TEXT("Frame")),Compact?TEXT("CompactFrame"):TEXT("AnchorFrame"),0,0,Width,Height);
        Frame->SetBrush(InkPresentationStyle::LeaderboardBackground(FVector2D(Width,Height)));CastChecked<UCanvasPanelSlot>(Frame->Slot)->SetZOrder(0);Shell->SetClipping(EWidgetClipping::ClipToBoundsAlways);
        Picture(Tree,Shell,*(Prefix+TEXT("Heading")),Compact?TEXT("SpeedHeading"):TEXT("AnchorHeading"),Compact?24:230,8,Compact?250:420,Compact?62:78);
        const float Top=Compact?100:266,RowsHeight=Compact?CompactRowsHeight:InkLeaderboard::FullRowLayout(false).Height*InkLeaderboard::AnchorVisibleRows;
        if(!Compact)
        {
            Picture(Tree,Shell,TEXT("RankAnchorColumns"),TEXT("AnchorColumns"),44,82,774,48);
            auto* Close=Tree->ConstructWidget<UInkFrontendActionButton>(UInkFrontendActionButton::StaticClass(),TEXT("RankClose"));
            Close->Action=TEXT("RankClose");auto Style=InkPresentationStyle::CloseButton();
            Style.SetNormalPadding(FMargin(0));Style.SetPressedPadding(FMargin(0));Close->SetStyle(Style);
            Place(Shell,Close,778,28,64,64);Close->BindFrontend(this);
            auto* Cross=AddText(Tree,nullptr,FText::FromString(TEXT("×")),36);Cross->SetAutoWrapText(false);Cross->SetJustification(ETextJustify::Center);Close->SetContent(Cross);
            CastChecked<UButtonSlot>(Cross->Slot)->SetVerticalAlignment(VAlign_Center);
        }
        auto* Scroll=Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),*(Prefix+TEXT("Scroll")));
        Scroll->SetScrollBarVisibility(ESlateVisibility::Collapsed);Scroll->SetAllowOverscroll(false);
        Scroll->SetAnimateWheelScrolling(true);Scroll->SetClipping(EWidgetClipping::ClipToBoundsAlways);
        Place(Shell,Scroll,Compact?8:32,Top,Compact?324:800,RowsHeight);
        auto* List=Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),*(Prefix+TEXT("List")));Scroll->AddChild(List);
        Picture(Tree,Shell,*(Prefix+TEXT("Rail")),TEXT("Rail"),Compact?340:829,Top,9,RowsHeight);
        auto* Bar=Tree->ConstructWidget<UInkInventoryScrollBar>(UInkInventoryScrollBar::StaticClass(),*(Prefix+TEXT("Thumb")));
        Bar->TargetScrollBox=Scroll;
        float ContentHeight=0;for(int32 Rank=1;Rank<=InkLeaderboard::RowCount;++Rank)ContentHeight+=(Compact?InkLeaderboard::CompactRowLayout(Rank):InkLeaderboard::FullRowLayout(false,Rank)).Height;
        Bar->ThumbFraction=RowsHeight/ContentHeight;
        FScrollBarStyle Style;FSlateBrush Empty;Empty.DrawAs=ESlateBrushDrawType::NoDrawType;
        Style.SetVerticalBackgroundImage(Empty).SetHorizontalBackgroundImage(Empty).SetVerticalTopSlotImage(Empty).SetVerticalBottomSlotImage(Empty);
        Style.SetNormalThumbImage(Art(TEXT("Thumb"))).SetHoveredThumbImage(Art(TEXT("Thumb"))).SetDraggedThumbImage(Art(TEXT("Thumb")));
        Bar->SetWidgetStyle(Style);Bar->SetThickness(FVector2D(14,14));Bar->SetPadding(FMargin(0));
        Place(Shell,Bar,Compact?337:826,Top,15,RowsHeight);
        auto* Foot=AddText(Tree,nullptr,FText::FromString(TEXT("共 30 名")),Compact?12:16,*(Prefix+TEXT("Count")));
        Foot->SetJustification(ETextJustify::Center);Foot->SetVisibility(ESlateVisibility::HitTestInvisible);
        Place(Shell,Foot,30,Top+RowsHeight+7,Width-60,22);
        if(Compact)Foot->SetVisibility(ESlateVisibility::Collapsed);
    };
    Build(Page,false);Build(Safe,true);
    auto* View=MakeButton(Tree,TEXT("HUDRankView"),TEXT("HUDRankView"),FText::FromString(TEXT("查看排行")));
    FSlateBrush Clear;Clear.DrawAs=ESlateBrushDrawType::RoundedBox;Clear.TintColor=FLinearColor(.025f,.04f,.04f,.9f);Clear.OutlineSettings.CornerRadii=FVector4(10,10,10,10);Clear.OutlineSettings.RoundingType=ESlateBrushRoundingType::FixedRadius;Clear.OutlineSettings.Width=1;Clear.OutlineSettings.Color=InkPresentationStyle::Gold();
    auto Hover=Clear;Hover.DrawAs=ESlateBrushDrawType::RoundedBox;Hover.TintColor=FLinearColor(.03f,.16f,.22f,.12f);
    auto Pressed=Hover;Pressed.TintColor=FLinearColor(.15f,.1f,.02f,.25f);
    FButtonStyle PaperStyle;PaperStyle.SetNormal(Clear).SetHovered(Hover).SetPressed(Pressed).SetDisabled(Clear).SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0,2,0,-2));View->SetStyle(PaperStyle);
    auto* PaperCanvas=Tree->ConstructWidget<UCanvasPanel>();View->SetContent(PaperCanvas);PaperCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* ContentSlot=CastChecked<UButtonSlot>(PaperCanvas->Slot);ContentSlot->SetPadding(FMargin(0));ContentSlot->SetHorizontalAlignment(HAlign_Fill);ContentSlot->SetVerticalAlignment(VAlign_Fill);
    auto* Paper=Tree->ConstructWidget<UInkHomeArtWidget>(UInkHomeArtWidget::StaticClass(),TEXT("HUDRankPaper"));Paper->Configure(TEXT("home-rank-paper"));Paper->SetVisibility(ESlateVisibility::Collapsed);
    auto* PS=PaperCanvas->AddChildToCanvas(Paper);PS->SetAnchors(FAnchors(0,0,1,1));PS->SetOffsets(FMargin(0));
    auto* Caption=AddText(Tree,nullptr,FText::FromString(TEXT("排行\n›")),24,TEXT("HUDRankCaption"));Caption->SetColorAndOpacity(InkPresentationStyle::Paper());Caption->SetAutoWrapText(false);Caption->SetJustification(ETextJustify::Center);Caption->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* CS=PaperCanvas->AddChildToCanvas(Caption);CS->SetAnchors(FAnchors(.08f,.12f,.92f,.94f));CS->SetOffsets(FMargin(0));CS->SetZOrder(2);
    auto* Entry=Safe->AddChildToCanvas(View);Entry->SetAnchors(FAnchors(1,.5f));Entry->SetAlignment(FVector2D(1,.5f));Entry->SetOffsets(FMargin(-24,0,88,88));Entry->SetZOrder(16);View->BindFrontend(this);
    if(auto* Shell=Find<UCanvasPanel>(Tree,TEXT("RankHUDShell")))
    {
        Header(Find<UImage>(Tree,TEXT("RankHUDHeading")),TEXT("CultivationSpeed"),CompactWidth,8,42);
        const TCHAR* Ids[]={TEXT("HUDRankSpeed"),TEXT("HUDRankDamage"),TEXT("HUDRankClose")};
        const TCHAR* Captions[]={TEXT("修炼速度"),TEXT("伤害排行"),TEXT("收起")};
        for(int32 I=0;I<3;++I){auto* B=MakeButton(Tree,Ids[I],Ids[I],FText::FromString(Captions[I]));if(auto* Size=Cast<USizeBox>(B->GetContent())){Size->ClearWidthOverride();Size->ClearHeightOverride();Size->SetMinDesiredHeight(0);if(auto* T=Cast<UTextBlock>(Size->GetContent())){auto F=T->GetFont();F.Size=I==2?12:14;T->SetFont(F);}}auto S=InkPresentationStyle::Button();S.SetNormalPadding(FMargin(0));S.SetPressedPadding(FMargin(0));B->SetStyle(S);Place(Shell,B,8+I*114,57,108,32);B->BindFrontend(this);}
    }
    RefreshLeaderboard(Tree);
}

void UInkFrontendWidget::RefreshLeaderboardRows(UWidgetTree* Tree,FName Category,bool Compact)
{
    using namespace InkRankUI;
    const FString Prefix=Compact?TEXT("RankHUD"):TEXT("RankAnchor");
    auto* List=Find<UVerticalBox>(Tree,*(Prefix+TEXT("List")));if(!List)return;
    if(Compact)Header(Find<UImage>(Tree,TEXT("RankHUDHeading")),Category,CompactWidth,8,42);
    const auto& Source=(!Compact&&LeaderboardScope==TEXT("Global"))?GlobalLeaderboardEntries:LeaderboardEntries;
    for(const auto& E:InkLeaderboard::MakeRows(Source,Category))
    {
        const FString Suffix=FString::FromInt(E.Rank),Key=Prefix+TEXT("_")+Suffix;
        auto Named=[&](const TCHAR* Part){return FName(*(Prefix+Part+Suffix));};
        auto* Sizing=Find<USizeBox>(Tree,Named(TEXT("Row_")));
        if(!Sizing)
        {
            Sizing=Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),Named(TEXT("Row_")));
            const auto InitialLayout=Compact?InkLeaderboard::CompactRowLayout(E.Rank):InkLeaderboard::FullRowLayout(bLeaderboardPortrait,E.Rank,Category);
            Sizing->SetHeightOverride(InitialLayout.Height);Sizing->SetWidthOverride(InitialLayout.Width);
            Sizing->SetClipping(EWidgetClipping::ClipToBoundsAlways);List->AddChild(Sizing);
            auto* Row=Tree->ConstructWidget<UCanvasPanel>();Sizing->SetContent(Row);Row->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
            auto* RowArt=Picture(Tree,Row,Named(TEXT("RowArt_")),TEXT("PlainRow"),0,1,Compact?254:774,Compact?54:58);
            auto RowBrush=InkPresentationStyle::Surface(InkPresentationStyle::ESurface::Card,FVector2D(Compact?254:774,54));RowBrush.TintColor=FLinearColor(1,1,1,.4f);RowArt->SetBrush(RowBrush);CastChecked<UCanvasPanelSlot>(RowArt->Slot)->SetZOrder(0);
            if(E.Rank<=10)
            {
                auto* FX=Tree->ConstructWidget<UInkNiagaraTitleWidget>(UInkNiagaraTitleWidget::StaticClass(),Named(TEXT("PrestigeEffect_")));
                if(GetOwningPlayer())FX->SetOwningPlayer(GetOwningPlayer());Place(Row,FX,0,1,Compact?254:774,Compact?54:58)->SetZOrder(1);
            }
            if(E.Rank<=3){auto* Medal=Picture(Tree,Row,Named(TEXT("Medal_")),TEXT("Medal")+Suffix,Compact?4:58,Compact?7:3,Compact?38:52,Compact?38:52);Medal->SetBrush(InkPresentationStyle::NavigationIcon(0));}
            {auto* Rank=AddText(Tree,nullptr,FText::AsNumber(E.Rank),Compact?19:23,Named(TEXT("Number_")));Rank->SetJustification(ETextJustify::Center);Rank->SetColorAndOpacity(InkPresentationStyle::Ink());Place(Row,Rank,Compact?4:58,Compact?11:14,Compact?36:52,30);}
            auto Text=[&](const TCHAR* Part,int32 Font,float X,float Y,float W,float H)
            {auto* T=AddText(Tree,nullptr,FText::GetEmpty(),Font,Named(Part));T->SetAutoWrapText(false);T->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);T->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Row,T,X,Y,W,H);return T;};
            auto* Name=Text(TEXT("Name_"),Compact?15:21,Compact?67:205,Compact?4:2,Compact?91:242,Compact?20:28);Name->SetJustification(Compact?ETextJustify::Left:ETextJustify::Center);
            auto* Avatar=Tree->ConstructWidget<UImage>(UImage::StaticClass(),Named(TEXT("Avatar_")));Place(Row,Avatar,Compact?43:135,Compact?3:10,Compact?22:40,Compact?22:40);Avatar->SetVisibility(ESlateVisibility::Hidden);
            auto* Placeholder=Text(TEXT("AvatarPlaceholder_"),Compact?14:22,Compact?43:135,Compact?3:10,Compact?22:40,Compact?22:40);Placeholder->SetText(FText::FromString(TEXT("灵")));Placeholder->SetJustification(ETextJustify::Center);
            auto* Value=Text(TEXT("Metric_"),Compact?14:21,Compact?162:590,Compact?5:17,Compact?84:140,30);Value->SetJustification(Compact?ETextJustify::Right:ETextJustify::Center);
            Picture(Tree,Row,Named(TEXT("Title_")),TEXT("Title_Novice"),Compact?43:450,Compact?24:4,Compact?125:130,Compact?30:50);
            auto* Exact=Text(TEXT("TitleText_"),Compact?14:22,Compact?43:450,Compact?27:17,Compact?125:130,30);Exact->SetText(FText::FromString(TEXT("无机态那")));Exact->SetJustification(ETextJustify::Center);Exact->SetColorAndOpacity(FLinearColor(.50f,.34f,.67f,1));
            auto* Realm=Tree->ConstructWidget<UImage>(UImage::StaticClass(),Named(TEXT("Realm_")));Realm->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Row,Realm,Compact?175:205,Compact?32:32,Compact?70:242,Compact?16:23);
            for(bool IsRealm:{false,true})
            {
                auto* FX=Tree->ConstructWidget<UInkNiagaraTitleWidget>(UInkNiagaraTitleWidget::StaticClass(),Named(IsRealm?TEXT("RealmEffect_"):TEXT("TitleEffect_")));
                if(GetOwningPlayer())FX->SetOwningPlayer(GetOwningPlayer());
                Place(Row,FX,IsRealm?(Compact?172:205):(Compact?43:450),IsRealm?(Compact?30:32):(Compact?22:4),IsRealm?(Compact?76:242):(Compact?128:130),IsRealm?(Compact?23:23):(Compact?32:50));
            }
        }
        const auto* Previous=PresentedLeaderboardRows.Find(Key);
        const bool Empty=E.DisplayName.IsEmpty();
        const auto Layout=Compact?InkLeaderboard::CompactRowLayout(E.Rank):InkLeaderboard::FullRowLayout(bLeaderboardPortrait,E.Rank,Category);
        const FVector2D RowSize(Layout.Width,Layout.Height-2);
        RowBackground(Find<UImage>(Tree,Named(TEXT("RowArt_"))),Category,E.Rank,RowSize,Empty);
        if(auto* FX=Find<UInkNiagaraTitleWidget>(Tree,Named(TEXT("PrestigeEffect_"))))
        {
            FX->Configure(EInkNiagaraTitleKind::Honor,FText::GetEmpty(),InkLeaderboardTheme::Quality(E.Rank),FSlateBrush());
            FX->SetEffectStyle(InkLeaderboardTheme::Color(Category),InkLeaderboardTheme::Strength(E.Rank));FX->SetPresentationVisible(!Empty);
        }
        // Metric/name/realm changes retain the same image, brush and async request.
        const bool SameAvatar=Previous&&Previous->ParticipantId==E.ParticipantId&&Previous->AvatarUrl==E.AvatarUrl&&Previous->AvatarTexture==E.AvatarTexture&&Previous->Category==E.Category;
        auto* Avatar=Find<UImage>(Tree,Named(TEXT("Avatar_")));
        if(!SameAvatar){Avatar->SetVisibility(ESlateVisibility::Hidden);Avatar->SetBrush(FSlateBrush());}
        if(auto* Placeholder=Find<UTextBlock>(Tree,Named(TEXT("AvatarPlaceholder_"))))Placeholder->SetVisibility(Empty||(SameAvatar&&Avatar->GetVisibility()!=ESlateVisibility::Hidden)?ESlateVisibility::Hidden:ESlateVisibility::HitTestInvisible);
        if(auto* Name=Find<UTextBlock>(Tree,Named(TEXT("Name_"))))Name->SetText(Empty?FText::FromString(TEXT("虚位以待")):E.DisplayName);
        if(auto* Value=Find<UTextBlock>(Tree,Named(TEXT("Metric_"))))Value->SetText(E.Metric.IsEmpty()?FText::FromString(Empty?TEXT("—"):FString::Printf(TEXT("%.2f"),E.NumericMetric)):E.Metric);
        if(auto* Title=Find<UImage>(Tree,Named(TEXT("Title_")))){Title->SetBrush(Art(TEXT("Title_")+(KnownTitle(E.TitleId)?E.TitleId.ToString():TEXT("Novice"))));Title->SetVisibility(!Empty&&KnownTitle(E.TitleId)&&E.TitleId!=TEXT("Cyberspace")?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);}
        if(auto* Exact=Find<UTextBlock>(Tree,Named(TEXT("TitleText_"))))Exact->SetVisibility(!Empty&&E.TitleId==TEXT("Cyberspace")?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);
        if(auto* Realm=Find<UImage>(Tree,Named(TEXT("Realm_"))))
        {
            FSlateBrush B;B.DrawAs=ESlateBrushDrawType::Image;B.SetResourceObject(InkUiAssets::Load<UTexture2D>(nullptr,TEXT("/Game/UI/Ink/Textures/Shared/T_RealmBanners.T_RealmBanners")));
            const auto UV=CultivationNameplate::BannerUV(E.RealmIndex);B.SetUVRegion(FBox2f(FVector2f(UV.Min),FVector2f(UV.Max)));Realm->SetBrush(B);
            Realm->SetVisibility(!Empty&&E.RealmIndex>=0&&E.RealmIndex<34?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);Realm->SetToolTipText(E.Realm);
        }
        for(const TCHAR* Part:{TEXT("Title_"),TEXT("TitleText_"),TEXT("Realm_")})if(auto* Old=Tree->FindWidget(Named(Part)))Old->SetVisibility(ESlateVisibility::Hidden);
        if(auto* FX=Find<UInkNiagaraTitleWidget>(Tree,Named(TEXT("TitleEffect_"))))
        {
            FX->Configure(EInkNiagaraTitleKind::Honor,CultivationRules::TitleName(E.TitleId),InkNiagaraTitlePresentation::QualityForTitleId(E.TitleId),InkNiagaraTitlePresentation::HonorBrush(E.TitleId));
            FX->SetReducedMotion(bReducedMotion);FX->SetPresentationVisible(!Empty&&KnownTitle(E.TitleId));
        }
        if(auto* FX=Find<UInkNiagaraTitleWidget>(Tree,Named(TEXT("RealmEffect_"))))
        {
            FX->Configure(EInkNiagaraTitleKind::Realm,E.Realm,InkNiagaraTitlePresentation::QualityForRealmIndex(E.RealmIndex),InkNiagaraTitlePresentation::RealmBrush(E.RealmIndex));
            FX->SetReducedMotion(bReducedMotion);FX->SetPresentationVisible(!Empty&&E.RealmIndex>=0&&E.RealmIndex<34);
        }
        LayoutRow(Tree,Prefix,E.Rank,Layout,Compact,Category,!Empty);
        PresentedLeaderboardRows.Add(Key,E);
    }
}

void UInkFrontendWidget::RefreshLeaderboard(UWidgetTree* SourceTree)
{
    auto* Tree=SourceTree?SourceTree:WidgetTree.Get();if(!Tree)return;
    RefreshLeaderboardRows(Tree,LeaderboardCategory,false);
    InkRankUI::Header(InkRankUI::Find<UImage>(Tree,TEXT("RankAnchorHeading")),LeaderboardCategory,bLeaderboardPortrait?720:880,18,80);
    RefreshLeaderboardRows(Tree,ActiveGameplayLeaderboard.IsNone()?FName(TEXT("CultivationSpeed")):ActiveGameplayLeaderboard,true);
    if(!SourceTree)
    {
        EnsureLeaderboardControls();UpdateLeaderboardAvatars();
        if(auto* Metric=Cast<UTextBlock>(GetWidgetFromName(TEXT("RankAnchorColumnMetric"))))Metric->SetText(FText::FromString(LeaderboardCategory==TEXT("Anchor")?TEXT("剑灵数"):LeaderboardCategory==TEXT("Damage")?TEXT("60秒 DPS"):TEXT("修为 / 秒")));
        for(const TCHAR* Id:{TEXT("RankLocal"),TEXT("RankGlobal"),TEXT("RankAnchor"),TEXT("RankCultivation"),TEXT("RankDamage")})
            if(auto* B=Cast<UInkFrontendActionButton>(GetWidgetFromName(Id)))
            {
                const FName A(Id);const bool Selected=(A==TEXT("RankLocal")&&LeaderboardScope==TEXT("Local"))||(A==TEXT("RankGlobal")&&LeaderboardScope==TEXT("Global"))||(A==TEXT("RankAnchor")&&LeaderboardCategory==TEXT("Anchor"))||(A==TEXT("RankCultivation")&&LeaderboardCategory==TEXT("CultivationSpeed"))||(A==TEXT("RankDamage")&&LeaderboardCategory==TEXT("Damage"));
                auto Style=InkPresentationStyle::Button(Selected);Style.SetNormalPadding(FMargin(0));Style.SetPressedPadding(FMargin(0));B->SetStyle(Style);
                if(auto* Size=Cast<USizeBox>(B->GetContent()))if(auto* Label=Cast<UTextBlock>(Size->GetContent()))
                {Label->SetColorAndOpacity(Selected?FLinearColor(.98f,.94f,.8f,1):InkPresentationStyle::Ink());Label->SetFont(InkPresentationStyle::ButtonFont(Label->GetFont(),Selected));}
            }
        if(auto* Status=Cast<UTextBlock>(GetWidgetFromName(TEXT("RankProviderStatus"))))
            Status->SetText(FText::FromString(LeaderboardScope==TEXT("Global")?(bGlobalLeaderboardConnected?TEXT("全服排行"):TEXT("全服排行暂未连接")):TEXT("本房排行 · 伤害按近60秒统计")));
        UpdateLeaderboardAdaptiveLayout(true);
    }
}

void UInkFrontendWidget::UpdateLeaderboardAdaptiveLayout(bool bForce)
{
    using namespace InkRankUI;
    auto* Safe=GetWidgetFromName(TEXT("SafeCanvas"));auto* Shell=Find<UCanvasPanel>(WidgetTree,TEXT("RankAnchorShell"));if(!Safe||!Shell)return;
    const auto View=Safe->GetCachedGeometry().GetLocalSize();if(View.X<=0||View.Y<=0)return;
    const bool Portrait=View.X<View.Y;
    if(!bForce&&bLeaderboardLayoutInitialized&&bLeaderboardPortrait==Portrait)return;
    bLeaderboardPortrait=Portrait;bLeaderboardLayoutInitialized=true;
    const float Width=Portrait?720:880,Height=Portrait?1220:800;
    Shell->SetClipping(EWidgetClipping::ClipToBoundsAlways);
    if(auto* Size=Find<USizeBox>(WidgetTree,TEXT("RankAnchorSize"))){Size->SetWidthOverride(Width);Size->SetHeightOverride(Height);}
    auto Move=[&](FName Name,float X,float Y,float W,float H){if(auto* Widget=GetWidgetFromName(Name))if(auto* LayoutSlot=Cast<UCanvasPanelSlot>(Widget->Slot))LayoutSlot->SetOffsets(FMargin(X,Y,W,H));};
    if(auto* Frame=Find<UImage>(WidgetTree,TEXT("RankAnchorFrame")))
    {
        Frame->SetBrush(InkPresentationStyle::LeaderboardBackground(FVector2D(Width,Height)));
    }
    Move(TEXT("RankAnchorFrame"),0,0,Width,Height);
    Move(TEXT("RankAnchorInkWash"),2,2,Width-4,Height-4);
    Header(Find<UImage>(WidgetTree,TEXT("RankAnchorHeading")),LeaderboardCategory,Width,18,80);
    auto* Title=Find<UTextBlock>(WidgetTree,TEXT("RankAnchorTitleText"));
    if(!Title){Title=AddText(WidgetTree,nullptr,FText::GetEmpty(),34,TEXT("RankAnchorTitleText"));Title->SetJustification(ETextJustify::Center);Title->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Shell,Title,0,0,1,1);}
    Title->SetText(FText::FromString(LeaderboardCategory==TEXT("Damage")?TEXT("伤害排行"):LeaderboardCategory==TEXT("CultivationSpeed")?TEXT("修炼排行"):TEXT("剑灵排行")));
    Move(TEXT("RankAnchorTitleText"),54,38,Width-108,48);Title->SetJustification(ETextJustify::Center);Title->SetRenderOpacity(0);
    Move(TEXT("RankClose"),Width-102,28,64,64);
    if(auto* Close=Find<UInkFrontendActionButton>(WidgetTree,TEXT("RankClose")))
    {
        Close->SetStyle(InkPresentationStyle::CloseButton());
        if(auto* Cross=Cast<UTextBlock>(Close->GetContent()))
        {auto Font=Cross->GetFont();Font.Size=36;Cross->SetFont(Font);Cross->SetAutoWrapText(false);CastChecked<UButtonSlot>(Cross->Slot)->SetVerticalAlignment(VAlign_Center);}
    }
    Move(TEXT("RankLocal"),Portrait?116:284,Portrait?130:112,Portrait?232:138,Portrait?54:42);
    Move(TEXT("RankGlobal"),Portrait?372:442,Portrait?130:112,Portrait?232:138,Portrait?54:42);
    const TCHAR* Categories[]={TEXT("RankAnchor"),TEXT("RankCultivation"),TEXT("RankDamage")};
    for(int32 I=0;I<3;++I)Move(Categories[I],Portrait?52+I*206:80+I*242,Portrait?202:172,Portrait?200:218,Portrait?54:42);
    for(const TCHAR* Id:{TEXT("RankLocal"),TEXT("RankGlobal"),TEXT("RankAnchor"),TEXT("RankCultivation"),TEXT("RankDamage")})
        if(auto* B=Find<UInkFrontendActionButton>(WidgetTree,Id))if(auto* Size=Cast<USizeBox>(B->GetContent()))
        {Size->ClearWidthOverride();Size->ClearHeightOverride();Size->SetMinDesiredHeight(0);if(auto* Label=Cast<UTextBlock>(Size->GetContent())){auto Font=Label->GetFont();Font.Size=Portrait?24:22;Label->SetFont(Font);Label->SetAutoWrapText(false);}}
    const auto R=InkLeaderboard::FullRowLayout(Portrait);
    const float ListX=Portrait?40:30,ColumnY=Portrait?270:228;
    Move(TEXT("RankAnchorColumnRank"),ListX+R.Number.X-16,ColumnY,64,30);
    Move(TEXT("RankAnchorColumnName"),ListX+R.Name.X,ColumnY,R.Name.Z,30);
    Move(TEXT("RankAnchorColumnTitle"),ListX+R.Title.X,ColumnY,R.Title.Z,30);
    if(auto* Column=GetWidgetFromName(TEXT("RankAnchorColumnTitle")))Column->SetVisibility(ESlateVisibility::Collapsed);
    Move(TEXT("RankAnchorColumnMetric"),ListX+R.Metric.X,ColumnY,R.Metric.Z,30);
    const float ListY=Portrait?312:266,ListH=Portrait?720:450,ListW=R.Width;
    Move(TEXT("RankAnchorScroll"),ListX,ListY,ListW,ListH);
    Move(TEXT("RankAnchorRail"),Portrait?668:842,ListY,9,ListH);Move(TEXT("RankAnchorThumb"),Portrait?665:839,ListY,15,ListH);
    Move(TEXT("RankAnchorCount"),30,Portrait?1060:760,Width-60,26);
    Move(TEXT("RankProviderStatus"),Portrait?80:60,Portrait?1100:726,Width-(Portrait?160:124),32);
    if(auto* Status=Find<UTextBlock>(WidgetTree,TEXT("RankProviderStatus"))){Status->SetAutoWrapText(false);Status->SetJustification(ETextJustify::Center);auto Font=Status->GetFont();Font.Size=16;Status->SetFont(Font);}
    float ContentHeight=0;for(int32 Rank=1;Rank<=InkLeaderboard::RowCount;++Rank)ContentHeight+=InkLeaderboard::FullRowLayout(Portrait,Rank).Height;
    if(auto* Thumb=Find<UInkInventoryScrollBar>(WidgetTree,TEXT("RankAnchorThumb")))Thumb->ThumbFraction=ListH/ContentHeight;
    for(int32 Rank=1;Rank<=InkLeaderboard::RowCount;++Rank)
    {
        const auto RowLayout=InkLeaderboard::FullRowLayout(Portrait,Rank,LeaderboardCategory);
        const FString S=FString::FromInt(Rank);
        const auto* Entry=PresentedLeaderboardRows.Find(TEXT("RankAnchor_")+S);
        LayoutRow(WidgetTree,TEXT("RankAnchor"),Rank,RowLayout,false,LeaderboardCategory,Entry&&!Entry->DisplayName.IsEmpty());
        RowBackground(Find<UImage>(WidgetTree,*(TEXT("RankAnchorRowArt_")+S)),LeaderboardCategory,Rank,FVector2D(RowLayout.Width,RowLayout.Height-2),!Entry||Entry->DisplayName.IsEmpty());
    }
}

void UInkFrontendWidget::EnsureLeaderboardControls()
{
    if(IsDesignTime()||!WidgetTree)return;using namespace InkRankUI;
    auto* Shell=Find<UCanvasPanel>(WidgetTree,TEXT("RankAnchorShell"));if(!Shell||GetWidgetFromName(TEXT("RankGlobal")))return;
    if(auto* Size=Find<USizeBox>(WidgetTree,TEXT("RankAnchorSize")))Size->SetHeightOverride(580);
    auto Move=[&](FName Name,float X,float Y,float W,float H){if(auto* Widget=GetWidgetFromName(Name))if(auto* LayoutSlot=Cast<UCanvasPanelSlot>(Widget->Slot))LayoutSlot->SetOffsets(FMargin(X,Y,W,H));};
    Move(TEXT("RankAnchorFrame"),0,0,880,580);Move(TEXT("RankAnchorColumns"),44,155,774,42);
    if(auto* Old=GetWidgetFromName(TEXT("RankAnchorColumns")))Old->SetRenderOpacity(0);
    const TCHAR* ColumnIds[]={TEXT("RankAnchorColumnRank"),TEXT("RankAnchorColumnName"),TEXT("RankAnchorColumnTitle"),TEXT("RankAnchorColumnMetric")};
    const TCHAR* ColumnLabels[]={TEXT("名次"),TEXT("修士"),TEXT("称号"),TEXT("剑灵数")};
    const float ColumnX[]={86,235,504,624},ColumnWidths[]={84,270,110,160};
    for(int32 C=0;C<4;++C){auto* Label=AddText(WidgetTree,nullptr,FText::FromString(ColumnLabels[C]),21,ColumnIds[C]);Label->SetJustification(ETextJustify::Center);Place(Shell,Label,ColumnX[C],162,ColumnWidths[C],30);}
    Move(TEXT("RankAnchorScroll"),44,200,774,300);Move(TEXT("RankAnchorRail"),829,200,9,300);Move(TEXT("RankAnchorThumb"),826,200,15,300);Move(TEXT("RankAnchorCount"),30,540,820,24);
    const TCHAR* Actions[]={TEXT("RankLocal"),TEXT("RankGlobal"),TEXT("RankAnchor"),TEXT("RankCultivation"),TEXT("RankDamage")};
    const TCHAR* Labels[]={TEXT("本房"),TEXT("全服"),TEXT("剑灵数量"),TEXT("修炼速度"),TEXT("60秒 DPS")};
    for(int32 I=0;I<5;++I)
    {
        auto* B=MakeButton(WidgetTree,Actions[I],Actions[I],FText::FromString(Labels[I]));
        if(auto* Size=Cast<USizeBox>(B->GetContent())){Size->SetMinDesiredHeight(0);Size->SetHeightOverride(42);Size->SetWidthOverride(I<2?138:218);if(auto* Label=Cast<UTextBlock>(Size->GetContent())){auto Font=Label->GetFont();Font.Size=22;Label->SetFont(Font);}}
        auto Style=InkPresentationStyle::Button();Style.SetNormalPadding(FMargin(0));Style.SetPressedPadding(FMargin(0));B->SetStyle(Style);
        Place(Shell,B,I<2?float(284+I*158):float(80+(I-2)*242),I<2?78:115,I<2?138:218,42);B->BindFrontend(this);
    }
    auto* Status=AddText(WidgetTree,nullptr,FText::GetEmpty(),16,TEXT("RankProviderStatus"));Status->SetAutoWrapText(false);Status->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);Place(Shell,Status,60,510,760,24);
    BindTree();
}

void UInkFrontendWidget::UpdateLeaderboardAvatars()
{
    if(!WidgetTree||!GetWorld())return;ULiveAudienceAdapterComponent* Adapter=nullptr;
    TArray<FString> Expired;for(const auto& Pair:LeaderboardAvatarRequests)if(!Pair.Value||Pair.Value->HasExpired())Expired.Add(Pair.Key);
    for(const auto& Key:Expired){if(auto* Request=LeaderboardAvatarRequests.FindRef(Key).Get())Request->Cancel();CompleteLeaderboardAvatar(Key,nullptr);}
    for(TActorIterator<ALiveGMOverlay> It(GetWorld());It;++It)if(It->Audience){Adapter=It->Audience;break;}
    if(!Adapter)if(auto* Mode=GetWorld()->GetAuthGameMode<ALiveWeaponGameMode>())Adapter=Mode->Audience;
    int32 RequestBudget=2;
    for(bool Compact:{false,true})
    {
        const FString Prefix=Compact?TEXT("RankHUD"):TEXT("RankAnchor");
        if(Compact?GameplayLeaderboardOpacity<=0:CurrentPage!=TEXT("Leaderboard"))continue;
        const auto& Entries=(!Compact&&LeaderboardScope==TEXT("Global"))?GlobalLeaderboardEntries:LeaderboardEntries;
        auto* Scroll=Cast<UScrollBox>(GetWidgetFromName(FName(*(Prefix+TEXT("Scroll")))));if(!Scroll)continue;
        const auto ScrollGeometry=Scroll->GetCachedGeometry();
        for(const auto& E:InkLeaderboard::MakeRows(Entries,Compact?(ActiveGameplayLeaderboard.IsNone()?FName(TEXT("CultivationSpeed")):ActiveGameplayLeaderboard):LeaderboardCategory))
        {
            if(E.DisplayName.IsEmpty())continue;
            auto* I=Cast<UImage>(GetWidgetFromName(FName(*(Prefix+TEXT("Avatar_")+FString::FromInt(E.Rank)))));if(!I)continue;
            // Hidden avatar images have no painted geometry yet. Use the visible row
            // so the first texture can load instead of remaining permanently culled.
            const auto* Row=I->GetParent();if(!Row)continue;
            const FVector2D P=ScrollGeometry.AbsoluteToLocal(Row->GetCachedGeometry().GetAbsolutePosition());
            if(P.Y < -64||P.Y > ScrollGeometry.GetLocalSize().Y+64)continue;
            UTexture2D* Texture=E.AvatarTexture.Get();bool Ready=Texture!=nullptr;
            if(Texture)I->SetBrushFromTexture(Texture);
            else if(!E.AvatarTexture.IsNull()&&!RequestedLeaderboardAvatars.Contains(E.AvatarTexture.ToSoftObjectPath()))
            {
                const auto Path=E.AvatarTexture.ToSoftObjectPath();RequestedLeaderboardAvatars.Add(Path);
                UAssetManager::GetStreamableManager().RequestAsyncLoad(Path,FStreamableDelegate::CreateWeakLambda(this,[this,Path](){if(Path.ResolveObject())RequestedLeaderboardAvatars.Remove(Path);LeaderboardAvatarPoll=0;}));
            }
            if(!Ready&&Adapter&&(Compact||LeaderboardScope!=TEXT("Global")))
            {
                Adapter->RequestVisibleAvatar(E.ParticipantId);FVector2D UV;
                if(Adapter->GetAvatarUV(E.ParticipantId,UV))
                {
                    FSlateBrush Brush;Brush.DrawAs=ESlateBrushDrawType::Image;Brush.SetResourceObject(Adapter->GetAvatarAtlas());Brush.SetUVRegion(FBox2f(FVector2f(UV),FVector2f(UV+FVector2D(1.f/32))));I->SetBrush(Brush);Ready=true;
                }
            }
            const FString Scope=(!Compact&&LeaderboardScope==TEXT("Global"))?TEXT("Global"):TEXT("Local");
            const FString Key=Scope+TEXT("|")+E.ParticipantId+TEXT("|")+E.AvatarUrl;
            if(!Ready)if(auto* Cached=LeaderboardAvatarCache.FindRef(Key).Get()){I->SetBrushFromTexture(Cached);Ready=true;}
            if(!Ready&&RequestBudget>0&&LeaderboardAvatarRequests.Num()<6&&!E.ParticipantId.IsEmpty()&&!E.AvatarUrl.IsEmpty()&&
                E.AvatarUrl.StartsWith(TEXT("https://"))&&!LeaderboardAvatarRequests.Contains(Key)&&FPlatformTime::Seconds()>=LeaderboardAvatarRetryAt.FindRef(Key))
            {
                --RequestBudget;auto* Request=NewObject<UInkLeaderboardAvatarRequest>(this);LeaderboardAvatarRequests.Add(Key,Request);Request->Start(this,Key,E.AvatarUrl);
            }
            I->SetVisibility(Ready?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);
            if(Ready)
            {
                auto Layout=Compact?InkLeaderboard::CompactRowLayout(E.Rank):InkLeaderboard::FullRowLayout(bLeaderboardPortrait,E.Rank,E.Category);
                InkLeaderboard::AlignTierContent(Layout,E.Rank,E.Category,Compact);InkRankUI::FitAvatar(I,Layout.Avatar);
            }
            if(auto* Placeholder=GetWidgetFromName(FName(*(Prefix+TEXT("AvatarPlaceholder_")+FString::FromInt(E.Rank)))))Placeholder->SetVisibility(Ready?ESlateVisibility::Hidden:ESlateVisibility::HitTestInvisible);
        }
    }
}

void UInkFrontendWidget::CompleteLeaderboardAvatar(const FString& Key,UTexture2D* Texture)
{
    LeaderboardAvatarRequests.Remove(Key);
    if(Texture){if(LeaderboardAvatarCache.Num()>=90)LeaderboardAvatarCache.Reset();LeaderboardAvatarCache.Add(Key,Texture);}
    else LeaderboardAvatarRetryAt.Add(Key,FPlatformTime::Seconds()+60.);
    // The next bounded poll applies the result only if the same identity and URL are still visible.
    LeaderboardAvatarPoll=0;
}

void UInkFrontendWidget::UpdateGameplayLeaderboard(float DeltaSeconds)
{
    using namespace InkRankUI;
    auto* Scale=Find<UScaleBox>(WidgetTree,TEXT("RankHUDScale"));if(!Scale)return;
    auto* Safe=Find<UCanvasPanel>(WidgetTree,TEXT("SafeCanvas"));
    if(Safe)
    {
        const auto Size=Safe->GetCachedGeometry().GetLocalSize();
        if(Size.X>0&&Size.Y>0)
        {
            const bool Landscape=Size.X>Size.Y;
            const float W=FMath::Min(FMath::Clamp(CompactLeaderboardWidth,240.f,400.f),Size.X*(Landscape?.24f:.42f));
            const float Ratio=FMath::Min(W/CompactWidth,Size.Y*.80f/CompactHeight);
            // Only the collapsed HUD entry changes; the expanded leaderboard keeps its geometry.
            if(auto* Entry=Find<UInkFrontendActionButton>(WidgetTree,TEXT("HUDRankView")))
            {
                // Serialized frontend trees receive the same compact entry without rebuilding rank rows.
                if(auto* Caption=Find<UTextBlock>(WidgetTree,TEXT("HUDRankCaption"));Caption&&Caption->GetText().ToString()!=TEXT("排行\n›"))
                {
                    FSlateBrush InkTag;InkTag.DrawAs=ESlateBrushDrawType::RoundedBox;InkTag.TintColor=FLinearColor(.025f,.04f,.04f,.9f);
                    InkTag.OutlineSettings.CornerRadii=FVector4(10,10,10,10);InkTag.OutlineSettings.RoundingType=ESlateBrushRoundingType::FixedRadius;InkTag.OutlineSettings.Width=1;InkTag.OutlineSettings.Color=InkPresentationStyle::Gold();
                    auto TagHover=InkTag;TagHover.TintColor=FLinearColor(.03f,.16f,.22f,.92f);
                    auto TagPressed=InkTag;TagPressed.TintColor=FLinearColor(.23f,.16f,.06f,.95f);
                    auto TagStyle=Entry->GetStyle();TagStyle.SetNormal(InkTag).SetHovered(TagHover).SetPressed(TagPressed).SetDisabled(InkTag);Entry->SetStyle(TagStyle);
                    Caption->SetText(FText::FromString(TEXT("排行\n›")));Caption->SetFont(InkUiTypography::MakeFont(EInkUiTextRole::Body,24));Caption->SetColorAndOpacity(InkPresentationStyle::Paper());Caption->SetAutoWrapText(false);
                    if(auto* CaptionSlot=Cast<UCanvasPanelSlot>(Caption->Slot)){CaptionSlot->SetAnchors(FAnchors(.08f,.12f,.92f,.94f));CaptionSlot->SetOffsets(FMargin(0));}
                    if(auto* OldPaper=Find<UInkHomeArtWidget>(WidgetTree,TEXT("HUDRankPaper")))OldPaper->SetVisibility(ESlateVisibility::Collapsed);
                }
                const auto Hud=InkUiLayout::ResolveHud(Size,ExtraStreamBottomInset);
                const auto LastShortcut=Hud.TopActions.Last();
                if(auto* ES=Cast<UCanvasPanelSlot>(Entry->Slot))
                {
                    ES->SetAnchors(FAnchors(0));ES->SetAlignment(FVector2D(0));
                    ES->SetOffsets(FMargin(LastShortcut.Left,LastShortcut.Bottom+12,LastShortcut.GetSize().X,88));
                }
            }
            if(auto* LayoutSlot=Cast<UCanvasPanelSlot>(Scale->Slot))LayoutSlot->SetOffsets(FMargin(-FMath::Max(0.f,LeaderboardRightInset),0,CompactWidth*Ratio,CompactHeight*Ratio));
        }
    }
    APawn* Pawn=GetOwningPlayer()?GetOwningPlayer()->GetPawn():nullptr;
    const bool bHUD=CurrentPage==TEXT("HUD")&&Pawn;
    if(Pawn!=LeaderboardObservedPawn.Get())
    {
        LeaderboardObservedPawn=Pawn;LeaderboardFade={};LeaderboardActivityPoll=0;bGameplayLeaderboardRequested=false;
        bLeaderboardCultivating=false;bLeaderboardAttacking=false;
    }
    LeaderboardActivityPoll-=DeltaSeconds;
    if(bHUD&&LeaderboardActivityPoll<=0)
    {
        LeaderboardActivityPoll=.05f;bLeaderboardCultivating=false;bLeaderboardAttacking=false;
        if(auto* Action=Pawn->FindComponentByClass<UCultivationGameplayComponent>())
            bLeaderboardCultivating=Action->State==ECultivationActionState::MeditationStart||Action->State==ECultivationActionState::MeditationLoop;
        if(GetWorld())for(TActorIterator<ALiveWeaponSwarm> It(GetWorld());It;++It)
            if(It->GetAnchor()==Pawn&&It->HasActiveAttacks()){bLeaderboardAttacking=true;break;}
    }
    if(!bHUD){bLeaderboardCultivating=false;bLeaderboardAttacking=false;LeaderboardActivityPoll=0;}
    if(!bHUD)bGameplayLeaderboardRequested=false;
    LeaderboardFade.TickRequested(DeltaSeconds,bHUD,bGameplayLeaderboardRequested?RequestedGameplayLeaderboard:NAME_None,LeaderboardFadeInSeconds,LeaderboardFadeOutSeconds);
    if(auto* Entry=GetWidgetFromName(TEXT("HUDRankView")))Entry->SetVisibility(bHUD&&!bGameplayLeaderboardRequested&&LeaderboardFade.Progress<=0?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    // The right shortcut rail yields its space while the existing expanded rank overlay is visible.
    for(const TCHAR* Name:{TEXT("HUD_OpenActivities"),TEXT("HUD_OpenSignIn"),TEXT("HUD_OpenExtensions"),TEXT("HUD_OpenLaunchEvent"),TEXT("HUD_OpenWuling")})
        if(auto* Shortcut=GetWidgetFromName(Name))Shortcut->SetVisibility(bHUD&&!bGameplayLeaderboardRequested&&LeaderboardFade.Progress<=0?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
    if(ActiveGameplayLeaderboard!=LeaderboardFade.Category)
    {
        ActiveGameplayLeaderboard=LeaderboardFade.Category;
        if(!ActiveGameplayLeaderboard.IsNone())RefreshLeaderboardRows(WidgetTree,ActiveGameplayLeaderboard,true);
        if(auto* Scroll=Find<UScrollBox>(WidgetTree,TEXT("RankHUDScroll")))Scroll->SetScrollOffset(0);
    }
    GameplayLeaderboardOpacity=LeaderboardFade.Opacity();Scale->SetRenderOpacity(GameplayLeaderboardOpacity);
    Scale->SetVisibility(GameplayLeaderboardOpacity>0?ESlateVisibility::SelfHitTestInvisible:ESlateVisibility::Collapsed);
    Scale->SetRenderTranslation(FVector2D(10.f*(1.f-GameplayLeaderboardOpacity),0));
    for(const TCHAR* Name:{TEXT("RankHUDThumb"),TEXT("RankAnchorThumb")})
        if(auto* Bar=Find<UInkInventoryScrollBar>(WidgetTree,Name))Bar->SynchronizeScrollPosition();
}
