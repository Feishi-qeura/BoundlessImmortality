#include "InkJourneyNewWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/GridSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"

namespace InkJourneyNew
{
const FLinearColor Ink(.10f,.16f,.14f,1.f), Jade(.16f,.42f,.35f,1.f), Gold(.67f,.48f,.20f,1.f), Paper(.95f,.92f,.83f,.94f), Muted(.35f,.39f,.35f,1.f), Red(.61f,.25f,.20f,1.f);
FSlateFontInfo Font(float Size)
{
 const float SafeSize=FMath::Max(26.f,Size);
 const TCHAR* FontName=SafeSize>=40.f?TEXT("F_InkDisplay"):TEXT("F_InkBody");
 const FString Path=FString::Printf(TEXT("/Game/newUI/Shared/Fonts/%s.%s"),FontName,FontName);
 if(UFont* Asset=LoadObject<UFont>(nullptr,*Path)) return FSlateFontInfo(Asset,SafeSize);
 // Source-only candidate fallback; the newUI fonts must exist before runtime acceptance.
 FSlateFontInfo F=FCoreStyle::GetDefaultFontStyle("Regular",FMath::RoundToInt(SafeSize)); F.Size=FMath::RoundToInt(SafeSize); return F;
}
UTextBlock* Text(UWidgetTree* T,const TCHAR* Name,const FText& Value,float Size,FLinearColor Color=Ink)
{ auto* W=T->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name); W->SetText(Value); W->SetFont(Font(Size)); W->SetColorAndOpacity(FSlateColor(Color)); W->SetAutoWrapText(true); return W; }
void Pad(UWidget* W,float L=0,float T=0,float R=0,float B=0)
{ if(auto* VS=Cast<UVerticalBoxSlot>(W->Slot)) VS->SetPadding(FMargin(L,T,R,B)); else if(auto* HS=Cast<UHorizontalBoxSlot>(W->Slot)) HS->SetPadding(FMargin(L,T,R,B)); }
UBorder* Card(UWidgetTree* T,const TCHAR* Name,UWidget* Child=nullptr)
{ auto* B=T->ConstructWidget<UBorder>(UBorder::StaticClass(),Name); B->SetBrushColor(Paper); B->SetPadding(FMargin(18)); if(Child) B->SetContent(Child); return B; }
UButton* Button(UWidgetTree* T,const TCHAR* Name,const FText& Caption,UTextBlock*& OutLabel)
{ auto* B=T->ConstructWidget<UButton>(UButton::StaticClass(),Name); B->SetBackgroundColor(FLinearColor(.14f,.39f,.32f,1)); OutLabel=Text(T,*FString(Name)+TEXT("Label"),Caption,30,FLinearColor(.98f,.96f,.88f,1)); if(auto* Slot=Cast<UButtonSlot>(B->AddChild(OutLabel))) Slot->SetPadding(FMargin(24,28)); return B; }
UVerticalBox* VBox(UWidgetTree* T,const TCHAR* Name) { return T->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),Name); }
UScrollBox* Scroll(UWidgetTree* T,const TCHAR* Name) { return T->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),Name); }
UWrapBox* Wrap(UWidgetTree* T,const TCHAR* Name) { return T->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(),Name); }
UWidget* WrapItem(UWidgetTree* T,UWrapBox* Parent,UWidget* Content,float Width,const TCHAR* Name)
{ auto* Box=T->ConstructWidget<USizeBox>(USizeBox::StaticClass(),Name); Box->SetWidthOverride(Width); Box->SetContent(Content); if(auto* Slot=Parent->AddChildToWrapBox(Box)) Slot->SetPadding(FMargin(6)); return Box; }
void AddRow(UWidgetTree* T,UScrollBox* To,const FInkJourneyNewDisplayRow& Row)
{ auto* Line=VBox(T,TEXT("JourneyDataRow")); auto* L=Text(T,TEXT("JourneyDataLabel"),Row.Label,17,Muted); Line->AddChild(L); auto* V=Text(T,TEXT("JourneyDataValue"),Row.Value,Row.bEmphasized?24:20,Row.bEmphasized?Gold:Ink); Line->AddChild(V); To->AddChild(Card(T,TEXT("JourneyDataCard"),Line)); }
FInkJourneyNewDisplayRow MakeRow(const FText& Label,const FText& Value,bool bEmphasized=false)
{ FInkJourneyNewDisplayRow R; R.Label=Label; R.Value=Value; R.bEmphasized=bEmphasized; return R; }
 void AddFill(UCanvasPanel* C,UWidget* W)
{ if(auto* S=C->AddChildToCanvas(W)){ S->SetAnchors(FAnchors(0,0,1,1)); S->SetOffsets(FMargin(0)); } }
FText StateLabel(EInkJourneyNewEntryState S) { switch(S){case EInkJourneyNewEntryState::Available:return FText::FromString(TEXT("可前往"));case EInkJourneyNewEntryState::Locked:return FText::FromString(TEXT("未开放"));default:return FText::FromString(TEXT("状态待同步"));} }
}

