#include "InkExtensionWidget.h"
#include "Frontend/UI/InkUiAssetPaths.h"
#include "InkFrontendWidget.h"
#include "InkPresentationStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SafeZone.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "InkBufferedProgressWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/ButtonSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/GameInstance.h"
#include "Cultivation/CultivationSubsystem.h"

namespace
{
    const FLinearColor Gold=InkPresentationStyle::Gold();
    const FLinearColor Muted(.39f,.38f,.32f,1);
    const FLinearColor Ink=InkPresentationStyle::Ink();
    void FillCell(UUniformGridSlot* LayoutSlot)
    {
        LayoutSlot->SetHorizontalAlignment(HAlign_Fill);LayoutSlot->SetVerticalAlignment(VAlign_Fill);
    }
    FString PageTitle(FName Id)
    {
        if(Id==TEXT("Activities"))return TEXT("仙缘活动");if(Id==TEXT("Expedition"))return TEXT("行医出诊");
        if(Id==TEXT("WorldCultivation"))return TEXT("世界修炼");if(Id==TEXT("ResourceContest"))return TEXT("资源争夺 · 本月");
        
        if(Id==TEXT("CharacterProfile"))return TEXT("角色名片");return TEXT("仙途百事");
    }
}

void UInkExtensionWidget::Configure(UInkFrontendWidget* InFrontend,FName InPage)
{Frontend=InFrontend;PageId=(InPage==TEXT("Sect")||InPage==TEXT("Spirit"))?FName(TEXT("Extensions")):InPage;SetIsFocusable(true);RebuildSurface();}

UTextBlock* UInkExtensionWidget::Text(UVerticalBox* Box,const FString& Value,int32 Size)
{
    auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(Value));
    FSlateFontInfo Font;Font.FontObject=Frontend?Frontend->TextFont.LoadSynchronous():nullptr;Font.Size=Size;T->SetFont(Font);
    T->SetColorAndOpacity(Frontend?Frontend->InkColor:FLinearColor(.08f,.10f,.11f,1));T->SetAutoWrapText(true);
    Box->AddChildToVerticalBox(T)->SetPadding(FMargin(0,5));return T;
}

UInkFrontendActionButton* UInkExtensionWidget::Button(UVerticalBox* Box,const FString& Label,const FString& Action,bool bSelected,bool bCompact)
{
    auto* B=WidgetTree->ConstructWidget<UInkFrontendActionButton>();B->Action=FName(*(TEXT("Ext_")+Action));B->BindFrontend(Frontend);
    auto Style=InkPresentationStyle::Button(bSelected,Action==TEXT("Back"));
    Style.SetNormalPadding(FMargin(bCompact?3:12,5));Style.SetPressedPadding(FMargin(bCompact?3:12,5));B->SetStyle(Style);
    const bool bDate=Action.StartsWith(TEXT("Day_"));
    auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetMinDesiredHeight(bDate?54:bCompact?42:48);if(bDate)Size->SetHeightOverride(54);B->SetContent(Size);
    auto* ButtonLayout=CastChecked<UButtonSlot>(Size->Slot);ButtonLayout->SetHorizontalAlignment(HAlign_Fill);ButtonLayout->SetVerticalAlignment(VAlign_Fill);ButtonLayout->SetPadding(bDate?FMargin(0):FMargin(6,2));
    auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(Label));FSlateFontInfo Font;Font.FontObject=Frontend?Frontend->TextFont.LoadSynchronous():nullptr;Font.Size=bCompact?19:21;T->SetFont(Font);T->SetColorAndOpacity(bSelected?FLinearColor(.96f,.88f,.64f,1):Ink);T->SetAutoWrapText(!bCompact);T->SetJustification(ETextJustify::Center);Size->SetContent(T);
    CastChecked<USizeBoxSlot>(T->Slot)->SetVerticalAlignment(VAlign_Center);
    Box->AddChildToVerticalBox(B)->SetPadding(FMargin(0,4));
    return B;
}

UImage* UInkExtensionWidget::Scene(int32 Tile)
{
    auto* Art=WidgetTree->ConstructWidget<UImage>();Art->SetVisibility(ESlateVisibility::HitTestInvisible);
    if(auto* Texture=InkUiAssets::Load<UTexture2D>(nullptr,TEXT("/Game/newUI/Ink/Textures/Shared/T_InkFeatureScenes.T_InkFeatureScenes"),nullptr,LOAD_NoWarn|LOAD_Quiet))
    {
        FSlateBrush Brush;Brush.DrawAs=ESlateBrushDrawType::Image;Brush.SetResourceObject(Texture);Brush.ImageSize=FVector2D(768,256);
        Brush.SetUVRegion(FBox2f(FVector2f((Tile%2)*.5f,(Tile/2)*.25f),FVector2f((Tile%2+1)*.5f,(Tile/2+1)*.25f)));Art->SetBrush(Brush);
    }
    return Art;
}

