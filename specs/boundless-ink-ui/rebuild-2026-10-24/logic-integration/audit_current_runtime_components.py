"""Read formal captures only; pre-layout diagnostics never become paint evidence."""
from pathlib import Path
import json,datetime
HERE=Path(__file__).resolve().parent
EXPECTED=[
 '/Game/newUI/Components/Player/T_MainActionOrnament_v1',
 '/Game/newUI/Components/Player/T_HudRouteGlyphAtlas_v1',
 '/Game/newUI/Components/Player/T_CharacterAttributeGlyphAtlas_v1',
 '/Game/newUI/Components/Player/T_InventoryCategoryGlyphAtlas_v1',
 '/Game/newUI/Textures/Journey/T_NUI_JourneyMapPortrait_V1',
 '/Game/newUI/Components/RankWuling/T_NUI_Wuling_SectInsignia_Normal_V1',
 '/Game/newUI/Activities/Icons/T_DailyActivityScroll_v1',
 '/Game/newUI/Activities/Icons/T_LimitedActivityCalendar_v1',
 '/Game/newUI/Activities/Icons/T_PermanentActivityLotus_v1',
 '/Game/newUI/Components/Player/T_MethodLoadoutGlyphAtlas_v1',
 '/Game/newUI/Components/Player/T_EquipmentFabaoGlyphAtlas_v1',
 '/Game/newUI/Components/Player/T_AppearancePreviewOrnamentAtlas_v1']
EXPECTED=[p+'.'+p.rsplit('/',1)[-1] for p in EXPECTED]
FOLDER=HERE/'default-root-pie-14';report=json.loads((FOLDER/'report.json').read_text(encoding='utf-8'))
proof={p:[] for p in EXPECTED}
for png in report['captures']:
 p=Path(png).with_suffix('.json');data=json.loads(p.read_text(encoding='utf-8'))
 for img in data['images']:
  path=img.get('resource')
  if path not in proof:continue
  if img.get('visible') and img.get('paint_expectation')=='expected' and img.get('geometry_arranged') and img.get('painted_width',0)>0 and img.get('painted_height',0)>0:
   proof[path].append(dict(snapshot=str(p.relative_to(HERE)),page=data['page'],image=img))
rows=[dict(resource=k,actually_painted=bool(v),evidence=v) for k,v in proof.items()]
result=dict(checked_at=datetime.datetime.now().astimezone().isoformat(),source_epoch_sha256=report['source_epoch_sha256'],source_build=26,formal_attempt=14,scope='Current source26 formal captures only. Expected-painted actual UImage resource plus nonzero independently intersected paint size. No pre-layout or logical-only images counted.',approved=False,imported_components=13,portrait_applicable_components=12,landscape_map_retained_out_of_scope=True,actual_painted_count=sum(x['actually_painted'] for x in rows),rows=rows)
result['passed']=result['actual_painted_count']==12
(HERE/'current-runtime-component-paint-audit.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps(dict(passed=result['passed'],actual_painted_count=result['actual_painted_count'],missing=[x['resource'] for x in rows if not x['actually_painted']]),ensure_ascii=False))
