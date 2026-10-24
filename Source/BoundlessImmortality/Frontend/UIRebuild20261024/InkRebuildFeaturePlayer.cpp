#include "InkRebuildFeatureHost.h"
#include "InkRebuildFeatureData.h"
#include "InkRebuildTheme.h"
#include "Frontend/InkFrontendWidget.h"
#include "Frontend/InkAnchorStatusWidget.h"
#include "Frontend/UiRollout20261024/LogicCore/InkCoreAdapter.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ScrollBox.h"
#include "Components/CanvasPanel.h"
#include "Wuling/WulingPrototypeSubsystem.h"
#include "Engine/Texture2D.h"
namespace D=InkNewFeatureData;
namespace C=InkPlayerMethodsEquipmentAppearanceWidgets;
namespace P=InkPlayerCraftSourcesWidgets;
namespace E=InkPlayerBreakthroughTribulationWidgets;
namespace
{
template<class T> T FeatureStyle()
{
    T S;S.Primary=InkRebuildUI::Primary();S.Secondary=InkRebuildUI::Muted();S.Accent=InkRebuildUI::Gold();
    S.Page=InkRebuildUI::Background();S.Paper=InkRebuildUI::Paper();S.Ink=InkRebuildUI::Ink();S.Muted=InkRebuildUI::Muted();
    S.Jade=InkRebuildUI::Jade();S.Warning=InkRebuildUI::Warning();S.Disabled=InkRebuildUI::Muted();
    S.TitleFont=InkRebuildUI::Font(InkRebuildUI::ETextRole::Title);S.SectionFont=InkRebuildUI::Font(InkRebuildUI::ETextRole::Section);
    S.BodyFont=InkRebuildUI::Font(InkRebuildUI::ETextRole::Body);S.ButtonFont=InkRebuildUI::Font(InkRebuildUI::ETextRole::Button);S.SecondaryFont=InkRebuildUI::Font(InkRebuildUI::ETextRole::Secondary);return S;
}
template<class T> void Provider(T& Slice,UWulingPrototypeSubsystem* W,bool Provided,bool Empty=false)
{
    using State=decltype(Slice.State);
    Slice.State=!W||!W->IsEnabled()?State::Disabled:!W->IsConnected()?State::Offline:Provided?(Empty?State::Empty:State::Data):State::Loading;
    Slice.Message=FText::FromString(W?W->GetStatus():TEXT("服务未启用"));
}
TArray<FName> OwnedMethods(const D::FObject& Loadout,bool& Known)
{const auto Methods=D::Object(Loadout,TEXT("methods"));Known=Methods.IsValid();TArray<FName> Out;if(Methods)for(const auto& Pair:Methods->Values)Out.Add(FName(*Pair.Key));return Out;}
bool SameSelection(const TMap<FString,int32>& A,const TMap<FString,int32>& B)
{if(A.Num()!=B.Num())return false;for(const auto& Pair:A){const int32* V=B.Find(Pair.Key);if(!V||*V!=Pair.Value)return false;}return true;}
}
void UInkRebuildFeatureHost::RetirePurePage()
{
    if(!PurePage)return;
    TArray<UWidget*> Retired;
    TFunction<void(UWidget*)> Gather=[&](UWidget* W){if(auto* Panel=Cast<UPanelWidget>(W))for(auto* Child:Panel->GetAllChildren())Gather(Child);Retired.Add(W);};
    Gather(PurePage);PurePage->RemoveFromParent();PurePage=nullptr;PureScroll=nullptr;
    for(auto* W:Retired)W->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);
}
void UInkRebuildFeatureHost::RefreshPlayer()
{
    const bool IsC=PageId==TEXT("CultivationMethods")||PageId==TEXT("EquipmentFabao")||PageId==TEXT("CharacterAppearance");
    const bool IsD=PageId==TEXT("WulingCraft")||PageId==TEXT("ItemSources");
    const bool IsE=PageId==TEXT("BreakthroughPrepare")||PageId==TEXT("Tribulation");
    if(!IsC&&!IsD&&!IsE)return;
    auto* W=Service();const auto View=InkCore::Read(FrontendOwner,IsActivePage());auto* Inventory=FrontendOwner->GetInventoryModel();
    TMap<FName,float> ScrollOffsets;
    TFunction<void(UWidget*,bool)> PreserveScroll=[&](UWidget* Widget,bool Restore)
    {
        if(!Widget)return;
        if(auto* Scroll=Cast<UScrollBox>(Widget))
        {if(Restore){if(const float* Offset=ScrollOffsets.Find(Scroll->GetFName()))Scroll->SetScrollOffset(*Offset);}else ScrollOffsets.Add(Scroll->GetFName(),Scroll->GetScrollOffset());}
        if(auto* Panel=Cast<UPanelWidget>(Widget))for(auto* Child:Panel->GetAllChildren())PreserveScroll(Child,Restore);
    };
    PreserveScroll(PurePage,false);RetirePurePage();UWidget* Body=nullptr;
    if(IsC)
    {
        const auto Style=FeatureStyle<C::FStyleTokens>();const auto Layout=LastSize.X>LastSize.Y?C::ELayoutMode::Landscape:LastSize.X<600?C::ELayoutMode::Compact:C::ELayoutMode::Portrait;
        const auto Callback=C::FActionRequested::CreateUObject(this,&ThisClass::HandlePlayerC);C::FPageAtlases Art;
        Art.MethodsGlyphs=LoadObject<UTexture2D>(nullptr,TEXT("/Game/newUI/Components/Player/T_MethodLoadoutGlyphAtlas_v1.T_MethodLoadoutGlyphAtlas_v1"));
        Art.EquipmentGlyphs=LoadObject<UTexture2D>(nullptr,TEXT("/Game/newUI/Components/Player/T_EquipmentFabaoGlyphAtlas_v1.T_EquipmentFabaoGlyphAtlas_v1"));
        Art.AppearanceOrnaments=LoadObject<UTexture2D>(nullptr,TEXT("/Game/newUI/Components/Player/T_AppearancePreviewOrnamentAtlas_v1.T_AppearancePreviewOrnamentAtlas_v1"));
        Art.CharacterAttributeGlyphs=LoadObject<UTexture2D>(nullptr,TEXT("/Game/newUI/Components/Player/T_CharacterAttributeGlyphAtlas_v1.T_CharacterAttributeGlyphAtlas_v1"));
        if(PageId==TEXT("CultivationMethods"))
        {
            C::FMethodsVm Vm;const auto Catalog=D::Catalog(DisplaySnapshot,TEXT("methods"));const auto Rows=D::Rows(Catalog,TEXT("rows"));
            Provider(Vm.CatalogState,W,D::Has(Catalog,TEXT("rows")),Rows.IsEmpty());Provider(Vm.InventoryState,W,Inventory&&(!W||!W->IsEnabled()||Inventory->IsAuthoritative()));
            Vm.Inventory=Inventory;Vm.SelectedInventoryEntryId=SelectedInventoryEntry;Vm.SelectedMethodId=FName(*SelectedEntry);
            const auto Loadout=D::Object(DisplaySnapshot,TEXT("loadout"));Vm.bMainKnown=D::Has(Loadout,TEXT("main"));Vm.MainMethodId=FName(*D::String(Loadout,TEXT("main")));
            Vm.SupportMethodIds=D::Names(Loadout,TEXT("supports"),&Vm.bSupportsKnown);bool OwnedKnown=false;Vm.OwnedMethodIds=OwnedMethods(Loadout,OwnedKnown);
            Provider(Vm.OwnedMethodsState,W,OwnedKnown,Vm.OwnedMethodIds.IsEmpty());Provider(Vm.MainState,W,Vm.bMainKnown);Provider(Vm.SupportsState,W,Vm.bSupportsKnown);
            Vm.DraftMainMethodId=DraftMainMethod;Vm.DraftSupportMethodIds=DraftSupportMethods;
            for(const auto& Row:Rows){if(D::String(Row,TEXT("item_type"))!=TEXT("book"))continue;C::FCatalogEntry Entry;Entry.ItemId=FName(*D::String(Row,TEXT("method_id")));Entry.Name=D::Named(Row,TEXT("name"),Entry.ItemId.ToString());Entry.Detail=D::Text(Row,TEXT("description"));Entry.bOwnedKnown=OwnedKnown;Entry.bOwned=Vm.OwnedMethodIds.Contains(Entry.ItemId);Vm.Catalog.Add(Entry);}
            const auto* Selected=Inventory?Inventory->FindEntry(SelectedInventoryEntry):nullptr;
            Vm.bLearnActionAvailable=CanDispatch()&&Selected&&Selected->Item.ItemId.ToString().StartsWith(TEXT("BOOK_"));Vm.LearnDisabledReason=FText::FromString(TEXT("选择实际持有的功法书，并等待服务就绪"));
            Vm.bConfigureActionAvailable=CanDispatch()&&OwnedKnown&&!DraftMainMethod.IsNone()&&Vm.OwnedMethodIds.Contains(DraftMainMethod);
            for(FName Support:DraftSupportMethods)Vm.bConfigureActionAvailable&=Vm.OwnedMethodIds.Contains(Support)&&Support!=DraftMainMethod;
            Body=C::BuildMethodsPage(WidgetTree,Vm,Style,Layout,Art,Callback);
        }
        else if(PageId==TEXT("EquipmentFabao"))
        {
            C::FEquipmentVm Vm;Provider(Vm.InventoryState,W,Inventory&&(!W||!W->IsEnabled()||Inventory->IsAuthoritative()));Provider(Vm.CatalogState,W,D::Catalog(DisplaySnapshot,TEXT("equipment")).IsValid());
            const auto Equipment=D::Object(DisplaySnapshot,TEXT("loadout"));Provider(Vm.EquipmentState,W,Equipment.IsValid());
            if(Inventory)for(const auto& Entry:Inventory->GetEntriesView())if(Entry.Item.Category==TEXT("Equipment")||Entry.Item.Category==TEXT("Treasure"))
            {
                C::FCatalogEntry Row;Row.EntryId=Entry.EntryId;Row.ItemId=Entry.Item.ItemId;Row.Name=Entry.Item.Name;Row.Detail=Entry.Item.Description;Row.Icon=Entry.Item.Icon.LoadSynchronous();Row.bOwnedKnown=true;Row.bOwned=true;Vm.OwnedItems.Add(Row);
                if(Entry.bEquipped){C::FEquipmentSlot WidgetSlot;WidgetSlot.SlotId=Entry.Item.EquipmentSlot;WidgetSlot.Label=FText::FromName(WidgetSlot.SlotId);WidgetSlot.bEntryKnown=true;WidgetSlot.EntryId=Entry.EntryId;WidgetSlot.ItemId=Entry.Item.ItemId;WidgetSlot.ItemName=Entry.Item.Name;WidgetSlot.Icon=Row.Icon;Vm.Slots.Add(WidgetSlot);}
            }
            Vm.SelectedEntryId=SelectedInventoryEntry;Vm.SelectedSlotId=FName(*SelectedEntry);const auto* Selected=Inventory?Inventory->FindEntry(SelectedInventoryEntry):nullptr;
            const bool Ready=IsActivePage()&&(!W||!W->IsEnabled()||CanDispatch());Vm.bEquipActionAvailable=Ready&&Selected&&Inventory->CanEquipEntry(SelectedInventoryEntry)&&!Selected->bEquipped;
            Vm.bUnequipActionAvailable=Ready&&Selected&&Selected->bEquipped;Vm.bFabaoKnown=D::Has(Equipment,TEXT("fabao"));Vm.FabaoName=Vm.bFabaoKnown&&D::String(Equipment,TEXT("fabao")).IsEmpty()?FText::FromString(TEXT("未装备法宝")):D::Text(Equipment,TEXT("fabao"));Vm.bActivateFabaoAvailable=CanDispatch()&&!D::String(Equipment,TEXT("fabao")).IsEmpty();
            Body=C::BuildEquipmentPage(WidgetTree,Vm,Style,Layout,Art,Callback);
        }
        else
        {
            C::FAppearanceVm Vm;const auto Identity=D::Object(DisplaySnapshot,TEXT("identity"));Provider(Vm.AppearanceState,W,Identity.IsValid());Vm.Identity=View.CharacterName;Vm.bIdentityKnown=!View.CharacterName.IsEmpty();Vm.ActiveTab=AppearanceTab;
            // Existing local presentation samples are not unlocks, ownership or saves.
            Vm.AppearanceState.State=C::EProviderState::Data;Vm.AppearanceState.Message=FText::FromString(TEXT("仅本地试戴，不改变持有或装备状态"));
            Vm.bFrameOptionsKnown=true;Vm.bBarOptionsKnown=true;
            for(const TCHAR* Name:{TEXT("素墨头像框"),TEXT("青金头像框"),TEXT("山水头像框")}){C::FPreviewOption Option;Option.StyleId=Name;Option.Label=FText::FromString(Name);Vm.FrameOptions.Add(Option);}
            for(const TCHAR* Name:{TEXT("普通"),TEXT("渐变"),TEXT("护盾"),TEXT("华丽")}){C::FPreviewOption Option;Option.StyleId=Name;Option.Label=FText::FromString(Name);Vm.BarOptions.Add(Option);}
            Vm.DraftPreviewFrame=DraftPreviewFrame;Vm.DraftPreviewBar=DraftPreviewBar;Vm.PreviewFrame=PreviewFrame;Vm.PreviewBar=PreviewBar;Vm.bPreviewFrameKnown=!PreviewFrame.IsNone();Vm.bPreviewBarKnown=!PreviewBar.IsNone();Vm.SelectedPreviewStyleId=AppearanceTab==TEXT("Bar")?DraftPreviewBar:DraftPreviewFrame;Vm.bPreviewActionAvailable=IsActivePage()&&!Vm.SelectedPreviewStyleId.IsNone();
            if(const auto* IdentityWidget=FrontendOwner->AnchorStatusWidget.Get()){Vm.Portrait=IdentityWidget->AvatarTexture;Vm.bPortraitKnown=Vm.Portrait!=nullptr;}
            Vm.PreviewDisabledReason=FText::FromString(TEXT("请选择本地试戴样式"));C::FCharacterAttributePreview Attributes;Provider(Attributes.State,W,!View.Attributes.IsEmpty());Attributes.Attributes=View.Attributes;
            Body=C::BuildAppearancePage(WidgetTree,Vm,Attributes,Style,Layout,Art,Callback);
        }
    }
    if(IsD)
    {
        const auto Style=FeatureStyle<P::FStyleTokens>();const auto Layout=LastSize.X>LastSize.Y?P::ELayoutMode::Landscape:LastSize.X<600?P::ELayoutMode::Compact:P::ELayoutMode::Portrait;
        const auto Callback=P::FActionRequested::CreateUObject(this,&ThisClass::HandlePlayerD);
        if(PageId==TEXT("WulingCraft"))
        {
            P::FRecipeVm Vm;P::FInventoryVm Bag;const auto Catalog=D::Catalog(DisplaySnapshot,TEXT("recipes"),QueryOffset);const auto Rows=D::Rows(Catalog,TEXT("rows"));
            Provider(Vm.CatalogState,W,D::Has(Catalog,TEXT("rows")),Rows.IsEmpty());Provider(Bag.State,W,Inventory&&Inventory->IsAuthoritative());
            for(const auto& Row:Rows){P::FRecipeRow Recipe;Recipe.RecipeId=FName(*D::CatalogId(Row,TEXT("recipes")));Recipe.Name=D::Named(Row,TEXT("name"),Recipe.RecipeId.ToString());Recipe.Summary=D::Text(Row,TEXT("description"));Vm.Recipes.Add(Recipe);}
            const auto Selected=D::FindCatalog(DisplaySnapshot,TEXT("recipes"),SelectedEntry);Vm.SelectedRecipeId=FName(*SelectedEntry);Provider(Vm.SelectionState,W,Selected.IsValid());
            bool IngredientsKnown=false;for(const auto& Row:D::Rows(Selected,TEXT("ingredients"),&IngredientsKnown)){P::FRecipeIngredient Ingredient;Ingredient.ItemId=FName(*D::String(Row,TEXT("item_id")));Ingredient.Name=D::Text(Row,TEXT("name"));int32 Count=0;Ingredient.bCountKnown=D::Integer(Row,TEXT("count"),Count);Ingredient.Count=Count;Vm.Ingredients.Add(Ingredient);}
            Provider(Vm.IngredientsState,W,IngredientsKnown,Vm.Ingredients.IsEmpty());const auto Output=Selected;Provider(Vm.OutputState,W,D::Has(Output,TEXT("output_item")));Vm.Output.ItemId=FName(*D::String(Output,TEXT("output_item")));Vm.Output.Name=FText::FromName(Vm.Output.ItemId);int32 Count=0;Vm.Output.bCountKnown=D::Integer(Output,TEXT("output_count"),Count);Vm.Output.Count=Count;
            if(Inventory)for(const auto& Entry:Inventory->GetEntriesView()){P::FInventoryEntry Item;Item.EntryId=Entry.EntryId;Item.ItemId=Entry.Item.ItemId;Item.Name=Entry.Item.Name;Item.Count=Entry.Item.Count;Bag.Entries.Add(Item);}
            Vm.QueryOffset=QueryOffset;Vm.bOwnedOnly=bOwnedOnly;Vm.bQueryAvailable=CanDispatch();Vm.bCraftAvailable=CanDispatch()&&Selected.IsValid()&&!SelectedEntry.IsEmpty();Vm.bCraftConfirmationOpen=bConfirmOpen;Vm.ConfirmationRecipeId=FName(*SecondaryEntry);Body=P::BuildCraftPage(WidgetTree,Vm,Bag,Style,Layout,Callback);
        }
        else
        {
            P::FItemSourceVm Vm;const auto Catalog=D::Catalog(DisplaySnapshot,TEXT("sources"),QueryOffset);const auto Rows=D::Rows(Catalog,TEXT("rows"));Provider(Vm.State,W,D::Has(Catalog,TEXT("rows")),Rows.IsEmpty());
            Vm.ItemId=FName(*SelectedEntry);Vm.SelectedSourceId=FName(*SecondaryEntry);Vm.QueryOffset=QueryOffset;Vm.bOwnedOnly=bOwnedOnly;Vm.bQueryAvailable=CanDispatch();
            for(const auto& Row:Rows){if(!SelectedEntry.IsEmpty()&&D::String(Row,TEXT("item_id"))!=SelectedEntry)continue;P::FItemSourceRow Source;Source.SourceId=FName(*D::String(Row,TEXT("source_id")));Source.ItemId=FName(*D::String(Row,TEXT("item_id")));Source.Method=FName(*D::String(Row,TEXT("method")));Source.Source=D::Text(D::Object(Row,TEXT("source")),TEXT("name"));Source.Activity=D::Text(Row,TEXT("note"));Vm.Sources.Add(Source);}
            const auto Selected=D::FindCatalog(DisplaySnapshot,TEXT("sources"),SecondaryEntry);Vm.bChooseFocusAvailable=CanDispatch()&&D::String(Selected,TEXT("method"))==TEXT("fixed_selected_reward")&&D::String(Selected,TEXT("item_id"))==SelectedEntry;Body=P::BuildItemSourcesPage(WidgetTree,Vm,Style,Layout,Callback);
        }
    }
    if(IsE)
    {
        auto Style=FeatureStyle<E::FStyleTokens>();Style.NumberFont=InkRebuildUI::Font(InkRebuildUI::ETextRole::Number);const auto Layout=LastSize.X>LastSize.Y?E::ELayoutMode::Landscape:LastSize.X<600?E::ELayoutMode::Compact:E::ELayoutMode::Portrait;
        const auto Callback=E::FActionRequested::CreateUObject(this,&ThisClass::HandlePlayerE);const auto Q=D::Object(DisplaySnapshot,TEXT("breakthrough_quote")),Stage=D::Object(Q,TEXT("stage")),T=D::Object(DisplaySnapshot,TEXT("tribulation"));
        const FString Fingerprint=D::StageSubjectIdentity(DisplaySnapshot);
        if(StageFingerprint!=Fingerprint){StageFingerprint=Fingerprint;++StageSubjectRevision;bConfirmOpen=false;ConfirmedMaterialSelection.Empty();}
        const auto Decision=InkCore::EvaluateQuote(DisplaySnapshot,MaterialSelection,bRework);
        if(PageId==TEXT("BreakthroughPrepare"))
        {
            E::FBreakthroughVm Vm;E::FInventoryVm Bag;Provider(Vm.State,W,Q.IsValid());Provider(Vm.SelectionState,W,true,MaterialSelection.IsEmpty());Provider(Bag.State,W,Inventory&&Inventory->IsAuthoritative());
            Vm.Stage.StageId=FName(*D::String(Stage,TEXT("stage_id")));Vm.Stage.bKnown=!Vm.Stage.StageId.IsNone();Vm.Stage.bFromRealmKnown=D::Integer(Stage,TEXT("from_realm_index"),Vm.Stage.FromRealmIndex);Vm.Stage.bToRealmKnown=D::Integer(Stage,TEXT("to_realm_index"),Vm.Stage.ToRealmIndex);
            Vm.StageSubjectRevision=StageSubjectRevision;Vm.SelectionRevision=SelectionRevision;Vm.bReworkMode=bRework;Vm.SelectedItemId=FName(*SelectedEntry);
            for(const auto& Pair:MaterialSelection){E::FBreakthroughMaterial M;M.ItemId=FName(*Pair.Key);M.Count=Pair.Value;M.bCountKnown=true;const auto Row=D::FindCatalog(DisplaySnapshot,TEXT("items"),Pair.Key);M.Name=D::Text(Row,TEXT("name"));Vm.Selection.Add(M);}
            if(Inventory)for(const auto& Entry:Inventory->GetEntriesView()){E::FInventoryRow Row;Row.EntryId=Entry.EntryId;Row.ItemId=Entry.Item.ItemId;Row.Name=Entry.Item.Name;Row.Count=Entry.Item.Count;Row.Icon=Entry.Item.Icon.LoadSynchronous();Bag.Entries.Add(Row);}
            Provider(Vm.Quote.State,W,Q.IsValid());Vm.Quote.Q=D::NumericText(Q,TEXT("q"));Vm.Quote.bQKnown=!Vm.Quote.Q.IsEmpty();Vm.Quote.Band=D::Text(Q,TEXT("band"));if(Vm.Quote.Band.IsEmpty())Vm.Quote.Band=D::Text(D::Object(Q,TEXT("band")),TEXT("name"));Vm.Quote.bBandKnown=!Vm.Quote.Band.IsEmpty();Vm.Quote.Kind=D::Text(Q,TEXT("kind"));Vm.Quote.bKindKnown=!Vm.Quote.Kind.IsEmpty();Vm.Quote.Quality=D::Text(Q,TEXT("quality"));Vm.Quote.bQualityKnown=!Vm.Quote.Quality.IsEmpty();
            Vm.bQuoteAvailable=CanDispatch()&&!MaterialSelection.IsEmpty();Vm.bPrepareAvailable=CanDispatch()&&Decision.CanSubmit&&!bRework;Vm.bReworkAvailable=CanDispatch()&&Decision.CanSubmit&&bRework;Vm.PrepareDisabledReason=FText::FromString(Decision.Reason);Vm.ReworkDisabledReason=Vm.PrepareDisabledReason;
            Vm.bConfirmationOpen=bConfirmOpen;Vm.bConfirmationReworkMode=bConfirmedRework;Vm.ConfirmationSelectionRevision=ConfirmedSelectionRevision;Vm.ConfirmationStageSubjectRevision=ConfirmedStageRevision;Body=E::BuildBreakthroughPage(WidgetTree,Vm,Bag,Style,Layout,Callback);
        }
        else
        {
            E::FTribulationVm Vm;Provider(Vm.State,W,T.IsValid());Vm.AttemptId=FName(*D::String(T,TEXT("id")));Vm.bAttemptKnown=!Vm.AttemptId.IsNone();const auto Phase=D::String(T,TEXT("status"));Vm.PhaseLabel=FText::FromString(Phase);
            Vm.Phase=Phase==TEXT("Prepare")?E::ETribulationPhase::Prepare:Phase==TEXT("Trial")?E::ETribulationPhase::Trial:Phase==TEXT("Reconcile")?E::ETribulationPhase::Reconcile:Phase==TEXT("Succeeded")?E::ETribulationPhase::Succeeded:Phase==TEXT("Failed")?E::ETribulationPhase::Failed:Phase==TEXT("Cancelled")?E::ETribulationPhase::Cancelled:E::ETribulationPhase::Unknown;
            Vm.TrialStart=D::DateText(T,TEXT("trial_start"));Vm.bTrialStartKnown=!Vm.TrialStart.IsEmpty();Vm.Deadline=D::DateText(T,TEXT("deadline"));Vm.bDeadlineKnown=!Vm.Deadline.IsEmpty();Vm.CleanupAt=D::DateText(T,TEXT("cleanup_at"));Vm.bCleanupAtKnown=!Vm.CleanupAt.IsEmpty();Vm.ServerUtc=D::DateText(DisplaySnapshot,TEXT("server_utc"));Vm.bServerUtcKnown=!Vm.ServerUtc.IsEmpty();
            int32 Helpers=0;Vm.bHelperCountKnown=D::Integer(T,TEXT("helper_count"),Helpers);Vm.HelperCount=Helpers;int32 Probability=0;Vm.bProbabilityKnown=D::Integer(T,TEXT("probability_bp"),Probability);Vm.ProbabilityBasisPoints=Probability;Vm.ExclusiveOutcome=D::Text(T,TEXT("outcome"));Vm.bOutcomeKnown=!Vm.ExclusiveOutcome.IsEmpty();Vm.ReceiptId=FName(*D::String(T,TEXT("receipt_id")));Vm.bReceiptKnown=!Vm.ReceiptId.IsNone();
            Vm.bCancelAvailable=CanDispatch()&&Decision.CanCancel;Vm.bReconcileAvailable=CanDispatch()&&Decision.CanReconcile;Vm.bCancelConfirmationOpen=bConfirmOpen;Vm.ConfirmationAttemptId=FName(*SecondaryEntry);Body=E::BuildTribulationPage(WidgetTree,Vm,Style,Layout,Callback);
        }
    }
    if(Body){PureScroll=WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(),TEXT("NewFeaturePlayerScroll"));PureScroll->AddChild(Body);MountPure(PureScroll,TEXT("newui-r1-player-feature-")+PageId.ToString());PreserveScroll(PurePage,true);}
}
void UInkRebuildFeatureHost::HandlePlayerC(FName Action,const C::FActionPayload& P)
{
    if(!IsActivePage())return;const auto Id=Action.ToString();if(Id==TEXT("ui.page.back")){HandleBack();return;}
    if(Id==TEXT("ui.methods.select")||Id==TEXT("ui.equipment.select-slot"))SelectedEntry=(Id.EndsWith(TEXT("slot"))?P.SlotId:P.ItemId).ToString();
    else if(Id==TEXT("ui.methods.select-inventory-entry")||Id==TEXT("ui.equipment.select-entry"))SelectedInventoryEntry=P.EntryId;
    else if(Id==TEXT("ui.methods.set-main-draft"))DraftMainMethod=P.ItemId;
    else if(Id==TEXT("ui.methods.toggle-support-draft")){if(DraftSupportMethods.Contains(P.ItemId))DraftSupportMethods.Remove(P.ItemId);else DraftSupportMethods.Add(P.ItemId);}
    else if(Id==TEXT("inventory-equip")||Id==TEXT("inventory-unequip")||Id==TEXT("learn-method"))
    {const auto* Entry=FrontendOwner->GetInventoryModel()->FindEntry(P.EntryId);bool IdentityMatches=Entry&&P.EntryId==SelectedInventoryEntry&&(P.ItemId.IsNone()||Entry->Item.ItemId==P.ItemId);
    if(Id==TEXT("learn-method")&&Entry){IdentityMatches=false;for(const auto& Row:D::Rows(D::Catalog(Service()?Service()->Snapshot():nullptr,TEXT("methods")),TEXT("rows")))if(P.EntryId==SelectedInventoryEntry&&D::String(Row,TEXT("method_id"))==P.ItemId.ToString()&&D::String(Row,TEXT("item_id"))==Entry->Item.ItemId.ToString()&&D::String(Row,TEXT("item_type"))==TEXT("book")){IdentityMatches=true;break;}}
    if(!IdentityMatches){Feedback(TEXT("物品选择已变化"));return;}Feedback(InkCore::InventoryAction(FrontendOwner,P.EntryId,Id==TEXT("inventory-equip")?InkCorePolicy::EInventoryAction::Equip:Id==TEXT("inventory-unequip")?InkCorePolicy::EInventoryAction::Unequip:InkCorePolicy::EInventoryAction::Use,1,IsActivePage()).Message);}
    else if(Id==TEXT("configure-methods"))
    {
        bool Known=false;const auto Owned=OwnedMethods(D::Object(Service()?Service()->Snapshot():nullptr,TEXT("loadout")),Known);
        if(!Known||P.MainMethodId.IsNone()||!Owned.Contains(P.MainMethodId))return;TSet<FName> Seen;TArray<TSharedPtr<FJsonValue>> Supports;
        for(FName Item:P.SupportMethodIds){if(Item==P.MainMethodId||!Owned.Contains(Item)||Seen.Contains(Item))return;Seen.Add(Item);Supports.Add(MakeShared<FJsonValueString>(Item.ToString()));}
        auto Args=MakeShared<FJsonObject>();Args->SetStringField(TEXT("main"),P.MainMethodId.ToString());Args->SetArrayField(TEXT("supports"),Supports);Submit(TEXT("ConfigureMethods"),Args);
    }
    else if(Id==TEXT("activate-fabao")){if(D::String(D::Object(Service()?Service()->Snapshot():nullptr,TEXT("loadout")),TEXT("fabao")).IsEmpty())return;Submit(TEXT("ActivateFabao"),MakeShared<FJsonObject>());}
    else if(Id==TEXT("ui.appearance.select-tab"))AppearanceTab=P.SlotId;
    else if(Id==TEXT("ui.appearance.select-frame")){if(P.PreviewStyleId==TEXT("素墨头像框")||P.PreviewStyleId==TEXT("青金头像框")||P.PreviewStyleId==TEXT("山水头像框"))DraftPreviewFrame=P.PreviewStyleId;}
    else if(Id==TEXT("ui.appearance.select-bar")){if(P.PreviewStyleId==TEXT("普通")||P.PreviewStyleId==TEXT("渐变")||P.PreviewStyleId==TEXT("护盾")||P.PreviewStyleId==TEXT("华丽"))DraftPreviewBar=P.PreviewStyleId;}
    else if(Id==TEXT("preview-appearance")){if(AppearanceTab==TEXT("Bar")&&P.PreviewStyleId==DraftPreviewBar)PreviewBar=DraftPreviewBar;else if(AppearanceTab!=TEXT("Bar")&&P.PreviewStyleId==DraftPreviewFrame)PreviewFrame=DraftPreviewFrame;else return;Feedback(TEXT("本地试戴已更新；未解锁、未装备、未保存"));}
    RefreshAuthority(true);
}
void UInkRebuildFeatureHost::HandlePlayerD(FName Action,const P::FActionPayload& P)
{
    if(!IsActivePage())return;const auto Id=Action.ToString();if(Id==TEXT("ui.page.back")){HandleBack();return;}
    if(Id==TEXT("query-recipes")||Id==TEXT("query-sources")){QueryCatalog(Id==TEXT("query-recipes")?TEXT("recipes"):TEXT("sources"),P.Offset,P.bOwnedOnly);return;}
    if(Id.EndsWith(TEXT("toggle-owned-only"))){QueryCatalog(PageId==TEXT("WulingCraft")?TEXT("recipes"):TEXT("sources"),0,P.bOwnedOnly);return;}
    if(Id==TEXT("ui.craft.select-recipe")){SelectedEntry=P.RecipeId.ToString();bConfirmOpen=false;}
    else if(Id==TEXT("ui.sources.select-entry")){SecondaryEntry=P.SourceId.ToString();SelectedEntry=P.ItemId.ToString();}
    else if(Id==TEXT("ui.craft.request-confirmation")){SelectedEntry=P.RecipeId.ToString();SecondaryEntry=SelectedEntry;bConfirmOpen=true;}
    else if(Id==TEXT("ui.craft.cancel-confirmation"))bConfirmOpen=false;
    else if(Id==TEXT("craft"))
    {const auto State=Service()?Service()->Snapshot():nullptr;if(!bConfirmOpen||SecondaryEntry!=P.RecipeId.ToString()||SelectedEntry!=SecondaryEntry||!D::FindCatalog(State,TEXT("recipes"),SecondaryEntry)||!CanDispatch())return;const bool Accepted=Service()->Craft(SecondaryEntry);Feedback(Accepted?TEXT("炼制请求已提交，等待确认"):Service()->GetStatus());bConfirmOpen=false;}
    else if(Id==TEXT("choose-focus"))
    {const auto Row=D::FindCatalog(Service()?Service()->Snapshot():nullptr,TEXT("sources"),P.SourceId.ToString());if(P.SourceId.ToString()!=SecondaryEntry||P.ItemId.ToString()!=SelectedEntry||D::String(Row,TEXT("item_id"))!=SelectedEntry||D::String(Row,TEXT("method"))!=TEXT("fixed_selected_reward"))return;auto Args=MakeShared<FJsonObject>();Args->SetStringField(TEXT("source_id"),SecondaryEntry);Args->SetStringField(TEXT("item_id"),SelectedEntry);Submit(TEXT("ChooseFocus"),Args);}
    RefreshAuthority(true);
}
void UInkRebuildFeatureHost::HandlePlayerE(FName Action,const E::FActionPayload& P)
{
    if(!IsActivePage())return;const auto Id=Action.ToString();if(Id==TEXT("ui.page.back")){HandleBack();return;}
    const bool Boundary=Id==TEXT("ui.breakthrough.request-confirmation")||Id==TEXT("prepare-breakthrough")||Id==TEXT("rework-quality");
    if(Boundary&&D::StageSubjectIdentity(Service()?Service()->Snapshot():nullptr)!=StageFingerprint){bConfirmOpen=false;ConfirmedStageFingerprint.Empty();Feedback(TEXT("突破阶段已变化，请重新查看并确认"));RefreshAuthority(true);return;}
    RefreshAuthority(true);
    if(Id==TEXT("ui.breakthrough.select-material"))SelectedEntry=P.ItemId.ToString();
    else if(Id==TEXT("ui.breakthrough.add-material")||Id==TEXT("ui.breakthrough.adjust-count"))
    {
        const FString Item=P.ItemId.ToString();int64 Held=0;for(const auto& Entry:FrontendOwner->GetInventoryModel()->GetEntriesView())if(Entry.Item.ItemId==P.ItemId)Held+=Entry.Item.Count;
        const int64 Count=int64(MaterialSelection.FindRef(Item))+(Id.EndsWith(TEXT("add-material"))?1:P.CountDelta);if(P.ItemId.IsNone()||Count<0||Count>Held||Count>MAX_int32)return;
        if(Count==0)MaterialSelection.Remove(Item);else MaterialSelection.Add(Item,int32(Count));++SelectionRevision;bConfirmOpen=false;
    }
    else if(Id==TEXT("ui.breakthrough.set-rework-mode")){bRework=P.bReworkMode;++SelectionRevision;bConfirmOpen=false;}
    else if(Id==TEXT("quote-breakthrough"))Feedback(InkCore::QuoteBreakthrough(this,MaterialSelection,bRework,IsActivePage()).Message);
    else if(Id==TEXT("ui.breakthrough.request-confirmation"))
    {if(P.SelectionRevision!=SelectionRevision||P.StageSubjectRevision!=StageSubjectRevision||P.bReworkMode!=bRework)return;ConfirmedStageFingerprint=D::StageSubjectIdentity(Service()?Service()->Snapshot():nullptr);ConfirmedMaterialSelection=MaterialSelection;ConfirmedSelectionRevision=SelectionRevision;ConfirmedStageRevision=StageSubjectRevision;bConfirmedRework=bRework;bConfirmOpen=true;}
    else if(Id==TEXT("prepare-breakthrough")||Id==TEXT("rework-quality"))
    {
        if(!D::ConfirmedStageMatches(Service()?Service()->Snapshot():nullptr,ConfirmedStageFingerprint)){bConfirmOpen=false;Feedback(TEXT("确认的突破阶段已失效，请重新确认"));RefreshAuthority(true);return;}
        if(!bConfirmOpen||ConfirmedSelectionRevision!=SelectionRevision||P.SelectionRevision!=SelectionRevision||ConfirmedStageRevision!=StageSubjectRevision||P.StageSubjectRevision!=StageSubjectRevision||bConfirmedRework!=bRework||P.bReworkMode!=bRework||!SameSelection(ConfirmedMaterialSelection,MaterialSelection))return;
        Feedback(InkCore::PrepareBreakthrough(this,MaterialSelection,bRework,IsActivePage()).Message);bConfirmOpen=false;
    }
    else if(Id==TEXT("ui.tribulation.request-cancel-confirmation")){const auto Attempt=D::String(D::Object(Service()?Service()->Snapshot():nullptr,TEXT("tribulation")),TEXT("id"));if(Attempt.IsEmpty()||Attempt!=P.AttemptId.ToString())return;SecondaryEntry=Attempt;bConfirmOpen=true;}
    else if(Id==TEXT("cancel-breakthrough")||Id==TEXT("reconcile-breakthrough"))
    {const auto State=Service()?Service()->Snapshot():nullptr;const auto Attempt=D::String(D::Object(State,TEXT("tribulation")),TEXT("id"));const auto Gate=InkCore::EvaluateQuote(State,{});if(Attempt.IsEmpty()||Attempt!=P.AttemptId.ToString()||(Id==TEXT("cancel-breakthrough")?(!bConfirmOpen||SecondaryEntry!=Attempt||!Gate.CanCancel):!Gate.CanReconcile))return;Feedback(InkCore::CancelOrReconcile(this,IsActivePage()).Message);bConfirmOpen=false;}
    else if(Id.EndsWith(TEXT("cancel-confirmation")))bConfirmOpen=false;
    RefreshAuthority(true);
}
