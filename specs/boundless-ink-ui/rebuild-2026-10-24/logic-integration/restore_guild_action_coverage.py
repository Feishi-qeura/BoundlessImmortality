from pathlib import Path
import runpy
R=Path.cwd();H=R/'specs/boundless-ink-ui/rebuild-2026-10-24/logic-integration';B='Source/BoundlessImmortality/Frontend/UIRebuild20261024/'
rec=runpy.run_path(str(R/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/record_source_change.py'),run_name='action_coverage_guild')
def edit(p,fn,reason):
 t=R/p;s=t.read_text(encoding='utf-8-sig');n=fn(s);assert s!=n,p;rec['begin'](p,reason);t.write_text(n,encoding='utf-8');rec['finish'](p)
p=B+'InkRebuildFeatureGuild.cpp';rec['begin'](p,'Restore actual create/rename/dissolve actions through native service APIs with actor/guild/version confirmation and typed user input');(R/p).write_text((H/'InkRebuildFeatureGuild.cpp.stage').read_text(encoding='utf-8'),encoding='utf-8');rec['finish'](p)
edit(B+'InkRebuildFeatureHost.h',lambda s:s.replace('    FString DraftQuantity=','    FString PendingGuildOperation,CapturedGuildActor,CapturedGuildId,CapturedGuildVersion;\n    FName DraftPreviewFrame,DraftPreviewBar;\n    FString DraftQuantity='),'Retain only local confirmation subject and appearance drafts; no authoritative ledger writes')
def rank(s):
 old='void UInkRebuildFeatureHost::HandleWulingAction(FName Action){if(SupportsRoute(Action))Navigate(Action);}\n';assert old in s;s=s.replace(old,'')
 marker='        for(const auto& Action:Actions){FInkWulingActionView Entry;'
 at=s.index(marker)
 s=s[:at]+'''        const auto Identity=D::Object(DisplaySnapshot,TEXT("identity"));const bool Manage=CanDispatch()&&!D::String(Guild,TEXT("id")).IsEmpty()&&D::String(Guild,TEXT("anchor"))==D::String(Identity,TEXT("user"));
        const TPair<const TCHAR*,const TCHAR*> Governance[]={{TEXT("GuildCreate"),TEXT("创建武灵")},{TEXT("GuildRename"),TEXT("武灵改名")},{TEXT("GuildDissolve"),TEXT("解散武灵")}};
        for(const auto& Choice:Governance){FInkWulingActionView Entry;Entry.ActionId=Choice.Key;Entry.Label=FText::FromString(Choice.Value);Entry.bEnabled=Entry.ActionId==TEXT("GuildCreate")?CanDispatch()&&DisplaySnapshot&&DisplaySnapshot->HasField(TEXT("guild"))&&!Guild:Manage;Vm.ExistingActions.Add(Entry);}
        FInkWulingActionView Shared;Shared.ActionId=TEXT("GuildShared");Shared.Label=FText::FromString(TEXT("武灵共修"));Shared.bEnabled=true;Vm.ExistingActions.Add(Shared);
'''+s[at:]
 return s
edit(B+'InkRebuildFeatureRank.cpp',rank,'Restore governance action entries and exact native guild-owner permissions; retain navigation and member operations')
def host(s):
 s=s.replace('FrontendOwner=InFrontend;PageId=InPage;SetIsFocusable(true);','FrontendOwner=InFrontend;PageId=InPage;bConfirmOpen=false;PendingGuildOperation.Empty();if(auto* Dialog=WidgetTree?WidgetTree->FindWidget(TEXT("NewGuildDialog")):nullptr)Dialog->SetVisibility(ESlateVisibility::Collapsed);SetIsFocusable(true);')
 s=s.replace('if(PurePage)PurePage->RemoveFromParent();PurePage=nullptr;RequestedCatalog.Empty();','RetirePurePage();RequestedCatalog.Empty();PendingGuildOperation.Empty();if(auto* Dialog=WidgetTree->FindWidget(TEXT("NewGuildDialog")))Dialog->SetVisibility(ESlateVisibility::Collapsed);')
 marker='    UClass* Class=nullptr;'
 s=s.replace(marker,marker)
 old='CompositionName=Class->GetPathName();auto* WidgetSlot=BodyCanvas->AddChildToCanvas(OwnerPage);WidgetSlot->SetAnchors(FAnchors(0,0,1,1));WidgetSlot->SetOffsets(FMargin(24));';assert old in s
 s=s.replace(old,'''CompositionName=Class->GetPathName();
    if(PageId==TEXT("Wuling"))
    {
        auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("NewFeatureWulingScroll"));PurePage=Scroll;auto* Layout=BodyCanvas->AddChildToCanvas(Scroll);Layout->SetAnchors(FAnchors(0,0,1,1));Layout->SetOffsets(FMargin(24));
        auto* Extent=WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("NewFeatureWulingExtent"));Scroll->AddChild(Extent);Extent->SetContent(OwnerPage);
    }
    else{auto* WidgetSlot=BodyCanvas->AddChildToCanvas(OwnerPage);WidgetSlot->SetAnchors(FAnchors(0,0,1,1));WidgetSlot->SetOffsets(FMargin(24));}''')
 marker='    Move(TEXT("NewFeatureBack"),FVector4(LastSize.X-112,24,88,88));'
 s=s.replace(marker,marker+'''
    if(PageId==TEXT("Wuling")&&OwnerPage)if(auto* Extent=Cast<USizeBox>(WidgetTree->FindWidget(TEXT("NewFeatureWulingExtent")))){Extent->SetWidthOverride(FMath::Max(88.f,LastSize.X-64));const float Desired=OwnerPage->GetDesiredSize().Y;if(Desired>0)Extent->SetHeightOverride(FMath::Max(LastSize.Y-136,Desired));}
    const float DialogWidth=FMath::Min(600.f,LastSize.X-48);Move(TEXT("NewGuildDialog"),FVector4((LastSize.X-DialogWidth)*.5f,(LastSize.Y-420)*.5f,DialogWidth,420));Move(TEXT("NewGuildDialogPaper"),FVector4(0,0,DialogWidth,420));Move(TEXT("NewGuildDialogTitle"),FVector4(24,24,DialogWidth-48,64));Move(TEXT("NewGuildDialogSubject"),FVector4(24,88,DialogWidth-48,96));Move(TEXT("NewGuildNameInput"),FVector4(24,188,DialogWidth-48,88));Move(TEXT("NewGuildCancel"),FVector4(24,308,(DialogWidth-72)*.5f,88));Move(TEXT("NewGuildConfirm"),FVector4(48+(DialogWidth-72)*.5f,308,(DialogWidth-72)*.5f,88));''')
 s=s.replace('const FString Id=Action.ToString();if(Id==TEXT("ToggleRelated"))','const FString Id=Action.ToString();if(Id==TEXT("GuildConfirm")||Id==TEXT("GuildCancel")){HandleWulingAction(Action);return;}if(Id==TEXT("ToggleRelated"))')
 s=s.replace('for(FName Route:Routes)if(Route!=Page)Out.Emplace','for(FName Route:Routes)if(Route!=Page&&(Route!=TEXT("QueryItems")||Page==TEXT("Market")))Out.Emplace')
 return s
edit(B+'InkRebuildFeatureHost.cpp',host,'Keep new governance controls bounded and Wuling actions scrollable; clear confirmations on reentry/actor change; expose item query only on Market')