void UInkJourneyMapPinButton::NativeOnInitialized(){ Super::NativeOnInitialized(); if(!CaptionText){ CaptionText=WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),TEXT("JourneyPinCaption")); CaptionText->SetJustification(ETextJustify::Center); CaptionText->SetFont(InkJourneyNew::Font(18)); CaptionText->SetColorAndOpacity(FSlateColor(InkJourneyNew::Ink)); AddChild(CaptionText); OnClicked.AddUniqueDynamic(this,&UInkJourneyMapPinButton::HandleClicked); } }
void UInkJourneyMapPinButton::Configure(const FString& Id,const FText& Caption,EInkJourneyNewEntryState State,bool bSelected,int32 Count)
{ EntryId=Id; if(CaptionText){ CaptionText->SetText(Count>1?FText::Format(NSLOCTEXT("Journey","GroupedPin","{0} · {1}"),Caption,FText::AsNumber(Count)):Caption); } SetIsEnabled(!Id.IsEmpty()); SetBackgroundColor(bSelected?InkJourneyNew::Gold:(State==EInkJourneyNewEntryState::Locked?FLinearColor(.63f,.63f,.59f,.86f):InkJourneyNew::Paper)); }
void UInkJourneyMapPinButton::HandleClicked(){ if(!EntryId.IsEmpty()) OnEntryPressed.Broadcast(EntryId); }

void UInkJourneyMilestoneButton::NativeOnInitialized(){ Super::NativeOnInitialized(); if(!CaptionText){ auto* Box=WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(),TEXT("MilestoneLabels")); CaptionText=InkJourneyNew::Text(WidgetTree,TEXT("MilestoneCaption"),FText::GetEmpty(),21,InkJourneyNew::Ink); StatusText=InkJourneyNew::Text(WidgetTree,TEXT("MilestoneStatus"),FText::GetEmpty(),16,InkJourneyNew::Muted); Box->AddChild(CaptionText); Box->AddChild(StatusText); AddChild(Box); OnClicked.AddUniqueDynamic(this,&UInkJourneyMilestoneButton::HandleClicked); } }
void UInkJourneyMilestoneButton::Configure(int32 Value,const FText& Caption,const FText& StatusValue,bool bSelected){ Milestone=Value; if(CaptionText) CaptionText->SetText(Caption); if(StatusText) StatusText->SetText(StatusValue); SetBackgroundColor(bSelected?InkJourneyNew::Gold:InkJourneyNew::Paper); }
void UInkJourneyMilestoneButton::HandleClicked(){ OnMilestoneSelected.Broadcast(Milestone); }

