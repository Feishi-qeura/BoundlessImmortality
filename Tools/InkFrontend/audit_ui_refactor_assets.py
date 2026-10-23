"""Read-only audit of real loaded WBP defaults/dependencies and actual source textures."""
import json
import re
from pathlib import Path
import unreal

def canonical(value):
    text=str(value)
    match=re.search(r"<Object '([^']+)'",text)
    return match.group(1) if match else re.sub(r'0x[0-9A-Fa-f]+','ADDRESS',text)

def run():
    root=Path(unreal.Paths.project_dir())
    out=root/'Saved/Codex/UIRefactor20261021'
    before=json.loads((out/'Baseline/baseline-report.json').read_text(encoding='utf-8'))
    result={'assets':[],'checks':[],'texture_exports':[]}
    registry=unreal.AssetRegistryHelpers.get_asset_registry()
    options=unreal.AssetRegistryDependencyOptions(include_hard_package_references=True,include_soft_package_references=True)
    for original in before['assets']:
        path=original['path'];asset=unreal.load_asset(path)
        entry={'path':path,'loaded':bool(asset),'defaults':{},'dependencies':[]}
        if asset:
            defaults=unreal.get_default_object(asset.generated_class())
            for key,old in original['defaults'].items():
                current=canonical(defaults.get_editor_property(key));entry['defaults'][key]=current
                if key!='text_font':
                    result['checks'].append({'name':path+': '+key+' preserved','passed':current==canonical(old),'before':canonical(old),'after':current})
                else:result['checks'].append({'name':path+': new font reference','passed':current.endswith('F_InkBody.F_InkBody'),'after':current})
            entry['dependencies']=[str(x) for x in registry.get_dependencies(path,options)]
        result['assets'].append(entry)
    result['passed']=all(c['passed'] for c in result['checks']) and all(x['loaded'] for x in result['assets'])
    export_dir=out/'ActualTextureExports';export_dir.mkdir(exist_ok=True)
    for path in ['/Game/UI/Ink/Textures/T_InkMeditation','/Game/UI/Ink/Cohesion20261021/T_CohesionIcons']:
        texture=unreal.load_asset(path)
        task=unreal.AssetExportTask();task.set_editor_property('object',texture)
        task.set_editor_property('filename',str(export_dir/(path.rsplit('/',1)[1]+'.png')))
        task.set_editor_property('exporter',unreal.TextureExporterPNG())
        task.set_editor_property('automated',True);task.set_editor_property('prompt',False)
        task.set_editor_property('replace_identical',True)
        result['texture_exports'].append({'asset':path,'exported':unreal.Exporter.run_asset_export_task(task)})
    (out/'asset-audit.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    return result

if __name__=='__main__':run()
