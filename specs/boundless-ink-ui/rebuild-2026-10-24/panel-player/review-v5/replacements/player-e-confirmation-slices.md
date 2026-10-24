# Player-e confirmation replacement slices

The applyable, context-rich version is `../diff/player-e-breakthrough-confirmation.diff`. It is intentionally not applied to the frozen player-e source.

This patch adds an opaque host-owned stage/realm subject revision to the typed intent and VM, includes it in the first-click request, requires the captured stage revision to match before showing the second-click control, and sends captured stage/selection/mode values on confirmation. It does not add a gate on visible stage/realm or optional quote fields.

## InkPlayerBreakthroughTribulationWidgets.h

```diff
--- a/specs/boundless-ink-ui/rebuild-2026-10-24/panel-player/player-e/staged-source/Frontend/UIRebuild20261024/Player/InkPlayerBreakthroughTribulationWidgets.h
+++ b/specs/boundless-ink-ui/rebuild-2026-10-24/panel-player/player-e/staged-source/Frontend/UIRebuild20261024/Player/InkPlayerBreakthroughTribulationWidgets.h
@@ -59,6 +59,7 @@
 		bool bReworkMode = false;
 		int32 CountDelta = 0;
 		uint64 SelectionRevision = 0;
+		uint64 StageSubjectRevision = 0;
 	};
 
 	DECLARE_DELEGATE_TwoParams(FActionRequested, FName /* canonical action id or ui.* intent */, const FActionPayload&);
@@ -127,6 +128,7 @@
 		FProviderSlice State;
 	FProviderSlice SelectionState;
 		FBreakthroughStage Stage;
+		uint64 StageSubjectRevision = 0;
 		TArray<FBreakthroughMaterial> Selection;
 		FName SelectedItemId;
 		uint64 SelectionRevision = 0;
@@ -141,6 +143,7 @@
 		bool bConfirmationOpen = false;
 		bool bConfirmationReworkMode = false;
 		uint64 ConfirmationSelectionRevision = 0;
+		uint64 ConfirmationStageSubjectRevision = 0;
 	};
 
 	struct FTribulationVm
```

## InkPlayerBreakthroughTribulationWidgets.cpp

```diff
--- a/specs/boundless-ink-ui/rebuild-2026-10-24/panel-player/player-e/staged-source/Frontend/UIRebuild20261024/Player/InkPlayerBreakthroughTribulationWidgets.cpp
+++ b/specs/boundless-ink-ui/rebuild-2026-10-24/panel-player/player-e/staged-source/Frontend/UIRebuild20261024/Player/InkPlayerBreakthroughTribulationWidgets.cpp
@@ -383,6 +383,7 @@
 
 	FActionPayload Request; Request.bReworkMode = ViewModel.bReworkMode;
 	Request.SelectionRevision = ViewModel.SelectionRevision;
+	Request.StageSubjectRevision = ViewModel.StageSubjectRevision;
 	for (const FBreakthroughMaterial& Material : ViewModel.Selection)
 	{
 		FMaterialSelection& Entry = Request.Selection.AddDefaulted_GetRef();
@@ -393,7 +394,9 @@
 		ViewModel.bReworkMode ? ViewModel.bReworkAvailable : ViewModel.bPrepareAvailable, true,
 		ViewModel.bReworkMode ? ViewModel.ReworkDisabledReason : ViewModel.PrepareDisabledReason));
 
-	if (ViewModel.bConfirmationOpen && ViewModel.ConfirmationSelectionRevision == ViewModel.SelectionRevision
+	if (ViewModel.bConfirmationOpen
+		&& ViewModel.ConfirmationStageSubjectRevision == ViewModel.StageSubjectRevision
+		&& ViewModel.ConfirmationSelectionRevision == ViewModel.SelectionRevision
 		&& ViewModel.ConfirmationReworkMode == ViewModel.bReworkMode)
 	{
 		auto* Confirm = Tree->ConstructWidget<UVerticalBox>();
@@ -401,6 +404,9 @@
 			NSLOCTEXT("InkPlayerE", "ConfirmWarning", "确认后将提交当前材料选择，由服务重新报价并校验"),
 			Style.SectionFont, Style.Warning));
 		FActionPayload Submit = Request;
+		Submit.StageSubjectRevision = ViewModel.ConfirmationStageSubjectRevision;
+		Submit.SelectionRevision = ViewModel.ConfirmationSelectionRevision;
+		Submit.bReworkMode = ViewModel.ConfirmationReworkMode;
 		Confirm->AddChildToVerticalBox(MakeButton(Tree,
 			ViewModel.bReworkMode ? NSLOCTEXT("InkPlayerE", "ConfirmRework", "确认重整")
 				: NSLOCTEXT("InkPlayerE", "ConfirmPrepare", "确认申请"), Style, OnAction,
```
