from pathlib import Path
import runpy
R=Path.cwd();H=R/'specs/boundless-ink-ui/rebuild-2026-10-24/logic-integration';B='Source/BoundlessImmortality/Frontend/UIRebuild20261024/'
rec=runpy.run_path(str(R/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/record_source_change.py'),run_name='supported_activity_actions')
def edit(p,fn,reason):
 t=R/p;s=t.read_text(encoding='utf-8-sig');n=fn(s);assert s!=n,p;rec['begin'](p,reason);t.write_text(n,encoding='utf-8');rec['finish'](p)
def activities(s):
 s=s.replace('Vm.StartSubject.RulesVersion=D::Version(DisplaySnapshot,TEXT("rules_version"));','Vm.StartSubject.RulesVersion=D::String(Manifest,TEXT("rule_version"));')
 s=s.replace('Captured.RulesVersion!=D::Version(State,TEXT("rules_version"))','Captured.RulesVersion!=D::String(Manifest,TEXT("rule_version"))')
 old='Vm.bHasSelectedActivity=true;}Detail->ApplyViewModel(Vm);';assert old in s
 s=s.replace(old,'''Vm.bHasSelectedActivity=true;}
        if(!Vm.bHasSelectedActivity)for(const auto& Row:View.Templates.Items)if(D::String(Row,TEXT("activity_id"))==SelectedEntry){Vm.SelectedActivity.Id=FName(*SelectedEntry);Vm.SelectedActivity.Cycle=FName(*D::String(Row,TEXT("cycle")));Vm.SelectedActivity.Title=D::Named(Row,TEXT("name"),SelectedEntry);Vm.SelectedActivity.Summary=D::Text(Row,TEXT("description"));Vm.SelectedActivity.State=ActivityState(W,true);Vm.bHasSelectedActivity=true;break;}
        if(Vm.bHasSelectedActivity&&(SelectedEntry==TEXT("ACT_SHARED")||SelectedEntry==TEXT("WEEK_GUARD")||SelectedEntry==TEXT("WEEK_IRON")))for(const auto& Row:View.Templates.Items)if(D::String(Row,TEXT("activity_id"))==SelectedEntry){Vm.SelectedActivity.AllowedAction=TEXT("start-supported-activity");Vm.SelectedActivity.ActionLabel=FText::FromString(TEXT("请求开始活动"));Vm.SelectedActivity.bActionAllowed=CanDispatch()&&D::Boolean(Row,TEXT("enabled"));Vm.SelectedActivity.ActionDisabledReason=Vm.SelectedActivity.bActionAllowed?FText():FText::FromString(TEXT("活动模板未开放或服务尚未就绪"));break;}
        Detail->ApplyViewModel(Vm);''')
 marker='    if(!Request.bConfirmed)return;const auto State=Service()?Service()->Snapshot():nullptr;';assert marker in s
 s=s.replace(marker,'''    if(Action==TEXT("start-supported-activity")||Action==TEXT("end-supported-activity"))
    {
        const FString Id=Request.EntryId.ToString();if(Id!=SelectedEntry||(Id!=TEXT("ACT_SHARED")&&Id!=TEXT("WEEK_GUARD")&&Id!=TEXT("WEEK_IRON"))||(Action==TEXT("end-supported-activity")&&Id!=TEXT("ACT_SHARED")))return;
        bool Enabled=false;const auto State=Service()?Service()->Snapshot():nullptr;for(const auto& Row:J::Read(J::Capture(Service(),IsActivePage())).Templates.Items)if(D::String(Row,TEXT("activity_id"))==Id)Enabled=D::Boolean(Row,TEXT("enabled"));if(!Enabled)return;
        auto Args=MakeShared<FJsonObject>();if(Action==TEXT("start-supported-activity"))Args->SetStringField(TEXT("activity_id"),Id);Submit(Action==TEXT("start-supported-activity")?TEXT("StartActivity"):TEXT("EndActivity"),Args);RefreshAuthority(true);return;
    }
'''+marker)
 return s
edit(B+'InkRebuildFeatureActivities.cpp',activities,'Bind only native supported activity IDs from current enabled templates; use actual manifest rule_version for launch confirmation')
def owner(s):
 marker='''        Add(Detail, Button(WidgetTree, Text(WidgetTree, ActionText, 24, White, ETextJustify::Center),
            Request, this), 0.f);''';assert s.count(marker)==1
 s=s.replace(marker,marker+'''
        if(Row.Id==TEXT("ACT_SHARED")&&Row.AllowedAction==TEXT("start-supported-activity"))Add(Detail,Button(WidgetTree,Text(WidgetTree,FText::FromString(TEXT("请求结束共修")),24,White,ETextJustify::Center),MakeRequest(TEXT("end-supported-activity"),Row.Id,Row.Cycle,ViewModel.ServiceDay),this),0.f);''')
 return s
edit(B+'Activities/RebuildActivityWidgets.cpp',owner,'Expose native end-meditation request without pretending current activity state or inventing rewards')
def appearance(s):
 old='IdentityPreview->SetBrush(SurfaceBrush(Style.Paper,PreviewTokens,ViewModel.bPreviewFrameKnown?3.f:1.f));';assert old in s
 return s.replace(old,'FSlateBrush PreviewBrush=SurfaceBrush(Style.Paper,PreviewTokens,ViewModel.bPreviewFrameKnown?3.f:1.f);PreviewBrush.DrawAs=ESlateBrushDrawType::RoundedBox;PreviewBrush.OutlineSettings.RoundingType=ESlateBrushRoundingType::FixedRadius;PreviewBrush.OutlineSettings.CornerRadii=FVector4(4,4,4,4);IdentityPreview->SetBrush(PreviewBrush);')
edit(B+'Player/InkPlayerMethodsEquipmentAppearanceWidgets.cpp',appearance,'Use actual rounded-box outline rendering for selected local frame color')
