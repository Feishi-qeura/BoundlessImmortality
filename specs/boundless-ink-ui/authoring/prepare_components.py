from pathlib import Path
import json, hashlib, re
import yaml

ROOT=Path(__file__).resolve().parents[1]
APPROVAL='批准首页v2及规范和屏幕地图，开始拆解首页组件'
def write(rel,text):
    p=ROOT/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(text.strip()+'\n',encoding='utf-8')
def read_yaml(rel):return yaml.safe_load((ROOT/rel).read_text(encoding='utf-8'))
def dump(rel,data):write(rel,yaml.safe_dump(data,allow_unicode=True,sort_keys=False,width=110))

approval=dict(date='2026-10-22',timezone='Asia/Hong_Kong',source='direct-user-message',quote=APPROVAL,
    approved=['home-v2','style-contract','screen-map','supporting-ui-planning-documentation'],
    authorized_next_stage='home-component-breakdown',new_component_assets_approved=False,
    runtime_changes_authorized=False)
write('evidence/approval-home-v2.json',json.dumps(approval,ensure_ascii=False,indent=2))
for name in ['gdd.md','prd.md','interaction.md','spec.md']:
    p=ROOT/name;t=p.read_text(encoding='utf-8')
    t=t.replace('- status: draft','- status: approved',1)
    if APPROVAL not in t:t+='\n批准记录：用户于2026-10-22批准本轮规范、屏幕地图和首页v2：'+APPROVAL+'。批准仅针对当前UI设计基线，不新增玩法或发布权限。\n'
    write(name,t)
style=read_yaml('contracts/style-contract.yaml');style['status']='approved';style['approval']=approval
dump('contracts/style-contract.yaml',style)
screen=read_yaml('contracts/screen-contract.yaml');screen['status']='approved';screen['scope_approval']=approval
for s in screen['screens']:
    s['specification_approved']=True
    if s['id']=='home':
        s['status']='approved';s['approval']=dict(approved=True,candidate='home-v2',evidence='evidence/approval-home-v2.json')
dump('contracts/screen-contract.yaml',screen)
manifest=read_yaml('contracts/asset-manifest.yaml')
for a in manifest['assets']:
    if a['id']=='home-v2':
        assert hashlib.sha256((ROOT/a['path']).read_bytes()).hexdigest()==a['sha256']
        a.update(status='approved',approved=True,review_status='user-approved',approval_evidence='evidence/approval-home-v2.json')
dump('contracts/asset-manifest.yaml',manifest)

