#include "LiveGMControlPanel.h"
#include "LiveGMOverlay.h"
#include "LiveGMTestSession.h"
#include "LiveWeaponSwarm.h"
#include "LiveTitleEffectsComponent.h"
#include "Frontend/InkPresentationStyle.h"
#include "Frontend/InkBufferedProgressWidget.h"
#include "Cultivation/CultivationSubsystem.h"
#include "Cultivation/CultivationRules.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/ButtonSlot.h"
#include "Components/ScrollBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/GameInstance.h"

namespace
{
    struct FGiftDescription { FString Summary,Detail; };
    FGiftDescription DescribeGift(int32 Index,const FCultivationRuleConfig& R)
    {
        const auto N=[](double Value){return FString::SanitizeFloat(Value,0);};
        const auto Seconds=[&](double Value){return N(Value)+TEXT("秒");};
        const auto Percent=[&](double Value){return N(Value*100)+TEXT("%");};
        const auto Title=CultivationRules::TitleName(CultivationRules::GiftTitle(static_cast<ECultivationGift>(Index))).ToString();
        switch(Index)
        {
        case 0:return {
            TEXT("自身修炼 ×")+N(R.WandBase)+TEXT(" · ")+Seconds(R.WandSeconds)+TEXT("\n连送叠加倍率与时长"),
            TEXT("送礼观众的修炼速度首份 ×")+N(R.WandBase)+TEXT("，每追加一份增加 ")+N(R.WandBase*R.WandStackCoefficient)+TEXT(" 倍。每份延长 ")+Seconds(R.WandSeconds)+TEXT("；到期后重置叠加。")};
        case 1:return {
            TEXT("自身修为 +")+N(R.LoveExperience)+TEXT("\n立即生效"),
            TEXT("每份立即为送礼观众增加 ")+N(R.LoveExperience)+TEXT(" 修为，按当前境界规则自动升级；不增加主播修为。")};
        case 2:return {
            TEXT("永久自身属性 ×")+N(R.SoulPersonalMultiplier)+TEXT("\n周期共享给全场"),
            TEXT("永久解锁「")+Title+TEXT("」，送礼观众自身属性 ×")+N(R.SoulPersonalMultiplier)+TEXT("。立即开启共享，之后每 ")+Seconds(R.SoulCooldown)+TEXT(" 自动开启，持续 ")+Seconds(R.SoulDuration)+TEXT("；其他观众获得持有者属性的 ")+Percent(R.SoulAudienceShare)+TEXT("，主播获得 ")+Percent(R.SoulAnchorShare)+TEXT("。同一观众重复赠送不叠加解锁倍率。")};
        case 3:return {
            TEXT("全场攻击/灵伤 +")+N(R.JetFlatBonus)+TEXT("\n主播另加生命与防御"),
            TEXT("永久解锁「")+Title+TEXT("」。立即开启全场增益，之后每 ")+Seconds(R.JetCooldown)+TEXT(" 自动开启，持续 ")+Seconds(R.JetDuration)+TEXT("：攻击、灵伤各 +")+N(R.JetFlatBonus)+TEXT("，主播另获得等额生命上限与防御。多名持有者不重复叠加这项全场增益。")};
        case 4:return {
            TEXT("场内怪物属性 -")+Percent(R.EnemyReduction)+TEXT("\n永久解锁 · 在场生效"),
            TEXT("永久解锁「")+Title+TEXT("」。持有者在场时，场内怪物属性降低 ")+Percent(R.EnemyReduction)+TEXT("；多名持有者不重复叠加。")};
        case 5:return {
            TEXT("主播低血恢复 ")+Percent(R.StarHealFraction)+TEXT("\n")+FString::FromInt(FMath::Clamp(R.StarCharges,1,64))+TEXT(" 次储备 · 独立冷却"),
            TEXT("永久解锁「")+Title+TEXT("」，获得 ")+FString::FromInt(FMath::Clamp(R.StarCharges,1,64))+TEXT(" 次治疗储备。持有者在场且主播受击后生命不高于 ")+Percent(R.StarHealthThreshold)+TEXT(" 时，消耗一次，恢复主播最大生命的 ")+Percent(R.StarHealFraction)+TEXT("。每次储备独立冷却 ")+Seconds(R.StarCooldown)+TEXT("；每次受击最多消耗一次。")};
        case 6:return {
            TEXT("主播 ")+Percent(R.ImmunityChance)+TEXT(" 概率免伤\n永久解锁 · 在场生效"),
            TEXT("永久解锁「")+Title+TEXT("」。持有者在场时，主播有 ")+Percent(R.ImmunityChance)+TEXT(" 概率免疫一次伤害；多名持有者不重复叠加概率。")};
        case 7:return {
            TEXT("自身修炼 ×")+N(R.PotionBase)+TEXT(" · ")+N(R.PotionSeconds/3600)+TEXT("小时\n同房×")+N(R.PotionAudienceMultiplier)+TEXT(" / 主播×")+N(R.PotionAnchorMultiplier),
            TEXT("送礼观众修炼速度首份 ×")+N(R.PotionBase)+TEXT("，每追加一份增加 ")+N(R.PotionBase*R.PotionStackCoefficient)+TEXT(" 倍，每份延长 ")+Seconds(R.PotionSeconds)+TEXT("。有效期内，同房未持有有效药水的观众修炼 ×")+N(R.PotionAudienceMultiplier)+TEXT("，主播 ×")+N(R.PotionAnchorMultiplier)+TEXT("；同房增益不重复叠加，自身药水倍率优先，可与仙女棒相乘。")};
        default:return {};
        }
    }
}

