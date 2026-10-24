#include "InkLaunchEventPanel.h"
#include "Frontend/InkFrontendWidget.h"
#include "Frontend/WulingFrontendData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#endif

namespace InkLaunchEventView
{
    using FObject=TSharedPtr<FJsonObject>;
    FObject Object(const FObject& O,const TCHAR* Field)
    {
        const FObject* Value=nullptr;
        return O&&O->TryGetObjectField(Field,Value)?*Value:nullptr;
    }
    FString String(const FObject& O,const TCHAR* Field)
    {
        FString Value;if(O)O->TryGetStringField(Field,Value);return Value;
    }
    double Number(const FObject& O,const TCHAR* Field)
    {
        double Value=0;if(O)O->TryGetNumberField(Field,Value);return Value;
    }
    bool Flag(const FObject& O,const TCHAR* Field)
    {
        bool Value=false;if(O)O->TryGetBoolField(Field,Value);return Value;
    }
    bool HasFlag(const FObject& O,const TCHAR* Field)
    {
        bool Value=false;return O&&O->TryGetBoolField(Field,Value);
    }
    bool HasNumber(const FObject& O,const TCHAR* Field)
    {
        const auto* Value=O?O->Values.Find(Field):nullptr;
        return Value&&Value->IsValid()&&(*Value)->Type==EJson::Number;
    }
    TArray<TSharedPtr<FJsonValue>> Rows(const FObject& O,const TCHAR* Field)
    {
        const TArray<TSharedPtr<FJsonValue>>* Value=nullptr;
        return O&&O->TryGetArrayField(Field,Value)?*Value:TArray<TSharedPtr<FJsonValue>>();
    }
    bool Contains(const FObject& O,const TCHAR* Field,const FString& Id)
    {
        for(const auto& Value:Rows(O,Field))if(Value&&Value->Type==EJson::String&&Value->AsString()==Id)return true;
        return false;
    }
    bool KnownAbsent(const FObject& State)
    {
        const auto* Value=State?State->Values.Find(TEXT("launch")):nullptr;
        return Value&&Value->IsValid()&&(*Value)->Type==EJson::Null;
    }
    bool AllowedProfile(const FObject& State)
    {
        return String(Object(State,TEXT("manifest")),TEXT("profile"))==TEXT("prototype_full");
    }
    bool CanStart(const FObject& State,bool Enabled,bool Connected,bool Busy,bool ActivePage)
    {
        const auto Identity=Object(State,TEXT("identity")),Guild=Object(State,TEXT("guild"));
        const double Now=Number(State,TEXT("server_utc"));
        return Enabled&&Connected&&!Busy&&ActivePage&&AllowedProfile(State)&&KnownAbsent(State)&&Guild&&
            !String(Guild,TEXT("id")).IsEmpty()&&
            String(Identity,TEXT("role"))==TEXT("anchor")&&
            !String(Identity,TEXT("user")).IsEmpty()&&
            String(Identity,TEXT("user"))==String(Guild,TEXT("anchor"))&&
            Now>0&&Number(Object(State,TEXT("manifest")),TEXT("window_end"))>Now;
    }
    FString Date(double Utc)
    {
        if(Utc<=0)return TEXT("待同步");
        // The activity contract uses Hong Kong time, independent of the PC clock/timezone.
        return (FDateTime::FromUnixTimestamp(int64(Utc))+FTimespan::FromHours(8)).ToString(TEXT("%m/%d %H:%M"))+TEXT("（香港）");
    }
    FString Remaining(double Seconds)
    {
        const int64 Minutes=FMath::Max<int64>(0,FMath::CeilToInt64(Seconds/60.));
        if(Minutes>=1440)return FString::Printf(TEXT("%lld天 %lld小时"),Minutes/1440,(Minutes%1440)/60);
        if(Minutes>=60)return FString::Printf(TEXT("%lld小时 %lld分钟"),Minutes/60,Minutes%60);
        return FString::Printf(TEXT("%lld分钟"),Minutes);
    }
    FObject OwnedTitle(const FObject& State)
    {
        const auto Personal=Object(Object(State,TEXT("launch")),TEXT("personal"));
        const auto Title=Object(Personal,TEXT("title"));
        if(Title)return Title;
        for(const auto& Value:Rows(Object(State,TEXT("honors")),TEXT("owned")))
            if(Value&&Value->Type==EJson::Object&&String(Value->AsObject(),TEXT("title_id"))==TEXT("QingyunFeiyan"))return Value->AsObject();
        return nullptr;
    }
    FString Phase(const FObject& State)
    {
        const auto Event=Object(State,TEXT("launch"));
        if(!Event)return KnownAbsent(State)?TEXT("尚未开启"):TEXT("资料待同步");
        const FString Status=String(Event,TEXT("status"));
        if(Status==TEXT("Settled"))return TEXT("活动已结算");
        if(Status==TEXT("Closing")||Number(Event,TEXT("end"))<=Number(State,TEXT("server_utc")))return TEXT("活动收尾中");
        return Status==TEXT("Active")?TEXT("活动进行中"):TEXT("活动状态待确认");
    }
    FString ViewKey(const FObject& State,bool Enabled,bool Connected,bool Busy,bool ActivePage)
    {
        // Exclude global version, server seconds, round simulation and personal round histories.
        // None affect this page until projected progress/confirmation changes.
        const FObject View=MakeShared<FJsonObject>();
        auto Copy=[](const FObject& From,const FObject& To,const TCHAR* Field)
        {
            if(From)if(const auto* Value=From->Values.Find(Field))To->SetField(Field,*Value);
        };
        Copy(State,View,TEXT("honors"));
        View->SetBoolField(TEXT("known_absent"),KnownAbsent(State));
        View->SetStringField(TEXT("phase"),Phase(State));
        const FObject Event=Object(State,TEXT("launch")),EventView=MakeShared<FJsonObject>();
        for(const TCHAR* Field:{TEXT("id"),TEXT("status"),TEXT("start"),TEXT("end"),TEXT("rules_hash"),TEXT("permissions"),TEXT("tasks"),TEXT("guild_milestones"),TEXT("milestones"),TEXT("complete_rounds"),TEXT("leaderboard"),TEXT("buff_start"),TEXT("buff_end")})Copy(Event,EventView,Field);
        const FObject Personal=Object(Event,TEXT("personal")),PersonalView=MakeShared<FJsonObject>();
        for(const TCHAR* Field:{TEXT("home_guild"),TEXT("enrolled_at"),TEXT("score"),TEXT("complete_count"),TEXT("title")})Copy(Personal,PersonalView,Field);
        EventView->SetObjectField(TEXT("personal"),PersonalView);View->SetObjectField(TEXT("launch"),EventView);
        for(const TCHAR* Group:{TEXT("identity"),TEXT("guild"),TEXT("manifest")})
        {
            const FObject Source=Object(State,Group),Target=MakeShared<FJsonObject>();
            for(const TCHAR* Field:{TEXT("id"),TEXT("name"),TEXT("role"),TEXT("user"),TEXT("anchor"),TEXT("profile"),TEXT("window_end")})Copy(Source,Target,Field);
            View->SetObjectField(Group,Target);
        }
        const double Now=Number(State,TEXT("server_utc"));
        View->SetBoolField(TEXT("title_expired"),Number(OwnedTitle(State),TEXT("expiry"))<=Now);
        View->SetBoolField(TEXT("buff_active"),Number(Event,TEXT("buff_end"))>Now);
        View->SetBoolField(TEXT("window_expired"),Number(Object(State,TEXT("manifest")),TEXT("window_end"))<=Now);
        FString Json;FJsonSerializer::Serialize(View.ToSharedRef(),TJsonWriterFactory<>::Create(&Json));
        return FString::Printf(TEXT("%d|%d|%d|%d|"),Enabled,Connected,Busy,ActivePage)+Json;
    }
    FString Condition(const FString& Id)
    {
        static const TMap<FString,FString> Names={
            {TEXT("join_and_free_response"),TEXT("正式加入 · 免费响应")},
            {TEXT("complete_world_rounds"),TEXT("完整参与世界历练")},
            {TEXT("correct_guard_response"),TEXT("第一阶段 · 正确护阵")},
            {TEXT("correct_break_response"),TEXT("第二阶段 · 正确破障")},
            {TEXT("route_vote_in_distinct_rounds"),TEXT("不同轮次 · 路线选择")}};
        if(const auto* Name=Names.Find(Id))return *Name;
        return TEXT("任务条件待说明");
    }
    FString CompletedStartFeedback(bool& Pending,const FObject& State,bool Connected,bool Busy,const FString& ServiceStatus)
    {
        if(!Pending||!Connected||Busy)return FString();
        Pending=false;
        if(Object(State,TEXT("launch")))return TEXT("青云初启已开启，服务已确认。");
        return ServiceStatus.IsEmpty()?FString(TEXT("开启尚未确认，请稍后重试。")):ServiceStatus;
    }
}

