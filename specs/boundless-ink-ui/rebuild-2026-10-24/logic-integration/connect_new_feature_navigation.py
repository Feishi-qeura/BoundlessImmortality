from pathlib import Path
import runpy
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
rec=runpy.run_path(str(ROOT/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/record_source_change.py'),run_name='feature_navigation_recorder')
def put(relative,fn):
    p=ROOT/relative;before=p.read_text(encoding='utf-8-sig');after=fn(before);assert before!=after
    rec['begin'](relative,'Full newUI feature host: preserve canonical distinct routes and existing real back stack; forbid legacy presentation fallback')
    p.write_text(after,encoding='utf-8');rec['finish'](relative)
base='Source/BoundlessImmortality/Frontend/'
def extension(text):
    marker='    virtual void HandleAction(FName Action);';assert text.count(marker)==1
    return text.replace(marker,marker+'\n    virtual void SetContext(const FString& Entry){SelectedEntry=Entry;}')
put(base+'InkExtensionWidget.h',extension)
def frontend(text):
    marker='    virtual void RefreshRebuiltPresentation(){}';assert text.count(marker)==1
    return text.replace(marker,marker+'\n    virtual UInkExtensionWidget* CreateRebuiltFeaturePage(FName Page){return nullptr;}\n    virtual FName ResolveRebuiltFeatureRoute(FName Page,const FString& Context)const{return Page;}')
put(base+'InkFrontendWidget.h',frontend)
def extensions(text):
    old='''    const auto Class=UInkFeaturePanel::ClassForRoute(Page);if(!Class)return false;
    auto* Pages=Cast<UWidgetSwitcher>(GetWidgetFromName(TEXT("PageSwitcher")));if(!Pages)return false;
    auto* Panel=CreateWidget<UInkFeaturePanel>(GetOwningPlayer(),Class);if(!Panel)return false;''';assert text.count(old)==1
    text=text.replace(old,'''    auto* Pages=Cast<UWidgetSwitcher>(GetWidgetFromName(TEXT("PageSwitcher")));if(!Pages)return false;
    UInkExtensionWidget* Panel=nullptr;
    if(UsesRebuiltPresentation()){Panel=CreateRebuiltFeaturePage(Page);if(!Panel)return false;}
    else{const auto Class=UInkFeaturePanel::ClassForRoute(Page);if(!Class)return false;Panel=CreateWidget<UInkFeaturePanel>(GetOwningPlayer(),Class);}
    if(!Panel)return false;''')
    start=text.index('    if(Page==TEXT("CultivationMethods"))');end=text.index('    const FName Requested=',start)
    text=text[:start]+'''    if(UsesRebuiltPresentation())Page=ResolveRebuiltFeatureRoute(Page,Context);
    else
    {
'''+text[start:end]+'''    }
    if(UsesRebuiltPresentation())for(const TCHAR* Core:{TEXT("Main"),TEXT("HUD"),TEXT("Saves"),TEXT("Settings"),TEXT("Character"),TEXT("Inventory"),TEXT("Leaderboard")})
        if(Page==Core){ShowPage(Page);return CurrentPage==Page;}
'''+text[end:]
    text=text.replace('if(Page==TEXT("MonthlySignIn")||Page==TEXT("DailyFortune"))Page=TEXT("SignIn");','if(!UsesRebuiltPresentation()&&(Page==TEXT("MonthlySignIn")||Page==TEXT("DailyFortune")))Page=TEXT("SignIn");')
    text=text.replace('if(!UInkFeaturePanel::ClassForRoute(Page))return false;','if(!UsesRebuiltPresentation()&&!UInkFeaturePanel::ClassForRoute(Page))return false;')
    old='if(auto* Panel=Cast<UInkFeaturePanel>(ExtensionPages.FindRef(Page)))Panel->SetContext(ResolvedContext);'
    assert text.count(old)==3;text=text.replace(old,'if(auto* Panel=ExtensionPages.FindRef(Page).Get())Panel->SetContext(ResolvedContext);')
    return text
put(base+'InkFrontendExtensions.cpp',extensions)
def root_header(text):
    marker='    virtual void SetRebuiltFeedback(const FText& Message)override;';assert text.count(marker)==1
    return text.replace(marker,marker+'\n    virtual UInkExtensionWidget* CreateRebuiltFeaturePage(FName Page)override;\n    virtual FName ResolveRebuiltFeatureRoute(FName Page,const FString& Context)const override;')
put(base+'UIRebuild20261024/InkRebuildFrontendWidget.h',root_header)
def root_cpp(text):
    text=text.replace('#include "InkRebuildTheme.h"','#include "InkRebuildTheme.h"\n#include "InkRebuildFeatureHost.h"',1)
    old='{return PageVersions.FindRef(Page);}'
    assert text.count(old)==1
    text=text.replace(old,'{if(const auto* Host=Cast<UInkRebuildFeatureHost>(ExtensionPages.FindRef(Page).Get()))return Host->GetLayoutVersion();return PageVersions.FindRef(Page);}')
    text+='''
UInkExtensionWidget* UInkRebuildFrontendWidget::CreateRebuiltFeaturePage(FName Page)
{
    if(!UInkRebuildFeatureHost::SupportsRoute(Page)||!GetOwningPlayer())return nullptr;
    return CreateWidget<UInkRebuildFeatureHost>(GetOwningPlayer());
}
FName UInkRebuildFrontendWidget::ResolveRebuiltFeatureRoute(FName Page,const FString& Context)const
{return UInkRebuildFeatureHost::CanonicalRoute(Page,Context);}
'''
    return text
put(base+'UIRebuild20261024/InkRebuildFrontendWidget.cpp',root_cpp)
def host_header(text):
    marker='    void MountPure(UWidget* Page,const FString& Name);';assert text.count(marker)==1
    text=text.replace(marker,marker+'\n    void RetirePurePage();')
    marker='    UFUNCTION() void HandleBack();';assert text.count(marker)==1
    return text.replace(marker,marker+'\n    UFUNCTION() void HandleMonthlyNavigation();')
put(base+'UIRebuild20261024/InkRebuildFeatureHost.h',host_header)
print('Connected strict new feature factory and generic context/back stack; canonical37 screens remain distinct')
