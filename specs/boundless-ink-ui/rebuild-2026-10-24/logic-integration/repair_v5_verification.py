from pathlib import Path
import runpy,json
H=Path('specs/boundless-ink-ui/rebuild-2026-10-24/logic-integration').resolve();R=Path.cwd()
rec=runpy.run_path(str(R/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/record_source_change.py'),run_name='verification_recorder')
def edit(p,fn,reason):
 t=R/p;s=t.read_text(encoding='utf-8-sig');n=fn(s);assert s!=n;rec['begin'](p,reason);t.write_text(n,encoding='utf-8');rec['finish'](p)
def avatar(s):
 old='if(NewRankRequests.Contains(Key)||NewRankAvatarRetryAt.FindRef(Key)>FPlatformTime::Seconds())return;'
 assert s.count(old)==1
 return s.replace(old,'if(NewRankRequests.Contains(Key))return;\n    if(NewRankAvatarRetryAt.FindRef(Key)>FPlatformTime::Seconds())\n    {NewRankBoard->SetParticipantAvatar(ParticipantId,nullptr);return;}')
edit('Source/BoundlessImmortality/Frontend/UIRebuild20261024/InkRebuildLeaderboard.cpp',avatar,'V5-AVATAR-01: release page request latch on host backoff rejection; preserve actual request latch and exact subject/budget/timeout')
def geometry(s):
 s=s.replace('#include "Layout/WidgetPath.h"','#include "Layout/WidgetPath.h"\n#include "Layout/ArrangedChildren.h"')
 marker='        auto DescribeChild=[&](UWidget* Child)'
 assert s.count(marker)==1
 independent='''        // Re-arrange the current Slate tree from its valid root, independently of each
        // UWidget cached geometry. A finite ancestor outside its finite culling rect
        // proves descendants are not painted; missing/invalid evidence stays Unknown.
        struct FPaintEvidence { bool bKnown=false,bCulled=false;FString CulledAncestor;FSlateRect Bounds,Clip; };
        TMap<const SWidget*,FPaintEvidence> PaintEvidence;
        auto FiniteRect=[](const FSlateRect& R){return FMath::IsFinite(R.Left)&&FMath::IsFinite(R.Top)&&FMath::IsFinite(R.Right)&&FMath::IsFinite(R.Bottom);};
        TSet<const SWidget*> ArrangedSeen;
        TFunction<void(const TSharedRef<SWidget>&,const FGeometry&,const FSlateRect&,bool,const FString&,int32)> Arrange;
        Arrange=[&](const TSharedRef<SWidget>& Slate,const FGeometry& G,const FSlateRect& Clip,bool AncestorCulled,const FString& Proof,int32 Depth)
        {
            if(Depth>256||ArrangedSeen.Contains(&Slate.Get()))return;ArrangedSeen.Add(&Slate.Get());
            const FSlateRect Bounds=G.GetRenderBoundingRect();
            const bool Valid=FiniteRect(Bounds)&&FiniteRect(Clip)&&G.GetLocalSize().X>0&&G.GetLocalSize().Y>0;
            const bool Outside=Valid&&!FSlateRect::DoRectanglesIntersect(Bounds,Clip);
            FPaintEvidence Evidence;Evidence.bKnown=AncestorCulled||Valid;Evidence.bCulled=AncestorCulled||Outside;
            Evidence.CulledAncestor=AncestorCulled?Proof:(Outside?Slate->GetTypeAsString():FString());Evidence.Bounds=Bounds;Evidence.Clip=Clip;
            PaintEvidence.Add(&Slate.Get(),Evidence);
            bool ClipBounds=false,AlwaysClip=false,Intersect=true;
            const FSlateRect ChildClip=Slate->CalculateCullingAndClippingRules(G,Clip,ClipBounds,AlwaysClip,Intersect);
            FArrangedChildren Children(EVisibility::Visible);Slate->ArrangeChildren(G,Children);
            for(int32 Index=0;Index<Children.Num();++Index)
                Arrange(Children[Index].Widget,Children[Index].Geometry,ChildClip,Evidence.bCulled,Evidence.CulledAncestor,Depth+1);
        };
        if(auto Slate=Widget->GetCachedWidget())
        {
            const auto& RootGeometry=Slate->GetPaintSpaceGeometry();const auto RootClip=RootGeometry.GetRenderBoundingRect();
            if(FiniteRect(RootClip)&&RootGeometry.GetLocalSize().X>0&&RootGeometry.GetLocalSize().Y>0)
                Arrange(Slate.ToSharedRef(),RootGeometry,RootClip,false,FString(),0);
        }
'''
 s=s.replace(marker,independent+marker)
 old='Entry->SetBoolField(TEXT("geometry_arranged"),Size.X>0&&Size.Y>0&&FMath::IsFinite(Position.X)&&FMath::IsFinite(Position.Y)&&FMath::IsFinite(PaintedEnd.X)&&FMath::IsFinite(PaintedEnd.Y));'
 assert s.count(old)==1
 new='''Entry->SetBoolField(TEXT("geometry_arranged"),Size.X>0&&Size.Y>0);
            Entry->SetStringField(TEXT("paint_expectation"),TEXT("unknown"));
            if(auto Slate=Child->GetCachedWidget())if(const auto* Evidence=PaintEvidence.Find(Slate.Get()))
            {
                Entry->SetStringField(TEXT("paint_expectation"),Evidence->bKnown?(Evidence->bCulled?TEXT("culled"):TEXT("expected")):TEXT("unknown"));
                Entry->SetStringField(TEXT("culling_proof_ancestor"),Evidence->CulledAncestor);
                if(Evidence->bKnown)
                {
                    Number(Entry,TEXT("independent_left"),Evidence->Bounds.Left);Number(Entry,TEXT("independent_top"),Evidence->Bounds.Top);
                    Number(Entry,TEXT("independent_right"),Evidence->Bounds.Right);Number(Entry,TEXT("independent_bottom"),Evidence->Bounds.Bottom);
                    Number(Entry,TEXT("independent_clip_left"),Evidence->Clip.Left);Number(Entry,TEXT("independent_clip_top"),Evidence->Clip.Top);
                    Number(Entry,TEXT("independent_clip_right"),Evidence->Clip.Right);Number(Entry,TEXT("independent_clip_bottom"),Evidence->Clip.Bottom);
                }
            }'''
 return s.replace(old,new)
edit('Source/BoundlessImmortalityEditor/Private/InkFrontendEditorLibrary.cpp',geometry,'V5-VERIFY-01: independently re-arrange actual Slate tree and prove finite ancestor culling; unknown nonfinite metrics must fail')
p=H/'status.json';j=json.loads(p.read_text(encoding='utf-8-sig'));j.update(runtime_verified=False,phase='core7_observed_geometry_provisional_full37_host_in_progress',source_compiled=False,geometry_provisional='PIE03 geometry filter circular; independent arrangement/culling repair pending fresh build/PIE');p.write_text(json.dumps(j,ensure_ascii=False,indent=2),encoding='utf-8')