void UInkExtensionWidget::Section(UVerticalBox* Box,const FString& Title)
{
    auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>();Box->AddChildToVerticalBox(Row)->SetPadding(FMargin(0,8,0,3));
    auto* Mark=WidgetTree->ConstructWidget<UBorder>();Mark->SetBrushColor(Gold);auto* MarkSize=WidgetTree->ConstructWidget<USizeBox>();MarkSize->SetWidthOverride(4);MarkSize->SetHeightOverride(22);Mark->SetContent(MarkSize);
    Row->AddChildToHorizontalBox(Mark)->SetVerticalAlignment(VAlign_Center);
    auto* LabelBox=WidgetTree->ConstructWidget<UVerticalBox>();Row->AddChildToHorizontalBox(LabelBox)->SetPadding(FMargin(9,0));Text(LabelBox,Title,23);
    auto* Line=WidgetTree->ConstructWidget<UBorder>();Line->SetBrushColor(FLinearColor(.53f,.40f,.20f,.35f));auto* LineSize=WidgetTree->ConstructWidget<USizeBox>();LineSize->SetHeightOverride(1);Line->SetContent(LineSize);
    auto* LineSlot=Row->AddChildToHorizontalBox(Line);LineSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));LineSlot->SetVerticalAlignment(VAlign_Center);
}

UVerticalBox* UInkExtensionWidget::Card(UVerticalBox* Box,const FString& Title,const FString& Caption,int32 SceneTile,FName Name)
{
    // A quiet, thin outline stays proportional even on a tall month calendar or nested slot card.
    auto* Paper=WidgetTree->ConstructWidget<UBorder>();FSlateBrush PaperBrush;PaperBrush.DrawAs=ESlateBrushDrawType::RoundedBox;PaperBrush.TintColor=InkPresentationStyle::Paper();
    PaperBrush.OutlineSettings.CornerRadii=FVector4(3,3,3,3);PaperBrush.OutlineSettings.RoundingType=ESlateBrushRoundingType::FixedRadius;PaperBrush.OutlineSettings.Color=FLinearColor(Gold.R,Gold.G,Gold.B,.45f);PaperBrush.OutlineSettings.Width=1;
    Paper->SetBrush(PaperBrush);Paper->SetPadding(FMargin(12,10));
    auto* Body=WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),Name.IsNone()?NAME_None:MakeUniqueObjectName(WidgetTree,UVerticalBox::StaticClass(),Name));Paper->SetContent(Body);
    auto* PaperLayout=Box->AddChildToVerticalBox(Paper);PaperLayout->SetHorizontalAlignment(HAlign_Fill);PaperLayout->SetPadding(FMargin(0,5));
    if(SceneTile>=0)
    {
        auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>();Body->AddChild(Row);
        auto* ArtSize=WidgetTree->ConstructWidget<USizeBox>();ArtSize->SetWidthOverride(96);ArtSize->SetHeightOverride(62);
        auto* Fit=WidgetTree->ConstructWidget<UScaleBox>();Fit->SetStretch(EStretch::ScaleToFill);Fit->SetClipping(EWidgetClipping::ClipToBounds);Fit->SetContent(Scene(SceneTile));ArtSize->SetContent(Fit);
        Row->AddChildToHorizontalBox(ArtSize)->SetVerticalAlignment(VAlign_Center);
        auto* Heading=WidgetTree->ConstructWidget<UVerticalBox>();auto* HeadingSlot=Row->AddChildToHorizontalBox(Heading);HeadingSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));HeadingSlot->SetPadding(FMargin(12,0,0,0));
        Text(Heading,Title,24);if(!Caption.IsEmpty())Text(Heading,Caption,17)->SetColorAndOpacity(Muted);
    }
    else{if(!Title.IsEmpty())Text(Body,Title,24);if(!Caption.IsEmpty())Text(Body,Caption,17)->SetColorAndOpacity(Muted);}
    return Body;
}

