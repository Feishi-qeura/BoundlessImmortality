from pathlib import Path
import runpy
R=Path.cwd();H=R/'specs/boundless-ink-ui/rebuild-2026-10-24/logic-integration'
rec=runpy.run_path(str(R/'specs/boundless-ink-ui/orchestration/2026-10-24-ui-component-rollout/logic-integration/visible-source-integration/record_source_change.py'),run_name='independent_culling_recorder')
p='Source/BoundlessImmortalityEditor/Private/InkFrontendEditorLibrary.cpp';t=R/p;s=t.read_text(encoding='utf-8')
s=s.replace('FSlateRect Bounds,Clip; }','FSlateRect Bounds,Clip,CulledBounds,CulledClip; }')
s=s.replace('const FString&,int32)> Arrange;','const FString&,const FSlateRect&,const FSlateRect&,int32)> Arrange;')
s=s.replace('const FString& Proof,int32 Depth)','const FString& Proof,const FSlateRect& ProofBounds,const FSlateRect& ProofClip,int32 Depth)')
s=s.replace('Evidence.Bounds=Bounds;Evidence.Clip=Clip;','Evidence.Bounds=Bounds;Evidence.Clip=Clip;Evidence.CulledBounds=AncestorCulled?ProofBounds:Bounds;Evidence.CulledClip=AncestorCulled?ProofClip:Clip;')
s=s.replace('Evidence.bCulled,Evidence.CulledAncestor,Depth+1);','Evidence.bCulled,Evidence.CulledAncestor,Evidence.CulledBounds,Evidence.CulledClip,Depth+1);')
s=s.replace('RootClip,false,FString(),0);','RootClip,false,FString(),FSlateRect(),FSlateRect(),0);')
marker='                if(Evidence->bKnown)\n                {'
assert s.count(marker)==1
s=s.replace(marker,'''                if(Evidence->bCulled)
                {
                    Number(Entry,TEXT("culled_left"),Evidence->CulledBounds.Left);Number(Entry,TEXT("culled_top"),Evidence->CulledBounds.Top);
                    Number(Entry,TEXT("culled_right"),Evidence->CulledBounds.Right);Number(Entry,TEXT("culled_bottom"),Evidence->CulledBounds.Bottom);
                    Number(Entry,TEXT("culled_clip_left"),Evidence->CulledClip.Left);Number(Entry,TEXT("culled_clip_top"),Evidence->CulledClip.Top);
                    Number(Entry,TEXT("culled_clip_right"),Evidence->CulledClip.Right);Number(Entry,TEXT("culled_clip_bottom"),Evidence->CulledClip.Bottom);
                }
'''+marker)
rec['begin'](p,'Retain exact finite ancestor rectangle and culling rectangle for independent per-record culling proof');t.write_text(s,encoding='utf-8');rec['finish'](p)
