#include "InkRebuildFeatureHost.h"
#include "InkRebuildFeatureData.h"
#include "InkRebuildTheme.h"
#include "Frontend/UiRollout20261024/LogicCore/InkCoreAdapter.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Wuling/WulingPrototypeSubsystem.h"
namespace D=InkNewFeatureData;
void UInkRebuildFeatureHost::HandleWulingAction(FName Action)
{
    if(!IsActivePage())return;
    if(SupportsRoute(Action)){Navigate(Action);return;}
    if(Action==TEXT("GuildShared")){Navigate(TEXT("DailyActivityDetail"),TEXT("ACT_SHARED"));return;}
    auto* Dialog=WidgetTree?WidgetTree->FindWidget(TEXT("NewGuildDialog")):nullptr;
    if(Action==TEXT("GuildCancel")){PendingGuildOperation.Empty();if(Dialog)Dialog->SetVisibility(ESlateVisibility::Collapsed);return;}
    auto* W=Service();const auto State=W?W->Snapshot():nullptr,Guild=D::Object(State,TEXT("guild")),Identity=D::Object(State,TEXT("identity"));
    const FString GuildId=D::String(Guild,TEXT("id")),Actor=D::Identity(State),Version=D::Version(State);
    if(Action==TEXT("GuildConfirm"))
    {
        if(!CanDispatch()||PendingGuildOperation.IsEmpty()||CapturedGuildActor!=Actor||CapturedGuildId!=GuildId||CapturedGuildVersion.IsEmpty()||CapturedGuildVersion!=Version)
        {Feedback(TEXT("组织、身份或服务状态已变化，请重新确认"));PendingGuildOperation.Empty();if(Dialog)Dialog->SetVisibility(ESlateVisibility::Collapsed);return;}
        const auto* Input=Cast<UEditableTextBox>(WidgetTree->FindWidget(TEXT("NewGuildNameInput")));const FString Name=Input?Input->GetText().ToString().TrimStartAndEnd():FString();
        if(PendingGuildOperation!=TEXT("DissolveGuild")&&Name.IsEmpty()){Feedback(TEXT("请输入武灵名称"));return;}
        if(PendingGuildOperation==TEXT("CreateGuild"))Feedback(InkCore::CreateGuild(this,Name,IsActivePage()).Message);
        else
        {
            if(GuildId.IsEmpty()||D::String(Guild,TEXT("anchor"))!=D::String(Identity,TEXT("user")))return;
            auto Args=MakeShared<FJsonObject>();if(PendingGuildOperation==TEXT("RenameGuild"))Args->SetStringField(TEXT("name"),Name);else Args->SetBoolField(TEXT("confirmed"),true);Submit(PendingGuildOperation,Args);
        }
        PendingGuildOperation.Empty();if(Dialog)Dialog->SetVisibility(ESlateVisibility::Collapsed);RefreshAuthority(true);return;
    }
    if(Action!=TEXT("GuildCreate")&&Action!=TEXT("GuildRename")&&Action!=TEXT("GuildDissolve"))return;
    if(!CanDispatch()||!State||!State->HasField(TEXT("guild")))return;
    const bool Create=Action==TEXT("GuildCreate"),Dissolve=Action==TEXT("GuildDissolve");
    if(Create?Guild.IsValid():(GuildId.IsEmpty()||D::String(Guild,TEXT("anchor"))!=D::String(Identity,TEXT("user"))))return;
    PendingGuildOperation=Create?TEXT("CreateGuild"):Dissolve?TEXT("DissolveGuild"):TEXT("RenameGuild");CapturedGuildActor=Actor;CapturedGuildId=GuildId;CapturedGuildVersion=Version;
    const float Width=FMath::Min(600.f,LastSize.X-48),Height=420;
    if(!Dialog)
    {
        auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("NewGuildDialog"));Dialog=Canvas;
        InkRebuildUI::Place(RootCanvas,Canvas,FVector4((LastSize.X-Width)*.5f,(LastSize.Y-Height)*.5f,Width,Height),100);
        InkRebuildUI::Card(WidgetTree,Canvas,TEXT("NewGuildDialogPaper"),FVector4(0,0,Width,Height));
        InkRebuildUI::Text(WidgetTree,Canvas,TEXT("NewGuildDialogTitle"),FText(),InkRebuildUI::ETextRole::Section,FVector4(24,24,Width-48,64));
        InkRebuildUI::Text(WidgetTree,Canvas,TEXT("NewGuildDialogSubject"),FText(),InkRebuildUI::ETextRole::Body,FVector4(24,88,Width-48,96));
        auto* Input=WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(),TEXT("NewGuildNameInput"));auto Style=Input->GetWidgetStyle();Style.SetFont(InkRebuildUI::Font(InkRebuildUI::ETextRole::Body));Input->SetWidgetStyle(Style);Input->SetHintText(FText::FromString(TEXT("输入武灵名称")));InkRebuildUI::Place(Canvas,Input,FVector4(24,188,Width-48,88));
        InkRebuildUI::Action(WidgetTree,Canvas,FrontendOwner,TEXT("NewGuildCancel"),TEXT("Ext_GuildCancel"),FText::FromString(TEXT("取消")),FVector4(24,308,(Width-72)*.5f,88));
        InkRebuildUI::Action(WidgetTree,Canvas,FrontendOwner,TEXT("NewGuildConfirm"),TEXT("Ext_GuildConfirm"),FText::FromString(TEXT("确认")),FVector4(48+(Width-72)*.5f,308,(Width-72)*.5f,88),true);
    }
    if(auto* Title=Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("NewGuildDialogTitle"))))Title->SetText(FText::FromString(Create?TEXT("创建武灵"):Dissolve?TEXT("确认解散武灵"):TEXT("武灵改名")));
    if(auto* Subject=Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("NewGuildDialogSubject"))))Subject->SetText(Dissolve?FText::Format(FText::FromString(TEXT("解散 {0} 后公库不返还，请确认当前组织。")),D::Named(Guild,TEXT("name"),GuildId)):FText::FromString(TEXT("名称和操作结果以服务确认记录为准。")));
    if(auto* Input=Cast<UEditableTextBox>(WidgetTree->FindWidget(TEXT("NewGuildNameInput")))){Input->SetText(Create?FText():D::Text(Guild,TEXT("name")));Input->SetVisibility(Dissolve?ESlateVisibility::Collapsed:ESlateVisibility::Visible);if(!Dissolve)Input->SetKeyboardFocus();}
    Dialog->SetVisibility(ESlateVisibility::Visible);LayoutShell();
}