void UInkExtensionWidget::Entry(UVerticalBox* Box,const FString& Title,const FString& Caption,const FString& Action,int32 Tile)
{
    auto* Body=Card(Box,Title,Caption,Tile);Button(Body,TEXT("查看详情  ›"),Action);
}
void UInkExtensionWidget::Tabs(UVerticalBox* Box,const TArray<FString>& Names)
{
    if(ActiveTab.IsEmpty()&&!Names.IsEmpty())ActiveTab=Names[0];
    auto* Grid=WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(),MakeUniqueObjectName(WidgetTree,UUniformGridPanel::StaticClass(),TEXT("ExtensionTabs")));Grid->SetSlotPadding(FMargin(3));Box->AddChild(Grid);
    for(int32 I=0;I<Names.Num();++I){auto* Cell=WidgetTree->ConstructWidget<UVerticalBox>();FillCell(Grid->AddChildToUniformGrid(Cell,I/3,I%3));Button(Cell,Names[I],TEXT("Tab_")+Names[I],ActiveTab==Names[I]);}
}
void UInkExtensionWidget::Choices(UVerticalBox* Box,const TArray<FString>& Names,const FString& Prefix)
{
    auto* Grid=WidgetTree->ConstructWidget<UUniformGridPanel>();Grid->SetSlotPadding(FMargin(3));Box->AddChild(Grid);
    for(int32 I=0;I<Names.Num();++I){auto* Cell=WidgetTree->ConstructWidget<UVerticalBox>();FillCell(Grid->AddChildToUniformGrid(Cell,I/2,I%2));Button(Cell,Names[I],Prefix+TEXT("_")+Names[I],(Prefix==TEXT("Style")&&SelectedStyle==Names[I])||(Prefix==TEXT("Frame")&&SelectedAvatarFrame==Names[I]));}
}
void UInkExtensionWidget::Progress(UVerticalBox* Box,const FString& Label,float Fraction)
{
    Text(Box,Label,19);auto* P=WidgetTree->ConstructWidget<UInkBufferedProgressWidget>();P->SetFraction(FMath::Clamp(Fraction,0.f,1.f));
    P->FillColor=SelectedStyle==TEXT("护盾")?FLinearColor(.25f,.48f,.8f,1):SelectedStyle==TEXT("华丽")?FLinearColor(.68f,.46f,.15f,1):SelectedStyle==TEXT("渐变")?FLinearColor(.55f,.3f,.6f,1):FLinearColor(.25f,.46f,.45f,1);
    auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetHeightOverride(20);Size->SetContent(P);Box->AddChildToVerticalBox(Size)->SetPadding(FMargin(0,3,0,10));
}
void UInkExtensionWidget::Slots(UVerticalBox* Box,const FString& Label,int32 Count)
{
    Section(Box,Label);auto* Grid=WidgetTree->ConstructWidget<UUniformGridPanel>();Grid->SetSlotPadding(FMargin(4));Box->AddChild(Grid);
    for(int32 I=1;I<=Count;++I)
    {
        auto* Cell=WidgetTree->ConstructWidget<UVerticalBox>();FillCell(Grid->AddChildToUniformGrid(Cell,(I-1)/3,(I-1)%3));
        const FString EntryName=FString::Printf(TEXT("%s %d · 空位"),*Label,I);
        auto* Inner=Card(Cell,FString::Printf(TEXT("%02d"),I),TEXT("未配置"));
        auto* Icon=WidgetTree->ConstructWidget<UImage>();const auto IconType=Label==TEXT("装备槽")?(I%2?InkPresentationStyle::EIcon::Sword:InkPresentationStyle::EIcon::Armor):Label==TEXT("技能槽")?InkPresentationStyle::EIcon::Manual:Label==TEXT("奖池奖励")?InkPresentationStyle::EIcon::Chest:InkPresentationStyle::EIcon::Jade;
        Icon->SetBrush(InkPresentationStyle::Icon(IconType,FVector2D(34,34)));Icon->SetColorAndOpacity(FLinearColor(1,1,1,.4f));Icon->SetVisibility(ESlateVisibility::HitTestInvisible);auto* IconSize=WidgetTree->ConstructWidget<USizeBox>();IconSize->SetWidthOverride(34);IconSize->SetHeightOverride(34);IconSize->SetContent(Icon);Inner->AddChildToVerticalBox(IconSize)->SetHorizontalAlignment(HAlign_Center);
        Button(Inner,TEXT("查看"),TEXT("Select_")+EntryName,false,true);
    }
}

void UInkExtensionWidget::Identity(UVerticalBox* Box)
{
    auto* Data=GetGameInstance()?GetGameInstance()->GetSubsystem<UCultivationSubsystem>():nullptr;
    const auto Snapshot=Data?Data->GetParticipantSnapshot(Data->GetAnchorId()):FCultivationParticipantSnapshot();
    const bool bHasIdentity=!Snapshot.ParticipantId.IsEmpty();
    auto* CardBody=Card(Box,bHasIdentity?Snapshot.DisplayName:TEXT("角色名片"),bHasIdentity?Snapshot.RealmName.ToString():TEXT("等待玩家身份数据"),6,TEXT("ExtensionIdentityCard"));
    if(bHasIdentity)
    {
        Text(CardBody,TEXT("UID  ")+Snapshot.ParticipantId,17)->SetColorAndOpacity(Muted);
        if(!Snapshot.TitleName.IsEmpty())Text(CardBody,Snapshot.TitleName.ToString(),20)->SetColorAndOpacity(Gold);
        Progress(CardBody,FString::Printf(TEXT("当前修为  %.0f / %.0f"),Snapshot.Cultivation,Snapshot.RequiredCultivation),Snapshot.RequiredCultivation>0?float(Snapshot.Cultivation/Snapshot.RequiredCultivation):0.f);
    }
    else Text(CardBody,TEXT("头像、境界与修为由实际玩家数据提供。"),17);
}