void UInkJourneyMapPageWidget::NativeOnInitialized(){ Super::NativeOnInitialized(); if(!WidgetTree) return; if(!WidgetTree->RootWidget) BuildTree(); RefreshDetails(); }
void UInkJourneyMapPageWidget::NativeTick(const FGeometry& G,float Delta){ Super::NativeTick(G,Delta); const FVector2D S=G.GetLocalSize(); if(S.X>0&&S.Y>0) LayoutResponsive(S); }
void UInkJourneyMapPageWidget::BuildTree()
{
 Root=InkJourneyNew::VBox(WidgetTree,TEXT("JourneyPageRoot")); WidgetTree->RootWidget=Root;
 auto* Header=WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("JourneyHeader")); Root->AddChild(Header);
 UTextBlock* BackText=nullptr; auto* Back=InkJourneyNew::Button(WidgetTree,TEXT("JourneyBack"),FText::FromString(TEXT("返回")),BackText); Header->AddChild(Back); Back->OnClicked.AddUniqueDynamic(this,&UInkJourneyMapPageWidget::HandleBackAction);
 PortraitTitleText=InkJourneyNew::Text(WidgetTree,TEXT("JourneyPortraitTitle"),PageTitle(),48); LandscapeTitleText=InkJourneyNew::Text(WidgetTree,TEXT("JourneyLandscapeTitle"),PageTitle(),40); Header->AddChild(PortraitTitleText);
 AuthorityStatusText=InkJourneyNew::Text(WidgetTree,TEXT("JourneyAuthorityStatus"),FText::FromString(TEXT("状态待同步")),18,InkJourneyNew::Muted); Root->AddChild(AuthorityStatusText);
 LayoutSwitcher=WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(),TEXT("JourneyLayoutSwitcher")); Root->AddChild(LayoutSwitcher);
 PortraitLayout=InkJourneyNew::VBox(WidgetTree,TEXT("JourneyPortraitLayout")); LayoutSwitcher->AddChild(PortraitLayout);
 auto* PMapBox=WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("JourneyPortraitMapBox")); PMapBox->SetWidthOverride(435); PMapBox->SetHeightOverride(650); if(auto* PSlot=Cast<UVerticalBoxSlot>(PortraitLayout->AddChild(PMapBox))) PSlot->SetHorizontalAlignment(HAlign_Center);
 auto* PMap=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("JourneyPortraitMap")); PMapBox->SetContent(PMap); PortraitMapCanvas=PMap;
 PortraitMapImage=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("JourneyPortraitMapArt")); InkJourneyNew::AddFill(PMap,PortraitMapImage);
 PortraitDetails=InkJourneyNew::VBox(WidgetTree,TEXT("JourneyPortraitDetails")); PortraitLayout->AddChild(InkJourneyNew::Card(WidgetTree,TEXT("JourneyPortraitDetailFrame"),PortraitDetails));
 PortraitSelectedTitleText=InkJourneyNew::Text(WidgetTree,TEXT("JourneyPortraitSelectedTitle"),FText::FromString(TEXT("选择地图节点")),30); PortraitDetails->AddChild(PortraitSelectedTitleText);
 PortraitDetailRows=InkJourneyNew::Scroll(WidgetTree,TEXT("JourneyPortraitRows")); PortraitDetails->AddChild(PortraitDetailRows);
 LandscapeLayout=WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("JourneyLandscapeLayout")); LayoutSwitcher->AddChild(LandscapeLayout);
 auto* LMapBox=WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("JourneyLandscapeMapBox")); LMapBox->SetWidthOverride(720); LMapBox->SetHeightOverride(480); LandscapeLayout->AddChild(LMapBox);
 auto* LMap=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("JourneyLandscapeMap")); LMapBox->SetContent(LMap); LandscapeMapCanvas=LMap;
 LandscapeMapImage=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("JourneyLandscapeMapArt")); InkJourneyNew::AddFill(LMap,LandscapeMapImage);
 LandscapeDetails=InkJourneyNew::VBox(WidgetTree,TEXT("JourneyLandscapeDetails")); auto* LDetailBox=WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("JourneyLandscapeDetailBox")); LDetailBox->SetWidthOverride(488); LDetailBox->SetHeightOverride(610); LDetailBox->SetContent(InkJourneyNew::Card(WidgetTree,TEXT("JourneyLandscapeDetailFrame"),LandscapeDetails)); LandscapeLayout->AddChild(LDetailBox);
 LandscapeTitleText=InkJourneyNew::Text(WidgetTree,TEXT("JourneyLandscapeDetailsTitle"),PageTitle(),40); LandscapeDetails->AddChild(LandscapeTitleText);
 LandscapeDetailRows=InkJourneyNew::Scroll(WidgetTree,TEXT("JourneyLandscapeRows")); LandscapeDetails->AddChild(LandscapeDetailRows);
 UTextBlock* PL=nullptr; PortraitActionButton=InkJourneyNew::Button(WidgetTree,TEXT("JourneyPortraitAction"),FText::FromString(TEXT("选择节点")),PL); PortraitActionText=PL; Root->AddChild(PortraitActionButton); PortraitActionButton->OnClicked.AddUniqueDynamic(this,&UInkJourneyMapPageWidget::HandlePrimaryAction);
 UTextBlock* LL=nullptr; LandscapeActionButton=InkJourneyNew::Button(WidgetTree,TEXT("JourneyLandscapeAction"),FText::FromString(TEXT("选择节点")),LL); LandscapeActionText=LL; LandscapeDetails->AddChild(LandscapeActionButton); LandscapeActionButton->OnClicked.AddUniqueDynamic(this,&UInkJourneyMapPageWidget::HandlePrimaryAction);
}
void UInkJourneyMapPageWidget::BuildMapPins(UCanvasPanel* Canvas,TArray<TObjectPtr<UInkJourneyMapPinButton>>& OutPins,float W,float H,bool bLandscape)
{
 if(!Canvas) return; Canvas->ClearChildren(); OutPins.Reset(); UImage* Art=bLandscape?LandscapeMapImage:PortraitMapImage;
 const TCHAR* AssetName=bLandscape?TEXT("T_NUI_JourneyMapLandscape_V1"):TEXT("T_NUI_JourneyMapPortrait_V1"); FString Path=FString::Printf(TEXT("/Game/newUI/Textures/Journey/%s.%s"),AssetName,AssetName); if(UTexture2D* Tex=LoadObject<UTexture2D>(nullptr,*Path)) Art->SetBrushFromTexture(Tex,true);
 InkJourneyNew::AddFill(Canvas,Art);
 TMap<FString,int32> GroupCounts; for(const auto& E:Entries) ++GroupCounts.FindOrAdd(EntryGroup(E));
 TSet<FString> AddedGroups;
 const auto* Selected=FindSelectedEntry();
 for(const auto& E:Entries){ const FString Group=EntryGroup(E); if(IsResourceMap()&&AddedGroups.Contains(Group)) continue; AddedGroups.Add(Group); const bool bGroupSelected=Selected&&(IsResourceMap()?EntryGroup(*Selected)==Group:Selected->EntryId==E.EntryId); auto* Pin=WidgetTree->ConstructWidget<UInkJourneyMapPinButton>(UInkJourneyMapPinButton::StaticClass(),*FString::Printf(TEXT("JourneyPin_%s"),*E.EntryId)); if(auto* Slot=Canvas->AddChildToCanvas(Pin)){ FVector2D P(FMath::Clamp(E.NormalizedPosition.X,0.f,1.f)*W,FMath::Clamp(E.NormalizedPosition.Y,0.f,1.f)*H); Slot->SetPosition(P); Slot->SetSize(FVector2D(130,88)); Slot->SetAlignment(FVector2D(.5f,.5f)); } Pin->Configure(E.EntryId,E.Name,E.State,bGroupSelected,GroupCounts.FindRef(Group)); Pin->OnEntryPressed.AddUniqueDynamic(this,&UInkJourneyMapPageWidget::HandlePinSelected); OutPins.Add(Pin); }
}
void UInkJourneyMapPageWidget::SetAuthorityData(const FText& State,const TArray<FInkJourneyNewMapEntry>& InEntries,const FString& InSelected)
{ const bool bChanged=SelectedEntryId!=InSelected; Entries=InEntries; AuthorityState=State; SelectedEntryId=InSelected; if(bChanged){bConfirmationOpen=false;if(!bRequestPending)TravelFeedback=FText::GetEmpty();} if(AuthorityStatusText) AuthorityStatusText->SetText(State); BuildMapPins(PortraitMapCanvas,PortraitPins,435,650,false); BuildMapPins(LandscapeMapCanvas,LandscapePins,720,480,true); RefreshDetails(); }
void UInkJourneyMapPageWidget::SetTravelFeedback(const FText& Feedback,bool bPending){ TravelFeedback=Feedback;bRequestPending=bPending;if(!bPending)bConfirmationOpen=false;RefreshDetails(); }
void UInkJourneyMapPageWidget::ResetInteractionForReopen(){bConfirmationOpen=false;if(!bRequestPending)TravelFeedback=FText::GetEmpty();RefreshDetails();}
const FInkJourneyNewMapEntry* UInkJourneyMapPageWidget::FindSelectedEntry() const { return Entries.FindByPredicate([this](const auto& E){return E.EntryId==SelectedEntryId;}); }
FString UInkJourneyMapPageWidget::EntryGroup(const FInkJourneyNewMapEntry& E) const { return IsResourceMap()?E.GroupId:E.EntryId; }
void UInkJourneyMapPageWidget::RefreshDetails()
{
 const auto* E=FindSelectedEntry(); auto Refresh=[&](UScrollBox* ScrollBox,UTextBlock* Title,UButton* Action,UTextBlock* ActionLabel){ if(!ScrollBox)return; ScrollBox->ClearChildren(); if(Title)Title->SetText(E?E->Name:PageTitle()); if(E&&IsResourceMap()){ for(const auto& Candidate:Entries){if(EntryGroup(Candidate)!=EntryGroup(*E))continue;auto* Option=WidgetTree->ConstructWidget<UInkJourneyMapPinButton>(UInkJourneyMapPinButton::StaticClass(),*FString::Printf(TEXT("ResourceEntry_%s"),*Candidate.EntryId));ScrollBox->AddChild(Option);Option->Configure(Candidate.EntryId,Candidate.Name,Candidate.State,Candidate.EntryId==SelectedEntryId,1);Option->OnEntryPressed.AddUniqueDynamic(this,&UInkJourneyMapPageWidget::HandlePinSelected);} } if(E){ for(const auto& Row:E->Details) InkJourneyNew::AddRow(WidgetTree,ScrollBox,Row); if(E->Details.Num()==0) InkJourneyNew::AddRow(WidgetTree,ScrollBox,InkJourneyNew::MakeRow(FText::FromString(TEXT("详情")),FText::FromString(TEXT("等待权威数据")))); } else InkJourneyNew::AddRow(WidgetTree,ScrollBox,InkJourneyNew::MakeRow(FText::FromString(TEXT("地图状态")),AuthorityState.IsEmpty()?FText::FromString(TEXT("选择地图节点")):AuthorityState)); const bool bCan=E&&E->State==EInkJourneyNewEntryState::Available&&!bRequestPending; Action->SetIsEnabled(bCan); ActionLabel->SetText(bRequestPending?FText::FromString(TEXT("请求处理中")):bConfirmationOpen?FText::FromString(TEXT("确认前往")):bCan?FText::FromString(TEXT("前往此处")):E?E->StateText:FText::FromString(TEXT("选择节点"))); if(!TravelFeedback.IsEmpty()) InkJourneyNew::AddRow(WidgetTree,ScrollBox,InkJourneyNew::MakeRow(FText::FromString(TEXT("请求结果")),TravelFeedback)); };
 if(PortraitSelectedTitleText) PortraitSelectedTitleText->SetText(E?E->Name:PageTitle()); Refresh(PortraitDetailRows,nullptr,PortraitActionButton,PortraitActionText); Refresh(LandscapeDetailRows,LandscapeTitleText,LandscapeActionButton,LandscapeActionText);
}
void UInkJourneyMapPageWidget::LayoutResponsive(const FVector2D& Size){const bool bNow=Size.X>Size.Y;if(!LayoutSwitcher|| (bHasLayout&&bPortrait==!bNow))return;bHasLayout=true;bPortrait=!bNow;LayoutSwitcher->SetActiveWidget(bNow?LandscapeLayout:PortraitLayout);}
void UInkJourneyMapPageWidget::HandlePinSelected(const FString& Id){SelectedEntryId=Id;bConfirmationOpen=false;TravelFeedback=FText::GetEmpty();BuildMapPins(PortraitMapCanvas,PortraitPins,435,650,false);BuildMapPins(LandscapeMapCanvas,LandscapePins,720,480,true);RefreshDetails();OnEntrySelected.Broadcast(Id);}
void UInkJourneyMapPageWidget::HandlePrimaryAction(){const auto* E=FindSelectedEntry();if(!E||E->State!=EInkJourneyNewEntryState::Available||bRequestPending)return;if(!bConfirmationOpen){bConfirmationOpen=true;RefreshDetails();return;}bConfirmationOpen=false;bRequestPending=true;RefreshDetails();OnTravelRequested.Broadcast(E->EntryId);}
void UInkJourneyMapPageWidget::HandleBackAction(){OnBackRequested.Broadcast();}

