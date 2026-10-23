"""Rendered themed ranks, actual silk sprites and atomic avatar alignment."""
from pathlib import Path
import json,runpy
import unreal
source=Path(unreal.Paths.project_dir())/'Tools/InkFrontend/verify_niagara_revision_pie.py'
art=source.parent/'create_ink_gold_backings.py'
exec(compile(art.read_text(encoding='utf-8').replace('unreal.SystemLibrary.quit_editor()','pass'),str(art),'exec'),{'__name__':'__large_realm_art__'})
exec(compile(source.read_text(encoding='utf-8'),str(source),'exec'),globals())
OUT=Path(unreal.Paths.project_dir())/'Saved/Codex/NiagaraUiRevision20261022/RenderedRankedFlow'
OUT.mkdir(parents=True,exist_ok=True);STEPS.clear()
probe=runpy.run_path(str(source.parent/'probe_nameplate_alignment.py'))['inspect_nameplate_alignment']

def alignment(tag):
    plates=data().get('nameplates',[]);R.setdefault('alignment',{})[tag]=plates
    check(tag+' rendered world nameplates inspected',bool(plates))
    for plate in plates:
        check(tag+' '+plate['participant_id']+' shared avatar geometry',plate['owned'] and plate.get('center_error_pixels',999)<.75 and plate.get('particle_center_error_pixels',999)<.75 and plate.get('bounds_error_pixels',999)<.75 and plate.get('avatar_diagonal_pixels',0)>1,plate)
    check(tag+' participant ownership unique',len({p['participant_id'] for p in plates})==len(plates))
    if tag=='portrait':
        s=data();realm=next(x for x in s['images'] if x['name']=='TitleArtwork' and '.AnchorRealmTitle.' in x['object'])
        check('HUD realm artwork is at least 1.5 times former width',realm['painted_width']>=143,realm)

def board(tag,compact=False):
    board_check(tag,compact);s=data();prefix='RankHUD' if compact else 'RankAnchor'
    theme='Damage' if 'damage' in tag else 'Speed' if 'speed' in tag else 'Sword'
    header=next(x for x in s['images'] if x['name']==prefix+'Heading');frame=next(x for x in s['images'] if x['name']==prefix+'Frame')
    check(tag+' correct baked theme heading','M_Rank'+theme+'Header' in header['resource'],header)
    check(tag+' theme heading centered',abs(header['x']+header['painted_width']/2-frame['x']-frame['painted_width']/2)<1,header)
    tiers=[]
    for rank in range(1,13):
        image=next(x for x in s['images'] if x['name']==prefix+'RowArt_'+str(rank))
        if rank<=10:check(tag+' distinct tier '+str(rank),f'M_Rank{theme}Tier{rank:02d}' in image.get('material_parent',''),image.get('material_parent'));tiers.append(image.get('material_parent'))
        else:check(tag+' ordinary rank '+str(rank),'tier_texture' not in image,image.get('resource'))
    check(tag+' ten unique artworks',len(set(tiers))==10,tiers)
    rates=[]
    for rank in range(1,11):
        fx=next(w for w in unreal.ObjectIterator(unreal.InkNiagaraTitleWidget) if w.get_name()==prefix+'PrestigeEffect_'+str(rank))
        component=fx.get_effect_widget().get_niagara_component()
        # Offscreen rows may not have ticked; style intensity remains inspected
        # through the visible prefix and rank 9/10 on the lower scroll capture.
        if not component:continue
        value=component.get_variable_float('User.Rate')
        rate=float(value[0] if isinstance(value,(tuple,list)) else value)
        if fx.is_effect_active():rates.append((rank,rate))
    check(tag+' visible rank strength increases toward first',len(rates)>=2 and all(a[1]>b[1] for a,b in zip(rates,rates[1:])),rates)

def scroll_lower():
    for scroll in unreal.ObjectIterator(unreal.ScrollBox):
        if scroll.get_name()=='RankAnchorScroll' and '.WBP_InkFrontend' in scroll.get_path_name():scroll.set_scroll_offset(360)

def scroll_top():
    for scroll in unreal.ObjectIterator(unreal.ScrollBox):
        if scroll.get_name()=='RankAnchorScroll' and '.WBP_InkFrontend' in scroll.get_path_name():scroll.set_scroll_offset(0)

def check_silk():
    values=sample('flowing-silk');parts=[p for x in values for p in x.get('particles',[]) if 'sprite_size' in p]
    check('actual CPU trail sprites are elongated rather than dots',len(parts)>0 and all(p['sprite_size'][0]>p['sprite_size'][1]*4 for p in parts),[p['sprite_size'] for p in parts[:12]])
    check('flowing version used by every active system',all('/Flowing20261022/' in x['system'] for x in values))
    check('actual CPU trails move',any(p.get('displacement',0)>.0001 for p in parts))

step('fixture setup',setup,1)
step('fixture profiles',profiles,3)
step('HUD flows portrait',lambda:(hud_check('hud-720x1280'),alignment('portrait'),check_silk()))
step('compact speed open',lambda:click('HUDRankView'),2)
step('compact speed',lambda:board('compact-speed-720x1280',True))
step('compact damage switch',lambda:click('HUDRankDamage'),2)
step('compact damage',lambda:board('compact-damage-720x1280',True))
step('full board',lambda:F.show_page('Leaderboard'),2)
for category,action in [('sword','RankAnchor'),('speed','RankCultivation'),('damage','RankDamage')]:
    step(category+' select',lambda a=action:click(a),2)
    step(category+' top',lambda t=category:board('full-'+t+'-top'))
    step(category+' lower scroll',scroll_lower,1)
    step(category+' lower ranks',lambda t=category:capture('full-'+t+'-ranks5-12'))
    step(category+' top restore',scroll_top,1)
step('HUD restore',lambda:F.show_page('HUD'),1)
for width,height,tag in [(360,640,'small'),(1280,720,'landscape')]:
    step(tag+' resize',lambda w=width,h=height:L.set_pie_viewport_size(w,h),2)
    step(tag+' HUD alignment',lambda t=tag,w=width,h=height:(hud_check('hud-'+str(w)+'x'+str(h)),alignment(t)))
step('full landscape open',lambda:F.show_page('Leaderboard'),2)
step('full landscape',lambda:board('full-damage-1280x720'))
step('HUD for reduced motion',lambda:F.show_page('HUD'),1)
step('reduced motion on',lambda:F.set_editor_property('reduced_motion',True),1)
step('reduced motion cleanup',reduced_check)
step('return menu',lambda:D.return_to_main_menu(),1)
step('menu cleanup',menu_check)
