"""Public mechanism/visual data. Never responds or modifies probabilities."""
from .effects import Effects


class Presentation:
    @staticmethod
    def snapshot(ctx,round_):
        names=dict(meditation_visual='静修香雾',world_cue_visual='灵光机制提示',rift_cue_visual='裂隙预告',break_ceremony_visual='突破灯影',
                   ceremony_message='愿剑心澄明，仙途长青',public_route_preview='公开路线预览',cue_advance_seconds='听风预警',move_speed_add='御风',
                   shield_max_hp='生命屏障',next_strike_add='镇山印',next_hits_add='星河盘',npc_slow='锁云绫',burn_attack_per_tick='离火幡',
                   hazard_damage_reduction='定界珠',negative_duration_reduction='镇魂钟',next_hit_armor_ignore='太虚尺')
        state=Effects.combat(ctx)
        active=[dict(key=key,name=names.get(key,key),end=value['end']) for key,value in state.get('effects',{}).items() if value.get('end',0)>ctx.now]
        result=dict(effects=active,weakness_remaining=max(0,state.get('weakness_until',0)-ctx.now),message='',route_preview=[],next_cue='')
        if any(r['key']=='ceremony_message' for r in active):result['message']='愿剑心澄明，仙途长青'
        if any(r['key']=='public_route_preview' for r in active):result['route_preview']=['护阵路线：回应护阵，守护阵眼','采集路线：靠近资源点，完成破障与共修']
        if round_ and round_['status'] in ('Preparing','Running'):
            lead=5+(1 if any(r['key']=='cue_advance_seconds' for r in active) else 0)
            elapsed=ctx.now-round_['start']
            for boundary,caption in zip(round_['boundaries'],('护阵','破障','共修','结算','结束')):
                if 0<boundary-elapsed<=lead:result['next_cue']=f'{caption} · {int(boundary-elapsed+0.999)}秒后';break
        marker=state.get('replay_marker')
        if marker:
            attempt=ctx.entity('attempt',marker['id'])
            if attempt:result['local_replay']=dict(status=attempt['status'],settled_at=attempt.get('settled_at'),q=attempt['quote']['q'])
        return result