FText UInkLaunchEventPanel::PanelTitle() const
{
    return FText::FromString(PageId==TEXT("LaunchRank")?TEXT("青云初启 · 荣誉榜"):TEXT("青云初启"));
}

void UInkLaunchEventPanel::BuildPanelContent()
{
    using namespace InkLaunchEventView;
    if(PageId==TEXT("LaunchRank")&&SelectedTab==TEXT("overview"))SelectedTab=TEXT("honors");
    ConnectionLabel=nullptr;TimeLabel=nullptr;HonorExpiryLabel=nullptr;
    auto* Service=WulingFrontendData::Service(this);
    const auto State=Service?Service->Snapshot():nullptr,Event=Object(State,TEXT("launch"));
    const auto Personal=Object(Event,TEXT("personal")),Permissions=Object(Event,TEXT("permissions"));
    const bool Enabled=Service&&Service->IsEnabled(),Connected=Service&&Service->IsConnected(),Busy=Service&&Service->IsBusy();
    const bool ActivePage=FrontendOwner&&FrontendOwner->CurrentPage==PageId;
    const bool Wide=BodyWidth>=900;
    const float W=BodyWidth,Gap=16,Hit=88;
    auto Text=[&](const FString& Value,float X,float Y,float Width,int32 Size=26,float MinHeight=0)->float
    {
        const int32 Capacity=FMath::Max(1,int32(Width/FMath::Max(1.f,Size*.92f)));
        TArray<FString> Lines;Value.ParseIntoArrayLines(Lines,false);int32 Count=0;
        for(const FString& Line:Lines)Count+=FMath::Max(1,FMath::DivideAndRoundUp(Line.Len(),Capacity));
        const float Height=FMath::Max(MinHeight,FMath::Max(1,Count)*Size*1.5f);
        Label(BodyCanvas,NAME_None,Value,X,Y,Width,Height,Size);return Height;
    };
    auto Progress=[&](double Current,double Target,float X,float Y,float Width)
    {
        auto* Bar=WidgetTree->ConstructWidget<UProgressBar>();Bar->SetWidgetStyle(InkPresentationStyle::Progress());
        Bar->SetPercent(Target>0?float(FMath::Clamp(Current/Target,0.,1.)):0.f);
        Bar->SetFillColorAndOpacity(InkPresentationStyle::Jade());Bar->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto* Slot=BodyCanvas->AddChildToCanvas(Bar);Slot->SetPosition({X,Y});Slot->SetSize({Width,12});Slot->SetZOrder(3);
    };
    const float HeroTextWidth=Wide?W*.57f:W;
    Surface(BodyCanvas,TEXT("LaunchHeroPaper"),InkPresentationStyle::ESurface::FeatureCard,0,0,HeroTextWidth,118);
    Surface(BodyCanvas,TEXT("LaunchHeroHeading"),InkPresentationStyle::ESurface::FeatureHeading,8,8,HeroTextWidth-16,48);
    Label(BodyCanvas,TEXT("LaunchHeroTitle"),TEXT("青云初启 · 同道共修"),16,8,HeroTextWidth-32,46,32);
    Text(TEXT("踏青云，聚同道。每一次免费参与，皆有迹可循。"),16,60,HeroTextWidth-32,24,Wide?48:52);
    const float MetaY=Wide?128:134;
    if(Wide)
    {
        Surface(BodyCanvas,TEXT("LaunchConnectionPaper"),InkPresentationStyle::ESurface::FeatureCard,0,MetaY-8,(W-Gap)/2,88);
        Surface(BodyCanvas,TEXT("LaunchTimePaper"),InkPresentationStyle::ESurface::FeatureCard,(W+Gap)/2,MetaY-8,(W-Gap)/2,88);
    }
    else Surface(BodyCanvas,TEXT("LaunchServiceStatePaper"),InkPresentationStyle::ESurface::FeatureCard,0,MetaY-8,W,152);
    ConnectionLabel=Label(BodyCanvas,TEXT("LaunchConnection"),TEXT(""),0,MetaY,Wide?(W-Gap)/2:W,68,26);
    TimeLabel=Label(BodyCanvas,TEXT("LaunchServerTime"),TEXT(""),Wide?(W+Gap)/2:0,Wide?MetaY:MetaY+76,Wide?(W-Gap)/2:W,76,24);
    const float TabY=Wide?MetaY+88:MetaY+164;
    const TCHAR* TabIds[]={TEXT("overview"),TEXT("tasks"),TEXT("guild"),TEXT("honors")};
    const TCHAR* TabNames[]={TEXT("活动总览"),TEXT("个人任务"),TEXT("武灵目标"),TEXT("荣誉规则")};
    const int32 TabColumns=Wide?4:2;
    const float TabWidth=(W-(TabColumns-1)*12)/TabColumns;
    Surface(BodyCanvas,TEXT("LaunchTabsHeadingPaper"),InkPresentationStyle::ESurface::FeatureHeading,0,TabY,W,Wide?88:188);
    for(int32 I=0;I<4;++I)
        Action(BodyCanvas,TEXT("LaunchTab_")+FString(TabIds[I]),TabNames[I],(I%TabColumns)*(TabWidth+12),TabY+(I/TabColumns)*100,TabWidth,Hit,false,false,SelectedTab==TabIds[I]);
    float Y=TabY+(Wide?100:200)+20;

    if(!Enabled||!AllowedProfile(State)||(!Event&&SelectedTab!=TEXT("honors")))
    {
        FString Title,Message;
        if(!Enabled){Title=TEXT("开服活动暂不可用");Message=TEXT("当前未启用武灵活动服务。活动资料同步后，此处展示本期真实任务与目标。");}
        else if(!Connected){Title=TEXT("等待连接活动服务");Message=TEXT("当前无法确认本期活动资料，请在恢复连接后查看。");}
        else if(!State||!State->HasField(TEXT("launch"))){Title=TEXT("活动资料同步中");Message=TEXT("尚未收到完整的活动快照，请稍候。");}
        else if(!AllowedProfile(State)){Title=TEXT("本期活动尚未开放");Message=TEXT("当前服务版本未开放青云初启。开放状态以服务确认结果为准。");}
        else if(!Object(State,TEXT("guild"))){Title=TEXT("先立山门，再赴青云");Message=TEXT("当前尚未加入武灵。主播创建武灵后可开启本期活动，正式成员通过免费响应参与。");}
        else if(String(Object(State,TEXT("identity")),TEXT("role"))==TEXT("anchor")){Title=TEXT("青云待启 · 静候君来");Message=TEXT("由本武灵主播开启本期活动。开启后，任务、进度与截止时间将由服务同步。");}
        else {Title=TEXT("等待主播开启活动");Message=TEXT("本武灵尚未开启青云初启。开启后，在本房世界历练中免费响应即可参与。");}
        const bool ShowSectButton=Connected&&Enabled&&AllowedProfile(State)&&State&&!Object(State,TEXT("guild"));
        Surface(BodyCanvas,TEXT("LaunchStateCard"),InkPresentationStyle::ESurface::FeatureCard,0,Y,W,ShowSectButton?344.f:260.f);
        Surface(BodyCanvas,TEXT("LaunchStateHeading"),InkPresentationStyle::ESurface::FeatureHeading,8,Y+8,W-16,54);
        Label(BodyCanvas,TEXT("LaunchStateTitle"),Title,16,Y+16,W-32,44,28);
        Y+=84;
        Y+=Text(Message,16,Y,W-32,24,48)+20;
        if(ShowSectButton){Action(BodyCanvas,TEXT("LaunchSect"),TEXT("前往武灵山门"),0,Y,W,Hit);Y+=Hit+16;}
    }
    else if(SelectedTab==TEXT("tasks"))
    {
        Y+=Text(TEXT("个人修行 · 以行证道"),0,Y,W,32)+12;
        Y+=Text(TEXT("进度与确认状态由服务记录，任务达成后自动结算。"),0,Y,W,26)+20;
        const auto Tasks=Rows(Event,TEXT("tasks"));const int32 Columns=Wide?2:1;
        const float CardW=(W-(Columns-1)*Gap)/Columns,CardH=240;
        int32 Index=0;
        for(const auto& Value:Tasks)
        {
            const auto Task=Value&&Value->Type==EJson::Object?Value->AsObject():nullptr;if(!Task)continue;
            const float X=(Index%Columns)*(CardW+Gap),Top=Y+(Index/Columns)*(CardH+Gap);
            const double Current=Number(Task,TEXT("progress")),Target=Number(Task,TEXT("target"));
            const bool HasProgress=HasNumber(Task,TEXT("progress"))&&HasNumber(Task,TEXT("target"));
            const bool Confirmed=Flag(Task,TEXT("confirmed"));
            Surface(BodyCanvas,NAME_None,InkPresentationStyle::ESurface::FeatureCard,X,Top,CardW,CardH);
            Surface(BodyCanvas,NAME_None,InkPresentationStyle::ESurface::FeatureHeading,X+8,Top+8,CardW-16,54);
            Label(BodyCanvas,NAME_None,Condition(String(Task,TEXT("condition_type"))),X+16,Top+16,CardW-32,50,28);
            const FString TaskState=Confirmed?TEXT("服务已确认"):HasProgress?(Target>0&&Current>=Target?TEXT("等待服务结算"):TEXT("进行中")):TEXT("任务状态待同步");
            const FString ProgressText=HasProgress?FString::Printf(TEXT("%s  ·  %.0f / %.0f"),*TaskState,Current,Target):TaskState+TEXT("  ·  进度待同步");
            auto* Status=Label(BodyCanvas,NAME_None,ProgressText,X+16,Top+72,CardW-32,40,24);
            if(Confirmed)Status->SetColorAndOpacity(InkPresentationStyle::Jade());
            if(HasProgress)Progress(Current,Target,X+16,Top+124,CardW-32);
            FString Reward=TEXT("奖励信息待同步");
            if(Flag(Permissions,TEXT("task_numeric_rewards")))
            {
                const FString Item=String(Task,TEXT("reward_item"));
                if(!Item.IsEmpty()&&HasNumber(Task,TEXT("reward_count")))
                    Reward=FString::Printf(TEXT("%s %.0f · %s"),Item==TEXT("STAT_QI")?TEXT("修为"):*Item,Number(Task,TEXT("reward_count")),Confirmed?TEXT("服务已确认"):TEXT("达成后自动结算"));
            }
            Text(Reward,X+16,Top+152,CardW-32,24,64);++Index;
        }
        if(!Index)Y+=Text(TEXT("本期任务尚未同步，请稍候。"),0,Y,W,28);
        else Y+=FMath::DivideAndRoundUp(Index,Columns)*(CardH+Gap);
    }
    else if(SelectedTab==TEXT("guild"))
    {
        const int32 Count=Rows(Event,TEXT("complete_rounds")).Num();
        Y+=Text(TEXT("同道共修 · 武灵目标"),0,Y,W,32)+12;
        Y+=Text(FString::Printf(TEXT("已确认共同完成 %d 轮。主播与正式成员完整、免费参与后，由服务计入。"),Count),0,Y,W,26)+20;
        const int32 Columns=Wide?2:1;const float CardW=(W-(Columns-1)*Gap)/Columns,CardH=264;
        int32 Index=0;
        for(const auto& Value:Rows(Event,TEXT("guild_milestones")))
        {
            const auto Tier=Value&&Value->Type==EJson::Object?Value->AsObject():nullptr;if(!Tier)continue;
            const double Target=Number(Tier,TEXT("complete_rounds"));
            const bool HasTarget=HasNumber(Tier,TEXT("complete_rounds"));
            const bool Confirmed=Contains(Event,TEXT("milestones"),String(Tier,TEXT("tier_id")));
            const float X=(Index%Columns)*(CardW+Gap),Top=Y+(Index/Columns)*(CardH+Gap);
            Surface(BodyCanvas,NAME_None,InkPresentationStyle::ESurface::FeatureCard,X,Top,CardW,CardH);
            Surface(BodyCanvas,NAME_None,InkPresentationStyle::ESurface::FeatureHeading,X+8,Top+8,CardW-16,54);
            Label(BodyCanvas,NAME_None,HasTarget?FString::Printf(TEXT("共修 %.0f 轮 · %s"),Target,Confirmed?TEXT("已确认"):TEXT("待达成")):TEXT("目标轮次待同步"),X+16,Top+16,CardW-32,48,28);
            Label(BodyCanvas,NAME_None,HasTarget?FString::Printf(TEXT("%d / %.0f 轮"),Count,Target):TEXT("当前轮次待同步"),X+16,Top+76,CardW-32,40,26);
            if(HasTarget&&HasNumber(Event,TEXT("complete_rounds")))Progress(Count,Target,X+16,Top+124,CardW-32);
            FString Reward=String(Tier,TEXT("guild_visual"));
            if(Reward.IsEmpty())Reward=TEXT("目标与奖励信息待同步");
            if(Flag(Permissions,TEXT("guild_numeric_rewards")))
            {
                if(HasNumber(Tier,TEXT("anchor_stone"))&&HasNumber(Tier,TEXT("anchor_break_material")))
                    Reward+=FString::Printf(TEXT("\n主播：灵石 %.0f · 破境石 %.0f"),Number(Tier,TEXT("anchor_stone")),Number(Tier,TEXT("anchor_break_material")));
                else Reward+=TEXT("\n奖励详情待同步");
            }
            else Reward+=HasFlag(Permissions,TEXT("guild_numeric_rewards"))?TEXT("\n本期未开放数值奖励"):TEXT("\n奖励权限待同步");
            Text(Reward,X+16,Top+150,CardW-32,24,96);++Index;
        }
        if(Index)Y+=FMath::DivideAndRoundUp(Index,Columns)*(CardH+Gap);
        else Y+=Text(TEXT("武灵目标尚未同步，请稍候。"),0,Y,W,28)+20;
        if(Flag(Permissions,TEXT("guild_numeric_buff")))
        {
            const bool BuffActive=Number(Event,TEXT("buff_end"))>Number(State,TEXT("server_utc"));
            Y+=Text(BuffActive?TEXT("共修增益生效中，具体截止以服务时间为准。"):TEXT("最终目标的共修增益，由服务确认达成后开启。"),0,Y,W,26);
        }
    }
    else if(SelectedTab==TEXT("honors"))
    {
        Y+=Text(TEXT("荣誉有期 · 修行有证"),0,Y,W,32)+12;
        const auto Title=OwnedTitle(State);
        Y+=Text(Title?String(Title,TEXT("name")):TEXT("尚未获得本期称号"),0,Y,W,28)+12;
        HonorExpiryLabel=Label(BodyCanvas,TEXT("LaunchHonorExpiry"),TEXT(""),0,Y,W,80,26);Y+=96;
        int32 RequiredTasks=0;
        for(const auto& Value:Rows(Event,TEXT("tasks")))
            if(Value&&Value->Type==EJson::Object&&Flag(Value->AsObject(),TEXT("title_required")))++RequiredTasks;
        if(Flag(Permissions,TEXT("title_reward"))&&RequiredTasks>0)
            Y+=Text(FString::Printf(TEXT("完成本期 %d 项指定任务，经服务确认后授予称号。称号没有数值加成。"),RequiredTasks),0,Y,W,26)+20;
        if(Title)Y+=Text(TEXT("观众可发送弹幕“展示青云踏飞雁”或“隐藏荣誉称号”选择展示。"),0,Y,W,26)+20;
        Y+=Text(TEXT("正式成员在所属武灵的本房历练中免费参与，进度按服务确认的完整阶段与正确响应记录。"),0,Y,W,26)+20;
        Y+=Text(TEXT("任务自动结算，称号期限以实际获得记录为准。贡献依据免费参与、正确响应与路线选择计算。"),0,Y,W,26)+20;
        Y+=Text(TEXT("同分同名次。荣誉榜不提供数值排行奖励；贡献不按礼物金额、伤害或境界计分。"),0,Y,W,26)+28;
        Y+=Text(TEXT("同道荣誉榜"),0,Y,W,32)+16;
        const auto Board=Rows(Event,TEXT("leaderboard"));
        if(!Board.Num())Y+=Text(TEXT("暂无符合荣誉榜条件的成员。入榜资格由服务端确认。"),0,Y,W,26);
        for(const auto& Value:Board)
        {
            const auto Entry=Value&&Value->Type==EJson::Object?Value->AsObject():nullptr;if(!Entry)continue;
            const FString Name=String(Entry,TEXT("name"));
            const FString Rank=HasNumber(Entry,TEXT("rank"))?FString::Printf(TEXT("第%.0f名"),Number(Entry,TEXT("rank"))):TEXT("名次待同步");
            const FString Score=HasNumber(Entry,TEXT("score"))?FString::Printf(TEXT("%.0f分"),Number(Entry,TEXT("score"))):TEXT("贡献值待同步");
            Surface(BodyCanvas,NAME_None,InkPresentationStyle::ESurface::FeatureCard,0,Y,W,64);
            Y+=Text(FString::Printf(TEXT("%s  ·  %s  ·  %s"),*Rank,*Name,*Score),12,Y+8,W-24,24,48)+20;
        }
        if(PageId==TEXT("LaunchEvent"))Action(BodyCanvas,TEXT("Open_LaunchRank"),TEXT("单独查看完整贡献榜"),0,Y+12,W,Hit,true);
    }
    else
    {
        Y+=Text(TEXT("本期修行札记"),0,Y,W,32)+16;
        const FString GuildName=String(Object(State,TEXT("guild")),TEXT("name"));
        if(!GuildName.IsEmpty())Y+=Text(GuildName,0,Y,W,28)+12;
        if(Personal)
        {
            int32 Confirmed=0;const auto Tasks=Rows(Event,TEXT("tasks"));
            for(const auto& Value:Tasks)if(Value&&Value->Type==EJson::Object&&Flag(Value->AsObject(),TEXT("confirmed")))++Confirmed;
            const FString Score=HasNumber(Personal,TEXT("score"))?FString::Printf(TEXT("%.0f"),Number(Personal,TEXT("score"))):TEXT("待同步");
            const FString Complete=HasNumber(Personal,TEXT("complete_count"))?FString::Printf(TEXT("%.0f"),Number(Personal,TEXT("complete_count"))):TEXT("待同步");
            Y+=Text(FString::Printf(TEXT("任务确认 %d / %d  ·  个人贡献 %s\n完整历练 %s 轮"),Confirmed,Tasks.Num(),*Score,*Complete),0,Y,W,28)+20;
        }
        else
        {
            const bool Anchor=String(Object(State,TEXT("identity")),TEXT("role"))==TEXT("anchor");
            Y+=Text(Anchor?TEXT("主播领同道共修，查看武灵目标可了解已确认的共同轮次。"):TEXT("本人尚未报名。正式加入武灵并免费响应本房历练后，服务会登记本期参与记录。"),0,Y,W,28)+20;
        }
        Y+=Text(TEXT("由历练启程"),0,Y,W,32)+12;
        Y+=Text(TEXT("查看个人任务，选择本轮修行目标；与同道完整参与世界历练，积累个人记录与武灵共同进度。"),0,Y,W,26)+24;
        const auto Title=OwnedTitle(State);
        Y+=Text(Title?String(Title,TEXT("name")):Flag(Permissions,TEXT("title_reward"))?TEXT("本期称号 · 青云踏飞雁"):TEXT("本期称号奖励未开放"),0,Y,W,28)+12;
        HonorExpiryLabel=Label(BodyCanvas,TEXT("LaunchHonorExpiry"),TEXT(""),0,Y,W,80,26);Y+=96;
    }

    // Use the shell's reserved primary-action area. It stays reachable as the body scrolls.
    FString Caption;bool CanAct=false;
    const bool Live=Event&&AllowedProfile(State)&&String(Event,TEXT("status"))==TEXT("Active")&&Number(Event,TEXT("end"))>Number(State,TEXT("server_utc"));
    if(!Enabled||!AllowedProfile(State))Caption=TEXT("活动暂不可用");
    else if(!Connected)Caption=TEXT("等待服务连接");
    else if(Busy)Caption=TEXT("请求确认中");
    else if(Live){Caption=TEXT("前往世界历练");CanAct=ActivePage;}
    else if(Event)Caption=Phase(State);
    else if(CanStart(State,Enabled,Connected,Busy,ActivePage)){Caption=TEXT("开启青云初启");CanAct=true;}
    else Caption=KnownAbsent(State)?TEXT("等待武灵开启"):TEXT("等待活动资料");
    UCanvasPanel* FooterParent=BodyCanvas;float FooterY=Y+24,FooterX=0,FooterW=W;
    if(!bEmbedded&&ShellCanvas)
        if(auto* FooterLayout=Cast<UCanvasPanelSlot>(ShellCanvas->Slot))
        {FooterParent=ShellCanvas;FooterX=24;FooterY=FooterLayout->GetSize().Y-96;FooterW=FooterLayout->GetSize().X-216;}
    Action(FooterParent,TEXT("LaunchPrimary"),Caption,FooterX,FooterY,FooterW,Hit,true)->SetIsEnabled(CanAct);
    RenderKey=ViewKey(State,Enabled,Connected,Busy,ActivePage);
    UpdateLiveLabels();
}