# Art primitives are generated once. Interaction states are separately specified as
# deterministic renderer recipes; no regenerated state is allowed to shift silhouette.
items=[
 ('home-identity-ink','身份墨底','panel',[512,192],[18,30,500,197],
  'The broad irregular horizontal black ink brushstroke behind the upper-left player identity. Extract and reconstruct ONLY this dark charcoal ink reading backing, from near the avatar right edge extending under the name and two bars. Organic dry-brush ends toward the right, denser left, low-contrast softly translucent interior. Absolutely no avatar, no frame, no bars, no letters, no numbers or gold ornaments. A single wide horizontal brushstroke on transparent canvas. Desired visible silhouette aspect about 3.2:1.'),
 ('home-avatar-frame','头像金框','frame',[320,320],[27,40,150,151],
  'ONLY the ornate circular aged-gold avatar RING from upper left. One circular frame with the small top gold diamond and dark ink accents. Both the round CENTER APERTURE and the exterior must be genuinely alpha-transparent; no face, portrait, opaque paper disc, sky or photo. Preserve thin sculpted gold contour, subtle soft upper-left highlights. The ring is one object, centered, complete and not clipped. No realm plaque.'),
 ('home-avatar-portrait','修士头像','character',[320,320],[43,61,120,114],
  'ONLY the fictional male cultivator portrait visible in the circular avatar: a youthful Chinese xianxia swordsman with long black hair tied high, pale face in three-quarter view facing right, dark charcoal robe with a little white inner collar. Match the approved avatar character identity and ink-illustrated rendering. Head-and-shoulders bust, hair complete, isolated on real transparent background. No circular border, no frame, no sky, no opaque square background, no typography. This is an authorized fictional game character.'),
 ('home-realm-plaque','境界空白牌','panel',[384,128],[22,178,160,47],
  'ONLY the small EMPTY dark ink-black realm plaque below the upper-left avatar. Low horizontal angular scroll silhouette, thin ornate aged-gold outline and small symmetric outward corner curls, matching the reference. Solid nearly black interior readable for later dynamic text. Absolutely no Chinese characters. Do not include the circular avatar, health bar, or surrounding brushstroke. One horizontal plaque on transparent background, visible aspect 3.4:1.'),
 ('home-bar-track','状态条金框轨道','progress',[768,96],[166,97,321,39],
  'ONLY one EMPTY horizontal ornate status-bar frame and dark inner TRACK, matching the red/green bars at upper left. Slender horizontal gold perimeter with a subtle beveled left point and a small engraved diamond-ended cap at right. Inner well is charcoal-black, no colored fill, no words or numbers. One unified frame plus empty track, not a sheet of different bars. Exterior transparent, dark inner track retained. Visible aspect about 9:1; keep right decorative tip complete.'),
 ('home-health-fill','血量红色填充','progress',[768,72],[179,104,280,25],
  'ONLY the red HEALTH FILL strip inside the upper-left status bar. Rich deep cinnabar-to-red lacquer subtle gradient with upper-left soft specular highlight, very long narrow ribbon with slanted or gently pointed short ends. No gold outline or engraved cap, no surrounding empty track, no letters, no numbers. Transparent exterior. Desired visible aspect about 11:1. Uniform visual density so a left-to-right shader clip can display the fraction.'),
 ('home-qi-fill','修为玉青填充','progress',[768,72],[179,147,280,25],
  'ONLY the green cultivation FILL strip from the lower upper-left status bar. Deep jade #3A8D83 to muted emerald, lacquer gradient with upper-left soft highlight, very long narrow ribbon with gently slanted short ends. No gold border, no track, no icons, no letters, no numbers. Transparent exterior. Match health fill length and silhouette, desired visible aspect about 11:1. Designed to be clipped left-to-right by a shader.'),
 ('home-top-tile','顶部入口空白底框','panel',[256,320],[542,55,115,144],
  'ONLY one EMPTY top-right utility tile backing, like the activity/sign-in/journey tiles. Tall compact dark blue-charcoal ink panel with subtle translucent surface, thin irregular ornate gold outline, small curled corner accents, gently chamfered corners. Entire interior blank: no lantern, no scroll, no pagoda, no Chinese labels or typography. One tile, visible aspect about 0.8:1, centered on transparent exterior.'),
 ('home-icon-activity','活动红灯笼','icon',[320,320],[565,63,70,77],
  'ONLY the red and gold hanging Chinese lantern icon in the first top-right tile. Single round red glowing lantern, fine ornate aged-gold suspension arch and short dangling tassel. Same front three-quarter UI icon perspective and stylized painterly material as reference. No dark utility tile, no background card, no label, no red notification dot. Complete lantern with tassel fully inside transparent canvas, no shadow rectangle.'),
 ('home-icon-sign-in','签到空白卷轴','icon',[320,320],[696,66,66,75],
  'ONLY the sign-in icon from the middle top-right tile: a little antique ivory paper calendar/scroll with dark bronze-gold rollers and a slight backward lean, elegant readable silhouette. Remove every Chinese stroke, grid number and writing from its paper; its paper is entirely BLANK warm xuan paper. No check mark, no label, no tile, no background. Isolated centered complete scroll on real transparency.'),
 ('home-icon-journey','仙途宝塔','icon',[320,320],[819,60,67,85],
  'ONLY the small Chinese tiered pagoda icon from the last top-right tile: dark bronze structure, aged-gold tiled eaves, tiny pointed finial, sparse pale ink-cloud foot. Faithful front three-quarter stylized game UI icon, not a scene or huge architectural landscape. Entire base and finial complete, isolated on transparent background. No black tile, no label or typography.'),
 ('home-icon-bag','背包锦囊','icon',[384,384],[232,1482,124,118],
  'ONLY the antique cream-colored cloth drawstring pouch from the bottom second navigation entry. Full plump ivory brocade bag, tied with gold rope and bow, dark teal and gold tassels at right. Same view and proportions as approved reference. Hand-painted xianxia game icon with aged gold accents and upper-left soft light. No label, no cloud base, no tile or scene. Complete silhouette and tassels on transparent background.'),
 ('home-icon-cultivate','修炼入定徽记','icon',[512,512],[386,1449,178,162],
  'ONLY the large central meditation navigation emblem: small anonymous dark golden seated meditator silhouette with crossed legs and hands together, enveloped in a circular flowing amber-gold flame/qi halo. Preserve reference shape, larger visual weight than other nav icons. Detailed layered gold filaments, glow fades to alpha at outer edge, transparent cutout gaps between filaments. No word 修炼, no paper label, no cloud band, no opaque dark circle or rectangle, no environment. Complete unclipped halo with at least 10 percent safe transparent margin.'),
 ('home-icon-rank','排行交叉双剑','icon',[384,384],[584,1480,123,122],
  'ONLY the crossed silver swords ranking icon at bottom fourth position, with gold hilts and a small aged-gold laurel arc. Preserve two clear crossed blades, symmetric readable silhouette and antique painterly metal. No trophy, shield, labels, cloud bands, tile, sky or scene. Complete tips and laurel on transparent canvas; same upper-left light as all icons.'),
 ('home-icon-menu','菜单古金齿轮','icon',[384,384],[767,1494,117,105],
  'ONLY the antique aged-gold gear icon at bottom right. A circular open gear ring with rectangular teeth, fine engraved inner rim, ink-dark edge accents. The round central HOLE must be alpha-transparent and the outside must be alpha-transparent. No opaque dark disc filling the hole. Match reference ring profile, not a modern flat vector settings icon. No label, no cloud strip, no panel or background.'),
 ('home-rank-paper','查看排行空白纸底','panel',[640,224],[672,794,250,87],
  'ONLY the horizontal pale xuan-paper button backing at right center that originally reads 查看排行. Reconstruct the entire undamaged BLANK ivory paper interior after removing the lettering. Thin ornate aged-gold border, clipped decorative corners and small pale gold/ivory cloud tails near left edge, refined and subordinate to the meditation icon. No text, no carved text-shaped holes or smudges, no numbers, no screenshot environment. Entire paper center stays opaque and empty; exterior genuinely transparent. Desired visible shape aspect about 3:1.'),
 ('home-preview-paper','仅预览标签纸底','panel',[512,128],[22,253,223,50],
  'ONLY the small pale paper and faint gold ink tag below the identity block that originally says 视觉预览. Reconstruct it completely WITHOUT any characters. Irregular horizontal ivory paper strip, faint soft-gold edge brushwork, small tapered rightward paper/cloud tail. Interior opaque enough for later dynamic text. One isolated blank tag, no label, no cutout holes where words used to be, transparent outside. Visible aspect about 4.5:1. This is a development-only preview tag.'),
 ('home-bottom-cloud','底栏淡金云带','decoration',[1024,128],[0,1551,941,65],
  'ONLY the very thin horizontal faded gold-and-ink cloud ornament connecting the five bottom navigation icons. One continuous low, airy, calligraphic cloud flourish band, warm aged-gold outline and ivory wisps fading to transparent, several restrained curls, low height. NO icons, no portrait, no lanterns, no meditation figure, no swords or gear, no text, no labels, no background rectangle. Desired visible aspect about 10:1. This ornamental band is subordinate and can be alpha-composited behind individual icons.'),
]

