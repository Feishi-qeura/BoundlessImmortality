"""Author design data only. Does not import or modify game code/assets."""
from pathlib import Path
import csv
import json

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'tables/items-v1.4'


def save(name, data):
    with (OUT / name).open('w', encoding='utf-8-sig', newline='') as f:
        w = csv.DictWriter(f, fieldnames=list(data[0]))
        w.writeheader()
        w.writerows(data)


def read(name):
    with (ROOT / 'tables' / name).open(encoding='utf-8-sig', newline='') as f:
        return list(csv.DictReader(f))


def js(name, data):
    (OUT / name).write_text(json.dumps(data, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')


stages_data = [
 ('筑基','练气九阶','筑基初期',8,9,0,'青云山','凝脉谷','筑基丹',3,3,
  '凝脉草 玉露芝 温经藤 青络参 守心兰',
  '百年灵髓 晨曦露珠 青岚玉液 云泉石乳 霞光灵砂',
  '天道筑基石 无尘道芽 紫府灵胚 太一凝露 九纹灵珀'),
 ('结晶','筑基大圆满','结晶一阶',12,13,1,'玄铁岭','凝晶地窟','凝晶丹',2,4,
  '固脉玉液 玉骨花 地心温髓 碧络石脂 归息灵膏',
  '地脉晶髓 玄岩灵乳 碧晶砂髓 丹霞晶露 寒玉地精',
  '无垢灵晶 太玄晶心 七窍地珀 玲珑灵胆 星纹晶胎'),
 ('金丹','结晶三阶','金丹一阶',15,16,2,'赤炎地','五行丹窟','结丹灵液',2,5,
  '护脉丹 五蕴灵脂 金络叶 玄霜护心莲 太和真露',
  '金灵精 木灵精 水灵精 火灵精 土灵精',
  '天罡丹髓 九转丹心 混元金莲 五气朝元露 紫霄丹魄'),
 ('元婴','金丹六阶','元婴',21,22,3,'妖兽林','古修育婴府','化婴丹',3,4,
  '镇魂香 养婴芝 玄牝护灵液 凝神魂莲 太素安魂玉',
  '千年魂髓 月魄灵乳 太阴魂露 紫府灵烟 婴华玉髓',
  '先天元胎 九窍灵胎 太初婴魄 玄黄胎玉 造化婴莲'),
 ('化神','元婴','化神',22,23,4,'星陨海','观神古台','化神灵引',1,5,
  '定魂珠 澄心玉髓 玄明养神草 无相护魂香 青冥定识莲',
  '神识结晶 星辉识露 太清神砂 月华神乳 玄光灵识髓',
  '天魂玉 九霄神胎 观天灵瞳 万象神珀 太清道镜心'),
 ('元神','化神','元神一阶',23,24,5,'星陨海','沉魂古战场','元神灵髓',2,6,
  '固神丹 不动魂玉 三花凝神露 玄极镇魄香 太乙守神芝',
  '神魂精华 星魂晶髓 纯阳神露 归一魂乳 万念清光砂',
  '太初魂晶 不灭神胎 三生魂莲 元始神珀 玄黄神心'),
 ('炼虚','元神二阶','炼虚一阶',25,26,6,'太虚境','定界裂隙','虚空晶核',2,5,
  '定界石 镇空灵玉 固界神砂 太虚护体液 不动界藤',
  '空间灵髓 虚潮凝露 界隙星砂 空明晶乳 太虚灵烟',
  '虚空本源 无量界心 太初空莲 乾坤道胎 两仪界珀'),
 ('大乘','炼虚五阶','大乘',30,31,7,'太虚境','问道天阙','大乘道果',3,8,
  '护道丹 太清镇身露 九华守道莲 玄黄定心石 无漏护神香',
  '天地精粹 山海道髓 日月灵华 四象真露 八荒清气',
  '道源之心 九霄道莲 太一大道芽 万法归元玉 天地玄黄胎'),
 ('飞升','大乘','飞升',31,32,8,'太虚境','九天劫台','飞升道印',3,10,
  '渡劫丹 避劫雷玉 九霄护体露 无相镇心莲 太清涤魂香',
  '九天仙髓 天河仙露 紫霄雷精 玄清仙气 太素天乳',
  '天劫本源 九天仙胎 造化雷莲 太初仙珀 飞仙道心'),
 ('混元','飞升','混元无极',32,33,9,'太虚境','混沌归一坛','混元道种',3,10,
  '无极灵液 太初护道莲 混元定神玉 鸿蒙守心香 玄黄归一露',
  '混沌精粹 太初道髓 无量玄气 两仪源露 元始灵光',
  '鸿蒙本源 无极道胎 混沌祖莲 太初源心 万道归一珀'),
]

grades = [dict(grade_id=f'G{i}',name=n,core_score=c,guard_weight=g,essence_weight=e,treasure_score=t)
          for i,(n,c,g,e,t) in enumerate([
              ('下品',10,3,6,4),('中品',18,6,14,9),('上品',26,10,22,15),('极品',35,15,30,20)],1)]
bands = [dict(band_id=f'QB{i}', name=n, q_min=lo, q_max=hi, source_indie_multiplier=raw, live_snapshot_factor=live)
         for i,(n,lo,hi,raw,live) in enumerate([
             ('凡品',0,39,2.5,1.00),('良品',40,59,3.0,1.02),('上品',60,74,3.5,1.04),
             ('极品',75,89,4.0,1.06),('天品',90,97,4.5,1.08),('完美',98,100,5.0,1.10)],1)]

master=[]; species=[]; materials=[]; stages=[]; sources=[]; edges=[]; recipes=[]; recipe_inputs=[]


def item(i,n,category,owner='anchor',min_major=0,use='',stack=9999,paid=False,binding='bound',trade=False,expiry='earliest_input_or_window',icon=''):
    r=dict(item_id=i,name=n,category=category,owner_type=owner,min_major=min_major,max_stack=stack,
           binding=binding,tradeable=trade,paid_origin=paid,expiry_policy=expiry,use=use,
           icon_key=icon or category,implementation_status='design_only')
    master.append(r)
    return r


def source(i,n,kind,region,min_major=0,condition='',quota='none',detail=''):
    sources.append(dict(source_id=i,name=n,channel_type=kind,region=region,min_major=min_major,
                        unlock_condition=condition,quota_group=quota,output_rule=detail,
                        profile='prototype_full',real_payment=False))


def link(i,s,qty=1,cycle=1,method='fixed_selected_reward',note=''):
    edges.append(dict(item_id=i,source_id=s,quantity=qty,required_full_rounds=cycle,method=method,note=note))


def recipe(rid,n,out,inputs,stone=0,count=1,min_major=0,owner='anchor',kind='craft',limit='no_extra_limit'):
    recipes.append(dict(recipe_id=rid,name=n,output_item=out,output_count=count,output_owner=owner,
                        stone_cost=stone,min_major=min_major,recipe_type=kind,limit_group=limit,
                        success_rate=1,expiry_policy='earliest_input_and_window',paid_input_allowed=False))
    for iid,amount in inputs:
        recipe_inputs.append(dict(recipe_id=rid,item_id=iid,count=amount))
    link(out,'CH_CRAFT',count,0,'recipe',rid)


# Normalize existing item IDs without changing the baseline CSV or old entitlements.
for b in read('items.csv'):
    item(b['item_id'],b['name'],'legacy_'+b['kind'],b['owner_type'],use=b['use'],stack=int(b['max_stack']),
         paid=b['paid_origin']=='true',binding=b['binding'],trade=b['tradeable']=='true',expiry=b['expiry_policy'])
source('CH_LEGACY','既有来源兼容','legacy','现有世界',condition='沿用v1.3收据与权限',detail='只引用原配置，不追加一份掉落')
source('CH_CRAFT','确定性炼制台','craft','任意安全区',condition='达到对应境界；无需武灵建筑',detail='按recipes与recipe_inputs原子扣料；100%成功，不抽品质')
source('CH_WORLD','世界历练基础供给','world','既有八区域',detail='原world_rewards.csv；每10分钟80灵石/12破境石等，未增加第二份基础供给')
source('CH_BUILD','武灵建设工坊','guild_craft','武灵驻地',detail='主播支付绑定投入，结果直接进入本武灵公库；不产生个人贡献兑换额度')
for b in read('items.csv'): link(b['item_id'],'CH_LEGACY',0,0,'baseline_reference',b['source'])

guard_traits=['经脉','肉身','神魂','灵压','心境']
essence_traits=['凝练','澄明','温养','贯通','归一']
treasure_traits=['天地共鸣','异象','道纹','灵光','根基']
for idx,d in enumerate(stages_data,1):
    label,from_name,to_name,fr,to,major,region,dungeon,core,gqty,emax,gn,en,tn=d
    sid=f'BR{idx:02}'
    base_cost=int(read('realm_targets.csv')[fr]['anchor_material_next'])
    stage=dict(stage_id=sid,name=f'{from_name}→{to_name}',from_realm_index=fr,to_realm_index=to,
               min_major=major,region=region,subarea=dungeon,core_species=f'MAT_{sid}_C01',core_count=1,
               guard_required=False,guard_max_count=gqty,essence_max_count=emax,treasure_max_count=1,
               guard_species_count=5,essence_species_count=5,treasure_species_count=5,
               old_major_break_material_cost=base_cost,old_major_stone_cost=base_cost*3,
               major_stone_at_commit=0,viewer_material_cost=0,tribulation=idx>=4)
    stages.append(stage)
    for cat,names,cycle,traits in [('core',[core],1,['资格']),('guard',gn.split(),1,guard_traits),
                                   ('essence',en.split(),2,essence_traits),('treasure',tn.split(),6,treasure_traits)]:
        tag={'core':'C','guard':'G','essence':'E','treasure':'T'}[cat]
        src=f'CH_{sid}_{tag}'
        source(src,f'{dungeon}·'+{'core':'核心试炼','guard':'护道采集','essence':'灵脉淬取','treasure':'奇珍探寻'}[cat],
               'focused_world',region,major,'进入本阶段前一大境界即可预备','FOCUS_WORLD_6',
               f'每轮只选一个专注目标；累计{cycle}个完整10分钟轮次，定向得所选下品1份；无随机掉率')
        for no,name in enumerate(names,1):
            sp=f'MAT_{sid}_{tag}{no:02}'
            species.append(dict(species_id=sp,name=name,stage_id=sid,category=cat,theme=traits[no-1],
                                region=region,subarea=dungeon,base_source_id=src,base_cycle_rounds=cycle,
                                baseline_required=cat=='core',grade_count=4))
            for g in grades:
                iid=f'{sp}_{g["grade_id"]}'
                score=g[{'core':'core_score','guard':'guard_weight','essence':'essence_weight','treasure':'treasure_score'}[cat]]
                use={'core':'主播大境界突破资格；整次只消耗1份','guard':'提升护道评分与缩短突破后虚弱；不增加渡劫成功率',
                     'essence':'提高突破品质评分','treasure':'提高突破品质与确定性异象；非必需'}[cat]
                item(iid,g['name']+name,'breakthrough_'+cat,min_major=major,use=use,icon='MAT_'+cat+'_'+str(idx))
                materials.append(dict(item_id=iid,species_id=sp,name=g['name']+name,stage_id=sid,
                                      category=cat,grade_id=g['grade_id'],quality_weight=score,
                                      trait=traits[no-1],min_major=major,owner_type='anchor',binding='bound',
                                      tradeable=False,source_id=src if g['grade_id']=='G1' else 'CH_CRAFT',
                                      alternative_source='CH_CRAFT' if cat=='core' else '',use=use))
                if g['grade_id']=='G1': link(iid,src,1,cycle,note='专注次数跨材料共享每日6轮；达到周期可选本池一种，不能每种各领一次')
                else:
                    prev=f'{sp}_G{int(g["grade_id"][1:])-1}'
                    recipe('R_'+iid,'淬炼'+g['name']+name,iid,[(prev,2)],stone=10*idx*int(g['grade_id'][1:]),min_major=major,kind='refine')
            if cat=='core':
                recipe(f'R_CORE_{sid}','炼制下品'+name,sp+'_G1',[('MAT_BREAK',base_cost)],stone=base_cost*3,min_major=major,kind='core_alchemy')

# Method items keep existing seven mechanics and six levels.
methods=[]
for i,m in enumerate(read('methods.csv'),1):
    mid=m['method_id']; minimum=0 if i in (1,3,4) else 1
    src='CH_METHOD_'+str(i)
    region=['青云山','星陨海','玄铁岭','星陨海','妖兽林','赤炎地','玄冰原'][i-1]
    source(src,m['name']+'传承试炼','focused_world',region,minimum,'首次解锁目标功法','FOCUS_WORLD_6',
           '专注累计2个完整轮次，确定取得所选功法原典1份；与突破材料共享每日6轮额度')
    for kind,ending,amount in [('book','原典',1),('study','研习卷',0)]:
        iid=f'BOOK_{mid}_{kind.upper()}'
        use='解锁已有功法；重复原典可转2绑定残页，不随机开新功法' if kind=='book' else '仅给对应功法增加1研习进度；沿用原六级成本'
        item(iid,m['name']+ending,'method_'+kind,min_major=minimum,use=use,stack=99)
        methods.append(dict(item_id=iid,name=m['name']+ending,method_id=mid,item_type=kind,
                            route=m['route'],min_major=minimum,max_method_level=6,study_units=1 if kind=='study' else 0,
                            main_mechanic=m['main_mechanic'],support_mechanic=m['support_mechanic'],
                            stacking='同功法主辅不重复叠加；既有规则优先',source_id=src if kind=='book' else 'CH_CRAFT'))
        if kind=='book':
            link(iid,src,1,2)
            recipe('R_DUP_'+mid,'重复原典转残页','MAT_PAGE',[(iid,1)],count=2,min_major=minimum,kind='duplicate_salvage')
        else: recipe('R_STUDY_'+mid,'制作'+m['name']+'研习卷',iid,[('MAT_PAGE',2)],stone=50,min_major=minimum)

# Equipment is for the anchor; the only weapon remains the existing innate sword.
equipment=[]
tiers=[(1,0,'青云'),(2,3,'玄罡'),(3,6,'太清'),(4,9,'无极')]
for tier,major,prefix in tiers:
    for slot,name,stat,vals in [('robe','法衣','health_add',[.03,.05,.08,.12]),('crown','道冠','defense_add',[.03,.04,.06,.09]),
                                ('boots','云履','move_speed_add',[.02,.03,.04,.05]),('pendant','灵佩','attack_add',[.02,.03,.05,.08])]:
        iid=f'EQ_{slot.upper()}_T{tier}'; amount=vals[tier-1]
        item(iid,prefix+name,'equipment',min_major=major,use=f'主播{slot}槽；{stat}+{amount:.0%}',stack=1)
        equipment.append(dict(item_id=iid,name=prefix+name,slot=slot,tier=tier,min_major=major,
                              stat=stat,additive_value=amount,max_equipped_in_slot=1,random_affixes=False,
                              enhancement_system=False,viewer_equippable=False,source_id='CH_CRAFT'))
        recipe('R_'+iid,'锻制'+prefix+name,iid,[('MAT_IRON_B',4*tier),('MAT_HERB_B',2*tier)],stone=40*tier,min_major=major)

fabao=[]
fabao_defs=[
 ('引灵葫芦',0,'heal_max_hp',.10,0,90,'恢复主播最大生命10%；满生命不消耗技能冷却'),
 ('镇山印',1,'next_strike_add',.30,8,60,'8秒内下一次本命剑命中增伤30%，最多一次'),
 ('玄罡镜',1,'shield_max_hp',.10,12,90,'10%最大生命护盾12秒；同类屏障取最高'),
 ('听风铃',2,'cue_advance_seconds',1,10,60,'后续10秒机制提示提前1秒，不自动答题'),
 ('锁云绫',3,'npc_slow',.20,6,60,'普通世界目标减速20%持续6秒，霸主免控时明确无效'),
 ('归元鼎',4,'heal_max_hp',.12,0,90,'恢复主播最大生命12%，不复活'),
 ('星河盘',4,'next_hits_add',.10,12,60,'后续3次本命剑命中增伤10%，12秒到期'),
 ('离火幡',5,'burn_attack_per_tick',.10,5,60,'每秒0.1倍本命剑基础攻击，共5次，不触发额外触发链'),
 ('定界珠',6,'hazard_damage_reduction',.20,6,90,'世界环境伤害减免20%持续6秒，受总减伤60%上限'),
 ('镇魂钟',7,'negative_duration_reduction',.10,10,90,'10秒内新施加的可清除负面时长缩短10%'),
 ('太虚尺',8,'next_hit_armor_ignore',.10,8,60,'下一次本命剑攻击破甲10%，受破甲总上限60%'),
 ('乾坤图',9,'public_route_preview',1,15,60,'显示已公开的两路线机制预览15秒，无隐藏掉率加成')]
for i,(n,major,key,amount,duration,cd,desc) in enumerate(fabao_defs,1):
    iid=f'FABAO_{i:02}'
    item(iid,n,'fabao',min_major=major,use=desc,stack=1)
    fabao.append(dict(item_id=iid,name=n,min_major=major,effect_key=key,magnitude=amount,duration_seconds=duration,
                      cooldown_seconds=cd,max_equipped=1,viewer_equippable=False,affects_tribulation_probability=False,
                      allowed_in_normalized_race=False,source_id='CH_CRAFT',description=desc))
    recipe('R_'+iid,'炼制'+n,iid,[('MAT_IRON_B',4+i),('MAT_PAGE',2+(i//3)),('MAT_HERB_B',2+i//2)],stone=60+20*i,min_major=major)

# Consumable/support tools. No item buys success probability, rank score or gift entitlements.
props=[]
prop_defs=[
 ('清心散',0,'cleanse_confusion',1,0,60,'清除1层可驱散心惑；无目标不消耗',2,0,20),
 ('温脉汤',0,'post_break_weakness_reduce_seconds',10,0,60,'缩短当前突破后虚弱10秒；无虚弱不消耗',2,0,25),
 ('凝神香',1,'meditation_visual',1,300,300,'静修香雾300秒，无修为或属性加成',1,0,10),
 ('护心符',1,'shield_max_hp',.05,10,90,'5%生命屏障10秒；与护阵符同类取最高',0,2,25),
 ('避寒珠',2,'cold_duration_reduce',.20,60,120,'60秒内新寒滞持续时间减少20%',1,1,30),
 ('清瘴丸',2,'cleanse_poison',1,0,60,'移除1层可清除瘴毒，不免疫后续伤害',3,0,30),
 ('定息丸',3,'post_break_weakness_reduce_seconds',20,0,60,'缩短当前突破后虚弱20秒，不增加渡劫成功率',3,0,40),
 ('赤阳符',3,'cold_cleanse',1,0,60,'解除一次可驱散寒滞，不驱散Boss锁定',1,2,35),
 ('灵光灯',4,'world_cue_visual',1,120,120,'强化已公开机制提示120秒，不自动响应',0,2,30),
 ('安魂露',4,'cleanse_fear',1,0,60,'清除1层可驱散魂惧',3,0,40),
 ('破障钉',5,'normal_obstacle_hit_add',.10,8,60,'下一次普通障碍命中增伤10%，不直接完成计分任务',0,3,45),
 ('御风符',5,'move_speed_add',.05,10,60,'主播移动速度+5%持续10秒，受新增装备总上限',1,2,40),
 ('镇界香',6,'rift_cue_visual',1,120,120,'强化裂隙预告120秒，不免除关卡条件',2,1,45),
 ('涤魂茶',6,'cleanse_confusion',2,0,60,'清除最多2层可驱散心惑',4,0,50),
 ('归真露',7,'heal_max_hp',.15,0,90,'恢复主播最大生命15%，与治疗道具共享冷却',4,1,60),
 ('太清符',7,'shield_max_hp',.08,10,90,'8%生命屏障10秒，同类取最高',1,4,60),
 ('问道灯',8,'break_ceremony_visual',1,120,120,'渡劫演出灯影120秒，无成功率加成',2,2,50),
 ('飞仙笺',8,'ceremony_message',1,30,60,'展示一条经审核祝语30秒，不广播私密ID',1,1,20),
 ('无极香',9,'break_ceremony_visual',1,180,180,'混元突破香雾180秒，无成功率加成',3,1,55),
 ('回影玉简',0,'local_replay_marker',1,0,30,'标记一条本机已发生突破演出记录，不存他人平台身份',0,1,10)]
for i,(n,major,key,amount,duration,cd,desc,herb,iron,stone) in enumerate(prop_defs,1):
    iid=f'PROP_{i:02}'
    item(iid,n,'prop',min_major=major,use=desc,stack=99)
    props.append(dict(item_id=iid,name=n,min_major=major,effect_key=key,magnitude=amount,duration_seconds=duration,
                      cooldown_seconds=cd,daily_craft_limit=2,shared_cooldown='HEAL_90' if key=='heal_max_hp' else 'PROP_60',
                      affects_tribulation_probability=False,source_id='CH_CRAFT',description=desc))
    inputs=[(key_,count) for key_,count in [('MAT_HERB_B',herb),('MAT_IRON_B',iron)] if count]
    recipe('R_'+iid,'制作'+n,iid,inputs,stone=stone,min_major=major,limit='PROP_ALL_6_PER_DAY_AND_ITEM_2')

build_materials=[]
for i,(n,h,iron,stone) in enumerate([('灵纹木',3,1,10),('青罡砖',1,3,10),('云纹绢',4,0,10),('阵纹砂',1,2,15)],1):
    iid=f'GUILD_MAT_{i:02}'
    item(iid,n,'guild_build_material',owner='guild',use='公共外观与功能入口建设，不兑换成员数值')
    build_materials.append(dict(item_id=iid,name=n,owner_type='guild',source_id='CH_BUILD',donation_origin='主播绑定原料确定性制作',
                                per_anchor_daily_output_cap=5,transferable_to_personal=False))
    recipe('R_'+iid,'公库制作'+n,iid,[(x,a) for x,a in [('MAT_HERB_B',h),('MAT_IRON_B',iron)] if a],stone=stone,owner='guild',kind='guild_craft',limit='GUILD_MAT_TOTAL_5_PER_ANCHOR_DAY')
    # Use a distinct channel so all parties and the transaction boundary are explicit.
    edges[-1]['source_id']='CH_BUILD'

buildings=[]
build_defs=[('武灵大殿','名称旗帜与组织总览'),('炼丹阁','打开已有炼丹配方入口，安全区同样可用'),
            ('炼器坊','打开已有炼器入口，无制作速度和品质加成'),('藏经阁','展示功法收藏与公开说明，不给额外研习进度'),
            ('演武场','使用统一属性演示构筑，不给训练经验'),('护山阵','公共阵法外显，不产生战斗护盾'),
            ('灵圃','静态药圃外观，不离线生产材料'),('万灵碑','展示允许的成员荣誉，不兑换属性')]
for idx,(n,desc) in enumerate(build_defs,1):
    for level,(cost,qty) in enumerate([(20,2),(60,6),(120,12)],1):
        buildings.append(dict(building_id=f'BUILD_{idx:02}',name=n,level=level,prerequisite_level=level-1,
                              supply_cost=cost,material_1=f'GUILD_MAT_{(idx-1)%4+1:02}',material_1_count=qty,
                              material_2=f'GUILD_MAT_{idx%4+1:02}',material_2_count=qty,
                              owner_type='guild',numeric_buff=False,benefit=desc+'；第'+str(level)+'档外观',
                              source_id='CH_BUILD',unlock='大殿0级免费已有；各设施不锁个人基础制作'))

currencies=[
 dict(asset_id='CUR_STONE',name='灵石',owner_type='anchor',is_currency=True,mint_source='世界每完整轮80；原NPC回收每日≤50',
      spend='既有小阶突破/炼制/装备/法宝/道具/原型市场',personal_transfer=False,guild_transfer=False,
      gift_minted=False,exchange_ratio='无现金抖币兑换',daily_cap='按既有各来源限额，不额外新铸币'),
 dict(asset_id='CUR_SUPPLY',name='武灵资材',owner_type='guild',is_currency=True,mint_source='正式成员完整事件每人1，每人每日6，武灵每日100',
      spend='只用于公共建设外观和入口',personal_transfer=False,guild_transfer=False,
      gift_minted=False,exchange_ratio='不能兑换灵石、个人修为或材料',daily_cap='100/武灵/自然日'),
 dict(asset_id='STAT_CONTRIB',name='个人贡献',owner_type='viewer',is_currency=False,mint_source='有效免费响应，沿原贡献上限',
      spend='只记录荣誉，不付价兑换数值',personal_transfer=False,guild_transfer=False,
      gift_minted=False,exchange_ratio='不可兑换',daily_cap='沿v1.3：120；开服贡献独立记账'),
 dict(asset_id='STAT_REPUTATION',name='武灵声望',owner_type='guild',is_currency=False,mint_source='共同事件，沿原每日60上限',
      spend='组织记录，不扣除兑换战斗权利',personal_transfer=False,guild_transfer=False,
      gift_minted=False,exchange_ratio='不可兑换',daily_cap='60/武灵/自然日')]

transactions=[
 ('TX_WORLD','world_instance/stage','世界阶段奖励','系统→主播','按world_rewards.csv','按同一原始收据只发一次'),
 ('TX_FOCUS','round/anchor/focus_slot','专注定向材料','系统→主播','完整轮每日最多6；只能推进一个选择','达到周期才发所选下品1份，余数保留'),
 ('TX_RECIPE','request_id','确定性炼制','主播投入→消耗；产物→声明所有者','按recipes/recipe_inputs','扣料与输出同事务，2合1不形成回环铸币'),
 ('TX_BREAK_SUCCESS','break_attempt_id','主播突破成功','材料与修为→消耗','核心1+所选辅材；修为沿原50%阈值','核心炼制已支付的石/钱不能再扣一遍'),
 ('TX_BREAK_FAILURE','break_attempt_id','渡劫失败','主播修为→销毁','min(1500,未花费修为)','不掉境界；材料解锁退还；原料不复制'),
 ('TX_GUILD_SUPPLY','round/user/guild','公共资材','系统→武灵','1/正式成员完整轮；6/人/日；100/武灵/日','不产生可提款个人分红'),
 ('TX_GUILD_CRAFT','request_id','公库制作','主播绑定投入→消耗；公库产物→入账','四类公库材料总共5份/主播/日','产物直接公库；退出不退回个人'),
 ('TX_BUILD','guild/building/target_level','设施建设','武灵资材与公库材料→消耗','20/60/120与两类材料2/6/12','相同目标级并发只成功一次'),
 ('TX_MARKET','order_id/version','原型寄售','买家→卖家及销毁','只允许原MAT_HERB_T/MAT_IRON_T；沿原税5%','新增所有材料/书/装备/法宝禁售'),
 ('TX_EXPIRE','instance_id/expiry','到期处理','资产→销毁','最早输入到期传递','退款与退货不延长到期；旧礼物不进入新清理器')]
tx=[dict(transaction_type=t,idempotency_key=k,name=n,direction=d,amount_rule=a,rule=r) for t,k,n,d,a,r in transactions]

config={
 'schema_version':1,'design_version':'wuling-items-v1.4','status':'design_only',
 'canonical_tables_directory':'tables/items-v1.4','default_profile':'prototype_full',
 'profiles':{'prototype_full':{'item_extension':True,'real_payment':False,'player_trade_new_items':False,'tribulation':True},
             'douyin_release_candidate':{'item_extension':False,'real_payment':False,'player_trade_new_items':False,'tribulation':False}},
 'ownership':{'breakthrough_materials':'anchor','method_items':'anchor','equipment':'anchor','fabao':'anchor','props':'anchor','build_materials':'guild','viewer_break_materials':False},
 'breakthrough':{'max_category_slots':4,'core_count':1,'all_guard_optional':True,
    'quality_core':'core_item.grade.core_score',
    'quality_guard':'floor(sum(grade.guard_weight * quantity) / stage.guard_max_count)',
    'quality_essence':'floor(sum(grade.essence_weight * quantity) / stage.essence_max_count)',
    'quality_treasure':'sum(grade.treasure_score * quantity)',
    'quality_total':'core + guard + essence + treasure','q_cap':100,'random_quality':False,
    'quality_mode':'bounded_additive_major_snapshots','quality_major_budget_divisor':10,
    'quality_total_bonus_cap':0.10,'quality_stat_rule':'1+min(0.10,sum((band.live_snapshot_factor-1)/10 for highest snapshot of each completed major))',
    'raw_indie_multiplier_enabled':False,'cumulative_quality_multiply':False,
    'current_major_quality_rework':True,'rework_probability_roll':False,'rework_requires_core':False,
    'rework_upgrade_core_consumes_one':True,
    'rework_only_higher_q':True,'rework_replaces_full_optional_set':True,
    'highest_quality_state_reset_on_new_major':False,'old_gift_migration':'legacy_preserve',
    'major_cost_mode':'core_consumed_once_no_extra_old_major_mat_or_stone',
    'minor_cost_mode':'legacy_v1.3_unchanged',
    'readiness':{'full_qi_required':True,'min_health_ratio':0.5,'safe_seconds_since_combat':15,
                 'weakness_seconds_at_no_guard':60,'weakness_defense_penalty':0.05,
                 'weakness_duration_rule':'60-floor(60*guard_score/15)','affects_probability':False}},
 'focus_sources':{'full_round_seconds':600,'eligible_rounds_per_anchor_day':6,
     'one_focus_per_round':True,'core_cycle':1,'guard_cycle':1,'essence_cycle':2,'treasure_cycle':6,
     'selected_species_not_random':True,'counter_scope':'app/anchor/realm_lane/profile/window',
     'progress_shared_with_method_focus':True,'optional_additive_budget':True,'no_race_bonus_double_grant':True},
 'refining':{'input_count':2,'output_count':1,'max_grade':4,'success_rate':1,'no_reverse_refund':True},
 'equipment':{'anchor_slots':['robe','crown','boots','pendant'],'additional_weapon_slot':False,'viewer_equipment':False,
              'change_only_safe_preparation':True,'freeze_loadout_on_event_start':True},
 'fabao':{'anchor_max_equipped':1,'viewer_equipment':False,'affects_tribulation_probability':False,
          'cooldown_survives_unequip':True,'cooldown_scope':'anchor/effect_key/profile/window'},
 'new_property_caps':{'attack_add':0.20,'defense_add':0.25,'health_add':0.25,'move_speed_add':0.10,
                      'excludes_legacy_paid_entitlements':True,'normalized_race_uses_equipment':False},
 'guild':{'supply_member_day_cap':6,'supply_guild_day_cap':100,'personal_numeric_exchange':False,
          'currency_donation':False,'public_material_craft_daily_cap':5,'build_level_supply':[20,60,120],
          'replaces_v1_1_flat_supply_cost_100':True,'numeric_build_buffs':False},
 'props':{'all_new_props_daily_craft_cap':6,'per_item_daily_craft_cap':2,'old_prop_caps_unchanged':True},
 'trade':{'new_item_whitelist':[],'legacy_whitelist':['MAT_HERB_T','MAT_IRON_T'],'guild_personal_transfer':False},
 'expiry':{'window_days_prototype':30,'derived_item_earliest_input':True,'gifts_preserved':True}}

tribulation={
 'version':'TRIBULATION_V1','status':'design_only','default_enabled_profile':'prototype_full',
 'first_transition':'BR04','start_to_realm_index':22,'base_success_basis_points':5000,
 'helper_command':'助力','helper_bonus_basis_points':100,'max_probability_helpers':50,
 'unique_helper_key':'app/attempt_id/platform_user_id','helper_can_be_formal_or_temporary':True,
 'anchor_can_help_self':False,'gift_affects_probability':False,'material_affects_probability':False,
 'pity_bonus_per_failure_basis_points':1000,'pity_max_basis_points':5000,
 'max_success_basis_points':10000,'max_attempts_without_help':6,
 'failure_qi_loss':1500,'loss_rule':'min(1500,unspent_qi)','lose_realm':False,
 'consume_materials_on_failure':False,'consume_breakthrough_qi_on_failure':False,
 'additional_failure_stone_cost':0,'qi_cost_on_success':'legacy_next_step_delta_times_anchor_ratio',
 'reservation_phase_seconds':15,'help_trial_seconds':60,'trial_stages_seconds':[20,20,20],
 'reveal_seconds':8,'material_min_remaining_seconds':83,
 'online_base_qi_during_trial':True,'trial_grants_world_rewards':False,
 'retry_min_interval_seconds':60,'retry_requires_full_qi_again':True,
 'failure_counter_key':'app/anchor/transition/profile/window','disconnect_resets_pity':False,
 'pity_reset_on_success':True,'pity_affects_other_transitions':False,
 'random_draw':'server_uniform_integer_0_to_9999_once_per_attempt',
 'success_rule':'roll < min(10000,5000+100*unique_helpers+1000*previous_failures)',
 'commit_atomic':True,'client_cannot_submit_roll_or_helpers':True,
 'sword_help_visual':'reuse_alert_lease_scoped_to_attempt',
 'restore_policy':'release_lease_then_recompute_current_gameplay_state',
 'preserve_gift_receipts_and_expiry':True,'trial_stages_have_no_extra_hidden_success_rolls':True,
 'roll_before_or_after_quality':'quality_deterministic_unchanged_by_roll',
 'release_candidate_enabled':False}

ui_tasks=[
 ('BreakthroughPrepare','突破准备','四类材料槽/数量/替换/评分来源/确定品质/材料获取跳转','InkUiBreakthroughPanels','UInkBreakthroughPreparePanel'),
 ('Tribulation','渡劫现场','三重天劫/唯一助力数/50%基础+助力+保底/倒计时/结果','InkUiBreakthroughPanels','UInkTribulationPanel'),
 ('ItemSources','物品来源','材料分型与品质/具体区域副本/专注进度/配方与共享额度','InkUiItemSourcePanel','UInkItemSourcePanel'),
 ('GuildConstruction','武灵建设','公库余额/材料/级别/成本/原子提交/外显预览','InkUiGuildBuildPanel','UInkGuildBuildPanel'),
 ('EquipmentFabao','装备法宝','主播四个非武器槽/一个法宝槽/属性上限/归属','InkUiEquipmentPanel','UInkEquipmentPanel')]
ui=[dict(route=r,title=t,data=d,source_header=f'Source/BoundlessImmortality/Frontend/UI/{f}.h',
         source_cpp=f'Source/BoundlessImmortality/Frontend/UI/{f}.cpp',native_class=c,
         widget_asset=f'Content/UI/Ink/Wuling/Widgets/WBP_Ink{r}.uasset',status='planned_only') for r,t,d,f,c in ui_tasks]

runtime_cases=[
 ('材料分类','每阶段核心1/其余各5种，四个品质均可通过确定渠道获得'),
 ('品质边界','Q39/40/59/60/74/75/89/90/97/98/100映射正确，核心10/18/26/35，不显示32'),
 ('混放计算','同类别不同材料和品质按数量求和后只floor一次，绝不逐件floor'),
 ('核心单独','合格核心可启动，没有奇珍也能成功突破；护道非硬门槛'),
 ('前置与亏损','修为未满/血量不足/战斗中不能启动；尚未入事务不扣料'),
 ('成本不重复','核心炼制消耗旧石钱后，大境界成功不再扣同一组石钱'),
 ('越阶材料','拒绝其他stage材料、超过槽位和超过数量上限、到期或托管材料'),
 ('重炼当前品质','只替换该境界最高Q，不乘旧倍率；默认保留原核心评分，主动升级核心则只消耗新核心1份，不重新抽渡劫'),
 ('归属','主播材料不向观众发；观众自动突破材料成本为零'),
 ('材料来源','一轮只推进一个专注，改名换图和跨房不多领6轮额度'),
 ('淬炼','2同种同品质合1下一品，材料来源和最早到期传递，最高品无上一级'),
 ('装备','唯一武器保留，只可穿4非武器槽；法宝同时1件'),
 ('属性上限','新装备道具加法上限有效，旧礼物不被截断，统一竞采不使用新增属性'),
 ('公库','个人不能提款，制作投入和公库输出同事务，退出不返还已确认投入'),
 ('建设并发','相同level两个请求只扣一次；不足钱料原状返回'),
 ('寄售','全部新绑定物和公库资材拒售，原草铁白名单保持'),
 ('渡劫概率','0/1/49/50个不同助力分别50/51/99/100%，重复评论不叠加'),
 ('助力身份','主播自己/其他房间/失效身份不计，正式与临时真实剑灵可计'),
 ('助力时限','以服务端截止计数，截止后与结果揭示期不再加概率'),
 ('渡劫保底','0助力前5次失败后第6次100%；重连改名不能清零'),
 ('失败扣修','只扣min(1500,可用修为)，不扣整笔成功成本，不掉境界或扣成负数'),
 ('失败材料','解锁原预留材料，不铸造退款副本，不刷新到期时间'),
 ('重试','修为重新达到阈值且过60秒后才可再次开始；单主播仅1个活跃渡劫'),
 ('概率权威','一次attempt只一份服务端roll，客户端不提交百分比和随机值'),
 ('断线结算','截止与事务不中断；回房按attempt查询结果，不能重新抽取'),
 ('剑灵生命周期','助力预警以attempt租约控制；成功失败取消房间切换断线都释放并重新评估状态'),
 ('预警冲突','已有攻击/预警/礼物演出不被粗暴复位；已有礼物时长正常按UTC流逝'),
 ('渡劫质量分离','材料决定确定品质；礼物法宝道具不会购买成功率'),
 ('平台开关','发布候选的物品扩展/渡劫/新交易入口与服务直连保持关闭'),
 ('来源保护','旧礼物权益/旧背包/旧境界数据未自动迁移；源CSV不会覆盖运行配置')]
accept=[dict(case_id=f'ITEM14-{i:03}',area=a,expected=e,current_result='not_run') for i,(a,e) in enumerate(runtime_cases,1)]

for name,data in [('item_master.csv',master),('breakthrough_species.csv',species),('breakthrough_material_items.csv',materials),
                  ('breakthrough_stages.csv',stages),('quality_grades.csv',grades),('quality_bands.csv',bands),
                  ('method_items.csv',methods),('equipment_items.csv',equipment),('fabao_items.csv',fabao),('prop_items.csv',props),
                  ('guild_build_materials.csv',build_materials),('guild_buildings.csv',buildings),('guild_currency.csv',currencies),
                  ('transaction_types.csv',tx),('acquisition_sources.csv',sources),('item_sources.csv',edges),
                  ('recipes.csv',recipes),('recipe_inputs.csv',recipe_inputs),('ui-extension.csv',ui),('acceptance.csv',accept)]:save(name,data)
js('item_system.json',config);js('tribulation.json',tribulation)
counts={'material_species':len(species),'material_item_variants':len(materials),'method_items':len(methods),
        'equipment_items':len(equipment),'fabao_items':len(fabao),'new_prop_items':len(props),'guild_materials':len(build_materials),
        'building_level_rows':len(buildings),'master_items_including_legacy':len(master),'acquisition_sources':len(sources),
        'source_edges':len(edges),'recipes':len(recipes),'recipe_input_rows':len(recipe_inputs),'pending_runtime_cases':len(accept)}
js('counts.json',counts)
print(json.dumps(counts,ensure_ascii=False,indent=2))
