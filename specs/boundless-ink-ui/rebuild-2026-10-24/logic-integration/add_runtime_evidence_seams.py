from pathlib import Path
import runpy
R=Path.cwd();H=R/'specs/boundless-ink-ui/rebuild-2026-10-24/logic-integration';B='Source/BoundlessImmortality/Frontend/UIRebuild20261024/'
rec=runpy.run_path(str(R/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/record_source_change.py'),run_name='runtime_evidence_seams')
def edit(p,fn,reason):
 t=R/p;s=t.read_text(encoding='utf-8-sig');n=fn(s);assert s!=n,p;rec['begin'](p,reason);t.write_text(n,encoding='utf-8');rec['finish'](p)
for name,helper,export in [('LiveWeaponHUD','HasVisibleAnchorStatusForWeaponHUD','InkNewUiSuppressesLegacyWeaponHud'),('LiveGMOverlay','HasVisibleAnchorStatus','InkNewUiSuppressesLegacyGMAnchor')]:
 p='Source/BoundlessImmortality/LiveWeapons/'+name+'.cpp';s=(R/p).read_text(encoding='utf-8-sig');at=s.index('\nvoid ALiveWeaponHUD::DrawHUD()') if name=='LiveWeaponHUD' else s.index('\nALiveGMOverlay::ALiveGMOverlay()')
 n=s[:at]+'\nBOUNDLESSIMMORTALITY_API bool '+export+'(UWorld* World,APlayerController* Player){return '+helper+'(World,Player);}\n'+s[at:]
 edit(p,lambda _:n,'Expose the exact existing legacy presentation suppression predicate for current PIE diagnostics; no gameplay changes')
def editor(s):
 s=s.replace('#include "Frontend/UIRebuild20261024/InkRebuildFrontendWidget.h"','#include "Frontend/UIRebuild20261024/InkRebuildFrontendWidget.h"\n#include "Frontend/UIRebuild20261024/InkRebuildFeatureHost.h"')
 marker='namespace InkInventoryEditor\n{';assert marker in s
 s=s.replace(marker,'''BOUNDLESSIMMORTALITY_API bool InkNewUiSuppressesLegacyWeaponHud(UWorld* World,APlayerController* Player);
BOUNDLESSIMMORTALITY_API bool InkNewUiSuppressesLegacyGMAnchor(UWorld* World,APlayerController* Player);
'''+marker)
 marker='        Root->SetStringField(TEXT("active_leaderboard"),Widget->ActiveGameplayLeaderboard.ToString());'
 s=s.replace(marker,'''        if(const auto* Extension=Widget->ExtensionPages.FindRef(Widget->CurrentPage).Get())
        {
            Root->SetStringField(TEXT("extension_runtime_class"),Extension->GetClass()->GetPathName());
            Root->SetStringField(TEXT("extension_root_widget"),Extension->WidgetTree&&Extension->WidgetTree->RootWidget?Extension->WidgetTree->RootWidget->GetName():TEXT(""));
            if(const auto* Host=Cast<UInkRebuildFeatureHost>(Extension))Root->SetStringField(TEXT("feature_composition"),Host->GetCompositionName());
        }
        Root->SetBoolField(TEXT("legacy_weapon_hud_suppression_condition"),InkNewUiSuppressesLegacyWeaponHud(Widget->GetWorld(),Widget->GetOwningPlayer()));
        Root->SetBoolField(TEXT("legacy_gm_anchor_suppression_condition"),InkNewUiSuppressesLegacyGMAnchor(Widget->GetWorld(),Widget->GetOwningPlayer()));
        Root->SetBoolField(TEXT("legacy_status_adapter_collapsed"),!Widget->AnchorStatusWidget||Widget->AnchorStatusWidget->GetVisibility()==ESlateVisibility::Collapsed);
'''+marker)
 return s
edit('Source/BoundlessImmortalityEditor/Private/InkFrontendEditorLibrary.cpp',editor,'Describe actual feature host/root/composition and exact native legacy suppression conditions in fresh default PIE')
def rank_owner(s):
 old='auto* HeaderSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());';assert s.count(old)==1
 return s.replace(old,'auto* HeaderSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("NewRankHeaderSize"));')
edit(B+'Rank/InkRankWulingPages.cpp',rank_owner,'Expose actual Rank title extent for compact landscape layout without replacing owner composition')
def rank(s):
 s=s.replace('#include "Engine/Font.h"','#include "Engine/Font.h"\n#include "Blueprint/WidgetTree.h"\n#include "Components/CanvasPanel.h"\n#include "Components/CanvasPanelSlot.h"\n#include "Components/SizeBox.h"')
 marker='        MountPageComponent(TEXT("Leaderboard"),NewRankBoard,TEXT("newui-r1-rank-leaderboard"));';assert marker in s
 s=s.replace(marker,marker+'''
        if(auto* Outer=GetWidgetFromName(TEXT("NewUIScroll_Leaderboard")))Outer->RemoveFromParent();
        NewRankBoard->RemoveFromParent();if(auto* Page=PageCanvases.FindRef(TEXT("Leaderboard")).Get())Page->AddChildToCanvas(NewRankBoard);''')
 marker='    const auto Board=InkCore::ReadBoard('
 s=s.replace(marker,'''    const auto Size=LastSafeSize.IsNearlyZero()?FVector2D(720,1280):LastSafeSize;
    if(auto* Header=Cast<USizeBox>(NewRankBoard->WidgetTree->FindWidget(TEXT("NewRankHeaderSize"))))Header->SetHeightOverride(Size.X>Size.Y?88:176);
    if(auto* Layout=Cast<UCanvasPanelSlot>(NewRankBoard->Slot))Layout->SetOffsets(FMargin(24,24,Size.X-48,Size.Y-96));
'''+marker)
 return s
edit(B+'InkRebuildLeaderboard.cpp',rank,'Mount full native Rank page directly in its canvas and compact landscape title; preserve inner row scroll and avatar lifecycle')
edit(B+'InkRebuildFeatureHost.cpp',lambda s:s.replace('Move(TEXT("NewFeatureFeedback"),FVector4(24,LastSize.Y-64,FMath::Max(88.f,LastSize.X-48),56));','Move(TEXT("NewFeatureFeedback"),FVector4(24,LastSize.Y-64,FMath::Max(88.f,LastSize.X-(RelatedRoutes(PageId).IsEmpty()?48:272)),56));'),'Reserve feedback width beside the native related-page menu action')
