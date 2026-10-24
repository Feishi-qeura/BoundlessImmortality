"""Write review dispositions and honest per-screen source coverage, not tests."""
from pathlib import Path
import csv
import json
import yaml
from collections import Counter

HERE = Path(__file__).resolve().parent
BASE = HERE.parents[1]
manifest = json.loads((HERE / 'source-delta-v2.json').read_text(encoding='utf-8'))
sources = {s['path']: s for s in manifest['sources']}
prefix = 'specs/boundless-ink-ui/rebuild-2026-10-24/'
rank = prefix + 'panel-rank/review-v2/staged-source/InkRankWulingPages.cpp'
rankbase = prefix + 'panel-rank/review-v2/staged-source/InkRankWulingPageBase.cpp'
act = prefix + 'panel-activities/review-v2/staged-source/RebuildActivityWidgets.cpp'
journey = prefix + 'panel-journey/review-v2/staged-source/Frontend/UIRebuild20261024/Journey/InkJourneyNewWidgets.cpp'
root = 'Source/BoundlessImmortality/Frontend/UIRebuild20261024/InkRebuildFrontendWidget.cpp'
player = 'Source/BoundlessImmortality/Frontend/UIRebuild20261024/Player/InkPlayerCharacterInventoryWidgets.cpp'
director = 'Source/BoundlessImmortality/Frontend/InkMenuDirector.cpp'
ext = 'Source/BoundlessImmortality/Frontend/InkFrontendExtensions.cpp'
rows = []
def add(id, owner, state, path, evidence, note):
    rows.append(dict(id=id, owner=owner, disposition=state, path=path,
                     sha256=sources[path]['sha256'], evidence=evidence, note=note,
                     production_verified=False, runtime_verified=False))
resolved = 'resolved-in-review-v2-source'
add('C01-rank', 'panel-rank', resolved, rank, 'AddButton uses UInkRankWulingCallbackButton; base has UFUNCTION click handler', 'Production promotion/build still pending')
add('C01-activities', 'panel-activities', resolved, act, 'URebuildActivityIntentButton and reflected RelayIntent', 'Typed request owned by button')
add('C02', 'panel-rank', resolved, rank, 'AddButton sets UButtonSlot padding', 'No UButton::SetPadding call')
add('C03', 'panel-rank', resolved, rank, 'SetWeightedFill assigns FSlateChildSize.Value', 'No two-argument child-size constructor')
add('C04', 'panel-rank', resolved, rankbase, 'MakeThemeFont uses UFont pointer/CoreStyle composite font', 'New complete-type include risk N08 is separate')
add('C05', 'panel-journey', resolved, journey, 'Pin/milestone now UUserWidget with contained HitButton', 'New race action component uses same composition')
add('C06', 'panel-journey', resolved, journey, '*(FString(Name)+TEXT("Label"))', 'String concatenated before dereference')
add('C07', 'panel-activities', resolved, act, 'FProgressBarStyle.SetBackgroundImage + SetWidgetStyle', 'UE5.8 exposes the used APIs')
add('C08', 'panel-activities', resolved, act, 'Borrowed const delegate argument removed', 'Typed intent relay replaces borrowed delegate')
add('V01', 'panel-activities', resolved, act, 'SetColorAndOpacity(FLinearColor::White)', 'No render evidence')
add('L01', 'panel-activities', resolved, act, 'NativeOnInitialized; StablePageRoot/Frame; saved scroll offset', 'Focus/scroll behavior still requires real Slate')
add('L02', 'panel-rank', resolved, rankbase, 'EnsureStableRoot assigns RootWidget once', 'Repeated content reconstruction still needs runtime verification')
add('L03', 'panel-rank + integration', 'partially-resolved-source-host-open', rank, 'ParticipantAvatarCache/AvatarImagesByParticipant/OnAvatarRequested/SetParticipantAvatar', 'Failure latch N07 and real host async wiring remain open')
add('ENTRY-01', 'logic-integration', 'resolved-in-production-source', director, 'EnsureFrontend loads newUI WBP and validates new Root subclass with new native fallback', 'No saved-package or actual player entry evidence')
add('ENTRY-02', 'logic-integration', 'resolved-in-production-source', root, 'RefreshLocalPage calls BuildCharacterPage and BuildInventoryPage', 'No rendered/player interaction evidence')
for id, owner, path, evidence, note in [
    ('N01','panel-activities',act,'HandleStartClicked requires EventId.IsNone()==false','P1: service creates ID only after known-absent start'),
    ('N02','panel-journey',journey,'Monthly SetPresentation replaces day/month without invalidating open confirmation','P2: capture exact day/month/milestone'),
    ('N03','logic-integration + panel-player',player,'InventorySelect_<SlotIndex> despite EntryId widget name','P2: capture visible stable EntryId at click'),
    ('N04','logic-integration + panel-journey',journey,'/Game/newUI/Shared/Fonts differs from cleanup canonical mapping','P2: align with Theme/mapped canonical package'),
    ('N05','logic-integration',ext,'EnsureFeaturePage creates legacy UInkFeaturePanel classes','P2: finish exact new route and host bindings; coverage is not 44'),
    ('N06','logic-integration',root,'Inventory content key excludes eligibility inputs and provider state','P2: prevent stale visual disablement and wrong Data state'),
    ('N07','panel-rank',rank,'SetParticipantAvatar null completion returns before clearing in-flight set','P2: release failed async request and permit controlled retry'),
    ('N08','panel-rank',rankbase,'UFont* to UObject* font constructor without defining Engine/Font.h include','Build risk only; no compiler diagnostic claimed'),
    ('N09','panel-rank',rank,'ButtonLabel casts wrapper content (the button) to UTextBlock','P2: resolve Button.GetContent or retain text pointer'),
]:
    add(id, owner, 'open-source-review', path, evidence, note)
