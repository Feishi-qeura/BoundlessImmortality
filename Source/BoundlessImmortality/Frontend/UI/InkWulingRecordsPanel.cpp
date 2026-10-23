#include "InkWulingRecordsPanel.h"
#include "Frontend/InkFrontendWidget.h"
#include "Frontend/WulingFrontendData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/EditableTextBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Engine/Font.h"
#include "Serialization/JsonSerializer.h"
using namespace WulingFrontendData;
namespace {
const TCHAR* Routes[]={TEXT("WulingGuild"),TEXT("WulingOrders"),TEXT("WulingOffline"),TEXT("WulingRights")};
const TCHAR* Titles[]={TEXT("组织管理"),TEXT("我的订单"),TEXT("离线记录"),TEXT("剑灵权益")};
FString Json(const TSharedPtr<FJsonObject>& O){FString V;if(O)FJsonSerializer::Serialize(O.ToSharedRef(),TJsonWriterFactory<>::Create(&V));return V;}
const TArray<TSharedPtr<FJsonValue>>& Array(const TSharedPtr<FJsonObject>& O,const TCHAR* K){static TArray<TSharedPtr<FJsonValue>> Empty;const TArray<TSharedPtr<FJsonValue>>* V=nullptr;return O&&O->TryGetArrayField(K,V)?*V:Empty;}
}
FText UInkWulingRecordsPanel::PanelTitle()const{for(int I=0;I<4;++I)if(PageId==Routes[I])return FText::FromString(Titles[I]);return FText::FromString(TEXT("武灵记录"));}
void UInkWulingRecordsPanel::BuildPanelContent()
{
    if(NameInput)DraftName=NameInput->GetText().ToString();NameInput=nullptr;
    const float OldList=RecordScroll?RecordScroll->GetScrollOffset():0,OldSummary=SummaryScroll?SummaryScroll->GetScrollOffset():0;
    RecordScroll=nullptr;SummaryScroll=nullptr;Rows.Reset();auto* W=Service(this);const auto S=W?W->Snapshot():nullptr;
    const bool Ready=W&&W->IsEnabled()&&W->IsConnected()&&!W->IsBusy();
    const float CW=(BodyWidth-36)/4,Top=164,Footer=BodyHeight-88;
    for(int I=0;I<4;++I)Action(BodyCanvas,TEXT("RecordTab_")+FString::FromInt(I),Titles[I],I*(CW+12),0,CW,88,false,false,PageId==Routes[I]);
    Label(BodyCanvas,TEXT("RecordStatus"),W?W->GetStatus():TEXT("服务未连接"),0,96,BodyWidth,60,24);
    const bool OfflinePage=PageId==TEXT("WulingOffline"),GuildPage=PageId==TEXT("WulingGuild"),OrdersPage=PageId==TEXT("WulingOrders");
    const float SummaryW=bPortraitLayout?BodyWidth:(BodyWidth-16)*.42f,ListX=bPortraitLayout?0:SummaryW+16,ListW=BodyWidth-ListX;
    const float SummaryH=GuildPage?536:OfflinePage?208:160,ListTop=bPortraitLayout?SummaryH+16:0;
    const int32 Count=8;
    auto MakeScroll=[&](const TCHAR* Name,float X,float Width,float Extent,TObjectPtr<UScrollBox>& Saved,float Offset)
    {
        auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),Name);Scroll->SetAllowOverscroll(false);Scroll->SetScrollbarThickness(FVector2D(6));
        auto* Slot=BodyCanvas->AddChildToCanvas(Scroll);Slot->SetOffsets(FMargin(X,Top,Width,FMath::Max(88.f,Footer-Top-12)));
        auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(Width-10);Size->SetHeightOverride(Extent);Scroll->AddChild(Size);
        auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();Size->SetContent(Canvas);Saved=Scroll;Scroll->SetScrollOffset(Offset);return Canvas;
    };
    const auto& Records=Array(S,TEXT("records"));
    const auto& Orders=Array(Object(S,TEXT("market")),TEXT("own_listings"));
    const int32 RowCount=GuildPage?FMath::Min(Array(S,TEXT("members")).Num(),Count):OrdersPage?FMath::Clamp(Orders.Num()-Offset,0,Count):OfflinePage?Records.Num():FMath::Clamp(Records.Num()-Offset,0,Count);
    auto* List=MakeScroll(TEXT("WulingRecordListScroll"),ListX,ListW,ListTop+FMath::Max(128.f,RowCount*120.f)+112,RecordScroll,OldList);
    auto* Summary=bPortraitLayout?List:MakeScroll(TEXT("WulingRecordSummaryScroll"),0,SummaryW,SummaryH,SummaryScroll,OldSummary);
    const float TextW=SummaryW-28;
    if(GuildPage)
    {
        const auto Guild=Object(S,TEXT("guild"));
        Label(Summary,TEXT("RecordGuildIdentity"),String(Guild,TEXT("name"))+TEXT("\n改名间隔7天；移除成员后24小时可再加入。"),0,0,TextW,120,26);
        NameInput=WidgetTree->ConstructWidget<UEditableTextBox>();NameInput->SetText(FText::FromString(DraftName));NameInput->SetHintText(FText::FromString(TEXT("新的武灵名")));
        FSlateFontInfo Font;Font.FontObject=FrontendOwner?FrontendOwner->TextFont.LoadSynchronous():nullptr;Font.Size=24;auto InputStyle=NameInput->GetWidgetStyle();InputStyle.SetFont(Font);NameInput->SetWidgetStyle(InputStyle);
        auto* Layout=Summary->AddChildToCanvas(NameInput);Layout->SetOffsets(FMargin(0,132,TextW,88));
        Action(Summary,TEXT("RecordRename"),TEXT("确认改名"),0,232,TextW,88)->SetIsEnabled(Ready&&Guild);
        Label(Summary,NAME_None,TEXT("解散后公库不返还，请确认组织和成员状态。"),0,336,TextW,80,24);
        Action(Summary,TEXT("RecordDissolve"),bConfirmDissolve?TEXT("再次确认解散"):TEXT("解散武灵…"),0,432,TextW,88)->SetIsEnabled(Ready&&Guild);
        if(Number(S,TEXT("member_cursor"))==Offset)for(int I=0;I<RowCount;++I)
        {
            auto O=Array(S,TEXT("members"))[I]->AsObject();Rows.Add(O);
            Action(List,TEXT("RecordRow_")+FString::FromInt(I),String(O,TEXT("name")),0,ListTop+I*120,ListW-16,88,false,false,SelectedMember==String(O,TEXT("user")))->SetIsEnabled(Ready);
            Label(List,NAME_None,FString::Printf(TEXT("成员 %d"),Offset+I+1),8,ListTop+I*120+88,ListW-32,32,24);
        }
        Action(BodyCanvas,TEXT("RecordRemove"),TEXT("移除已选成员"),0,Footer,BodyWidth,88)->SetIsEnabled(Ready&&!SelectedMember.IsEmpty());
    }
    else if(OrdersPage)
    {
        Label(Summary,TEXT("RecordOrderRules"),TEXT("订单与收据\n出售立即结算，到账扣除5%手续费；到期按原期限退回。"),0,0,TextW,152,26);
        if(!SelectedListing.IsEmpty()&&!Orders.ContainsByPredicate([this](const TSharedPtr<FJsonValue>& V){const auto O=V->AsObject();return String(O,TEXT("id"))==SelectedListing&&String(O,TEXT("status"))==TEXT("Listed");}))SelectedListing.Reset();
        const TMap<FString,FString> Status={{TEXT("Listed"),TEXT("寄售中")},{TEXT("Sold"),TEXT("已售出")},{TEXT("Cancelled"),TEXT("已撤单")},{TEXT("Expired"),TEXT("已到期")}};
        for(int I=Offset;I<FMath::Min(Orders.Num(),Offset+Count);++I)
        {
            auto O=Orders[I]->AsObject();Rows.Add(O);const float Y=ListTop+(I-Offset)*120;
            Action(List,TEXT("RecordRow_")+FString::FromInt(Rows.Num()-1),String(O,TEXT("name")),0,Y,ListW-16,88,false,false,SelectedListing==String(O,TEXT("id")))->SetIsEnabled(Ready);
            Label(List,NAME_None,FString::Printf(TEXT("×%d · 单价%d · %s"),Number(O,TEXT("count")),Number(O,TEXT("unit_price")),*Status.FindRef(String(O,TEXT("status")))),8,Y+88,ListW-32,32,24);
        }
        Action(BodyCanvas,TEXT("RecordCancel"),TEXT("撤回已选订单"),0,Footer,BodyWidth,88)->SetIsEnabled(Ready&&!SelectedListing.IsEmpty());
    }
    else if(OfflinePage)
    {
        const auto Offline=Object(S,TEXT("offline_reward"));
        Label(Summary,TEXT("OfflineActual"),FString::Printf(TEXT("已入账离线修为 %d\n已确认阅读 %d\n主播没有离线成长；观众10%%、每次最多8小时。\n确认只标记阅读，不再次发放。"),Number(Offline,TEXT("total_qi")),Number(Offline,TEXT("acknowledged_qi"))),0,0,TextW,208,26);
        int32 I=0;for(const auto& V:Records){auto O=V->AsObject();const auto Reward=Object(O,TEXT("offline_reward"));if(!Reward)continue;
            Label(List,NAME_None,String(O,TEXT("name"))+FString::Printf(TEXT("\n离线已入账 %d 修为"),Number(Reward,TEXT("total_qi"))),8,ListTop+I++*120,ListW-32,104,26);}
        Action(BodyCanvas,TEXT("RecordAcknowledge"),TEXT("确认已入账展示"),0,Footer,BodyWidth,88,true)->SetIsEnabled(Ready&&Offline);
    }
    else
    {
        Label(Summary,TEXT("SwordRightsRules"),TEXT("隔离原型成长档\n荣誉称号只改变展示，保留原礼物被动。旧权益与原存档不自动迁移。"),0,0,TextW,152,26);
        for(int I=Offset;I<FMath::Min(Records.Num(),Offset+Count);++I){auto O=Records[I]->AsObject();const FString Realm=String(O,TEXT("realm_name"));
            Label(List,I==Offset?FName(TEXT("SwordActualRights")):NAME_None,String(O,TEXT("name"))+TEXT(" · ")+(Realm.IsEmpty()?FString(TEXT("境界读取中")):Realm)+TEXT("\n")+String(O,TEXT("honor_title")),8,ListTop+(I-Offset)*120,ListW-32,104,26);}
        Label(BodyCanvas,TEXT("SwordRightsCommand"),TEXT("弹幕：展示青云踏飞雁 / 隐藏荣誉称号"),0,Footer,BodyWidth,88,24);
    }
    if(RowCount==0)Label(List,TEXT("RecordEmpty"),Ready?TEXT("暂无记录"):TEXT("记录待同步"),8,ListTop+8,ListW-32,100,26);
    if(!OfflinePage)
    {
        const float PageY=ListTop+FMath::Max(128.f,RowCount*120.f),PageW=(ListW-28)/2;
        Action(List,TEXT("RecordPrevious"),TEXT("上一页"),0,PageY,PageW,88)->SetIsEnabled(Offset>0);
        Action(List,TEXT("RecordNext"),TEXT("下一页"),PageW+12,PageY,PageW,88)->SetIsEnabled(GuildPage?RowCount==Count:Offset+Count<(OrdersPage?Orders.Num():Records.Num()));
    }
}
void UInkWulingRecordsPanel::RefreshLiveData()
{
    if(!FrontendOwner||FrontendOwner->CurrentPage!=PageId)return;auto* W=Service(this);if(!W)return;
    if(PageId==TEXT("WulingGuild")&&W->IsConnected()&&Number(W->Snapshot(),TEXT("member_cursor"))!=Offset)W->RequestMemberPage(Offset);
    const auto S=W->Snapshot();FString Key=FString::FromInt(Number(S,TEXT("version")))+W->GetStatus()+FString::FromInt(Number(S,TEXT("member_cursor")))+Json(Object(S,TEXT("offline_reward")));
    if(Key!=StateKey){StateKey=Key;FeedbackMessage(W->GetStatus());RebuildPanel();}
}
void UInkWulingRecordsPanel::HandlePanelAction(const FString& Id)
{
    auto* W=Service(this);if(!W)return;auto Args=MakeShared<FJsonObject>();
    if(Id.StartsWith(TEXT("RecordTab_"))){const int I=FCString::Atoi(*Id.Mid(10));if(I>=0&&I<4)Open(Routes[I]);return;}
    if(Id==TEXT("RecordPrevious")||Id==TEXT("RecordNext")){Offset=FMath::Max(0,Offset+(Id==TEXT("RecordNext")?1:-1)*8);if(PageId==TEXT("WulingGuild"))W->RequestMemberPage(Offset);RebuildPanel();return;}
    if(Id.StartsWith(TEXT("RecordRow_"))){const int I=FCString::Atoi(*Id.Mid(10));if(Rows.IsValidIndex(I)){SelectedMember=String(Rows[I],TEXT("user"));SelectedListing=String(Rows[I],TEXT("status"))==TEXT("Listed")?String(Rows[I],TEXT("id")):TEXT("");ListingVersion=Number(Rows[I],TEXT("version"));RebuildPanel();}return;}
    FString Operation;
    if(Id==TEXT("RecordRename")){Operation=TEXT("RenameGuild");Args->SetStringField(TEXT("name"),NameInput?NameInput->GetText().ToString():DraftName);}
    else if(Id==TEXT("RecordRemove")){Operation=TEXT("RemoveMember");Args->SetStringField(TEXT("user_id"),SelectedMember);SelectedMember.Reset();}
    else if(Id==TEXT("RecordDissolve")){if(!bConfirmDissolve){bConfirmDissolve=true;RebuildPanel();return;}Operation=TEXT("DissolveGuild");Args->SetBoolField(TEXT("confirmed"),true);bConfirmDissolve=false;}
    else if(Id==TEXT("RecordCancel")){Operation=TEXT("CancelListing");Args->SetStringField(TEXT("listing_id"),SelectedListing);Args->SetNumberField(TEXT("listing_version"),ListingVersion);}
    else if(Id==TEXT("RecordAcknowledge"))Operation=TEXT("AcknowledgeOfflineReward");
    else{Super::HandlePanelAction(Id);return;}
    FeedbackMessage(W->SubmitPrototypeCommand(Operation,Json(Args))?TEXT("请求已记录，等待服务确认"):W->GetStatus());
}
