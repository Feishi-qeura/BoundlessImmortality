"""Export display thresholds from the runtime library, never a second implementation."""
import csv,hashlib,json
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir())
out=root/'docs/design/2026-10-22-character-attribute-ratings'
out.mkdir(parents=True,exist_ok=True)
ids=['Health','Attack','Defense','SpiritDamage','Critical','Evasion','MoveSpeed','CultivationSpeed']
starts=[0,9,13,16,22,23,24,26,31,32,33]
ends=[8,12,15,21,22,23,25,30,31,32,33]
rows=[]
lib=unreal.InkAttributeRatingLibrary
for mode in ('LegacyDefault','V14'):
    for first,last in zip(starts,ends):
        context=unreal.InkAttributeRatingContext()
        context.set_editor_property('realm_index',first)
        context.set_editor_property('bV14Rules',mode=='V14')
        for attribute in ids:
            values=lib.thresholds(attribute,context)
            assert len(values)==7
            for minor in range(first,last+1):
                context.set_editor_property('realm_index',minor)
                assert list(lib.thresholds(attribute,context))==list(values)
            context.set_editor_property('realm_index',first)
            rows.append([mode,lib.major_name(first),first,last,attribute,*[format(v,'.10g') for v in values]])
with (out/'attribute-rating-thresholds.csv').open('w',encoding='utf-8-sig',newline='') as f:
    w=csv.writer(f);w.writerow(['Rules','MajorRealm','RealmIndexStart','RealmIndexEnd','Attribute','D','C','B','A','S','SS','SSS']);w.writerows(rows)
sources=['docs/design/2026-10-21-wuling-resource-v1.4/03-数值策划案.md','docs/design/2026-10-21-wuling-resource-v1.4/15-分类物品与武灵经济扩展策划案.md','docs/design/2026-10-21-wuling-resource-v1.4/tables/realm_targets.csv','docs/design/2026-10-21-wuling-resource-v1.4/tables/items-v1.4/equipment_items.csv']
(out/'export-evidence.json').write_text(json.dumps({'rows':len(rows),'minor_stage_equality':True,'source_sha256':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in sources}},ensure_ascii=False,indent=2),encoding='utf-8')
