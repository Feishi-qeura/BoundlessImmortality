from pathlib import Path
import runpy
R=Path.cwd();H=R/'specs/boundless-ink-ui/rebuild-2026-10-24/logic-integration'
rec=runpy.run_path(str(R/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/record_source_change.py'),run_name='slate_service_helpers')
def edit(p,fn,reason):
 t=R/p;s=t.read_text(encoding='utf-8-sig');n=fn(s);assert s!=n;rec['begin'](p,reason);t.write_text(n,encoding='utf-8');rec['finish'](p)
def cpp(s):
 marker='    bool MoveTo(UWidget* Widget)';assert marker in s
 s=s.replace(marker,'''    UWidget* FindCurrentNamedWidget(UInkFrontendWidget* Root,FName Name)
    {
        TArray<UWidget*> Matches;TSet<const UUserWidget*> Seen;
        TFunction<void(UUserWidget*)> Visit=[&](UUserWidget* User)
        {if(!User||!User->WidgetTree||Seen.Contains(User))return;Seen.Add(User);User->WidgetTree->ForEachWidget([&](UWidget* Child){if(Child->GetFName()==Name&&IsEffectivelyVisible(Child))Matches.AddUnique(Child);if(auto* Nested=Cast<UUserWidget>(Child);Nested&&Nested!=User)Visit(Nested);});};
        Visit(Root);if(Root)Visit(Root->ExtensionPages.FindRef(Root->CurrentPage));return Matches.Num()==1?Matches[0]:nullptr;
    }

'''+marker)
 old='''    auto* Target=Widget?Widget->GetWidgetFromName(Name):nullptr;if(!Target)return false;
    for(auto* Parent=Target->GetParent();Parent;Parent=Parent->GetParent())if(auto* Scroll=Cast<UScrollBox>(Parent))
    {Scroll->ScrollWidgetIntoView(Target,false,EDescendantScrollDestination::TopOrLeft);return true;}
    return false;''';assert old in s
 new='''    auto* Target=InkInventoryEditor::FindCurrentNamedWidget(Widget,Name);if(!Target)return false;
    TSet<const UWidget*> Seen;UWidget* Current=Target;
    while(Current&&!Seen.Contains(Current))
    {
        Seen.Add(Current);UWidget* Parent=Current->GetParent();
        if(!Parent){auto* Owner=Current->GetTypedOuter<UUserWidget>();if(!Owner||Owner==Current)break;Parent=Owner;}
        if(auto* Scroll=Cast<UScrollBox>(Parent)){Scroll->ScrollWidgetIntoView(Target,false,EDescendantScrollDestination::TopOrLeft);return true;}Current=Parent;
    }
    return false;'''
 s=s.replace(old,new)
 at=s.index('\nFString UInkFrontendEditorLibrary::DescribeInventory(')
 s=s[:at]+'''
bool UInkFrontendEditorLibrary::TypeFrontendText(UInkFrontendWidget* Widget,FName Name,const FString& Text)
{
    auto* Input=Cast<UEditableTextBox>(InkInventoryEditor::FindCurrentNamedWidget(Widget,Name));if(!Input||!InkInventoryEditor::MoveTo(Input))return false;
    auto& Slate=FSlateApplication::Get();Slate.SetKeyboardFocus(Input->GetCachedWidget(),EFocusCause::SetDirectly);
    const FKeyEvent SelectAll(EKeys::A,FModifierKeysState(false,false,true,false,false,false,false,false,false),0,false,0,0);Slate.ProcessKeyDownEvent(SelectAll);Slate.ProcessKeyUpEvent(SelectAll);
    const FKeyEvent Backspace(EKeys::BackSpace,FModifierKeysState(),0,false,0,0);Slate.ProcessKeyDownEvent(Backspace);Slate.ProcessKeyUpEvent(Backspace);
    for(const TCHAR Character:Text)Slate.ProcessKeyCharEvent(FCharacterEvent(Character,FModifierKeysState(),0,false));return Input->GetText().ToString()==Text;
}
'''+s[at:]
 return s
edit('Source/BoundlessImmortalityEditor/Private/InkFrontendEditorLibrary.cpp',cpp,'Real Slate typing and current visible nested-widget scroll helper for new hosts; traverse actual UUserWidget ancestry, no direct action substitute')
edit('Source/BoundlessImmortalityEditor/Public/InkFrontendEditorLibrary.h',lambda s:s.replace('    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool TypeInventorySearch(', '    UFUNCTION(BlueprintCallable,Category="newUI|Editor") static bool TypeFrontendText(UInkFrontendWidget* Widget,FName Name,const FString& Text);\n    UFUNCTION(BlueprintCallable,Category="Ink Frontend|Editor") static bool TypeInventorySearch('),'Expose actual Slate keyboard input for new guild/market fields in isolated runtime verification')
p=H/'verify_all44_default_pie.py';s=p.read_text(encoding='utf-8-sig');s=s.replace("    check('real scroll to action '+action,LIB.scroll_to_frontend_widget(FRONTEND,matches[0]['name']))","    REPORT.setdefault('action_scroll_before',{})[action]=matches[0];save()\n    check('real scroll to action '+action,LIB.scroll_to_frontend_widget(FRONTEND,matches[0]['name']))")
marker='def click(action):'
s=s.replace(marker,'''def click_after_scroll(action):
    snapshot=describe();matches=[x for x in snapshot.get('buttons',[]) if x.get('action')==action and x.get('visible')]
    check('one painted target after actual scroll '+action,len(matches)==1,matches)
    target=matches[0];REPORT.setdefault('action_scroll_after',{})[action]=target;save()
    check('actual scrolled target expected painted '+action,target.get('paint_expectation')=='expected' and target.get('geometry_arranged'),target)
    check('actual scrolled target inside clipping bounds '+action,target['independent_left']>=target['independent_clip_left']-1 and target['independent_top']>=target['independent_clip_top']-1 and target['independent_right']<=target['independent_clip_right']+1 and target['independent_bottom']<=target['independent_clip_bottom']+1,target)
    click(action)

'''+marker)
s=s.replace("lambda r=route:click('Open'+r),1)","lambda r=route:click_after_scroll('Open'+r),1)")
p.write_text(s,encoding='utf-8')
