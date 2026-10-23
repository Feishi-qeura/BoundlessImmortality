"""Short real-pixel regression for supplementary Unicode HUD names; no reference images."""
import json
import time
from pathlib import Path
import unreal

OUT=Path(unreal.Paths.project_dir())/'Saved/Codex/UIRefactor20261021/Resume20261021/GlyphVerification'
OUT.mkdir(parents=True,exist_ok=True)
LIB=unreal.InkFrontendEditorLibrary
REPORT={'checks':[],'captures':[],'manual_review':'pending','boundary':'Supplementary Unicode HUD pixels only; not reference comparison'}
INDEX=0
NEXT=0
FRONTEND=None
HANDLE=None

def check(name,value):
    REPORT['checks'].append({'name':name,'passed':bool(value)})
    (OUT/'glyph-report.json').write_text(json.dumps(REPORT,ensure_ascii=False,indent=2),encoding='utf-8')

def tick(delta):
    global INDEX,NEXT,FRONTEND
    if time.monotonic()<NEXT:return
    try:
        if INDEX==0:check('PIE started',LIB.begin_frontend_pie(1280,720));NEXT=time.monotonic()+12
        elif INDEX==1:
            worlds=unreal.EditorLevelLibrary.get_pie_worlds(False)
            if not worlds:NEXT=time.monotonic()+2;return
            directors=unreal.GameplayStatics.get_all_actors_of_class(worlds[0],unreal.InkMenuDirector)
            if not directors:NEXT=time.monotonic()+2;return
            FRONTEND=directors[0].get_editor_property('frontend_widget')
            if not FRONTEND:NEXT=time.monotonic()+2;return
            check('start game',LIB.click_action(FRONTEND,'StartGame'));NEXT=time.monotonic()+5
        elif INDEX==2:check('Unicode identity set',LIB.set_frontend_test_anchor_name(FRONTEND,'问道𠮷仙客 Ω'));NEXT=time.monotonic()+3
        elif INDEX in (3,5):
            tag='glyph-landscape' if INDEX==3 else 'glyph-portrait'
            png=OUT/(tag+'.png')
            check(tag+' screenshot',LIB.capture_frontend_viewport(str(png)))
            (OUT/(tag+'.json')).write_text(LIB.describe_widget(FRONTEND),encoding='utf-8')
            REPORT['captures'].append(str(png));NEXT=time.monotonic()+2
        elif INDEX==4:check('portrait resize',LIB.set_pie_viewport_size(720,1280));NEXT=time.monotonic()+3
        else:
            REPORT['sequence_finished']=True;check('structural checks passed',all(c['passed'] for c in REPORT['checks']))
            unreal.unregister_slate_post_tick_callback(HANDLE);unreal.EditorLevelLibrary.editor_end_play();unreal.SystemLibrary.quit_editor();return
        INDEX+=1
    except Exception as exc:
        REPORT['error']=str(exc);check('script error',False)
        unreal.unregister_slate_post_tick_callback(HANDLE);unreal.EditorLevelLibrary.editor_end_play();unreal.SystemLibrary.quit_editor()

HANDLE=unreal.register_slate_post_tick_callback(tick)