void ULiveGMControlButton::Bind(ULiveGMControlPanel* Owner,FName Id){Panel=Owner;Action=Id;InitIsFocusable(false);OnClicked.AddUniqueDynamic(this,&ThisClass::Apply);}
void ULiveGMControlButton::Apply(){if(Panel.IsValid())Panel->ExecuteControl(Action);}
void ULiveGMControlPanel::Configure(ALiveGMOverlay* Owner){Host=Owner;SetIsFocusable(false);Rebuild();}
UTextBlock* ULiveGMControlPanel::Text(const FString& Label,float X,float Y,float W,float H,int32 Size)
{
    auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(Label));FSlateFontInfo Font;Font.FontObject=Host.IsValid()?Host->NameFont.Get():nullptr;Font.Size=Size;T->SetFont(Font);T->SetColorAndOpacity(InkPresentationStyle::Ink());T->SetAutoWrapText(true);T->SetVisibility(ESlateVisibility::HitTestInvisible);auto* Layout=Canvas->AddChildToCanvas(T);Layout->SetOffsets(FMargin(X,Y,W,H));return T;
}
void ULiveGMControlPanel::Button(FName Id,const FString& Label,float X,float Y,float W,float H,const TCHAR* IconPath,const FString& Description)
{
    auto* B=WidgetTree->ConstructWidget<ULiveGMControlButton>();B->Bind(this,Id);B->SetStyle(InkPresentationStyle::Button(Id==FName(*(TEXT("Tab_")+SelectedTab.ToString()))));
    auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(Label));FSlateFontInfo Font;Font.FontObject=Host.IsValid()?Host->NameFont.Get():nullptr;Font.Size=H<=42?18:20;T->SetFont(Font);T->SetAutoWrapText(false);T->SetJustification(ETextJustify::Center);T->SetColorAndOpacity(Id==FName(*(TEXT("Tab_")+SelectedTab.ToString()))?FLinearColor(.97f,.95f,.86f,1):InkPresentationStyle::Ink());B->SetContent(T);auto* Content=CastChecked<UButtonSlot>(T->Slot);Content->SetVerticalAlignment(VAlign_Center);Content->SetHorizontalAlignment(HAlign_Fill);Content->SetPadding(FMargin(0));auto* Layout=Canvas->AddChildToCanvas(B);Layout->SetOffsets(FMargin(X,Y,W,H));Buttons.Add(B);
    if(IconPath)if(auto* Texture=LoadObject<UTexture2D>(nullptr,IconPath))
    {
        auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>();Row->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* IconSize=WidgetTree->ConstructWidget<USizeBox>();IconSize->SetWidthOverride(44);IconSize->SetHeightOverride(44);
        auto* Fit=WidgetTree->ConstructWidget<UScaleBox>();Fit->SetStretch(EStretch::ScaleToFit);IconSize->SetContent(Fit);
        auto* Icon=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),FName(*(Id.ToString()+TEXT("Icon"))));
        Icon->SetBrushFromTexture(Texture,true);Fit->SetContent(Icon);
        auto* IconSlot=Row->AddChildToHorizontalBox(IconSize);IconSlot->SetPadding(FMargin(8,0,8,0));IconSlot->SetVerticalAlignment(VAlign_Center);
        B->SetContent(Row);
        UWidget* Labels=T;
        if(!Description.IsEmpty())
        {
            Font.Size=18;T->SetFont(Font);T->SetLineHeightPercentage(.8f);
            auto* Stack=WidgetTree->ConstructWidget<UVerticalBox>();Stack->AddChildToVerticalBox(T);
            auto* Effect=WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),FName(*(Id.ToString()+TEXT("Description"))));
            Font.Size=13;Effect->SetFont(Font);Effect->SetLineHeightPercentage(.8f);Effect->SetText(FText::FromString(Description));Effect->SetColorAndOpacity(FLinearColor(.20f,.27f,.24f,1));Effect->SetJustification(ETextJustify::Center);Effect->SetAutoWrapText(false);
            auto* EffectSlot=Stack->AddChildToVerticalBox(Effect);EffectSlot->SetPadding(FMargin(0,3,0,0));Labels=Stack;
        }
        auto* LabelSlot=Row->AddChildToHorizontalBox(Labels);LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));LabelSlot->SetVerticalAlignment(VAlign_Center);LabelSlot->SetPadding(FMargin(0,0,8,0));
        auto* RowSlot=CastChecked<UButtonSlot>(Row->Slot);RowSlot->SetHorizontalAlignment(HAlign_Fill);RowSlot->SetVerticalAlignment(VAlign_Fill);RowSlot->SetPadding(FMargin(0));
    }
}
UEditableTextBox* ULiveGMControlPanel::Input(const FString& Value,float X,float Y,float W)
{
    auto* Edit=WidgetTree->ConstructWidget<UEditableTextBox>();Edit->SetText(FText::FromString(Value));auto Style=Edit->GetWidgetStyle();Style.SetFont(FSlateFontInfo(Host.IsValid()?Host->NameFont.Get():nullptr,18));Edit->SetWidgetStyle(Style);auto* Layout=Canvas->AddChildToCanvas(Edit);Layout->SetOffsets(FMargin(X,Y,W,44));return Edit;
}
void ULiveGMControlPanel::Rebuild()
{
    if(!Host.IsValid())return;auto* Session=Host->TestSession.Get();if(!Session)return;
    if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this);Buttons.Reset();ShieldSample=CultivationSample=nullptr;ViewerId=ViewerName=AvatarUrl=Comment=nullptr;
    if(!Canvas)
    {
        auto* Scale=WidgetTree->ConstructWidget<UScaleBox>();Scale->SetStretch(EStretch::ScaleToFit);WidgetTree->RootWidget=Scale;
        auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(600);Size->SetHeightOverride(820);Scale->SetContent(Size);
        Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();Canvas->SetClipping(EWidgetClipping::ClipToBoundsAlways);Size->SetContent(Canvas);
    }
    else Canvas->ClearChildren();
    auto* Paper=WidgetTree->ConstructWidget<UBorder>();Paper->SetBrush(InkPresentationStyle::Surface(InkPresentationStyle::ESurface::Card,FVector2D(600,820)));Paper->SetVisibility(ESlateVisibility::HitTestInvisible);auto* Background=Canvas->AddChildToCanvas(Paper);Background->SetOffsets(FMargin(0,0,600,820));
    Text(TEXT("GM · 本地演示与界面测试"),24,20,470,40,27);Button(TEXT("GMToggle"),TEXT("收起"),496,18,80,42);
    const TCHAR* Ids[]={TEXT("Session"),TEXT("Audience"),TEXT("Events"),TEXT("Visual"),TEXT("Layout"),TEXT("Results")};const TCHAR* Labels[]={TEXT("会话"),TEXT("观众"),TEXT("互动"),TEXT("表现"),TEXT("布局"),TEXT("结果")};
    for(int32 I=0;I<6;++I)Button(FName(*(FString(TEXT("Tab_"))+Ids[I])),Labels[I],24+I*92,78,86,42);
    Status=Text(Session->DescribeSession(),26,140,548,104,16);Status->SetClipping(EWidgetClipping::ClipToBoundsAlways);ResultText=nullptr;
    if(SelectedTab==TEXT("Session"))
    {
        Button(TEXT("EnableTest"),TEXT("确认本地模拟"),28,264,544);
        Button(TEXT("QA_Checks"),TEXT("运行本地链路与阈值检查"),28,330,544);
        Text(TEXT("当前版本默认本地演示。\n观众、评论、点赞、关注和礼物均由本机模拟。\n\n普通编辑器中即可直接使用，无需 SDK 账号或提审权限。\n抖音 SDK 在提审后单独接入。"),30,410,540,240,20);
    }
    else if(SelectedTab==TEXT("Audience"))
    {
        Text(TEXT("测试 ID"),28,252,110,34,18);ViewerId=Input(Session->ViewerId,146,246,426);
        Text(TEXT("显示昵称"),28,308,110,34,18);ViewerName=Input(Session->ViewerName,146,302,426);
        Text(TEXT("头像 HTTPS"),28,364,118,34,18);AvatarUrl=Input(Session->ViewerAvatarUrl,146,358,426);
        Button(TEXT("QA_Join"),TEXT("加入 / 更新资料"),28,422);Button(TEXT("QA_Leave"),TEXT("离开"),316,422);
        Button(TEXT("QA_Duplicate"),TEXT("重复进场去重"),28,484);Button(TEXT("Add10"),TEXT("加入 10 名"),316,484);
        Button(TEXT("Add100"),TEXT("加入 100 名"),28,546);Button(TEXT("Fill1000"),TEXT("补足 1000 名"),316,546);
        Button(TEXT("ClearAudience"),TEXT("清理模拟观众"),28,608,544);
        Text(TEXT("同 ID 更新身份，同名不同 ID 分别生成。\n留空头像显示占位；失败重试不阻止加入。"),28,680,544,80,18);
    }
    else if(SelectedTab==TEXT("Events"))
    {
        Text(TEXT("评论内容"),28,250,130,34,18);Comment=Input(Session->Comment,160,244,412);
        Button(TEXT("QA_Comment"),TEXT("发送评论"),28,306);Button(TEXT("QA_Like"),TEXT("点赞 ×10"),316,306);
        Button(TEXT("QA_Follow"),TEXT("关注"),28,366);Button(TEXT("Sword"),TEXT("全体换剑"),316,366);
        const TCHAR* Gifts[]={TEXT("仙女棒"),TEXT("爱的爆炸"),TEXT("稀有宝箱"),TEXT("超能喷射"),TEXT("神秘空投"),TEXT("草莓甜点"),TEXT("超级空投"),TEXT("生命药水")};
        // Keep the event names and Gift0..Gift7 actions intact; two supplied filenames are aliases.
        const TCHAR* Icons[]={TEXT("Wand"),TEXT("LoveExplosion"),TEXT("RareChest"),TEXT("SuperJet"),TEXT("MysteryAirdrop"),TEXT("StrawberryDessert"),TEXT("SuperAirdrop"),TEXT("LifePotion")};
        for(int32 I=0;I<UE_ARRAY_COUNT(Gifts);++I)
        {
            const FString Path=FString::Printf(TEXT("/Game/UI/Ink/Gifts/T_Gift_%s.T_Gift_%s"),Icons[I],Icons[I]);
            Button(FName(*FString::Printf(TEXT("Gift%d"),I)),Gifts[I],28+(I%2)*288,430+(I/2)*80,244,76,*Path,TEXT(" "));
        }
        RefreshGiftDescriptions();
        Text(TEXT("每次赠送 1 份 · 悬停查看完整效果与冷却"),28,750,544,22,14);
    }
    else if(SelectedTab==TEXT("Visual"))
    {
        Text(TEXT("精确伤害比例样本 · 不修改生命"),28,250,540,36,20);
        const TCHAR* Ratios[]={TEXT("19.9"),TEXT("20"),TEXT("25.5"),TEXT("26"),TEXT("41.9"),TEXT("42"),TEXT("60"),TEXT("60.1")};
        for(int32 I=0;I<8;++I)Button(FName(*(FString(TEXT("QA_Damage_"))+Ratios[I])),FString(Ratios[I])+TEXT("%"),28+(I%4)*140,300+(I/4)*62,128);
        Button(TEXT("QA_HitHealth"),TEXT("实际受击 / 残量条"),28,452);Button(TEXT("QA_Heal"),TEXT("恢复生命"),316,452);
        Button(TEXT("SpawnMonsters"),TEXT("生成测试怪物"),28,516);Button(TEXT("ClearMonsters"),TEXT("清除测试怪物"),316,516);
        Button(TEXT("Hitboxes"),TEXT("攻击检测框"),28,580);Button(TEXT("QA_Checks"),TEXT("运行边界检查"),316,580);
        Button(TEXT("QA_PreviewTitle"),TEXT("称号试映"),28,642,172,42);Button(TEXT("QA_PreviewRealm"),TEXT("境界试映"),214,642,172,42);Button(TEXT("QA_PreviewAdvance"),TEXT("晋阶试映"),400,642,172,42);
        Button(TEXT("SampleBars"),TEXT("护盾/修为样本"),28,696,260,42);Button(TEXT("QA_ContinuousDamage"),TEXT("连续小额受击"),316,696,256,42);
        ShieldSample=WidgetTree->ConstructWidget<UInkBufferedProgressWidget>();CultivationSample=WidgetTree->ConstructWidget<UInkBufferedProgressWidget>();
        ShieldSample->FillColor=FLinearColor(.2f,.48f,.78f,1);ShieldSample->SetFraction(1,true);CultivationSample->SetFraction(.2f,true);
        auto* S=Canvas->AddChildToCanvas(ShieldSample);S->SetOffsets(FMargin(28,750,260,12));auto* C=Canvas->AddChildToCanvas(CultivationSample);C->SetOffsets(FMargin(316,750,256,12));
    }
    else if(SelectedTab==TEXT("Layout"))
    {
        const TCHAR* Routes[]={TEXT("SignIn"),TEXT("Activities"),TEXT("Draw"),TEXT("CharacterProfile"),TEXT("CharacterAppearance"),TEXT("ResourceContest")};const TCHAR* Names[]={TEXT("签到页签"),TEXT("活动页签"),TEXT("抽奖网格"),TEXT("真实属性"),TEXT("头像框预览"),TEXT("资源页签")};
        for(int32 I=0;I<6;++I)Button(FName(*(FString(TEXT("QA_Layout_"))+Routes[I])),Names[I],28+(I%2)*288,270+(I/2)*64);
        Text(TEXT("打开页面后按 F10 收起 GM，检查边界。\n自动验收脚本会导出多分辨率截图与断言。\n视觉协调结果须结合截图审阅，未运行不显示通过。"),28,500,540,180,20);
    }
    else
    {
        Button(TEXT("QA_Checks"),TEXT("运行检查"),28,252,172);Button(TEXT("QA_LoadUIReport"),TEXT("读取界面验收"),214,252,172);Button(TEXT("QA_Export"),TEXT("导出 JSON"),400,252,172);
        ResultText=Text(TEXT("尚未运行检查"),28,320,544,430,18);
        ResultText->RemoveFromParent();ResultText->SetWrapTextAt(520);
        auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>();Scroll->SetClipping(EWidgetClipping::ClipToBoundsAlways);Scroll->SetAllowOverscroll(false);Scroll->AddChild(ResultText);
        auto* ResultsLayout=Canvas->AddChildToCanvas(Scroll);ResultsLayout->SetOffsets(FMargin(28,320,544,426));
    }
    Text(TEXT("F10 收起 / 展开 · 开发工具"),28,774,544,30,17);
}
void ULiveGMControlPanel::ExecuteControl(FName Action)
{
    if(!Host.IsValid()||!Host->TestSession)return;auto* Session=Host->TestSession.Get();
    if(ViewerId)Session->ViewerId=ViewerId->GetText().ToString();if(ViewerName)Session->ViewerName=ViewerName->GetText().ToString();if(AvatarUrl)Session->ViewerAvatarUrl=AvatarUrl->GetText().ToString();if(Comment)Session->Comment=Comment->GetText().ToString();
    const FString Id=Action.ToString();if(Id.StartsWith(TEXT("Tab_"))){SelectedTab=FName(*Id.Mid(4));Rebuild();return;}
    if(Action==TEXT("SampleBars"))
    {
        if(!Session->CanMutate()||!ShieldSample||!CultivationSample)return;
        const bool Down=ShieldSample->Fraction>.5f;ShieldSample->SetFraction(Down?.25f:1);CultivationSample->SetFraction(Down?.85f:.2f);
        FLiveGMCheckResult R;R.Name=TEXT("护盾/修为双层样本");R.Expected=TEXT("前景即时，暗层保留上次位置");R.Actual=FString::Printf(TEXT("护盾 %.2f / %.2f，修为 %.2f / %.2f"),ShieldSample->Fraction,ShieldSample->TrailFraction,CultivationSample->Fraction,CultivationSample->TrailFraction);R.Status=FMath::Abs(ShieldSample->Fraction-ShieldSample->TrailFraction)>.1f?TEXT("通过"):TEXT("失败");R.Source=TEXT("本地展示样本");R.Time=FDateTime::UtcNow().ToIso8601();Session->Results.Add(R);
        Host->Feedback=TEXT("护盾与修为展示样本，不修改角色数值");return;
    }
    const bool Applied=Id.StartsWith(TEXT("QA_"))?Session->RunAction(FName(*Id.Mid(3))):Host->ExecuteGMAction(Action);
    Host->Feedback=Applied?TEXT("本地模拟已执行 · ")+Id:TEXT("未执行：本地模拟未就绪或条件不满足");RefreshTime=.2f;
}
void ULiveGMControlPanel::NativeTick(const FGeometry& G,float Delta)
{
    Super::NativeTick(G,Delta);RefreshTime+=Delta;if(RefreshTime<.2f||!Host.IsValid()||!Host->TestSession)return;RefreshTime=0;auto* Session=Host->TestSession.Get();
    FString State=Session->DescribeSession()+FString::Printf(TEXT("\n已接收 %d 个事件 · "),Session->ReceivedEvents)+Session->LastEvent.Left(14);
    if(Host->Swarm)State+=FString::Printf(TEXT("\n剑灵 %d · 称号动效 %d"),Host->Swarm->GetParticipantCount(),Host->Swarm->TitleEffects?Host->Swarm->TitleEffects->ActiveEffectCount:0);
    if(Status)Status->SetText(FText::FromString(State));
    if(SelectedTab==TEXT("Events"))RefreshGiftDescriptions();
    if(ResultText){FString Lines;for(const auto& R:Session->Results)Lines+=R.Status+TEXT(" · ")+R.Name+TEXT("\n预期 ")+R.Expected+TEXT(" / 实测 ")+R.Actual+TEXT("\n来源 ")+R.Source+TEXT(" · ")+R.Time+TEXT("\n\n");if(!Session->ReportPath.IsEmpty())Lines=TEXT("导出：")+Session->ReportPath+TEXT("\n\n")+Lines;if(Lines.IsEmpty())Lines=TEXT("未运行 · 尚无测试结果");ResultText->SetText(FText::FromString(Lines));}
}

void ULiveGMControlPanel::RefreshGiftDescriptions()
{
    const auto* State=GetGameInstance()?GetGameInstance()->GetSubsystem<UCultivationSubsystem>():nullptr;
    if(!State||!WidgetTree)return;
    for(const auto& B:Buttons)
    {
        const FString Action=B->Action.ToString();if(!Action.StartsWith(TEXT("Gift")))continue;
        const auto Description=DescribeGift(FCString::Atoi(*Action.Mid(4)),State->Rules);
        const FText Summary=FText::FromString(Description.Summary),Detail=FText::FromString(Description.Detail);
        if(auto* Effect=Cast<UTextBlock>(WidgetTree->FindWidget(FName(*(Action+TEXT("Description"))))))
            if(!Effect->GetText().EqualTo(Summary))Effect->SetText(Summary);
        if(!B->GetToolTipText().EqualTo(Detail))B->SetToolTipText(Detail);
    }
}
