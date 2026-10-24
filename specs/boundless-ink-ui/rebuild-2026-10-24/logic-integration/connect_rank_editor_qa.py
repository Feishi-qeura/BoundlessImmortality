from pathlib import Path
import runpy
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
rec=runpy.run_path(str(ROOT/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/record_source_change.py'),run_name='rank_editor_qa_recorder')
def put(relative,fn):
    p=ROOT/relative;before=p.read_text(encoding='utf-8-sig');after=fn(before);assert after!=before
    rec['begin'](relative,'Actual nested newUI readback and real Slate clicks for retained typed owner callback buttons')
    p.write_text(after,encoding='utf-8');rec['finish'](relative)
def header(text):
    marker='    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool ClickGMAction';assert marker in text
    return text.replace(marker,'    UFUNCTION(BlueprintCallable,Category="newUI|Editor") static bool ClickLabeledButton(UInkFrontendWidget* Widget,const FText& Caption);\n'+marker)
put('Source/BoundlessImmortalityEditor/Public/InkFrontendEditorLibrary.h',header)
def cpp(text):
    start=text.index('        Widget->WidgetTree->ForEachWidget(DescribeChild);');end=text.index('\n    }\n    Root->SetArrayField(TEXT("buttons")',start)
    text=text[:start]+'''        TSet<const UUserWidget*> Visited;
        TFunction<void(const UUserWidget*)> Visit=[&](const UUserWidget* User)
        {
            if(!User||!User->WidgetTree||Visited.Contains(User))return;Visited.Add(User);
            User->WidgetTree->ForEachWidget([&](UWidget* Child){DescribeChild(Child);if(const auto* Nested=Cast<UUserWidget>(Child);Nested&&Nested!=User)Visit(Nested);});
        };
        Visit(Widget);Visit(Widget->AnchorStatusWidget.Get());
        Visit(Widget->ExtensionPages.FindRef(Widget->CurrentPage).Get());
'''+text[end:]
    marker='        Root->SetStringField(TEXT("runtime_class"),Widget->GetClass()->GetPathName());';assert text.count(marker)==1
    text=text.replace(marker,marker+'\n        Root->SetStringField(TEXT("board_category"),Widget->GetPresentedLeaderboardCategory().ToString());\n        Root->SetStringField(TEXT("board_scope"),Widget->LeaderboardScope.ToString());')
    # A culling/layout state is not a painted viewport presence claim.
    marker='const auto PaintedEnd=Widget->GetCachedGeometry().AbsoluteToLocal(Geometry.LocalToAbsolute(Size));';assert text.count(marker)==1
    text=text.replace(marker,marker+'\n            Entry->SetBoolField(TEXT("geometry_arranged"),Size.X>0&&Size.Y>0&&FMath::IsFinite(Position.X)&&FMath::IsFinite(Position.Y)&&FMath::IsFinite(PaintedEnd.X)&&FMath::IsFinite(PaintedEnd.Y));')
    text+='''
bool UInkFrontendEditorLibrary::ClickLabeledButton(UInkFrontendWidget* Widget,const FText& Caption)
{
    if(!Widget||!FSlateApplication::IsInitialized())return false;
    TArray<UButton*> Candidates;TSet<const UUserWidget*> Visited;
    TFunction<bool(UWidget*)> HasCaption=[&](UWidget* Child)
    {
        if(const auto* Text=Cast<UTextBlock>(Child))return Text->GetText().EqualTo(Caption);
        if(const auto* Panel=Cast<UPanelWidget>(Child))for(auto* Sub:Panel->GetAllChildren())if(HasCaption(Sub))return true;
        return false;
    };
    TFunction<void(UUserWidget*)> Visit=[&](UUserWidget* User)
    {
        if(!User||!User->WidgetTree||Visited.Contains(User))return;Visited.Add(User);
        User->WidgetTree->ForEachWidget([&](UWidget* Child)
        {
            if(auto* Button=Cast<UButton>(Child);Button&&Button->GetIsEnabled()&&InkInventoryEditor::IsEffectivelyVisible(Button)&&HasCaption(Button->GetContent()))Candidates.Add(Button);
            if(auto* Nested=Cast<UUserWidget>(Child);Nested&&Nested!=User)Visit(Nested);
        });
    };
    Visit(Widget);
    if(Candidates.Num()!=1)return false;
    auto* Button=Candidates[0];if(!InkInventoryEditor::MoveTo(Button))return false;
    const auto& Geometry=Button->GetCachedGeometry();const FVector2D P=Geometry.LocalToAbsolute(Geometry.GetLocalSize()*.5f);auto& Slate=FSlateApplication::Get();
    const auto Hit=Slate.LocateWindowUnderMouse(P,Slate.GetInteractiveTopLevelWindows());if(!Hit.ContainsWidget(Button->GetCachedWidget().Get()))return false;
    const bool Down=Slate.ProcessMouseButtonDownEvent(nullptr,FPointerEvent(FSlateApplication::CursorPointerIndex,P,P,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0,FModifierKeysState()));
    Slate.ProcessMouseButtonUpEvent(FPointerEvent(FSlateApplication::CursorPointerIndex,P,P,TSet<FKey>{},EKeys::LeftMouseButton,0,FModifierKeysState()));return Down;
}
'''
    return text
put('Source/BoundlessImmortalityEditor/Private/InkFrontendEditorLibrary.cpp',cpp)
print('Editor readback recursively visits nested new pages; exact-label native buttons use actual Slate hit testing')