void UInkExtensionWidget::RebuildSurface()
{
    if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this);
    if(Content)
    {
        Content->ClearChildren();RenderContent();if(Feedback)Feedback->SetText(FText::FromString(PreviewFeedback));
        if(Frontend)Frontend->RefreshExtensionActions();return;
    }
    if(WidgetTree->RootWidget)
    {
        TArray<UWidget*> Old;WidgetTree->GetAllWidgets(Old);WidgetTree->RootWidget=nullptr;
        for(auto* W:Old)W->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);
    }
    auto* Root=WidgetTree->ConstructWidget<UOverlay>();WidgetTree->RootWidget=Root;
    auto* Paper=WidgetTree->ConstructWidget<UBorder>();Paper->SetBrushColor(FLinearColor(.93f,.90f,.81f,.99f));Paper->SetPadding(FMargin(18));
    if(Frontend)if(auto* Texture=Frontend->PanelTexture.LoadSynchronous())Paper->SetBrushFromTexture(Texture);
    auto* PaperSlot=Root->AddChildToOverlay(Paper);PaperSlot->SetHorizontalAlignment(HAlign_Fill);PaperSlot->SetVerticalAlignment(VAlign_Fill);
    auto* Safe=WidgetTree->ConstructWidget<USafeZone>();Safe->SetSidesToPad(true,true,true,true);Paper->SetContent(Safe);
    auto* Body=WidgetTree->ConstructWidget<UVerticalBox>();Safe->SetContent(Body);
    auto* Heading=WidgetTree->ConstructWidget<UBorder>();Heading->SetBrush(InkPresentationStyle::Surface(InkPresentationStyle::ESurface::Heading,FVector2D(600,54)));Heading->SetPadding(FMargin(16,5));Body->AddChild(Heading);
    auto* HeadingBody=WidgetTree->ConstructWidget<UVerticalBox>();Heading->SetContent(HeadingBody);auto* Title=Text(HeadingBody,PageTitle(PageId),30);Title->SetJustification(ETextJustify::Center);Title->SetColorAndOpacity(Gold);
    Text(Body,TEXT("功能预览 · 奖励、购买及活动规则待接入"),15)->SetColorAndOpacity(Muted);
    auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>();Scroll->SetScrollBarVisibility(ESlateVisibility::Visible);
    Body->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    auto* ContentWidth=WidgetTree->ConstructWidget<USizeBox>();ContentWidth->SetWidthOverride(900);ContentWidth->SetMaxDesiredWidth(900);
    CastChecked<UScrollBoxSlot>(Scroll->AddChild(ContentWidth))->SetHorizontalAlignment(HAlign_Center);
    Content=WidgetTree->ConstructWidget<UVerticalBox>();ContentWidth->SetContent(Content);RenderContent();
    Feedback=Text(Body,PreviewFeedback,18);Feedback->SetColorAndOpacity(FLinearColor(.48f,.18f,.10f,1));
    Button(Body,PageId==TEXT("Extensions")?TEXT("返回游戏"):TEXT("返回仙途百事"),TEXT("Back"));
    if(Frontend)Frontend->RefreshExtensionActions();
}

void UInkExtensionWidget::Detail(UVerticalBox* Box)
{
    const int32 Tile=PageId==TEXT("Expedition")?1:PageId==TEXT("WorldCultivation")?2:PageId==TEXT("ResourceContest")?3:PageId==TEXT("CharacterProfile")?6:0;
    Box=Card(Box,SelectedEntry,TEXT("详情预览 · 规则待接入"),Tile);
    if(PageId==TEXT("WorldCultivation")){Text(Box,TEXT("区域产出：灵气、修为、突破材料；修炼阶段：入定 → 周天 → 收功。"));Progress(Box,TEXT("区域修炼进度 · 预览"),.45f);Choices(Box,{TEXT("开始修炼"),TEXT("查看产出"),TEXT("领取收益")},TEXT("Preview"));}
    else if(PageId==TEXT("Expedition")){Text(Box,TEXT("委托目标：诊治、调查与采药。预览收益：修为、灵草。"));Slots(Box,TEXT("队伍槽"),3);Choices(Box,{TEXT("自动编队"),TEXT("出发"),TEXT("加速"),TEXT("领取收益")},TEXT("Preview"));Progress(Box,TEXT("行程进度 · 预览"),.35f);}
    else if(PageId==TEXT("ResourceContest")){Text(Box,TEXT("本月资源点 · 占领后产出预览：灵石、灵草、阵法碎片。"));Progress(Box,TEXT("资源占领进度 · 预览"),.3f);Choices(Box,{TEXT("占领"),TEXT("占领排行"),TEXT("奖励预览")},TEXT("Preview"));}

    else{Text(Box,TEXT("该条目的条件、内容和奖励将在规则接入后展示。"));Choices(Box,{TEXT("内容预览"),TEXT("奖励预览"),TEXT("领取")},TEXT("Preview"));}
    Button(Box,TEXT("返回列表"),TEXT("List"));
}

