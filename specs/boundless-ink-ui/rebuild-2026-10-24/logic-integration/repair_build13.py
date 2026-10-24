from pathlib import Path
import runpy,re
R=Path.cwd();H=R/'specs/boundless-ink-ui/rebuild-2026-10-24/logic-integration';B='Source/BoundlessImmortality/Frontend/UIRebuild20261024/'
rec=runpy.run_path(str(R/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/record_source_change.py'),run_name='build13_repairs')
def edit(p,fn,reason='build13 exact UE5.8 compiler diagnostics; retain staged source behavior'):
 t=R/p;s=t.read_text(encoding='utf-8-sig');n=fn(s);assert s!=n,p;rec['begin'](p,reason);t.write_text(n,encoding='utf-8');rec['finish'](p)
def localsafe(s):
 pattern=r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\bSlot\b|\bCursor\b'
 def replacement(m):
  if m[0] not in ('Slot','Cursor'):return m[0]
  before=s[:m.start()].rstrip()
  if before.endswith(('->','.','::')):return m[0]
  return 'MemberCursor' if m[0]=='Cursor' else 'WidgetSlot'
 return re.sub(pattern,replacement,s)
def journey(s):
 s=localsafe(s).replace('bNow?LandscapeLayout:PortraitLayout','bNow?static_cast<UWidget*>(LandscapeLayout.Get()):static_cast<UWidget*>(PortraitLayout.Get())')
 at=s.index('void UInkJourneyDailySignInPageWidget::NativeOnInitialized')
 s=s[:at]+re.sub(r'(?<![:\w])Card\(', 'InkJourneyNew::Card(', s[at:])
 old='if(auto* S=CalendarGrid->AddChildToUniformGrid(CellBox,(D-1)/7,(D-1)%7))S->SetPadding(FMargin(2));'
 assert old in s;s=s.replace(old,'CalendarGrid->SetSlotPadding(FMargin(2));CalendarGrid->AddChildToUniformGrid(CellBox,(D-1)/7,(D-1)%7);')
 s=s.replace('FSlateFontInfo(Asset,SafeSize)','FSlateFontInfo(Asset,FMath::RoundToInt(SafeSize*.75f))').replace('FMath::RoundToInt(SafeSize)','FMath::RoundToInt(SafeSize*.75f)')
 return s
edit(B+'Journey/InkJourneyNewWidgets.cpp',journey)
for name in ['InkRebuildFeatureHost.cpp','InkRebuildFeaturePlayer.cpp','InkRebuildFeatureRank.cpp','Activities/RebuildActivitySecondaryWidgets.cpp']:
 edit(B+name,localsafe)
edit(B+'Player/InkPlayerBreakthroughTribulationWidgets.cpp',lambda s:s.replace('ViewModel.ConfirmationReworkMode','ViewModel.bConfirmationReworkMode'))
def activities(s):
 return localsafe(s).replace('CategoryName(', 'ActivityRebuildUI::CategoryName(').replace('FText ActivityRebuildUI::CategoryName(', 'FText CategoryName(').replace('FSlateChildSize(ESlateSizeRule::Fill, 1.f)','FSlateChildSize(ESlateSizeRule::Fill)')
edit(B+'Activities/RebuildActivityWidgets.cpp',activities)
def economy(s):
 s=s.replace('FindByPredicate([&Model](const auto& Row) { return Row.ItemId == ConfirmedItemId; })','FindByPredicate([this](const auto& Row) { return Row.ItemId == ConfirmedItemId; })')
 s=s.replace('!ReadPositiveCount(ViewModel.DraftQuantityText, Count)','!RebuildEconomyUI::ReadPositiveCount(ViewModel.DraftQuantityText, Count)')
 for name in ['QuantityInput','PriceInput']:
  old=name+'->SetFont(InkRebuildUI::Font(InkRebuildUI::ETextRole::Body, 20));';assert old in s
  s=s.replace(old,'{auto InputStyle='+name+'->GetWidgetStyle();InputStyle.SetFont(InkRebuildUI::Font(InkRebuildUI::ETextRole::Body,26));'+name+'->SetWidgetStyle(InputStyle);}')
 return s
edit(B+'Activities/RebuildEconomyWidgets.cpp',economy)
def player(s):
 s=s.replace('const FString Fingerprint=D::String(Stage,TEXT("stage_id"))+TEXT("|")+D::Json(Stage);','const FString Fingerprint=D::String(Stage,TEXT("stage_id"))+TEXT("|")+D::Version(Stage,TEXT("from_realm_index"))+TEXT("|")+D::Version(Stage,TEXT("to_realm_index"));')
 s=s.replace('const auto Output=D::Object(Selected,TEXT("output"));Provider(Vm.OutputState,W,Output.IsValid());Vm.Output.ItemId=FName(*D::String(Output,TEXT("item_id")));Vm.Output.Name=D::Text(Output,TEXT("name"));int32 Count=0;Vm.Output.bCountKnown=D::Integer(Output,TEXT("count"),Count);','const auto Output=Selected;Provider(Vm.OutputState,W,D::Has(Output,TEXT("output_item")));Vm.Output.ItemId=FName(*D::String(Output,TEXT("output_item")));Vm.Output.Name=FText::FromName(Vm.Output.ItemId);int32 Count=0;Vm.Output.bCountKnown=D::Integer(Output,TEXT("output_count"),Count);')
 s=s.replace('Source.Source=D::Text(Row,TEXT("source"));Source.Activity=D::Text(Row,TEXT("activity"));','Source.Source=D::Text(D::Object(Row,TEXT("source")),TEXT("name"));Source.Activity=D::Text(Row,TEXT("note"));')
 old='if(!Entry||Entry->Item.ItemId!=P.ItemId){Feedback(TEXT("物品选择已变化"));return;}'
 assert old in s
 s=s.replace(old,'''bool IdentityMatches=Entry&&Entry->Item.ItemId==P.ItemId;
    if(Id==TEXT("learn-method")&&Entry){IdentityMatches=false;for(const auto& Row:D::Rows(D::Catalog(Service()?Service()->Snapshot():nullptr,TEXT("methods")),TEXT("rows")))if(D::String(Row,TEXT("method_id"))==P.ItemId.ToString()&&D::String(Row,TEXT("item_id"))==Entry->Item.ItemId.ToString()&&D::String(Row,TEXT("item_type"))==TEXT("book")){IdentityMatches=true;break;}}
    if(!IdentityMatches){Feedback(TEXT("物品选择已变化"));return;}''')
 return s
edit(B+'InkRebuildFeaturePlayer.cpp',player,'Use exact current stage tuple and native flat recipe/source fields; validate captured learning GUID against actual book-to-method mapping')
edit(B+'Player/InkPlayerMethodsEquipmentAppearanceWidgets.cpp',lambda s:s.replace('LearnPayload.ItemId = ViewModel.SelectedMethodId;','LearnPayload.ItemId = ViewModel.SelectedMethodId;\n        LearnPayload.EntryId = ViewModel.SelectedInventoryEntryId;'),'Capture the actual displayed learning inventory GUID in the typed button intent')
edit(B+'InkRebuildFeatureJourney.cpp',lambda s:s.replace('TEXT("world_round")','TEXT("round")'),'Use the existing authoritative world round snapshot key')
edit(B+'InkRebuildFeatureRank.cpp',lambda s:s.replace('FInkWulingMembersData Vm;Vm.State=State;','FInkWulingMembersData Vm;Vm.PageSize=20;Vm.State=State;'),'Match existing 20-member native service page protocol for previous cursor')
