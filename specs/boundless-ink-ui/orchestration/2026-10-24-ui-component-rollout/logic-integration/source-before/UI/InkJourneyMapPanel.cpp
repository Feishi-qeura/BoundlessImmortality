#include "InkJourneyMapPanel.h"
#include "InkJourneyData.h"
#include "Frontend/InkPresentationStyle.h"
#include "InkUiTheme.h"
#include "Frontend/InkFrontendWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ButtonSlot.h"
#include "Components/ScrollBox.h"
#include "Engine/Texture2D.h"

FText UInkJourneyMapPanel::PanelTitle()const{return FText::FromString(TEXT("仙途 · 山海寻道"));}

void UInkJourneyMapPanel::BuildPanelContent()
{
    using namespace WulingFrontendData;
    if(!bTabChosen)bResourceMap=PageId==TEXT("ResourceMap");
    const float TabW=(BodyWidth-12)/2;
    Action(BodyCanvas,TEXT("AtlasWorld"),TEXT("世界修炼"),0,0,TabW,88,false,false,!bResourceMap);
    Action(BodyCanvas,TEXT("AtlasResource"),TEXT("资源争夺"),TabW+12,0,TabW,88,false,false,bResourceMap);
    const bool Wide=BodyWidth>=850;
    const float MapY=110;
    const float MapW=Wide?FMath::Min(BodyWidth*.56f,FMath::Max(400.f,(BodyHeight-MapY-8)*4/3)):BodyWidth,MapH=MapW*.75f;
    auto* Map=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("JourneyMapArtwork"));
    auto* Texture=bResourceMap?ResourceMapTexture.LoadSynchronous():WorldMapTexture.LoadSynchronous();
    if(!Texture)Texture=LoadObject<UTexture2D>(nullptr,bResourceMap?TEXT("/Game/UI/Ink/Textures/Journey/T_ResourceAtlas_v1.T_ResourceAtlas_v1"):TEXT("/Game/UI/Ink/Textures/Journey/T_WorldAtlas_v1.T_WorldAtlas_v1"));
    Map->SetBrushFromTexture(Texture);Map->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* M=BodyCanvas->AddChildToCanvas(Map);M->SetPosition({0,MapY});M->SetSize({MapW,MapH});
    const auto Entries=InkJourneyData::Rows(this,bResourceMap);
    TMap<FString,int32> TypeCount;int32 LockedPins=0;
    for(const auto& Entry:Entries)
    {
        const FString Id=String(Entry,TEXT("id")),Node=String(Entry,TEXT("node_id"));
        const bool Available=InkJourneyData::Open(Entry);
        if(!Available&&LockedPins++>=2)continue;
        // Extra resource instances are listed separately below; the pin opens their first entry.
        if(bResourceMap&&TypeCount.FindOrAdd(Node)++>0)continue;
        const float PinW=FMath::Min(152.f,MapW*.255f),PinH=88;
        const float X=FMath::Clamp(float(InkJourneyData::Number(Entry,TEXT("x")))*MapW-PinW/2,0.f,MapW-PinW);
        const float MapPositionY=MapH<360&&Id==TEXT("xuantie")?.17f:float(InkJourneyData::Number(Entry,TEXT("y")));
        const float Y=MapY+FMath::Clamp(MapPositionY*MapH-PinH/2,0.f,MapH-PinH);
        auto* Pin=Action(BodyCanvas,TEXT("AtlasNode_")+Id,TEXT(""),X,Y,PinW,PinH);
        auto Normal=Pin->GetStyle();auto Brush=Normal.Normal,Hover=Normal.Hovered,Pressed=Normal.Pressed;
        Brush.TintColor=Available?FLinearColor(.80f,.89f,.82f,1):FLinearColor(.79f,.79f,.74f,1);
        Brush.OutlineSettings.Color=Available?InkPresentationStyle::Jade():FLinearColor(.50f,.53f,.48f,1);
        Brush.OutlineSettings.Width=Available?1.75f:1.f;
        Hover.TintColor=Available?FLinearColor(.88f,.94f,.87f,1):FLinearColor(.85f,.85f,.80f,1);
        Pressed.TintColor=Available?FLinearColor(.72f,.83f,.75f,1):FLinearColor(.72f,.72f,.68f,1);
        Normal.SetNormal(Brush).SetHovered(Hover).SetPressed(Pressed);Pin->SetStyle(Normal);
        auto* Text=Cast<UTextBlock>(Pin->GetContent());
        Text->SetText(FText::FromString(String(Entry,TEXT("name"))+(Available?TEXT("\n查看详情"):TEXT("\n未开放"))));Text->SetFont(InkUiTypography::MakeFont(EInkUiTextRole::Button,24));
        Text->SetColorAndOpacity(Available?InkPresentationStyle::Ink():FLinearColor(.34f,.37f,.34f,1));Text->SetAutoWrapText(false);
        CastChecked<UCanvasPanelSlot>(Pin->Slot)->SetZOrder(7);Pin->SetToolTipText(FText::FromString(Available?TEXT("查看地点与前往条件"):TEXT("未开放区域，无法前往")));
    }
    const float SX=Wide?MapW+24:0,SY=Wide?MapY:MapY+MapH+24,SW=BodyWidth-SX;
    const float SideHeight=bResourceMap?FMath::Max(360.f,170.f+Entries.Num()*198.f+312.f):FMath::Max(520.f,540.f+Entries.Num()*72.f);
    Surface(BodyCanvas,TEXT("AtlasDetailsPaper"),InkPresentationStyle::ESurface::FeatureCard,SX,SY,SW,SideHeight);
    Label(BodyCanvas,TEXT("AtlasSection"),bResourceMap?TEXT("灵脉竞采"):TEXT("山海行旅"),SX,SY,SW,48,32);
    Label(BodyCanvas,TEXT("AtlasStatus"),InkJourneyData::Ready(this)?(bResourceMap?TEXT("点击灵脉，查看产出与归属主播"):TEXT("点击地点，查看难度与奖励")):TEXT("尚未连接 · 可浏览地图\n前往条件与数量等待同步"),SX,SY+60,SW,92,24);
    float RowY=SY+170;
    if(bResourceMap)
    {
        if(Entries.IsEmpty())Label(BodyCanvas,TEXT("ResourceAtlasEmpty"),TEXT("资源数量尚未同步\n连接后按节点参数生成可进入的地点。"),SX,RowY,SW,100,26);
        for(const auto& Entry:Entries)
        {
            const FString Caption=String(Entry,TEXT("name"))+(Number(Entry,TEXT("entry_count"))>1?FString::Printf(TEXT(" · %d"),Number(Entry,TEXT("index"))):TEXT(""));
            Action(BodyCanvas,TEXT("AtlasEntry_")+String(Entry,TEXT("id")),Caption,SX+12,RowY,SW-24,88,false,true);
            Label(BodyCanvas,NAME_None,InkJourneyData::Yield(Entry)+TEXT("\n归属：")+InkJourneyData::Owners(Entry),SX,RowY+98,SW,80,24);RowY+=198;
        }
        Action(BodyCanvas,TEXT("Open_WulingRights"),TEXT("查看开采权益"),SX,RowY+8,SW,88);
        Action(BodyCanvas,TEXT("Open_ResourceContest"),TEXT("进入灵脉竞采中心"),SX,RowY+112,SW,88,true);
        Action(BodyCanvas,TEXT("Open_ResourceRanking"),TEXT("查看资源贡献榜"),SX,RowY+216,SW,88,true);
    }
    else
    {
        Label(BodyCanvas,TEXT("AtlasLegend"),TEXT("青玉地点 · 可查看服务条件\n墨隐地点 · 尚未开放\n历练奖励与阶段以服务数据为准"),SX+16,RowY+12,SW-32,136,24);
        Action(BodyCanvas,TEXT("Open_Wuling"),TEXT("我的武灵"),SX+12,RowY+160,SW-24,88,false,true);
        Action(BodyCanvas,TEXT("Open_WulingWorld"),TEXT("历练准备与当前轮次"),SX+12,RowY+264,SW-24,88,true);
        float LockedY=RowY+376;
        Label(BodyCanvas,TEXT("AtlasFutureHeading"),TEXT("墨隐诸域 · 尚未开放"),SX,LockedY,SW,48,28);LockedY+=64;
        for(const auto& Entry:Entries)if(!InkJourneyData::Open(Entry))
        {Action(BodyCanvas,TEXT("AtlasEntry_")+String(Entry,TEXT("id")),String(Entry,TEXT("name"))+TEXT(" · 未开放"),SX,LockedY,SW,88);LockedY+=100;}
    }
    AtlasKey=InkJourneyData::Key(this,bResourceMap);
}