void UInkJourneyDailySignInPageWidget::NativeOnInitialized(){Super::NativeOnInitialized();if(!WidgetTree->RootWidget)BuildTree();RefreshPresentation();}
void UInkJourneyDailySignInPageWidget::BuildTree()
{
 auto* Root=InkJourneyNew::VBox(WidgetTree,TEXT("DailySignInRoot")); WidgetTree->RootWidget=Root;
 auto* Head=WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("DailySignInHeader")); Root->AddChild(Head);
 UTextBlock* BackLabel=nullptr; BackButton=InkJourneyNew::Button(WidgetTree,TEXT("DailySignInBack"),FText::FromString(TEXT("返回")),BackLabel); Head->AddChild(BackButton); BackButton->OnClicked.AddUniqueDynamic(this,&UInkJourneyDailySignInPageWidget::HandleBack);
 Head->AddChild(InkJourneyNew::Text(WidgetTree,TEXT("DailySignInTitle"),FText::FromString(TEXT("每日签到")),48));
 ContentWrap=InkJourneyNew::Wrap(WidgetTree,TEXT("DailySignInResponsiveWrap")); Root->AddChild(ContentWrap);
 DayText=InkJourneyNew::Text(WidgetTree,TEXT("DailySignInDay"),FText::GetEmpty(),32,InkJourneyNew::Gold); InkJourneyNew::WrapItem(WidgetTree,ContentWrap,Card(WidgetTree,TEXT("DailySignInDayCard"),DayText),600,TEXT("DailySignInDayWidth"));
 StatusText=InkJourneyNew::Text(WidgetTree,TEXT("DailySignInStatus"),FText::GetEmpty(),26); InkJourneyNew::WrapItem(WidgetTree,ContentWrap,Card(WidgetTree,TEXT("DailySignInStatusCard"),StatusText),600,TEXT("DailySignInStatusWidth"));
 auto* VersePanel=InkJourneyNew::VBox(WidgetTree,TEXT("DailyVersePanel")); VersePanel->AddChild(InkJourneyNew::Text(WidgetTree,TEXT("DailyVerseHeading"),FText::FromString(TEXT("每日剑诀")),32,InkJourneyNew::Gold)); VerseText=InkJourneyNew::Text(WidgetTree,TEXT("DailyVerse"),FText::GetEmpty(),28); VersePanel->AddChild(VerseText); InkJourneyNew::WrapItem(WidgetTree,ContentWrap,Card(WidgetTree,TEXT("DailyVerseCard"),VersePanel),600,TEXT("DailyVerseWidth"));
 MonthCountText=InkJourneyNew::Text(WidgetTree,TEXT("MonthCount"),FText::GetEmpty(),26); InkJourneyNew::WrapItem(WidgetTree,ContentWrap,Card(WidgetTree,TEXT("MonthCountCard"),MonthCountText),600,TEXT("MonthCountWidth"));
 UTextBlock* ClaimLabel=nullptr; ClaimButton=InkJourneyNew::Button(WidgetTree,TEXT("DailySignInClaim"),FText::FromString(TEXT("签到")),ClaimLabel); ClaimButtonText=ClaimLabel; InkJourneyNew::WrapItem(WidgetTree,ContentWrap,ClaimButton,600,TEXT("DailyClaimWidth")); ClaimButton->OnClicked.AddUniqueDynamic(this,&UInkJourneyDailySignInPageWidget::HandleClaim);
 UTextBlock* MonthLabel=nullptr; MonthlyButton=InkJourneyNew::Button(WidgetTree,TEXT("OpenMonthlySignIn"),FText::FromString(TEXT("查看月签到")),MonthLabel); InkJourneyNew::WrapItem(WidgetTree,ContentWrap,MonthlyButton,600,TEXT("MonthlyOpenWidth")); MonthlyButton->OnClicked.AddUniqueDynamic(this,&UInkJourneyDailySignInPageWidget::HandleOpenMonthly);
}
void UInkJourneyDailySignInPageWidget::SetPresentation(const FString& Day,EInkJourneyNewClaimState State,const FText& InStatus,bool HasCount,int32 Count,bool HasVerse,const FText& InVerse){ServerDay=Day;ClaimState=State;ClaimStatus=InStatus;bHasMonthCount=HasCount;MonthCount=Count;bVerseProvided=HasVerse;Verse=InVerse;RefreshPresentation();}
void UInkJourneyDailySignInPageWidget::SetClaimFeedback(const FText& Feedback,bool Pending){ClaimFeedback=Feedback;bLocalPending=Pending;RefreshPresentation();}
void UInkJourneyDailySignInPageWidget::RefreshPresentation(){if(DayText)DayText->SetText(ServerDay.IsEmpty()?FText::FromString(TEXT("服务日期待同步")):FText::FromString(ServerDay));if(StatusText)StatusText->SetText(!ClaimFeedback.IsEmpty()?ClaimFeedback:ClaimStatus);if(VerseText){VerseText->SetText(bVerseProvided?Verse:FText::FromString(TEXT("每日剑诀暂未提供")));VerseText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);}if(MonthCountText)MonthCountText->SetText(bHasMonthCount?FText::Format(NSLOCTEXT("Journey","MonthSigned","本月已签到 {0} 天"),FText::AsNumber(MonthCount)):FText::FromString(TEXT("本月签到数待同步")));if(ClaimButton){const bool bCan=ClaimState==EInkJourneyNewClaimState::Available&&!bLocalPending;ClaimButton->SetIsEnabled(bCan);ClaimButtonText->SetText(bLocalPending?FText::FromString(TEXT("提交中")):ClaimState==EInkJourneyNewClaimState::Claimed?FText::FromString(TEXT("今日已签到")):bCan?FText::FromString(TEXT("确认签到")):ClaimStatus.IsEmpty()?FText::FromString(TEXT("签到状态待同步")):ClaimStatus);}}
void UInkJourneyDailySignInPageWidget::HandleBack(){OnBackRequested.Broadcast();}void UInkJourneyDailySignInPageWidget::HandleClaim(){if(ClaimState==EInkJourneyNewClaimState::Available&&!bLocalPending){bLocalPending=true;RefreshPresentation();OnClaimRequested.Broadcast(ServerDay);}}void UInkJourneyDailySignInPageWidget::HandleOpenMonthly(){OnMonthlySignInRequested.Broadcast();}