void UInkExtensionWidget::RenderContent()
{
    auto* Box=Content.Get();if(!Box)return;
    if(PageId==TEXT("Extensions"))
    {
        Section(Box,TEXT("探索与成长"));
        const TArray<FString> Ids={TEXT("Activities"),TEXT("Expedition"),TEXT("WorldCultivation"),TEXT("ResourceContest"),TEXT("CharacterProfile")};
        const TArray<FString> Captions={TEXT("月历签到 / 每日一签 / 限时仙缘"),TEXT("山村义诊 / 调查采药 / 个人委托"),TEXT("五域山川 / 区域修炼 / 产出预览"),TEXT("灵矿灵草 / 阵眼 / 本月排行"),TEXT("真实属性 / 装备槽 / 名片外观")};
        const int32 Tiles[]={0,1,2,3,6};
        for(int32 I=0;I<Ids.Num();++I)Entry(Box,PageTitle(FName(*Ids[I])),Captions[I],TEXT("Open_")+Ids[I],Tiles[I]);return;
    }
    if(PageId==TEXT("Activities"))Tabs(Box,{TEXT("签到"),TEXT("每日一签"),TEXT("修行任务"),TEXT("限时仙缘")});
    else if(PageId==TEXT("Expedition"))Tabs(Box,{TEXT("全部"),TEXT("紧急"),TEXT("常规"),TEXT("个人委托")});
    else if(PageId==TEXT("WorldCultivation"))Tabs(Box,{TEXT("东域"),TEXT("南域"),TEXT("西域"),TEXT("北域"),TEXT("秘境")});
    else if(PageId==TEXT("ResourceContest"))Tabs(Box,{TEXT("全部"),TEXT("灵矿"),TEXT("灵草"),TEXT("阵法")});
    else if(PageId==TEXT("CharacterProfile"))Tabs(Box,{TEXT("属性"),TEXT("装备"),TEXT("技能"),TEXT("法宝"),TEXT("仙缘"),TEXT("外观")});
    if(!SelectedEntry.IsEmpty()){Detail(Box);return;}
    if(PageId==TEXT("Activities"))
    {
        if(ActiveTab==TEXT("签到"))
        {
            FDateTime Date=FDateTime::Now();int32 Year=Date.GetYear(),Month=Date.GetMonth()+CalendarOffset;while(Month<1){Month+=12;--Year;}while(Month>12){Month-=12;++Year;}
            auto* MonthCard=Card(Box,FString::Printf(TEXT("%d 年 %d 月  ·  仙历"),Year,Month),TEXT("日历依据本机日期排列；签到记录尚未接入"),0);
            Choices(MonthCard,{TEXT("上个月"),TEXT("下个月")},TEXT("Month"));
            auto* Calendar=WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(),MakeUniqueObjectName(WidgetTree,UUniformGridPanel::StaticClass(),TEXT("ExtensionCalendar")));Calendar->SetSlotPadding(FMargin(2));MonthCard->AddChild(Calendar);
            const TCHAR* Weekdays[]={TEXT("一"),TEXT("二"),TEXT("三"),TEXT("四"),TEXT("五"),TEXT("六"),TEXT("日")};
            for(int32 I=0;I<7;++I){auto* Cell=WidgetTree->ConstructWidget<UVerticalBox>();FillCell(Calendar->AddChildToUniformGrid(Cell,0,I));auto* Label=Text(Cell,Weekdays[I],17);Label->SetJustification(ETextJustify::Center);Label->SetColorAndOpacity(Muted);}
            const int32 Days=FDateTime::DaysInMonth(Year,Month);const int32 Offset=int32(FDateTime(Year,Month,1).GetDayOfWeek());
            for(int32 D=1;D<=Days;++D){auto* Cell=WidgetTree->ConstructWidget<UVerticalBox>();FillCell(Calendar->AddChildToUniformGrid(Cell,1+(D-1+Offset)/7,(D-1+Offset)%7));Button(Cell,FString::FromInt(D),TEXT("Day_")+FString::FromInt(D),CalendarOffset==0&&D==Date.GetDay(),true);}
            Choices(MonthCard,{TEXT("签到"),TEXT("已签到记录"),TEXT("一键领取"),TEXT("补签")},TEXT("Preview"));Section(Box,TEXT("累计签到 · 奖励预览"));
            Choices(Box,{TEXT("3天奖励"),TEXT("7天奖励"),TEXT("14天奖励"),TEXT("21天奖励"),TEXT("28天奖励")},TEXT("Select"));
        }
        else if(ActiveTab==TEXT("每日一签"))
        {
            auto* Fortune=Card(Box,TEXT("每日一签"),TEXT("签文样式预览 · 非今日实际签运"),0,TEXT("ExtensionFortuneCard"));
            Text(Fortune,TEXT("静  ·  明"),38)->SetJustification(ETextJustify::Center);
            auto* Verse=Text(Fortune,TEXT("心静自明\n行稳致远"),29);Verse->SetJustification(ETextJustify::Center);Verse->SetColorAndOpacity(Gold);
            Text(Fortune,TEXT("一念澄明，山水皆可入怀。"),19)->SetJustification(ETextJustify::Center);
            Choices(Fortune,{TEXT("求签"),TEXT("签文预览")},TEXT("Preview"));
        }
        else if(ActiveTab==TEXT("修行任务"))
        {
            Entry(Box,TEXT("日常修炼"),TEXT("入定静心 · 任务条件与奖励待接入"),TEXT("Select_日常修炼"),2);
            Entry(Box,TEXT("秘境挑战"),TEXT("山川历练 · 任务条件与奖励待接入"),TEXT("Select_秘境挑战"),3);
            Entry(Box,TEXT("炼丹试炼"),TEXT("草木问道 · 任务条件与奖励待接入"),TEXT("Select_炼丹试炼"),7);
        }
        else
        {
            auto* Banner=Card(Box,TEXT("限时仙缘  ·  寻宝预览"),TEXT("开放时间与倒计时：等待活动数据"),7);
            Choices(Banner,{TEXT("仙缘寻宝"),TEXT("天降福缘")},TEXT("Select"));
            auto* Pool=Card(Box,FString::Printf(TEXT("奖池预览 · 第 %d 页"),PrizePage+1),TEXT("空位用于展示奖池布局；不代表可获得物品"),-1,TEXT("ExtensionPrizePool"));
            Slots(Pool,TEXT("奖池奖励"),4);Choices(Pool,{TEXT("抽一次"),TEXT("抽十次"),bSkipAnimation?TEXT("跳过动画：开"):TEXT("跳过动画：关"),TEXT("上一奖池"),TEXT("下一奖池")},TEXT("Draw"));
        }
    }
    else if(PageId==TEXT("Expedition"))
    {
        Section(Box,ActiveTab+TEXT("委托 · 预览"));
        if(ActiveTab==TEXT("全部")||ActiveTab==TEXT("常规"))Entry(Box,TEXT("山村义诊"),TEXT("常规 · 采药诊治 / 收益规则待接入"),TEXT("Select_山村义诊"),1);
        if(ActiveTab==TEXT("全部")||ActiveTab==TEXT("紧急"))Entry(Box,TEXT("瘟疫调查"),TEXT("紧急 · 寻访病因 / 收益规则待接入"),TEXT("Select_瘟疫调查"),1);
        if(ActiveTab==TEXT("全部")||ActiveTab==TEXT("个人委托"))Entry(Box,TEXT("名医请托"),TEXT("个人委托 · 草木辨识 / 收益规则待接入"),TEXT("Select_名医请托"),1);
        Slots(Box,TEXT("队伍槽"),3);Choices(Box,{TEXT("自动编队"),TEXT("收益预览"),TEXT("加速"),TEXT("领取")},TEXT("Preview"));
    }
    else if(PageId==TEXT("WorldCultivation"))
    {
        const FString Area=ActiveTab==TEXT("东域")?TEXT("青云山"):ActiveTab==TEXT("南域")?TEXT("落霞谷"):ActiveTab==TEXT("西域")?TEXT("妖兽林"):ActiveTab==TEXT("北域")?TEXT("玄冰原"):TEXT("赤炎岭");
        auto* Region=Card(Box,ActiveTab+TEXT("  ·  ")+Area,TEXT("山水区域预览 · 修炼条件与产出待接入"),2);
        auto* ArtSize=WidgetTree->ConstructWidget<USizeBox>();ArtSize->SetHeightOverride(140);auto* Fit=WidgetTree->ConstructWidget<UScaleBox>();Fit->SetStretch(EStretch::ScaleToFit);Fit->SetContent(Scene(2));ArtSize->SetContent(Fit);Region->AddChild(ArtSize);
        Button(Region,TEXT("查看区域与产出  ›"),TEXT("Select_")+Area);Progress(Region,TEXT("入定  →  周天  →  收功  ·  阶段样式预览"),.4f);Choices(Region,{TEXT("区域详情"),TEXT("产出预览"),TEXT("收益预览")},TEXT("Preview"));
    }
    else if(PageId==TEXT("ResourceContest"))
    {
        Section(Box,TEXT("本月争夺"));Text(Box,TEXT("占领状态 / 排行 / 结算时间：等待服务数据"),17)->SetColorAndOpacity(Muted);
        if(ActiveTab==TEXT("全部")||ActiveTab==TEXT("灵矿"))Entry(Box,TEXT("玄铁灵矿"),TEXT("灵矿资源点 · 占领与产出规则预览"),TEXT("Select_玄铁灵矿"),3);
        if(ActiveTab==TEXT("全部")||ActiveTab==TEXT("灵草"))Entry(Box,TEXT("千年灵药园"),TEXT("灵草资源点 · 占领与产出规则预览"),TEXT("Select_千年灵药园"),1);
        if(ActiveTab==TEXT("全部")||ActiveTab==TEXT("阵法"))Entry(Box,TEXT("上古阵眼"),TEXT("阵法资源点 · 占领与产出规则预览"),TEXT("Select_上古阵眼"),3);
        Choices(Box,{TEXT("占领排行"),TEXT("本月奖励")},TEXT("Preview"));
    }


    else if(PageId==TEXT("CharacterProfile"))
    {
        Identity(Box);
        if(ActiveTab==TEXT("属性"))
        {
            auto* Attributes=Card(Box,TEXT("角色属性"),TEXT("当前真实属性 · 数值由游戏状态提供"));
            if(Frontend)
            {
                const FName IconIds[]={TEXT("Health"),TEXT("Attack"),TEXT("Defense"),TEXT("SpiritDamage"),TEXT("Critical"),TEXT("Evasion"),TEXT("Luck"),TEXT("MoveSpeed"),TEXT("Swordsmanship"),TEXT("CultivationSpeed"),TEXT("Realm")};
                for(const auto& Attribute:Frontend->GetPresentedCharacterAttributes())
                {
                    if(Attribute.AttributeId==TEXT("Realm"))continue;
                    // Stacked label/value keeps large gift totals readable on narrow portrait pages.
                    auto* Row=WidgetTree->ConstructWidget<UVerticalBox>();Attributes->AddChildToVerticalBox(Row)->SetPadding(FMargin(4,4,4,8));
                    auto* LabelRow=WidgetTree->ConstructWidget<UHorizontalBox>();Row->AddChild(LabelRow);
                    auto* IconTexture=Attribute.Icon.LoadSynchronous();const bool bCustomIcon=IconTexture!=nullptr;
                    int32 IconIndex=INDEX_NONE;for(int32 I=0;I<UE_ARRAY_COUNT(IconIds);++I)if(IconIds[I]==Attribute.AttributeId){IconIndex=I;break;}
                    if(!IconTexture&&IconIndex!=INDEX_NONE)IconTexture=Frontend->AttributeIconAtlas.LoadSynchronous();
                    if(IconTexture)
                    {
                        auto* Icon=WidgetTree->ConstructWidget<UImage>();FSlateBrush Brush;Brush.DrawAs=ESlateBrushDrawType::Image;Brush.SetResourceObject(IconTexture);Brush.ImageSize=FVector2D(28,28);
                        if(!bCustomIcon)Brush.SetUVRegion(FBox2f(FVector2f((IconIndex%4)/4.f,(IconIndex/4)/3.f),FVector2f((IconIndex%4+1)/4.f,(IconIndex/4+1)/3.f)));
                        Icon->SetBrush(Brush);Icon->SetVisibility(ESlateVisibility::HitTestInvisible);auto* IconSize=WidgetTree->ConstructWidget<USizeBox>();IconSize->SetWidthOverride(28);IconSize->SetHeightOverride(28);IconSize->SetContent(Icon);LabelRow->AddChildToHorizontalBox(IconSize)->SetVerticalAlignment(VAlign_Center);
                    }
                    auto* LabelBox=WidgetTree->ConstructWidget<UVerticalBox>();LabelRow->AddChildToHorizontalBox(LabelBox)->SetPadding(FMargin(7,0));Text(LabelBox,Attribute.Label.ToString(),17)->SetColorAndOpacity(Muted);
                    Text(Row,Attribute.Value.ToString(),24);
                    auto* Rule=WidgetTree->ConstructWidget<UBorder>();Rule->SetBrushColor(FLinearColor(.53f,.40f,.20f,.18f));auto* Height=WidgetTree->ConstructWidget<USizeBox>();Height->SetHeightOverride(1);Rule->SetContent(Height);Row->AddChild(Rule);
                }
            }
            Button(Attributes,TEXT("查看当前实际角色属性  ›"),TEXT("Open_Character"));Choices(Box,{TEXT("人物名片"),TEXT("境界展示")},TEXT("Select"));
        }
        else if(ActiveTab==TEXT("装备"))Slots(Box,TEXT("装备槽"),6);
        else if(ActiveTab==TEXT("技能"))Slots(Box,TEXT("技能槽"),4);
        else if(ActiveTab==TEXT("法宝")){Slots(Box,TEXT("法宝槽"),3);Progress(Box,TEXT("法宝进度 · 样式预览"),.35f);}
        else if(ActiveTab==TEXT("仙缘")){Choices(Box,{TEXT("仙缘资料"),TEXT("人物名片")},TEXT("Select"));Progress(Box,TEXT("炼丹进度 · 样式预览"),.7f);}
        else
        {
            auto* Appearance=Card(Box,TEXT("名片外观"),TEXT("仅预览显示样式，不改变装备或实际血量值"));
            Text(Appearance,TEXT("头像框：")+SelectedAvatarFrame,19);
            auto* Frame=WidgetTree->ConstructWidget<UBorder>();Frame->SetPadding(FMargin(3));Frame->SetBrushColor(SelectedAvatarFrame==TEXT("青金头像框")?FLinearColor(.2f,.44f,.52f,1):SelectedAvatarFrame==TEXT("山水头像框")?FLinearColor(.35f,.42f,.32f,1):Gold);
            auto* Preview=WidgetTree->ConstructWidget<UVerticalBox>();Frame->SetContent(Preview);
            auto* Portrait=WidgetTree->ConstructWidget<USizeBox>();Portrait->SetHeightOverride(100);auto* Fit=WidgetTree->ConstructWidget<UScaleBox>();Fit->SetStretch(EStretch::ScaleToFit);Fit->SetContent(Scene(6));Portrait->SetContent(Fit);Preview->AddChild(Portrait);Appearance->AddChild(Frame);
            Choices(Appearance,{TEXT("素墨头像框"),TEXT("青金头像框"),TEXT("山水头像框")},TEXT("Frame"));Section(Appearance,TEXT("血条样式"));Choices(Appearance,{TEXT("普通"),TEXT("渐变"),TEXT("护盾"),TEXT("华丽")},TEXT("Style"));Progress(Appearance,SelectedStyle+TEXT("血条 · 样式预览 72%"),.72f);Choices(Box,{TEXT("人物名片"),TEXT("境界展示")},TEXT("Select"));
        }
    }
}