void UInkJourneyMapPanel::RefreshLiveData()
{
    if(!FrontendOwner||FrontendOwner->CurrentPage!=PageId)return;
    const FString Key=InkJourneyData::Key(this,bResourceMap);if(Key!=AtlasKey){AtlasKey=Key;RebuildPanel();}
}
void UInkJourneyMapPanel::HandlePanelAction(const FString& Id)
{
    if(Id==TEXT("AtlasWorld")||Id==TEXT("AtlasResource")){bTabChosen=true;bResourceMap=Id==TEXT("AtlasResource");RebuildPanel();if(ResponsiveScroll)ResponsiveScroll->SetScrollOffset(0);return;}
    if(Id.StartsWith(TEXT("AtlasNode_"))||Id.StartsWith(TEXT("AtlasEntry_")))
    {
        const FString Entry=Id.Mid(Id.StartsWith(TEXT("AtlasNode_"))?10:11);
        Open(bResourceMap?TEXT("JourneyResourceDetail"):TEXT("JourneyWorldDetail"),Entry);return;
    }
    Super::HandlePanelAction(Id);
}

bool UInkJourneyDestinationPanel::IsResource()const{return PageId==TEXT("JourneyResourceDetail")||PageId==TEXT("ResourceDetail");}
FText UInkJourneyDestinationPanel::PanelTitle()const{return FText::FromString(IsResource()?TEXT("灵脉 · 资源详情"):TEXT("山海 · 地点详情"));}
bool UInkJourneyDestinationPanel::CanTravel()const
{return InkJourneyData::Ready(this)&&InkJourneyData::Open(InkJourneyData::Find(this,IsResource(),SelectedEntry))&&!bTravelPending;}