void UInkLaunchEventPanel::UpdateLiveLabels()
{
    using namespace InkLaunchEventView;
    auto* Service=WulingFrontendData::Service(this);const auto State=Service?Service->Snapshot():nullptr;
    const auto Event=Object(State,TEXT("launch"));
    if(ConnectionLabel)
    {
        FString Status=Phase(State);
        if(!Service||!Service->IsEnabled())Status=TEXT("活动服务尚未启用");
        else if(!Service->IsConnected())Status=Event?TEXT("连接已断开 · 上次确认记录"):TEXT("等待连接活动服务");
        else if(!State||!State->HasField(TEXT("launch")))Status=TEXT("活动资料同步中");
        else if(!AllowedProfile(State))Status=TEXT("当前版本未开放活动");
        else if(Service->IsBusy())Status+=TEXT(" · 请求确认中");
        ConnectionLabel->SetText(FText::FromString(Status));
    }
    if(TimeLabel)
    {
        FString Time=TEXT("开启后同步服务截止时间");
        if(Event)
        {
            if(HasNumber(Event,TEXT("end"))&&HasNumber(State,TEXT("server_utc")))
            {
                const double End=Number(Event,TEXT("end")),Now=Number(State,TEXT("server_utc"));
                Time=FString::Printf(TEXT("截止 %s\n%s%s"),*Date(End),Service&&Service->IsConnected()?TEXT("服务剩余 "):TEXT("上次确认剩余 "),*Remaining(End-Now));
            }
            else Time=TEXT("截止时间或服务时间待同步");
        }
        TimeLabel->SetText(FText::FromString(Time));
    }
    if(HonorExpiryLabel)
    {
        const auto Title=OwnedTitle(State);FString Text;
        if(Title)
        {
            const bool Expired=Number(Title,TEXT("expiry"))<=Number(State,TEXT("server_utc"));
            Text=(Expired?FString(TEXT("称号已到期 · ")):FString(TEXT("称号到期 · ")))+Date(Number(Title,TEXT("expiry")));
        }
        else Text=TEXT("称号尚未入账，获得后显示真实到期时间。");
        HonorExpiryLabel->SetText(FText::FromString(Text));
    }
}

