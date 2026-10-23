"""Import approved bytes through UE asset tools; never rebuild other Widgets."""
import hashlib,json,traceback
from pathlib import Path
import unreal
ROOT=Path(unreal.Paths.project_dir());OUT=ROOT/'Saved/Codex/HomeV2Integration20261022'
rows=json.loads((ROOT/'Tools/InkFrontend/home_v2_imports.json').read_text('utf-8'))
R={'assets':[],'errors':[],'passed':False}
try:
 tools=unreal.AssetToolsHelpers.get_asset_tools()
 for r in rows:
  src=Path(r['source']);assert hashlib.sha256(src.read_bytes()).hexdigest()==r['sha256'],str(src)
  task=unreal.AssetImportTask();task.set_editor_property('filename',str(src));task.set_editor_property('destination_path','/Game/UI/Ink/HomeV2/Textures');task.set_editor_property('destination_name',r['name']);task.set_editor_property('automated',True);task.set_editor_property('replace_existing',False);task.set_editor_property('save',False)
  tex=unreal.load_asset(r['asset']) if unreal.EditorAssetLibrary.does_asset_exist(r['asset']) else None
  if not tex:
   tools.import_asset_tasks([task]);tex=unreal.load_asset(r['asset'])
  assert isinstance(tex,unreal.Texture2D),r
  tex.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
  tex.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
  tex.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
  tex.set_editor_property('srgb',True);tex.set_editor_property('never_stream',True)
  tex.set_editor_property('filter',unreal.TextureFilter.TF_BILINEAR)
  tex.set_editor_property('address_x',unreal.TextureAddress.TA_CLAMP);tex.set_editor_property('address_y',unreal.TextureAddress.TA_CLAMP)
  unreal.EditorAssetLibrary.set_metadata_tag(tex,'HomeV2SourceSHA256',r['sha256'])
  unreal.EditorAssetLibrary.set_metadata_tag(tex,'HomeV2Component',r['id'])
  unreal.EditorAssetLibrary.set_metadata_tag(tex,'HomeV2ApprovedPolicy','v4')
  assert unreal.EditorAssetLibrary.save_loaded_asset(tex)
  actual=[tex.blueprint_get_size_x(),tex.blueprint_get_size_y()]
  assert actual==r['size'],(actual,r['size'])
  R['assets'].append(dict(asset=tex.get_path_name(),size=actual,source_sha256=r['sha256'],compression=str(tex.get_editor_property('compression_settings')),srgb=tex.get_editor_property('srgb'),mips=str(tex.get_editor_property('mip_gen_settings'))))
 # Circular mask for real provider avatars. The approved illustration is only a role icon.
 target='/Game/UI/Ink/HomeV2/Materials/M_HomeAvatarCircle'
 mat=unreal.load_asset(target) if unreal.EditorAssetLibrary.does_asset_exist(target) else None
 if not mat:
  mat=tools.create_asset('M_HomeAvatarCircle','/Game/UI/Ink/HomeV2/Materials',unreal.Material,unreal.MaterialFactoryNew())
  mat.set_editor_property('material_domain',unreal.MaterialDomain.MD_UI);mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
  mel=unreal.MaterialEditingLibrary
  custom=mel.create_material_expression(mat,unreal.MaterialExpressionCustom,0,0)
  custom.set_editor_property('code','float4 c=Texture2DSample(Avatar,AvatarSampler,UV); float mask=saturate((1.0-length(UV*2.0-1.0))*128.0); return float4(c.rgb,c.a*mask);')
  custom.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT4)
  inputs=[]
  for name in ['UV','Avatar']:
   ci=unreal.CustomInput();ci.set_editor_property('input_name',name);inputs.append(ci)
  custom.set_editor_property('inputs',inputs)
  uv=mel.create_material_expression(mat,unreal.MaterialExpressionTextureCoordinate,-500,0)
  tex=mel.create_material_expression(mat,unreal.MaterialExpressionTextureObjectParameter,-500,150)
  tex.set_editor_property('parameter_name','Avatar');tex.set_editor_property('texture',unreal.load_asset('/Engine/EngineResources/WhiteSquareTexture'))
  assert mel.connect_material_expressions(uv,'',custom,'UV') and mel.connect_material_expressions(tex,'',custom,'Avatar')
  for color in [True,False]:
   node=mel.create_material_expression(mat,unreal.MaterialExpressionComponentMask,250,0 if color else 180)
   for channel in ['r','g','b']:node.set_editor_property(channel,color)
   node.set_editor_property('a',not color)
   assert mel.connect_material_expressions(custom,'',node,'')
   assert mel.connect_material_property(node,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR if color else unreal.MaterialProperty.MP_OPACITY)
  mel.recompile_material(mat)
  assert unreal.EditorAssetLibrary.save_loaded_asset(mat)
 R['avatar_mask']=mat.get_path_name();R['passed']=len(R['assets'])==18
except Exception:R['errors'].append(traceback.format_exc())
(OUT/'import-report.json').write_text(json.dumps(R,ensure_ascii=False,indent=2),'utf-8')
if not R['passed']:raise RuntimeError(R['errors'])