matrix = dict(captured_utc=manifest['captured_utc'], evidence_level='source-only',
              counts=dict(Counter(r['disposition'] for r in rows)), rows=rows)
(HERE / 'resolved-open-matrix-v2.json').write_text(json.dumps(matrix, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
with (HERE / 'resolved-open-matrix-v2.csv').open('w', newline='', encoding='utf-8-sig') as f:
    writer = csv.DictWriter(f, fieldnames=list(rows[0])); writer.writeheader(); writer.writerows(rows)

screens = yaml.safe_load((BASE / 'contracts/screen-contract.yaml').read_text(encoding='utf-8'))['screens']
mounted = dict(main='BuildMainMenuSurface', home='BuildHudSurface', saves='BuildSavesBody',
               settings='BuildSettingsBody', character='BuildCharacterPage', inventory='BuildInventoryPage')
coverage = []
for s in screens:
    state = 'new-composition-connected-in-source' if s['id'] in mounted else 'new-root-placeholder' if s['id']=='leaderboard' else 'no-new-production-component-observed'
    coverage.append(dict(screen_id=s['id'], entry=s['existing_route']['entry'], resolved_route=s['existing_route']['resolved'],
                         state=state, new_component=mounted.get(s['id'], ''),
                         evidence_path=root if s['id'] in mounted or s['id']=='leaderboard' else ext,
                         evidence_sha256=sources[root if s['id'] in mounted or s['id']=='leaderboard' else ext]['sha256'],
                         saved_wbp_verified=False, runtime_verified=False, player_entry_verified=False))
assert len(coverage)==44 and len({r['screen_id'] for r in coverage})==44
with (HERE / 'screen-coverage-v2.csv').open('w', newline='', encoding='utf-8-sig') as f:
    writer=csv.DictWriter(f,fieldnames=list(coverage[0])); writer.writeheader(); writer.writerows(coverage)
(HERE / 'screen-coverage-v2.json').write_text(json.dumps(dict(captured_utc=manifest['captured_utc'], counts=dict(Counter(r['state'] for r in coverage)), screens=coverage), ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
print(json.dumps(dict(matrix_counts=matrix['counts'], screen_counts=dict(Counter(r['state'] for r in coverage)))))
