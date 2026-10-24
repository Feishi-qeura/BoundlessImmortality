"""Strict numeric check; missing independent culling evidence never excuses NaN."""
import math
KINDS=('texts','images','buttons','borders','scrolls','inputs')
def culled_proven(record):
    if record.get('paint_expectation')!='culled' or not record.get('culling_proof_ancestor'):return False
    keys=('culled_left','culled_top','culled_right','culled_bottom','culled_clip_left','culled_clip_top','culled_clip_right','culled_clip_bottom')
    values=[record.get(k) for k in keys]
    if not all(isinstance(v,(int,float)) and math.isfinite(v) for v in values):return False
    left,top,right,bottom,cl,ct,cr,cb=values
    return right>left and bottom>top and (right<=cl or left>=cr or bottom<=ct or top>=cb)
def inspect(snapshot):
    invalid=[dict(x,kind=k) for k in KINDS for x in snapshot.get(k,[]) if x.get('has_nonfinite_metrics')]
    failures=[x for x in invalid if x.get('visible') and not culled_proven(x)]
    return invalid,failures
if __name__=='__main__':
    expected=dict(name='deliberately_bad_expected_painted',visible=True,geometry_arranged=False,paint_expectation='expected',has_nonfinite_metrics=True,x=None,width=32,height=32)
    unknown=dict(expected,paint_expectation='unknown')
    culled=dict(expected,paint_expectation='culled',culling_proof_ancestor='finite_ancestor',culled_left=0,culled_top=100,culled_right=20,culled_bottom=120,culled_clip_left=0,culled_clip_top=0,culled_clip_right=50,culled_clip_bottom=50)
    assert len(inspect({'buttons':[expected]})[1])==1
    assert len(inspect({'buttons':[unknown]})[1])==1
    assert len(inspect({'buttons':[culled]})[1])==0
    assert len(inspect({'buttons':[dict(culled,culled_top=25,culled_bottom=45)]})[1])==1
    assert len(inspect({'buttons':[dict(culled,culled_top=None)]})[1])==1
    print('5 checks passed: expected-painted bad metric, unknown, independent culling, false culling, invalid proof')