void UInkExtensionWidget::HandleAction(FName Action)
{
    const FString A=Action.ToString();
    if(A==TEXT("Back")){Frontend->ShowPage(PageId==TEXT("Extensions")?FName(TEXT("HUD")):FName(TEXT("Extensions")));return;}
    if(A==TEXT("Open_Sect")||A==TEXT("Open_Spirit")){PreviewFeedback=TEXT("此入口不在当前版本开放");RebuildSurface();return;}
    if(A.StartsWith(TEXT("Open_"))){Frontend->ShowPage(FName(*A.Mid(5)));return;}
    if(A.StartsWith(TEXT("Tab_"))){ActiveTab=A.Mid(4);SelectedEntry.Empty();PreviewFeedback.Empty();}
    else if(A.StartsWith(TEXT("Select_"))){SelectedEntry=A.Mid(7);PreviewFeedback.Empty();}
    else if(A==TEXT("List")){SelectedEntry.Empty();}
    else if(A.StartsWith(TEXT("Style_"))){SelectedStyle=A.Mid(6);PreviewFeedback=TEXT("外观预览：")+SelectedStyle+TEXT("；未消耗物品。");}
    else if(A.StartsWith(TEXT("Frame_"))){SelectedAvatarFrame=A.Mid(6);PreviewFeedback=TEXT("头像框预览：")+SelectedAvatarFrame+TEXT("；未消耗物品。");}
    else if(A.StartsWith(TEXT("Month_"))){CalendarOffset+=A.Contains(TEXT("上个月"))?-1:1;CalendarOffset=FMath::Clamp(CalendarOffset,-24,24);}
    else if(A.StartsWith(TEXT("Day_"))){PreviewFeedback=TEXT("签到日期 ")+A.Mid(4)+TEXT(" 日 · 功能预览，尚无签到记录。");}
    else if(A.StartsWith(TEXT("Draw_"))){if(A.Contains(TEXT("跳过动画")))bSkipAnimation=!bSkipAnimation;else if(A.Contains(TEXT("上一奖池")))PrizePage=FMath::Max(0,PrizePage-1);else if(A.Contains(TEXT("下一奖池")))PrizePage=(PrizePage+1)%3;PreviewFeedback=TEXT("功能预览：")+A.Mid(5)+TEXT("；未抽取奖励或扣除货币。");}
    else PreviewFeedback=TEXT("功能预览：")+(A.StartsWith(TEXT("Preview_"))?A.Mid(8):A)+TEXT("；未发放奖励、购买或扣除物品。");
    RebuildSurface();
}

FReply UInkExtensionWidget::NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event)
{
    if(Event.GetKey()==EKeys::Escape){HandleAction(SelectedEntry.IsEmpty()?FName(TEXT("Back")):FName(TEXT("List")));return FReply::Handled();}
    return Super::NativeOnKeyDown(Geometry,Event);
}