void UInkJourneyMonthlySignInPageWidget::NativeOnInitialized(){Super::NativeOnInitialized();if(!WidgetTree->RootWidget)BuildTree();RefreshPresentation();}
void UInkJourneyMonthlySignInPageWidget::BuildTree()
{
 auto* Root=InkJourneyNew::VBox(WidgetTree,TEXT("MonthlySignInRoot")); WidgetTree->RootWidget=Root;
 auto* Head=WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("MonthlySignInHeader")); Root->AddChild(Head);
 UTextBlock* BackLabel=nullptr; BackButton=InkJourneyNew::Button(WidgetTree,TEXT("MonthlySignInBack"),FText::FromString(TEXT("返回")),BackLabel); Head->AddChild(BackButton); BackButton->OnClicked.AddUniqueDynamic(this,&UInkJourneyMonthlySignInPageWidget::HandleBack);
 MonthText=InkJourneyNew::Text(WidgetTree,TEXT("MonthlySignInMonth"),FText::GetEmpty(),48); Head->AddChild(MonthText);
 CountText=InkJourneyNew::Text(WidgetTree,TEXT("MonthlySignInCount"),FText::GetEmpty(),28); Root->AddChild(CountText);
 CalendarStateText=InkJourneyNew::Text(WidgetTree,TEXT("MonthlyCalendarEvidence"),FText::FromString(TEXT("仅显示服务提供的日期状态；未提供的历史保持未知")),26,InkJourneyNew::Muted); Root->AddChild(CalendarStateText);
 StatusText=InkJourneyNew::Text(WidgetTree,TEXT("MonthlySignInStatus"),FText::GetEmpty(),26); Root->AddChild(StatusText);
 ContentWrap=InkJourneyNew::Wrap(WidgetTree,TEXT("MonthlySignInResponsiveWrap")); Root->AddChild(ContentWrap);
 CalendarGrid=WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(),TEXT("MonthlyCalendarGrid")); InkJourneyNew::WrapItem(WidgetTree,ContentWrap,Card(WidgetTree,TEXT("MonthlyCalendarFrame"),CalendarGrid),660,TEXT("MonthlyCalendarWidth"));
 auto* MilestonePanel=InkJourneyNew::VBox(WidgetTree,TEXT("MonthlyMilestonePanel")); InkJourneyNew::WrapItem(WidgetTree,ContentWrap,MilestonePanel,520,TEXT("MonthlyMilestoneWidth"));
 MilestoneText=InkJourneyNew::Text(WidgetTree,TEXT("MonthlyMilestoneTitle"),FText::FromString(TEXT("月度奖励")),32); MilestonePanel->AddChild(MilestoneText);
 MilestoneList=InkJourneyNew::VBox(WidgetTree,TEXT("MonthlyMilestoneList")); MilestonePanel->AddChild(MilestoneList);
 UTextBlock* Confirm=nullptr; ConfirmButton=InkJourneyNew::Button(WidgetTree,TEXT("MonthlyMilestoneClaim"),FText::FromString(TEXT("选择可领取奖励")),Confirm); ConfirmText=Confirm; MilestonePanel->AddChild(ConfirmButton); ConfirmButton->OnClicked.AddUniqueDynamic(this,&UInkJourneyMonthlySignInPageWidget::HandleConfirmClaim);
}
void UInkJourneyMonthlySignInPageWidget::SetPresentation(const FString& InExpectedServiceDay,const FString& Month,int32 Days,int32 Today,bool HasCount,int32 Count,const TArray<FInkJourneyNewMilestone>& InMilestones,const FText& InStatus){ExpectedServiceDay=InExpectedServiceDay;ServerMonth=Month;CalendarDayCount=FMath::Clamp(Days,0,31);ServerDay=FMath::Clamp(Today,0,31);bHasMonthCount=HasCount;MonthCount=Count;Milestones=InMilestones;Status=InStatus;RefreshPresentation();}
void UInkJourneyMonthlySignInPageWidget::SetMilestoneFeedback(const FText& InFeedback,bool Pending){Feedback=InFeedback;bRequestPending=Pending;if(!Pending)bConfirmationOpen=false;RefreshPresentation();}
void UInkJourneyMonthlySignInPageWidget::ResetConfirmationForReopen(){if(!bRequestPending)bConfirmationOpen=false;RefreshPresentation();}
void UInkJourneyMonthlySignInPageWidget::RefreshPresentation(){if(MonthText)MonthText->SetText(ServerMonth.IsEmpty()?FText::FromString(TEXT("月份待同步")):FText::FromString(ServerMonth));if(CountText)CountText->SetText(bHasMonthCount?FText::Format(NSLOCTEXT("Journey","MonthSignCount","累计签到 {0} 天"),FText::AsNumber(MonthCount)):FText::FromString(TEXT("累计签到数待同步")));if(StatusText)StatusText->SetText(!Feedback.IsEmpty()?Feedback:Status);if(CalendarGrid){CalendarGrid->ClearChildren();for(int32 D=1;D<=CalendarDayCount;++D){auto* Cell=InkJourneyNew::Text(WidgetTree,*FString::Printf(TEXT("MonthDay_%02d"),D),FText::AsNumber(D),26,D==ServerDay?InkJourneyNew::Gold:InkJourneyNew::Ink);auto* CellBox=WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),*FString::Printf(TEXT("MonthDayBox_%02d"),D));CellBox->SetWidthOverride(88);CellBox->SetHeightOverride(88);CellBox->SetContent(Cell);if(auto* S=CalendarGrid->AddChildToUniformGrid(CellBox,(D-1)/7,(D-1)%7))S->SetPadding(FMargin(2));}}RefreshMilestones();}
void UInkJourneyMonthlySignInPageWidget::RefreshMilestones(){if(!MilestoneList)return;MilestoneList->ClearChildren();MilestoneButtons.Reset();for(const auto& M:Milestones){auto* B=WidgetTree->ConstructWidget<UInkJourneyMilestoneButton>(UInkJourneyMilestoneButton::StaticClass(),*FString::Printf(TEXT("Milestone_%d"),M.Milestone));FText Caption=M.bKnown?FText::Format(NSLOCTEXT("Journey","MilestoneLabel","累计 {0} 天"),FText::AsNumber(M.Milestone)):FText::FromString(TEXT("奖励条件待同步"));MilestoneList->AddChild(Card(WidgetTree,TEXT("MonthlyMilestoneCard"),B));B->Configure(M.Milestone,Caption,M.Status,M.Milestone==SelectedMilestone);B->OnMilestoneSelected.AddUniqueDynamic(this,&UInkJourneyMonthlySignInPageWidget::HandleMilestoneSelected);B->SetIsEnabled(M.bKnown);MilestoneButtons.Add(B);}const auto* Selected=Milestones.FindByPredicate([this](const auto& M){return M.Milestone==SelectedMilestone;});const bool bCan=Selected&&Selected->bKnown&&Selected->bEligible&&!Selected->bClaimed&&!bRequestPending;if(ConfirmButton){ConfirmButton->SetIsEnabled(bCan||bConfirmationOpen);ConfirmText->SetText(bRequestPending?FText::FromString(TEXT("提交中")):bConfirmationOpen?FText::FromString(TEXT("确认领取")):bCan?FText::FromString(TEXT("领取此档奖励")):FText::FromString(TEXT("选择可领取奖励")));}}
void UInkJourneyMonthlySignInPageWidget::HandleBack(){OnBackRequested.Broadcast();}void UInkJourneyMonthlySignInPageWidget::HandleMilestoneSelected(int32 M){SelectedMilestone=M;bConfirmationOpen=false;RefreshMilestones();}
void UInkJourneyMonthlySignInPageWidget::HandleConfirmClaim(){const auto* M=Milestones.FindByPredicate([this](const auto& V){return V.Milestone==SelectedMilestone;});if(!M||!M->bKnown||!M->bEligible||M->bClaimed||bRequestPending)return;if(!bConfirmationOpen){bConfirmationOpen=true;RefreshMilestones();return;}bConfirmationOpen=false;bRequestPending=true;RefreshMilestones();OnMilestoneClaimRequested.Broadcast(ExpectedServiceDay,SelectedMilestone);}

