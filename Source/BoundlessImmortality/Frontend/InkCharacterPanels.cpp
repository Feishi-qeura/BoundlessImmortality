#include "Frontend/UI/InkUiAssetPaths.h"
#include "InkFeaturePanel.h"
#include "InkFrontendWidget.h"
#include "WulingFrontendData.h"
#include "InkAnchorStatusWidget.h"
#include "InkBufferedProgressWidget.h"
#include "InkCharacterCultivationWidget.h"
#include "InkAttributeRating.h"
#include "InkInventoryModel.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Engine/GameInstance.h"

using ESurface=InkPresentationStyle::ESurface;
namespace
{
void FitAttribute(UTextBlock* Text,int32 Maximum,float Width)
{
    auto Font=Text->GetFont();Font.Size=Maximum;
    if(FSlateApplication::IsInitialized()){const auto M=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();while(Font.Size>12&&M->Measure(Text->GetText(),Font).X>Width-4)--Font.Size;}
    Text->SetFont(Font);Text->SetAutoWrapText(false);
}
InkPresentationStyle::EIcon AttributeIcon(FName Id)
{
    using E=InkPresentationStyle::EIcon;
    if(Id==TEXT("Health"))return E::Health;if(Id==TEXT("Defense"))return E::Shield;
    if(Id==TEXT("Attack")||Id==TEXT("SpiritDamage")||Id==TEXT("Swordsmanship"))return E::Sword;
    if(Id==TEXT("Critical"))return E::Critical;if(Id==TEXT("Evasion")||Id==TEXT("MoveSpeed"))return E::Movement;
    if(Id==TEXT("Luck"))return E::Jade;if(Id==TEXT("RealmStone"))return E::Crystal;
    return E::Experience;
}
FString DisciplineName(UInkFrontendWidget* F,FName Category)
{if(F)for(const auto& D:F->GetPresentedCharacterDisciplines())if(D.Category==Category&&!D.Name.IsEmpty())return D.Name.ToString();return TEXT("未配置");}
bool PresentedAttribute(FName Id){return Id!=TEXT("Realm")&&Id!=TEXT("RealmStone");}
bool RatedAttribute(FName Id)
{return Id==TEXT("Health")||Id==TEXT("Attack")||Id==TEXT("Defense")||Id==TEXT("SpiritDamage")||Id==TEXT("Critical")||Id==TEXT("Evasion")||Id==TEXT("MoveSpeed")||Id==TEXT("CultivationSpeed");}
}