components=[];queue=[]
common='''Use case: precise-object-edit
Asset type: 单独的游戏 UI 组件素材 / one standalone production game UI art primitive.
Input image is the USER-APPROVED home-v2 visual. Match that exact component design and material vocabulary, not an unrelated variation. The requested element must be extracted and semantically reconstructed cleanly, never delivered as a rectangle cropped from the screenshot.
Style contract: Chinese cultivation ink-wash, xuan paper #F3EBDD, ink #202823, aged gold #B88B45 with restrained #E9CA84, soft upper-left illumination, crisp complete edges and low-detail readable silhouette. Keep art in the same perspective as the approved source.
'''
for sid,name,category,size,bbox,description in items:
    prompt=common+f'Component ID: {sid}. Component category: {category}. State: default. Intended engine source scale around {size[0]}x{size[1]}; actual returned canvas dimensions will be measured, not assumed.\nRequest: '+description+'''
Composition: exactly ONE centered complete component on a genuinely transparent RGBA background. Allow ample transparent safe padding all around. Preserve alpha in openings and translucent wisps. Use the canvas aspect closest to the requested visible component aspect where possible. Do not include a checkerboard graphic as part of the pixels.
STRICT NO-TEXT policy: no letters, numerals, Chinese glyphs, pseudo-writing, labels, watermarks, arrows, grid lines or annotation. Buttons and reading surfaces retain a fully restored empty material center, not holes where text was. No other UI pieces, no full screen, no device, no sprite sheet, no multiple variants. No opaque screenshot background, sky, rocks, bamboo or scenic props. Keep thin edges intact and do not clip glow or tassels.
'''
    write(f'prompts/components/{sid}-default-v1.txt',prompt)
    write(f'prompts/components/{sid}.md',f'''# {name}
## 来源与用途
来源：已批准assets/pages/home-v2.png；组件ID：{sid}；类别：{category}；页面图中的参考范围{xstr(bbox) if False else bbox}，仅定位，不做硬裁切交付。
## 尺寸和状态
目标素材尺度{size}，实际输出尺寸与Alpha另行实测。图像状态default。交互态通过独立state-recipes记录，共享基础图，保持轮廓。
## 必须继承的契约字段
style-contract.colors/visual.materials/visual.lighting；透明；无文字；完整轮廓；source=home-v2。
## 图像生成提示词
完整最终文本：{sid}-default-v1.txt。使用内置image_gen，逐组件单独调用。
## 反向限制
无文字数字/伪字/标签/截屏背景/合图；不截断、不抠字留洞；不擅自改变材质。
## 验收记录
pending-generation；非背景组件必须实测透明角与干净边缘，源图不得在批准后被覆盖。
''')
    component=dict(id=sid,name=name,category=category,reusable=sid not in {'home-avatar-portrait','home-icon-cultivate'},
        source_asset_id='home-v2',source_bbox=bbox,transparent_background=True,text_policy='no-text',
        target_size=size,states=['default'],material='paper-ink-aged-gold',atlas_group='home-art',
        slice=dict(type='full',margins=[0,0,0,0]),export_targets=['generic'],
        style_dependencies=['colors','visual.materials','visual.lighting'],
        prompt=f'prompts/components/{sid}-default-v1.txt',output=f'assets/components/{sid}-default-v1.png',
        status='pending-generation',review='pending',dev_only=sid=='home-preview-paper',
        layout_rule='Use measured alpha-content bounds for art placement; keep aspect; do not stretch ornamental art.',
        notes='full/Fit until actual image edges reviewed; no guessed 9-slice margins')
    components.append(component)
    queue.append(dict(component_id=sid,prompt_path=component['prompt'],output=component['output']))
    if not any(a['id']==f'{sid}-default-v1' for a in manifest['assets']):
        manifest['assets'].append(dict(id=f'{sid}-default-v1',kind='component',type='component',screen_id='home',component_id=sid,
            state='default',path=component['output'],dimensions=size,dimensions_basis='target-until-generated',transparent_background=True,
            status='pending-generation',approved=False,prompt_path=component['prompt'],source_assets=['home-v2'],version=1))