void UInkLaunchEventPanel::RefreshLiveData()
{
    if(!FrontendOwner||FrontendOwner->CurrentPage!=PageId)return;
    auto* Service=WulingFrontendData::Service(this);const auto State=Service?Service->Snapshot():nullptr;
    if(Service)
    {
        const FString StartResponse=InkLaunchEventView::CompletedStartFeedback(bStartRequestPending,State,Service->IsConnected(),Service->IsBusy(),Service->GetStatus());
        if(!StartResponse.IsEmpty())FeedbackMessage(StartResponse);
    }
    const FString Key=InkLaunchEventView::ViewKey(State,Service&&Service->IsEnabled(),Service&&Service->IsConnected(),Service&&Service->IsBusy(),true);
    if(Key!=RenderKey)RebuildPanel();
    else UpdateLiveLabels();
}

void UInkLaunchEventPanel::HandlePanelAction(const FString& Id)
{
    using namespace InkLaunchEventView;
    if(Id.StartsWith(TEXT("LaunchTab_")))
    {
        const FString Tab=Id.Mid(10);
        if(Tab!=TEXT("overview")&&Tab!=TEXT("tasks")&&Tab!=TEXT("guild")&&Tab!=TEXT("honors"))return;
        if(Tab==SelectedTab)return;
        if(ResponsiveScroll)TabScrollOffsets.Add(SelectedTab,ResponsiveScroll->GetScrollOffset());
        SelectedTab=Tab;RebuildPanel();
        if(ResponsiveScroll)ResponsiveScroll->SetScrollOffset(TabScrollOffsets.FindRef(Tab));return;
    }
    if(Id==TEXT("LaunchSect")){Open(TEXT("Wuling"));return;}
    if(Id==TEXT("LaunchPrimary"))
    {
        auto* Service=WulingFrontendData::Service(this);const auto State=Service?Service->Snapshot():nullptr;
        const bool Active=FrontendOwner&&FrontendOwner->CurrentPage==PageId;
        if(!Active||!Service||!Service->IsEnabled()||!Service->IsConnected()||Service->IsBusy()||!AllowedProfile(State))
        {FeedbackMessage(TEXT("服务未就绪，请等待连接或当前请求确认。"));return;}
        const auto Event=Object(State,TEXT("launch"));
        if(Event)
        {
            if(String(Event,TEXT("status"))==TEXT("Active")&&Number(Event,TEXT("end"))>Number(State,TEXT("server_utc")))Open(TEXT("WulingWorld"));
            else FeedbackMessage(TEXT("本期活动已结束，请查看已确认的修行记录。"));
            return;
        }
        if(!CanStart(State,true,true,false,Active)){FeedbackMessage(TEXT("仅本武灵主播可在活动开放时开启，请先同步完整资料。"));return;}
        bStartRequestPending=Service->StartLaunchEvent();
        FeedbackMessage(bStartRequestPending?TEXT("开启请求已提交，等待服务确认。" ):Service->GetStatus());
        RefreshLiveData();return;
    }
    Super::HandlePanelAction(Id);
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkLaunchStartFeedbackTest,"Boundless.InkUI.Launch.StartFeedback",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkLaunchStartFeedbackTest::RunTest(const FString& Parameters)
{
    using namespace InkLaunchEventView;
    auto State=MakeShared<FJsonObject>();bool Pending=true;
    TestTrue(TEXT("Busy start keeps pending feedback"),CompletedStartFeedback(Pending,State,true,true,TEXT("忙碌")).IsEmpty());
    TestTrue(TEXT("Busy start retains pending tracking"),Pending);
    TestTrue(TEXT("Disconnected start keeps pending feedback"),CompletedStartFeedback(Pending,State,false,false,TEXT("断线")).IsEmpty());
    TestTrue(TEXT("Disconnected start retains pending tracking"),Pending);
    State->SetObjectField(TEXT("launch"),MakeShared<FJsonObject>());
    TestEqual(TEXT("Idle authority event confirms the submitted start"),CompletedStartFeedback(Pending,State,true,false,TEXT("操作已确认")),FString(TEXT("青云初启已开启，服务已确认。")));
    TestFalse(TEXT("Confirmed start clears pending tracking"),Pending);
    TestTrue(TEXT("Later polls do not overwrite feedback again"),CompletedStartFeedback(Pending,State,true,false,TEXT("后续状态")).IsEmpty());
    Pending=true;State->SetField(TEXT("launch"),MakeShared<FJsonValueNull>());
    TestEqual(TEXT("Idle rejection displays the service reason"),CompletedStartFeedback(Pending,State,true,false,TEXT("活动未开放")),FString(TEXT("活动未开放")));
    TestFalse(TEXT("Rejected start clears pending tracking"),Pending);return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkLaunchAuthorityGuardTest,"Boundless.InkUI.Launch.AuthorityGuard",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkLaunchAuthorityGuardTest::RunTest(const FString& Parameters)
{
    using namespace InkLaunchEventView;
    auto State=MakeShared<FJsonObject>(),Identity=MakeShared<FJsonObject>(),Guild=MakeShared<FJsonObject>(),Manifest=MakeShared<FJsonObject>();
    Identity->SetStringField(TEXT("role"),TEXT("anchor"));Identity->SetStringField(TEXT("user"),TEXT("host"));
    Guild->SetStringField(TEXT("anchor"),TEXT("host"));Guild->SetStringField(TEXT("id"),TEXT("guild"));
    Manifest->SetStringField(TEXT("profile"),TEXT("prototype_full"));Manifest->SetNumberField(TEXT("window_end"),2000);
    State->SetObjectField(TEXT("identity"),Identity);State->SetObjectField(TEXT("guild"),Guild);State->SetObjectField(TEXT("manifest"),Manifest);State->SetNumberField(TEXT("server_utc"),1000);
    TestFalse(TEXT("An incomplete snapshot cannot start an activity"),CanStart(State,true,true,false,true));
    State->SetField(TEXT("launch"),MakeShared<FJsonValueNull>());
    TestTrue(TEXT("Known absent event with matching authority can start"),CanStart(State,true,true,false,true));
    TestFalse(TEXT("Disconnected commands are rejected"),CanStart(State,true,false,false,true));
    TestFalse(TEXT("Busy commands are rejected"),CanStart(State,true,true,true,true));
    TestFalse(TEXT("Inactive page commands are rejected"),CanStart(State,true,true,false,false));
    TestFalse(TEXT("Disabled service cannot start"),CanStart(State,false,true,false,true));
    Identity->SetStringField(TEXT("role"),TEXT("viewer"));TestFalse(TEXT("Viewers cannot start"),CanStart(State,true,true,false,true));Identity->SetStringField(TEXT("role"),TEXT("anchor"));
    Guild->SetStringField(TEXT("anchor"),TEXT("other"));TestFalse(TEXT("Other guild cannot start"),CanStart(State,true,true,false,true));Guild->SetStringField(TEXT("anchor"),TEXT("host"));
    Manifest->SetStringField(TEXT("profile"),TEXT("douyin_release_candidate"));TestFalse(TEXT("Closed release profile cannot start"),CanStart(State,true,true,false,true));Manifest->SetStringField(TEXT("profile"),TEXT("prototype_full"));
    State->SetNumberField(TEXT("server_utc"),2000);TestFalse(TEXT("Expired entitlement cannot start"),CanStart(State,true,true,false,true));
    State->SetNumberField(TEXT("server_utc"),1000);State->SetObjectField(TEXT("launch"),MakeShared<FJsonObject>());
    TestFalse(TEXT("Existing event cannot start again"),CanStart(State,true,true,false,true));return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInkLaunchViewKeyTest,"Boundless.InkUI.Launch.StableViewKey",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FInkLaunchViewKeyTest::RunTest(const FString& Parameters)
{
    using namespace InkLaunchEventView;
    auto State=MakeShared<FJsonObject>(),Event=MakeShared<FJsonObject>(),Personal=MakeShared<FJsonObject>(),Task=MakeShared<FJsonObject>();
    State->SetObjectField(TEXT("launch"),Event);State->SetNumberField(TEXT("server_utc"),1000);State->SetNumberField(TEXT("version"),1);
    Event->SetStringField(TEXT("status"),TEXT("Active"));Event->SetNumberField(TEXT("end"),2000);Event->SetObjectField(TEXT("personal"),Personal);
    Task->SetNumberField(TEXT("progress"),1);Task->SetBoolField(TEXT("confirmed"),false);Event->SetArrayField(TEXT("tasks"),{MakeShared<FJsonValueObject>(Task)});
    const FString Before=ViewKey(State,true,true,false,true);
    State->SetNumberField(TEXT("server_utc"),1001);State->SetNumberField(TEXT("version"),900);
    Personal->SetObjectField(TEXT("rounds"),MakeShared<FJsonObject>());
    TestEqual(TEXT("Unrelated polls and clock ticks preserve the widget tree"),ViewKey(State,true,true,false,true),Before);
    Task->SetBoolField(TEXT("confirmed"),true);const FString Confirmed=ViewKey(State,true,true,false,true);
    TestTrue(TEXT("Authority confirmation refreshes the rows"),Confirmed!=Before);
    TestTrue(TEXT("Connection loss refreshes the action state"),ViewKey(State,true,false,false,true)!=Confirmed);
    State->SetNumberField(TEXT("server_utc"),2000);
    TestTrue(TEXT("Server expiry refreshes the action state"),ViewKey(State,true,true,false,true)!=Confirmed);return true;
}
#endif