UInkCharacterProfilePanel::UInkCharacterProfilePanel(const FObjectInitializer& Init):Super(Init)
{
    NodeQualities.Init(0,8);
    for(int32 I=0;I<5;++I)DantianColors.Add(InkCharacterCultivation::StageColor(EInkDantianStage(I)));
    for(auto C:{FColor(142,156,150),FColor(65,162,118),FColor(68,153,231),FColor(172,98,224),FColor(230,151,56),FColor(222,80,75),FColor(238,191,74)})QualityColors.Add(FLinearColor::FromSRGBColor(C));
}
void UInkCharacterProfilePanel::BuildPanelContent()
{
    auto* State=GetGameInstance()?GetGameInstance()->GetSubsystem<UCultivationSubsystem>():nullptr;
    const auto Identity=State?State->GetParticipantSnapshot(State->GetAnchorId()):FCultivationParticipantSnapshot();
    const float GraphW=bPortraitLayout?BodyWidth-16:FMath::Min(BodyWidth*.52f,(BodyHeight-12)*624/670);
    const float S=GraphW/624;VisualScale=S;VisualSize=FVector2D(GraphW,666*S);
    const float ArtSize=384*S;const FVector2D ArtPosition((GraphW-ArtSize)/2,164*S);
    DantianPosition=ArtPosition+FVector2D(FMath::Clamp(DantianUV.X,0.,1.),FMath::Clamp(DantianUV.Y,0.,1.))*ArtSize;
    auto* Name=Label(BodyCanvas,TEXT("ProfileName"),Identity.DisplayName.IsEmpty()?TEXT("修行者"):Identity.DisplayName,GraphW*.08f,0,GraphW*.48f,40*S,FMath::Max(18,FMath::RoundToInt(26*S)),true);Name->SetAutoWrapText(false);Name->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
    FitAttribute(Name,FMath::Max(18,FMath::RoundToInt(26*S)),GraphW*.48f);Name->SetClipping(EWidgetClipping::ClipToBoundsAlways);
    auto* RealmText=Label(BodyCanvas,TEXT("ProfileRealm"),Identity.RealmName.ToString(),GraphW*.58f,4*S,GraphW*.38f,32*S,FMath::Max(14,FMath::RoundToInt(20*S)));
    FitAttribute(RealmText,FMath::Max(14,FMath::RoundToInt(20*S)),GraphW*.38f);RealmText->SetClipping(EWidgetClipping::ClipToBoundsAlways);
    auto* Art=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("ProfileMeditation"));
    Art->SetBrushFromTexture(InkUiAssets::Load<UTexture2D>(nullptr,TEXT("/Game/UI/Ink/Textures/Cultivation/T_InkMeditation.T_InkMeditation")));
    Art->SetVisibility(ESlateVisibility::HitTestInvisible);auto* ArtSlot=BodyCanvas->AddChildToCanvas(Art);ArtSlot->SetPosition(ArtPosition);ArtSlot->SetSize(FVector2D(ArtSize));
    auto BuildSlot=[&](const FString& Id,FName Name,const FString& Caption,const FString& Status,int32 Tile,float X,float Y,float W,float H,bool Unlocked)
    {
        auto* B=Action(BodyCanvas,Id,TEXT(""),X,Y,W,H);B->SetIsEnabled(Unlocked);
        auto Style=B->GetStyle();auto Brush=InkPresentationStyle::Surface(ESurface::Card,FVector2D(W,H));
        Brush.TintColor=FLinearColor(.92f,.87f,.75f,.75f);Brush.OutlineSettings.Width=.8f;Brush.OutlineSettings.Color=InkPresentationStyle::Gold();
        Style.SetNormal(Brush);auto Hover=Brush;Hover.TintColor=FLinearColor(.72f,.86f,.87f,.9f);Style.SetHovered(Hover);
        auto Pressed=Brush;Pressed.TintColor=FLinearColor(.93f,.73f,.35f,.9f);Style.SetPressed(Pressed);
        auto Disabled=Brush;Disabled.TintColor=FLinearColor(.68f,.66f,.60f,.55f);Style.SetDisabled(Disabled);B->SetStyle(Style);
        B->SetToolTipText(FText::FromString(Unlocked?Caption+TEXT(" · ")+Status:TEXT("功法未解锁")));
        auto* Icon=AtlasIcon(BodyCanvas,FName(*(Name.ToString()+TEXT("Icon"))),Tile,X+(W-44*S)/2,Y+5*S,44*S);
        if(!Unlocked)Icon->SetColorAndOpacity(FLinearColor(.18f,.19f,.18f,.55f));
        auto* T=Label(BodyCanvas,Name,Caption,X+3,Y+52*S,W-6,28*S,FMath::Max(11,FMath::RoundToInt(18*S)),true);FitAttribute(T,FMath::Max(11,FMath::RoundToInt(18*S)),W-6);
        CastChecked<UCanvasPanelSlot>(T->Slot)->SetSize(FVector2D(W-6,28*S));T->SetClipping(EWidgetClipping::ClipToBoundsAlways);
        auto* Hint=Label(BodyCanvas,FName(*(Name.ToString()+TEXT("State"))),Status,X+3,Y+80*S,W-6,22*S,FMath::Max(11,FMath::RoundToInt(14*S)),true);
        auto HintFont=Hint->GetFont();HintFont.Size=FMath::Max(10,FMath::RoundToInt(14*S));Hint->SetFont(HintFont);
        Hint->SetAutoWrapText(false);Hint->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);Hint->SetClipping(EWidgetClipping::ClipToBoundsAlways);
        CastChecked<UCanvasPanelSlot>(Hint->Slot)->SetSize(FVector2D(W-6,22*S));
        Hint->SetColorAndOpacity(Unlocked?FLinearColor(.26f,.34f,.30f,1):FLinearColor(.25f,.24f,.22f,1));
        CastChecked<UCanvasPanelSlot>(Icon->Slot)->SetZOrder(3);CastChecked<UCanvasPanelSlot>(T->Slot)->SetZOrder(4);CastChecked<UCanvasPanelSlot>(Hint->Slot)->SetZOrder(4);
        return B;
    };
    NodePositions.Reset();NodeSizes.Reset();
    for(int32 I=0;I<5;++I)
    {
        const float X=I*GraphW/5+12*S;
        BuildSlot(FString::Printf(TEXT("ProfileMethod_%d"),I),FName(*FString::Printf(TEXT("ProfileMethodName_%d"),I)),I==2?TEXT("主修功法"):TEXT("功法"),I==2?TEXT("已解锁"):TEXT("锁定"),9,X,52*S,100*S,104*S,I==2);
        NodePositions.Add(FVector2D(X+50*S,104*S));NodeSizes.Add(FVector2D(100,104)*S);
    }
    const TCHAR* EquipNames[]={TEXT("法袍"),TEXT("冠饰"),TEXT("步履"),TEXT("佩饰"),TEXT("法宝"),TEXT("道具使用")};
    for(int32 I=0;I<6;++I)
    {
        const bool Left=I%2==0;const float X=Left?6*S:GraphW-98*S,Y=(198+(I/2)*112)*S;
        const FString Id=I<4?FString::Printf(TEXT("ProfileEquipment_%d"),I):I==4?TEXT("ProfileFabao"):TEXT("ProfileItem");
        BuildSlot(Id,FName(*FString::Printf(TEXT("ProfileEquip_%d"),I)),EquipNames[I],TEXT("空槽"),I==5?10:I==4?15:5+I%3,X,Y,92*S,104*S,true);
    }
    const TCHAR* SkillIds[]={TEXT("ProfileLightness"),TEXT("ProfileInner"),TEXT("ProfileWushen")};
    const TCHAR* SkillNames[]={TEXT("轻功"),TEXT("内功"),TEXT("武神")};
    const int32 SkillIcons[]={7,5,6};
    for(int32 I=0;I<3;++I)
    {
        const float X=(106+I*150)*S;
        BuildSlot(SkillIds[I],FName(*FString::Printf(TEXT("ProfileDiscipline_%d"),I)),SkillNames[I],TEXT("未配置"),SkillIcons[I],X,560*S,112*S,104*S,true);
        NodePositions.Add(FVector2D(X+56*S,612*S));NodeSizes.Add(FVector2D(112,104)*S);
    }
    CultivationVisual=WidgetTree->ConstructWidget<UInkCharacterCultivationWidget>(UInkCharacterCultivationWidget::StaticClass(),TEXT("ProfileCultivationVisual"));
    CultivationVisual->SetVisibility(ESlateVisibility::HitTestInvisible);auto* VS=BodyCanvas->AddChildToCanvas(CultivationVisual);VS->SetSize(VisualSize);VS->SetZOrder(1);
    const int32 Columns=3;int32 Count=0;if(FrontendOwner)for(const auto& A:FrontendOwner->GetPresentedCharacterAttributes())if(PresentedAttribute(A.AttributeId))++Count;
    const float AX=bPortraitLayout?0:GraphW+34,AW=BodyWidth-AX,W=AW/Columns,HeadingY=bPortraitLayout?682*S:6,Top=HeadingY+42;
    AttributeWidth=W;
    const float RowHeight=FMath::Max(62.f,FMath::Min(76.f,(BodyHeight-Top-8)/FMath::Max(1,FMath::DivideAndRoundUp(Count,Columns))));
    Label(BodyCanvas,TEXT("ProfileAttributesHeading"),TEXT("基础属性"),AX,HeadingY,AW,32,24);
    int32 I=0;PresentedFields.Reset();
    if(FrontendOwner)for(const auto& Attribute:FrontendOwner->GetPresentedCharacterAttributes())
    {
        if(!PresentedAttribute(Attribute.AttributeId))continue;PresentedFields.Add(Attribute.AttributeId);
        const float X=AX+(I%Columns)*W,Y=Top+(I/Columns)*RowHeight;
        auto* IconImage=Icon(BodyCanvas,FName(*(TEXT("ProfileAttributeIcon_")+Attribute.AttributeId.ToString())),AttributeIcon(Attribute.AttributeId),X+4,Y+2,28);
        if(auto* Texture=Attribute.Icon.LoadSynchronous())IconImage->SetBrushFromTexture(Texture);
        auto* L=Label(BodyCanvas,FName(*(TEXT("ProfileLabel_")+Attribute.AttributeId.ToString())),Attribute.Label.ToString(),X+38,Y,W-44,25,17);FitAttribute(L,17,W-44);
        auto* Value=Label(BodyCanvas,FName(*(TEXT("ProfileValue_")+Attribute.AttributeId.ToString())),Attribute.Value.ToString(),X+38,Y+28,W-44,30,23);FitAttribute(Value,23,W-44);
        if(RatedAttribute(Attribute.AttributeId))
        {
            auto* Grade=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),FName(*(TEXT("ProfileRating_")+Attribute.AttributeId.ToString())));
            auto* Layout=BodyCanvas->AddChildToCanvas(Grade);Layout->SetPosition(FVector2D(X+2,Y+28));Layout->SetSize(FVector2D(30));Grade->SetVisibility(ESlateVisibility::Hidden);
        }
        ++I;
    }
    RefreshLiveData();
}
void UInkCharacterProfilePanel::RefreshLiveData()
{
    if(!FrontendOwner||!WidgetTree||!bSurfaceInitialized)return;TArray<FName> Fields;
    for(const auto& A:FrontendOwner->GetPresentedCharacterAttributes())if(PresentedAttribute(A.AttributeId))Fields.Add(A.AttributeId);
    if(Fields!=PresentedFields){RebuildPanel();return;}
    if(auto* Name=Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ProfileName")))){Name->SetText(FrontendOwner->GetPresentedCharacterName());FitAttribute(Name,FMath::Max(18,FMath::RoundToInt(26*VisualScale)),VisualSize.X*.48f);}
    auto SetLabel=[&](const FString& Key,const FString& Value){if(auto* T=Cast<UTextBlock>(WidgetTree->FindWidget(FName(*Key)))){T->SetText(FText::FromString(Value));auto* Layout=Cast<UCanvasPanelSlot>(T->Slot);FitAttribute(T,T->GetFont().Size,Layout?Layout->GetSize().X:92*VisualScale-6);}};
    auto* Wuling=WulingFrontendData::Service(this);const auto Loadout=Wuling&&Wuling->IsEnabled()?WulingFrontendData::Object(Wuling->Snapshot(),TEXT("loadout")):nullptr;
    const auto Equipment=WulingFrontendData::Object(Loadout,TEXT("equipment"));
    for(int32 I=0;I<5;++I)
    {
        const TCHAR* Slots[]={TEXT("robe"),TEXT("crown"),TEXT("boots"),TEXT("pendant")};
        FString Caption=TEXT("空槽");
        if(Loadout)Caption=WulingFrontendData::OwnedName(Wuling->Snapshot(),WulingFrontendData::String(I==4?Loadout:Equipment,I==4?TEXT("fabao"):Slots[I]));
        else if(auto* Inventory=FrontendOwner->GetInventoryModel())for(const auto& E:Inventory->GetEntriesView())if(E.bEquipped)
        {const FName SlotIds[]={TEXT("Armor"),TEXT("Head"),TEXT("Boots"),TEXT("Necklace")};if((I==4&&E.Item.Category==TEXT("Treasure"))||(I<4&&E.Item.EquipmentSlot==SlotIds[I])){Caption=E.Item.Name.ToString();break;}}
        SetLabel(FString::Printf(TEXT("ProfileEquip_%dState"),I),Caption);
    }
    const FString Main=WulingFrontendData::String(Loadout,TEXT("main"));
    const FString MainName=WulingFrontendData::String(WulingFrontendData::Object(Loadout,TEXT("method_names")),*Main);
    const FString SuppliedMain=DisciplineName(FrontendOwner,TEXT("Manual"));
    SetLabel(TEXT("ProfileMethodName_2"),!MainName.IsEmpty()?MainName:SuppliedMain!=TEXT("未配置")?SuppliedMain:TEXT("主修功法"));
    const FName Categories[]={TEXT("Movement"),TEXT("Inner"),TEXT("Martial")};
    for(int32 I=0;I<3;++I)SetLabel(FString::Printf(TEXT("ProfileDiscipline_%dState"),I),DisciplineName(FrontendOwner,Categories[I]));
    for(const auto& A:FrontendOwner->GetPresentedCharacterAttributes())
    {
        const FName Key=A.AttributeId==TEXT("Realm")?FName(TEXT("ProfileRealm")):FName(*(TEXT("ProfileValue_")+A.AttributeId.ToString()));
        if(auto* Value=Cast<UTextBlock>(WidgetTree->FindWidget(Key))){Value->SetText(A.Value);if(A.AttributeId!=TEXT("Realm"))FitAttribute(Value,23,AttributeWidth-44);}
    }
    auto* State=GetGameInstance()?GetGameInstance()->GetSubsystem<UCultivationSubsystem>():nullptr;
    const int32 Realm=bPreviewRealm?PreviewRealmIndex:State?State->GetParticipantSnapshot(State->GetAnchorId()).RealmIndex:0;
    FInkAttributeRatingContext Rating;Rating.RealmIndex=Realm;
    if(State){Rating.bV14Rules=State->IsWulingAuthorityEnabled();Rating.BaseAttributes=State->Rules.BaseAttributes;Rating.LegacyRealmGrowth=State->Rules.AttributeGrowthPerRealm;Rating.LegacyMeditationPerSecond=State->Rules.MeditationPerSecond;}
    if(RatingTextures.Num()!=7)
    {RatingTextures.Reset();for(int32 I=0;I<7;++I)RatingTextures.Add(InkUiAssets::Load<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/UI/Ink/CharacterRating20261022/T_Rating_%s.T_Rating_%s"),*UInkAttributeRatingLibrary::GradeName(EInkAttributeRating(I)),*UInkAttributeRatingLibrary::GradeName(EInkAttributeRating(I)))));}
    for(const auto& A:FrontendOwner->GetPresentedCharacterAttributes())if(auto* Image=Cast<UImage>(WidgetTree->FindWidget(FName(*(TEXT("ProfileRating_")+A.AttributeId.ToString())))))
    {
        auto Context=Rating;if(A.AttributeId==TEXT("MoveSpeed")&&A.RatingBaseValue>0)Context.BaseMoveSpeed=A.RatingBaseValue;
        const auto Grade=A.bHasNumericValue?UInkAttributeRatingLibrary::Evaluate(A.AttributeId,A.NumericValue,Context):EInkAttributeRating::Unrated;
        const bool Valid=int32(Grade)<7&&RatingTextures.IsValidIndex(int32(Grade))&&RatingTextures[int32(Grade)];
        Image->SetVisibility(Valid?ESlateVisibility::Visible:ESlateVisibility::Hidden);
        if(Valid)
        {
            Image->SetBrushFromTexture(RatingTextures[int32(Grade)]);
            const auto Bounds=UInkAttributeRatingLibrary::Thresholds(A.AttributeId,Context);
            FString Tip=FString::Printf(TEXT("%s · %s %s\n同一大境界采用相同标准"),*UInkAttributeRatingLibrary::MajorName(Realm),*A.Label.ToString(),*UInkAttributeRatingLibrary::GradeName(Grade));
            for(int32 I=0;I<Bounds.Num();++I)Tip+=FString::Printf(TEXT("\n%s ≥ %.4g"),*UInkAttributeRatingLibrary::GradeName(EInkAttributeRating(I)),Bounds[I]);
            Image->SetToolTipText(FText::FromString(Tip));
        }
    }
    const auto Stage=InkCharacterCultivation::StageForRealm(Realm);
    if(bPreviewRealm)SetLabel(TEXT("ProfileRealm"),FString::Printf(TEXT("%s · 预览"),State?*State->GetRealmName(Realm).ToString():TEXT("境界")));
    TArray<FInkCharacterSlotEffect> Nodes;
    for(int32 I=0;I<NodePositions.Num();++I)
    {
        FInkCharacterSlotEffect N;N.Position=NodePositions[I];N.FrameSize=NodeSizes[I];N.bUnlocked=I==2||I>=5;
        int32 Quality=0;bool Equipped=false;
        if(N.bUnlocked)
        {
            const FName Category=I==2?FName(TEXT("Manual")):Categories[I-5];
            for(const auto& D:FrontendOwner->GetPresentedCharacterDisciplines())if(D.Category==Category&&!D.Name.IsEmpty())
            {Quality=FMath::Clamp(D.QualityTier,0,6);Equipped=true;break;}
            if(I==2&&Loadout)
            {Quality=WulingFrontendData::Number(WulingFrontendData::Object(WulingFrontendData::Object(Loadout,TEXT("methods")),*Main),TEXT("level"));Equipped=!Main.IsEmpty();}
            if(bPreviewQualities&&NodeQualities.IsValidIndex(I)){Quality=NodeQualities[I];Equipped=Quality>0;}
        }
        N.QualityTier=FMath::Clamp(Quality,0,6);N.bEquipped=Equipped&&N.QualityTier>0;
        N.Color=QualityColors.IsValidIndex(N.QualityTier)?QualityColors[N.QualityTier]:InkPresentationStyle::Jade();Nodes.Add(N);
    }
    if(CultivationVisual)CultivationVisual->Configure(FrontendOwner,VisualSize,Nodes,DantianPosition,Realm,DantianColors.IsValidIndex(int32(Stage))?DantianColors[int32(Stage)]:InkCharacterCultivation::StageColor(Stage),SlotEffectSpeed,SlotParticleSize*VisualScale,DantianRadius*VisualScale);
}
void UInkCharacterProfilePanel::HandlePanelAction(const FString& Id)
{
    if(Id.StartsWith(TEXT("ProfileMethod_")))
    {if(Id!=TEXT("ProfileMethod_2"))return;Open(TEXT("CharacterSkills"));return;}
    if(Id==TEXT("ProfileLightness")||Id==TEXT("ProfileInner")||Id==TEXT("ProfileWushen"))
    {
        const FName Category=Id==TEXT("ProfileLightness")?TEXT("Movement"):Id==TEXT("ProfileInner")?TEXT("Inner"):TEXT("Martial");
        FString Details=DisciplineName(FrontendOwner,Category);if(FrontendOwner)for(const auto& D:FrontendOwner->GetPresentedCharacterDisciplines())if(D.Category==Category&&!D.Detail.IsEmpty())Details+=TEXT(" · ")+D.Detail.ToString();
        FeedbackMessage(Details==TEXT("未配置")?TEXT("尚未配置，可通过功法配置查看已学功法。"):Details);return;
    }
    if(Id.StartsWith(TEXT("ProfileEquipment_"))||Id==TEXT("ProfileFabao"))
    {if(FrontendOwner){FrontendOwner->ShowPage(TEXT("Inventory"));FrontendOwner->TriggerAction(Id==TEXT("ProfileFabao")?TEXT("FilterTreasure"):TEXT("FilterEquipment"));}return;}
    if(Id==TEXT("ProfileItem"))
    {if(FrontendOwner){FrontendOwner->ShowPage(TEXT("Inventory"));FrontendOwner->TriggerAction(TEXT("FilterConsumable"));}return;}
    Super::HandlePanelAction(Id);
}
void UInkFeaturePanel::BuildCharacterSection(const FString& Section)
{
    auto* Wuling=WulingFrontendData::Service(this);const bool Authority=Wuling&&Wuling->IsEnabled();
    const float SummaryW=bPortraitLayout?BodyWidth:(BodyWidth-20)*.36f,ListX=bPortraitLayout?0:SummaryW+20,ListW=BodyWidth-ListX;
    const float Top=bPortraitLayout?184:0,Footer=BodyHeight-88;
    Label(BodyCanvas,TEXT("CharacterSectionHeading"),Section+TEXT("配置"),0,0,SummaryW,52,34);
    Label(BodyCanvas,TEXT("CharacterSectionHint"),Authority?Wuling->GetStatus():Section==TEXT("装备")?TEXT("装备格预留；已有物品可在背包管理。"):TEXT("配置预览，尚未接入获得与升级。"),0,64,SummaryW,104,24);
    if(!bPortraitLayout)Scene(BodyCanvas,6,0,184,SummaryW,FMath::Max(100.f,Footer-196));
    TArray<TPair<FString,FString>> Entries;
    if(Authority)
    {
        const auto State=Wuling->Snapshot();const auto Loadout=WulingFrontendData::Object(State,TEXT("loadout"));
        auto Name=[&](const FString& Item){const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;if(State&&State->TryGetArrayField(TEXT("item_instances"),Rows))for(const auto& V:*Rows){auto O=V->AsObject();const TSharedPtr<FJsonObject>* Def=nullptr;if(WulingFrontendData::String(O,TEXT("item"))==Item&&O->TryGetObjectField(TEXT("definition"),Def))return WulingFrontendData::String(*Def,TEXT("name"));}return Item.IsEmpty()?FString(TEXT("空槽")):Item;};
        if(!Loadout)Entries.Emplace(TEXT("当前配置"),TEXT("等待服务同步装备与成长数据。"));
        else if(Section==TEXT("装备"))
        {
            const auto Equipment=WulingFrontendData::Object(Loadout,TEXT("equipment"));
            const TCHAR* Keys[]={TEXT("robe"),TEXT("crown"),TEXT("boots"),TEXT("pendant")};const TCHAR* Names[]={TEXT("法袍"),TEXT("冠饰"),TEXT("步履"),TEXT("佩饰")};
            for(int I=0;I<4;++I)Entries.Emplace(Names[I],Name(WulingFrontendData::String(Equipment,Keys[I])));
        }
        else if(Section==TEXT("法宝")){Entries.Emplace(TEXT("已装备法宝"),Name(WulingFrontendData::String(Loadout,TEXT("fabao"))));Entries.Emplace(TEXT("装备规则"),TEXT("同时只能装备一件；卸下不清除冷却。"));}
        else if(Section==TEXT("技能"))
        {
            const auto Names=WulingFrontendData::Object(Loadout,TEXT("method_names"));
            Entries.Emplace(TEXT("当前主修"),WulingFrontendData::String(Names,*WulingFrontendData::String(Loadout,TEXT("main"))));
            const TArray<TSharedPtr<FJsonValue>>* Supports=nullptr;if(Loadout->TryGetArrayField(TEXT("supports"),Supports))for(const auto& V:*Supports)Entries.Emplace(TEXT("辅修功法"),WulingFrontendData::String(Names,*V->AsString()));
            Entries.Emplace(TEXT("配置条件"),TEXT("功法等级与可配置槽位由服务确认。"));
        }
        else Entries.Emplace(TEXT("仙缘开放条件"),TEXT("当前规则档未开放额外仙缘配置。"));
    }
    else
    {
        const int32 Count=Section==TEXT("技能")?4:Section==TEXT("法宝")?3:6;
        for(int32 I=0;I<Count;++I)Entries.Emplace(FString::Printf(TEXT("%s槽位 %d"),*Section,I+1),TEXT("配置空槽 · 尚未开放"));
    }
    auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("CharacterSectionScroll"));Scroll->SetAllowOverscroll(false);Scroll->SetScrollbarThickness(FVector2D(6));
    auto* SectionScrollLayout=BodyCanvas->AddChildToCanvas(Scroll);SectionScrollLayout->SetPosition({ListX,Top});SectionScrollLayout->SetSize({ListW,FMath::Max(88.f,Footer-Top-12)});
    auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(ListW-10);Size->SetHeightOverride(Entries.Num()*144);Scroll->AddChild(Size);
    auto* SectionCanvas=WidgetTree->ConstructWidget<UCanvasPanel>();Size->SetContent(SectionCanvas);
    for(int32 I=0;I<Entries.Num();++I)
    {
        Label(SectionCanvas,NAME_None,Entries[I].Key,8,I*144,ListW-32,42,28);
        Label(SectionCanvas,I==0?FName(TEXT("CharacterAuthorityLoadout")):NAME_None,Entries[I].Value,8,I*144+48,ListW-32,88,26);
    }
    if(Authority)Action(BodyCanvas,Section==TEXT("技能")?TEXT("Open_WulingEquipment|methods"):TEXT("Open_WulingEquipment|equipment"),TEXT("管理装备、功法与法宝"),0,Footer,BodyWidth,88,true);
    else if(Section==TEXT("装备"))Action(BodyCanvas,TEXT("OriginalInventory"),TEXT("打开背包"),0,Footer,BodyWidth,88,true);
}
void UInkCharacterEquipmentPanel::BuildPanelContent(){BuildCharacterSection(TEXT("装备"));}
void UInkCharacterSkillsPanel::BuildPanelContent(){BuildCharacterSection(TEXT("技能"));}
void UInkCharacterArtifactsPanel::BuildPanelContent(){BuildCharacterSection(TEXT("法宝"));}
void UInkCharacterFatePanel::BuildPanelContent(){BuildCharacterSection(TEXT("仙缘"));}

void UInkCharacterAppearancePanel::BuildPanelContent()
{
    const TCHAR* Frames[]={TEXT("素墨"),TEXT("青玉"),TEXT("流金"),TEXT("云鹤")};int32 Selected=0;for(int32 I=0;I<4;++I)if(FrameStyle==Frames[I])Selected=I;
    const float PreviewW=bPortraitLayout?BodyWidth:(BodyWidth-20)*.36f,GridX=bPortraitLayout?0:PreviewW+20,GridW=BodyWidth-GridX;
    const float GridTop=bPortraitLayout?212:0;
    Scene(BodyCanvas,0,0,0,PreviewW,188);
    AtlasIcon(BodyCanvas,NAME_None,4,12,40,104);AtlasIcon(BodyCanvas,TEXT("AppearanceSelectedFrame"),Selected,6,34,116);
    Label(BodyCanvas,TEXT("AppearanceFrame"),TEXT("名片 · 试戴预览"),132,16,PreviewW-144,76,30);
    Label(BodyCanvas,TEXT("AppearancePreviewName"),TEXT("试剑修士\n")+FrameStyle+TEXT(" · 本地预览"),132,100,PreviewW-144,88,24);
    auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("AppearanceOptionsScroll"));Scroll->SetAllowOverscroll(false);Scroll->SetScrollbarThickness(FVector2D(6));
    auto* ScrollSlot=BodyCanvas->AddChildToCanvas(Scroll);ScrollSlot->SetPosition({GridX,GridTop});ScrollSlot->SetSize({GridW,FMath::Max(88.f,BodyHeight-GridTop)});
    auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(GridW-10);Size->SetHeightOverride(1000);Scroll->AddChild(Size);
    auto* SectionCanvas=WidgetTree->ConstructWidget<UCanvasPanel>();Size->SetContent(SectionCanvas);
    const float CellW=(GridW-34)/3,CellH=228;
    for(int32 I=0;I<4;++I)
    {
        const float X=(I%3)*(CellW+12),Y=(I/3)*CellH;
        AtlasIcon(SectionCanvas,NAME_None,4,X+(CellW-88)/2,Y+6,88);AtlasIcon(SectionCanvas,FName(*FString::Printf(TEXT("FramePreview_%d"),I)),I,X+(CellW-100)/2,Y,100);
        Action(SectionCanvas,TEXT("Frame_")+FString(Frames[I]),Frames[I],X,Y+112,CellW,88,false,false,I==Selected);
    }
    Label(SectionCanvas,NAME_None,TEXT("血量条样式 · 预览"),0,468,GridW-24,44,28);
    const TCHAR* Bars[]={TEXT("普通"),TEXT("渐变"),TEXT("护盾"),TEXT("华丽")};const float BW=(GridW-26)/2;
    for(int32 I=0;I<4;++I)Action(SectionCanvas,TEXT("Style_")+FString(Bars[I]),Bars[I],(I%2)*(BW+12),524+(I/2)*100,BW,88,false,false,BarStyle==Bars[I]);
    auto* Progress=WidgetTree->ConstructWidget<UInkBufferedProgressWidget>(UInkBufferedProgressWidget::StaticClass(),TEXT("AppearanceBufferedBar"));Progress->SetFraction(.72f,true);Progress->FillColor=BarStyle==TEXT("护盾")?FLinearColor(.28f,.54f,.8f,1):BarStyle==TEXT("华丽")?FLinearColor(.7f,.48f,.2f,1):FLinearColor(.22f,.48f,.42f,1);
    auto* Layout=SectionCanvas->AddChildToCanvas(Progress);Layout->SetOffsets(FMargin(0,736,GridW-24,24));
    const TCHAR* Contexts[]={TEXT("头标预览"),TEXT("排行预览"),TEXT("名片预览")};const float ContextW=(GridW-34)/3;
    for(int32 I=0;I<3;++I)
    {
        const float X=I*(ContextW+12);Label(SectionCanvas,FName(*FString::Printf(TEXT("AppearanceContext_%d"),I)),Contexts[I],X,788,ContextW,64,24,true);
        AtlasIcon(SectionCanvas,NAME_None,4,X+(ContextW-64)/2,860,64);AtlasIcon(SectionCanvas,NAME_None,Selected,X+(ContextW-72)/2,856,72);
        Label(SectionCanvas,NAME_None,TEXT("试剑修士"),X,936,ContextW,48,24,true);
    }
}
void UInkCharacterAppearancePanel::HandlePanelAction(const FString& Id)
{
    if(Id.StartsWith(TEXT("Frame_"))){FrameStyle=Id.Mid(6);RebuildPanel();FeedbackMessage(TEXT("头像框试戴预览"));}
    else if(Id.StartsWith(TEXT("Style_"))){BarStyle=Id.Mid(6);RebuildPanel();FeedbackMessage(TEXT("双层进度条样式预览"));}
    else Super::HandlePanelAction(Id);
}