void UInkJourneyDestinationPanel::BuildPanelContent()
{
    using namespace WulingFrontendData;
    const bool Resource=IsResource();const auto Entry=InkJourneyData::Find(this,Resource,SelectedEntry);
    const bool Available=InkJourneyData::Open(Entry);const bool Wide=BodyWidth>=850;
    const float ArtW=Wide?BodyWidth*.45f:BodyWidth,ArtH=ArtW*.55f;
    auto* Image=WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("DestinationArtwork"));
    Image->SetBrushFromTexture(LoadObject<UTexture2D>(nullptr,Resource?TEXT("/Game/UI/Ink/Textures/Journey/T_ResourceAtlas_v1.T_ResourceAtlas_v1"):TEXT("/Game/UI/Ink/Textures/Journey/T_WorldAtlas_v1.T_WorldAtlas_v1")));
    Image->SetVisibility(ESlateVisibility::HitTestInvisible);auto* A=BodyCanvas->AddChildToCanvas(Image);A->SetPosition({0,0});A->SetSize({ArtW,ArtH});
    const float X=Wide?ArtW+28:0,Y=Wide?0:ArtH+24,W=BodyWidth-X;
    Label(BodyCanvas,TEXT("DestinationName"),Entry?String(Entry,TEXT("name")):TEXT("地点待同步"),X,Y,W,64,44);
    float RowY=Y+84;
    if(Resource)
    {
        Surface(BodyCanvas,TEXT("ResourceDetailPaper"),InkPresentationStyle::ESurface::FeatureCard,X,RowY-8,W,472);
        Label(BodyCanvas,TEXT("DestinationOwner"),TEXT("当前拥有的主播\n")+(Entry?InkJourneyData::Owners(Entry):TEXT("待同步")),X,RowY,W,104,28);RowY+=128;
        Label(BodyCanvas,TEXT("DestinationQuantity"),Entry?FString::Printf(TEXT("同类地点数量：%d · 当前第%d处"),Number(Entry,TEXT("entry_count")),Number(Entry,TEXT("index"))):TEXT("地点数量待同步"),X,RowY,W,72,26);RowY+=88;
        Label(BodyCanvas,TEXT("DestinationYield"),Entry?InkJourneyData::Yield(Entry)+FString::Printf(TEXT("\n开采权最长 %.0f 分钟 · 按有效在线结算"),InkJourneyData::Number(Entry,TEXT("lease_max_seconds"))/60):TEXT("产出等待权威数据"),X,RowY,W,112,26);RowY+=132;
        Label(BodyCanvas,TEXT("DestinationContestRule"),TEXT("前往后参与灵脉竞采，结果由真实参赛武灵决定。"),X,RowY,W,100,24);RowY+=116;
    }
    else
    {
        const TArray<TSharedPtr<FJsonValue>>* RewardRows=nullptr;
        if(Entry)Entry->TryGetArrayField(TEXT("rewards"),RewardRows);
        const int32 RewardCount=RewardRows?RewardRows->Num():0;
        Surface(BodyCanvas,TEXT("WorldDetailPaper"),InkPresentationStyle::ESurface::FeatureCard,X,RowY-8,W,480+RewardCount*52);
        Label(BodyCanvas,TEXT("DestinationDifficulty"),TEXT("关卡难度：")+String(Entry,TEXT("difficulty")),X,RowY,W,56,28);RowY+=72;
        Label(BodyCanvas,TEXT("DestinationDescription"),String(Entry,TEXT("description")),X,RowY,W,152,26);RowY+=172;
        Label(BodyCanvas,TEXT("DestinationRewardsHeading"),TEXT("三阶段完成奖励"),X,RowY,W,48,32);RowY+=64;
        const TArray<TSharedPtr<FJsonValue>>* Rewards=nullptr;
        if(Entry&&Entry->TryGetArrayField(TEXT("rewards"),Rewards)&&Rewards->Num())
        {
            for(int32 I=0;I<Rewards->Num();++I){auto R=(*Rewards)[I]->AsObject();AtlasIcon(BodyCanvas,NAME_None,8+I%4,X,RowY,40);Label(BodyCanvas,NAME_None,String(R,TEXT("name"))+FString::Printf(TEXT(" × %d"),Number(R,TEXT("quantity"))),X+52,RowY,W-52,44,26);RowY+=52;}
        }
        else {Label(BodyCanvas,TEXT("DestinationRewardEmpty"),Available?TEXT("奖励尚未同步"):TEXT("区域尚未开放"),X,RowY,W,64,26);RowY+=80;}
        Label(BodyCanvas,TEXT("DestinationWorldRule"),TEXT("前往后进入十分钟历练；与当前场景内剑灵协作完成各阶段。"),X,RowY+12,W,100,24);RowY+=136;
    }
    const auto* S=Service(this);
    Label(BodyCanvas,TEXT("DestinationAvailability"),bTravelPending?TEXT("前往请求已提交，正在确认……"):!Available?TEXT("此地点尚未开放"):!InkJourneyData::Ready(this)?TEXT("服务未连接或有操作待确认"):bConfirmTravel?TEXT("确认前往所选地点？"):TEXT("可前往 · 返回保留地图位置"),X,RowY,W,76,26);
    // Confirmation lives in the fixed shell footer so lengthy rewards cannot hide it.
    const auto* ShellLayout=ShellCanvas?Cast<UCanvasPanelSlot>(ShellCanvas->Slot):nullptr;
    const float ShellW=ShellLayout?ShellLayout->GetSize().X:BodyWidth+64,ShellH=ShellLayout?ShellLayout->GetSize().Y:BodyHeight+256;
    auto* Go=Action(ShellCanvas,bConfirmTravel?TEXT("DestinationConfirm"):TEXT("DestinationGo"),bTravelPending?TEXT("等待确认"):!Available?TEXT("尚未开放"):bConfirmTravel?TEXT("确认前往"):Resource?TEXT("前往争夺"):TEXT("前往历练"),24,ShellH-96,ShellW-216,88,true);
    Go->SetIsEnabled(CanTravel());CastChecked<UCanvasPanelSlot>(Go->Slot)->SetZOrder(10);
    if(bConfirmTravel&&!bTravelPending)Action(BodyCanvas,TEXT("DestinationCancel"),TEXT("取消前往"),X,RowY+100,W,88);
    DetailKey=InkJourneyData::Key(this,Resource,SelectedEntry)+(bTravelPending?TEXT("pending"):TEXT(""));
}
void UInkJourneyDestinationPanel::RefreshLiveData()
{
    if(!FrontendOwner||FrontendOwner->CurrentPage!=PageId)return;
    auto* W=WulingFrontendData::Service(this);
    if(bTravelPending&&W&&W->IsConnected()&&!W->IsBusy())
    {
        const auto S=W->Snapshot(),Round=WulingFrontendData::Object(S,TEXT("round"));
        if(!IsResource()&&WulingFrontendData::String(Round,TEXT("id"))!=PreviousRound&&WulingFrontendData::String(Round,TEXT("region_id"))==WulingFrontendData::String(InkJourneyData::Find(this,false,SelectedEntry),TEXT("id")))
        {FrontendOwner->CloseFeaturePanel(true);return;}
        const auto Queue=WulingFrontendData::Object(S,TEXT("race_queue"));
        const FString QueueStatus=WulingFrontendData::String(Queue,TEXT("status"));
        if(IsResource()&&Queue&&WulingFrontendData::String(Queue,TEXT("id"))!=PreviousQueue&&
            WulingFrontendData::String(Queue,TEXT("node_id"))==WulingFrontendData::String(InkJourneyData::Find(this,true,SelectedEntry),TEXT("node_id"))&&
            (QueueStatus==TEXT("Queued")||QueueStatus==TEXT("Matched")))
        {bTravelPending=false;Open(TEXT("WulingRace"));return;}
        bTravelPending=false;bConfirmTravel=false;FeedbackMessage(W->GetStatus());
    }
    const FString Key=InkJourneyData::Key(this,IsResource(),SelectedEntry)+(bTravelPending?TEXT("pending"):TEXT(""));
    if(Key!=DetailKey){DetailKey=Key;RebuildPanel();}
}
void UInkJourneyDestinationPanel::HandlePanelAction(const FString& Id)
{
    if(Id==TEXT("DestinationCancel")){bConfirmTravel=false;RebuildPanel();return;}
    if(Id==TEXT("DestinationGo")){if(CanTravel()){bConfirmTravel=true;RebuildPanel();}return;}
    if(Id==TEXT("DestinationConfirm"))
    {
        if(!bConfirmTravel||!CanTravel())return;
        auto* W=WulingFrontendData::Service(this);const auto Entry=InkJourneyData::Find(this,IsResource(),SelectedEntry);
        PreviousRound=WulingFrontendData::String(WulingFrontendData::Object(W->Snapshot(),TEXT("round")),TEXT("id"));
        PreviousQueue=WulingFrontendData::String(WulingFrontendData::Object(W->Snapshot(),TEXT("race_queue")),TEXT("id"));
        bTravelPending=IsResource()?W->QueueRace(WulingFrontendData::String(Entry,TEXT("node_id"))):W->StartWorldRound(TEXT("ENC_SOLO"),TEXT("guard"),WulingFrontendData::String(Entry,TEXT("id")));
        FeedbackMessage(bTravelPending?TEXT("正在前往，等待确认"):W->GetStatus());RebuildPanel();return;
    }
    Super::HandlePanelAction(Id);
}