void UInkJourneyDetailResultPageWidget::NativeOnInitialized(){Super::NativeOnInitialized();if(!WidgetTree->RootWidget)BuildTree();Refresh();}
void UInkJourneyDetailResultPageWidget::BuildTree(){auto* Root=InkJourneyNew::VBox(WidgetTree,TEXT("JourneyResultRoot"));WidgetTree->RootWidget=Root;auto* Head=WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(),TEXT("JourneyResultHeader"));Root->AddChild(Head);UTextBlock* BackLabel=nullptr;BackButton=InkJourneyNew::Button(WidgetTree,TEXT("JourneyResultBack"),FText::FromString(TEXT("返回")),BackLabel);Head->AddChild(BackButton);BackButton->OnClicked.AddUniqueDynamic(this,&UInkJourneyDetailResultPageWidget::HandleBack);TitleText=InkJourneyNew::Text(WidgetTree,TEXT("JourneyResultTitle"),FText::GetEmpty(),32);Head->AddChild(TitleText);StatusText=InkJourneyNew::Text(WidgetTree,TEXT("JourneyResultStatus"),FText::GetEmpty(),20);Root->AddChild(StatusText);RowsScroll=InkJourneyNew::Scroll(WidgetTree,TEXT("JourneyResultRows"));Root->AddChild(RowsScroll);UTextBlock* Label=nullptr;ActionButton=InkJourneyNew::Button(WidgetTree,TEXT("JourneyResultAction"),FText::FromString(TEXT("继续")),Label);ActionText=Label;Root->AddChild(ActionButton);ActionButton->OnClicked.AddUniqueDynamic(this,&UInkJourneyDetailResultPageWidget::HandlePrimary);}
void UInkJourneyDetailResultPageWidget::SetPageData(const FText& InTitle,const FString& Source,const FText& InStatus,const TArray<FInkJourneyNewDisplayRow>& InRows,bool CanTravel){const bool bChanged=ExactSourceId!=Source;Title=InTitle;ExactSourceId=Source;Status=InStatus;Rows=InRows;bCanTravel=CanTravel;if(bChanged){bConfirmationOpen=false;if(!bPending)Feedback=FText::GetEmpty();}Refresh();}
void UInkJourneyDetailResultPageWidget::SetTravelFeedback(const FText& InFeedback,bool Pending){Feedback=InFeedback;bPending=Pending;if(!Pending)bConfirmationOpen=false;Refresh();}
void UInkJourneyDetailResultPageWidget::ResetForReopen(){bConfirmationOpen=false;if(!bPending)Feedback=FText::GetEmpty();Refresh();}
void UInkJourneyDetailResultPageWidget::Refresh(){if(TitleText)TitleText->SetText(Title);if(StatusText)StatusText->SetText(!Feedback.IsEmpty()?Feedback:Status);if(RowsScroll){RowsScroll->ClearChildren();for(const auto& Row:Rows)InkJourneyNew::AddRow(WidgetTree,RowsScroll,Row);if(!ExactSourceId.IsEmpty())InkJourneyNew::AddRow(WidgetTree,RowsScroll,InkJourneyNew::MakeRow(FText::FromString(TEXT("来源")),FText::FromString(ExactSourceId)));}if(ActionButton){ActionButton->SetIsEnabled(bCanTravel&&!bPending);ActionText->SetText(bPending?FText::FromString(TEXT("处理中")):bConfirmationOpen?FText::FromString(TEXT("确认继续")):FText::FromString(TEXT("继续")));}}
void UInkJourneyDetailResultPageWidget::HandleBack(){OnBackRequested.Broadcast();}void UInkJourneyDetailResultPageWidget::HandlePrimary(){if(!bCanTravel||bPending)return;if(!bConfirmationOpen){bConfirmationOpen=true;Refresh();return;}bConfirmationOpen=false;bPending=true;Refresh();OnTravelRequested.Broadcast(ExactSourceId);}