dump('contracts/component-contract.yaml',dict(schema_version=1,project_id=ROOT.name,source_screen_id='home',source_asset_id='home-v2',
    source_screen_status='approved',status='in-progress',components=components,
    excluded_art=['真实3D场景与主角：复用现有场景，不生成/替换','全部动态文字：TextBlock，不烘焙','状态差异与底栏阅读渐变：state-recipes渲染定义'],
    review=dict(complete=False,user_approved=False),
    note='本阶段只生成18个独立基础图。完整控件交互态是共享图层加确定性状态参数，不冒充已烘焙状态PNG。'))
dump('contracts/asset-manifest.yaml',manifest)
write('evidence/component-generation-queue.json',json.dumps(queue,ensure_ascii=False,indent=2))
home=next(s for s in screen['screens'] if s['id']=='home');home['components']=[c['id'] for c in components]
dump('contracts/screen-contract.yaml',screen)

recipes={
 'normal':dict(tint=[1,1,1,1],translation=[0,0],scale=1,focus=False,submit_enabled=True),
 'hover':dict(tint=[0.92,0.98,1,1],translation=[0,0],scale=1,overlay='blue-ink-at-0.12',duration_ms=120,desktop_only=True),
 'pressed':dict(tint=[0.85,0.85,0.85,1],translation=[0,2],scale=1,duration_ms=80),
 'focused':dict(tint=[1,1,1,1],translation=[0,0],scale=1,outline='thin-aged-gold',focus=True),
 'selected':dict(tint=[1,1,1,1],translation=[0,0],scale=1,outline='gold-selected-mark',persists_until_selection_changes=True),
 'disabled':dict(tint=[0.62,0.62,0.62,0.70],translation=[0,0],scale=1,submit_enabled=False,reason_visible=True,label_contrast_min=4.5),
 'pending':dict(tint=[0.78,0.78,0.78,0.90],translation=[0,0],scale=1,submit_enabled=False,return_enabled=True,show_progress=True,
    retry_request='same-request-id',success_only_from_authoritative_snapshot=True),
}
controls=[]
for sid,art,label,action in [
 ('activity',['home-top-tile','home-icon-activity'],'活动','OpenActivities'),
 ('sign-in',['home-top-tile','home-icon-sign-in'],'签到','OpenSignIn'),
 ('journey',['home-top-tile','home-icon-journey'],'仙途','OpenExtensions'),
 ('character',['home-avatar-portrait','home-avatar-frame'],'角色','OpenCharacter'),
 ('bag',['home-icon-bag'],'背包','OpenInventory'),
 ('cultivate',['home-icon-cultivate'],'修炼/收功','ToggleCultivation'),
 ('leaderboard',['home-icon-rank'],'排行','OpenLeaderboard'),
 ('menu',['home-icon-menu'],'菜单','ToggleHUDMenu'),
 ('view-rank',['home-rank-paper'],'查看排行','HUDRankView'),
]:
    controls.append(dict(id='home-control-'+sid,art=art,label=label,action=action,
        states=[dict(id=k,art_mode='shared-base-textures',render_recipe=v) for k,v in recipes.items()],
        minimum_hit_size=[88,88],label_font='LXGWWenKai',label_is_dynamic=True,
        event_dispatch='one release inside widget triggers one action; cancel drag or release outside',
        runtime_implementation='not-created-this-stage'))
