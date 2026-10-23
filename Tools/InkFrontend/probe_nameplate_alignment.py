"""Inspect native painted geometry; Python Geometry bindings are intentionally unnecessary."""
import json
import unreal

def inspect_nameplate_alignment(world):
    directors=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.InkMenuDirector)
    frontend=directors[0].get_editor_property('frontend_widget') if directors else None
    plates=json.loads(unreal.InkFrontendEditorLibrary.describe_widget(frontend)).get('nameplates',[]) if frontend else []
    checks=[dict(name='rendered world nameplates inspected',passed=bool(plates))]
    for p in plates:
        checks.append(dict(name=p['participant_id']+' avatar/frame/particles share painted geometry',passed=p['owned'] and p.get('center_error_pixels',999)<.75 and p.get('particle_center_error_pixels',999)<.75 and p.get('bounds_error_pixels',999)<.75 and p.get('avatar_diagonal_pixels',0)>1,actual=p))
    checks.append(dict(name='participant ownership unique',passed=len({p['participant_id'] for p in plates})==len(plates)))
    return dict(passed=all(x['passed'] for x in checks),checks=checks,nameplates=plates,boundary='Native painted UMG geometry in rendered Windows PIE')