dump('contracts/home-state-recipes.yaml',dict(schema_version=1,project_id=ROOT.name,source_asset_id='home-v2',
    status='specified-not-engine-implemented',controls=controls,
    overlay_layers=dict(navigation_veil='runtime vertical transparent-to-ink gradient, HitTestInvisible',
    health_fraction='clip red fill left-to-right, clamp0..1; unknown value shows pending label, no fabricated health',
    qi_fraction='clip jade fill left-to-right, share same well geometry',
    cultivation='idle/cultivating/interrupted/blocked/pending; label from authoritative state',
    badge='only real unread/claimable data; no baked red dots',
    reduced_motion='stop looping glow, retain static selection and labels')))

write('components.md','''# 首页组件拆解
来源：用户已批准的home-v2；本轮只做首页组件素材与交接说明，保留运行UI与现有场景。

## 结构
场景与角色沿用UE真实3D；不要从视觉稿抠出岩石人物替代场景。UI分为身份墨底、头像肖像与透明框、境界空白牌、血量/修为的共享轨道与独立填充、三个顶部空白tile及各自图标、底部五入口图标、侧边排行纸底、开发预览空白纸签和装饰云带。
18个基础图逐一用内置image_gen语义重建，非截图矩形裁切；动态文字不进入纹理。底部“角色”复用同一肖像与框。没有因为首页批准而修改保留角色页或背包。

## 状态交付
9个可交互控件分别登记Normal/Hover/Pressed/Focused/Selected/Disabled/Pending七态。所有状态共享同一基础图，差异由独立状态参数表达：悬停微蓝、按下暗化并下移2单位、焦点细框、选中金、禁用带理由、Pending锁提交并保留返回。它们是可实现的渲染规则，不是63张已烘焙的PNG，不声称已经接入UMG。
这样能保持图形轮廓、点击框与文字位置稳定；运行实现时先确认已有按钮行为。状态表见contracts/home-state-recipes.yaml。

## 无字原则与装配
姓名、血量、修为、境界、活动/签到/仙途、角色/背包/修炼/排行/菜单、查看排行和预览标签均用现有字体排版。预览标签只在demo/profile启用，正式显示数据不能从当前日期虚构。头像未知时使用当前项目占位，不擅自套本素材为真人头像。
所有组件默认完整等比显示。9-slice暂不批准：有机墨边、特殊牌匾和云尾不能直接写死Margin。实际Alpha内容框、像素尺寸与文件哈希在生成后登记。包围框用于资产放置参考，不是新交互热点坐标。
素材尚待单独审核；首页获批不等于所有组件自动获批。后续图集/切图/UE接线按实际素材验收继续。
''')
write('tasks.md',(ROOT/'tasks.md').read_text(encoding='utf-8').replace('- [ ] [approval] 用户批准首页；当前未批准。','- [x] [approval] 用户于2026-10-22批准首页v2、规范和屏幕地图；证据见approval-home-v2.json。').replace('- [ ] [component] 批准后拆首页组件，逐状态出透明无字图。','- [ ] [component] 生成18个透明无字基础素材，9控件×7态独立渲染规则；生成中。'))
print(json.dumps(dict(component_count=len(components),controls=len(controls),state_records=len(controls)*len(recipes),approved_home=True),ensure_ascii=False))
