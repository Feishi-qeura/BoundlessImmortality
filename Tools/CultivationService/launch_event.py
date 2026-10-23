from .contracts import require, uid
from .guilds import Guilds
from .growth import Growth
from .inventory import Inventory


class LaunchEvent:
    @staticmethod
    def honors(ctx):
        enrollment=ctx.entity('enrollment',ctx.user,{})
        title=enrollment.get('title')
        owned=[title] if title and title['expiry']>ctx.now else []
        selected=(ctx.entity('honor_display',ctx.user) or {}).get('title_id','')
        return dict(owned=owned,selected=selected if any(t['title_id']==selected for t in owned) else '')

    @staticmethod
    def select_honor(ctx,args):
        title=args.get('title_id','')
        require(not title or any(t['title_id']==title for t in LaunchEvent.honors(ctx)['owned']),'TITLE_NOT_OWNED')
        ctx.put('honor_display',ctx.user,dict(title_id=title))

    @staticmethod
    def on_action(ctx,round_,action,stage):
        if ctx.role!='viewer':return
        guild=Guilds.current(ctx)
        if not guild:return
        if round_['anchor']!=guild['anchor']:return
        event=ctx.entity('launch',guild['anchor'])
        if not event or event['status']!='Active' or not event['start']<=round_['start'] or round_['start']+600>event['end'] or ctx.now>=event['end']:return
        enrollment=ctx.entity('enrollment',ctx.user)
        if enrollment and enrollment['home_guild']!=guild['id']:return
        enrollment=enrollment or dict(user=ctx.user,home_guild=guild['id'],enrolled_at=ctx.now,rounds={},tasks=[],score=0,complete_count=0,title=None)
        record=enrollment['rounds'].setdefault(round_['id'],dict(start=round_['start'],free=False,guard=0,break_=0,route=False,stages=[],observed_stages=[],completed=False))
        observed=record.setdefault('observed_stages',[])
        if stage not in observed:observed.append(stage)
        if action!='strike':record['free']=True
        if action=='route':record['route']=True
        if action=='护阵' and stage==1:record['guard']=min(2,record['guard']+1)
        if action=='破障' and stage==2:record['break_']=min(2,record['break_']+1)
        ctx.put('enrollment',ctx.user,enrollment)
        LaunchEvent.reward(ctx,event,enrollment)

    @staticmethod
    def on_complete(ctx,round_):
        guild=Guilds.current(ctx,round_['anchor'])
        if not guild:return
        event=ctx.entity('launch',guild['anchor'])
        if not event or not event['start']<=round_['start'] or round_['start']+600>event['end'] or ctx.now>event['end']+60:return
        full=len(round_['completed'])==3
        eligible=False
        for user,participant in round_['participants'].items():
            enrollment=ctx.entity('enrollment',user)
            if not enrollment or enrollment['home_guild']!=guild['id'] or round_['id'] not in enrollment['rounds']:continue
            record=enrollment['rounds'][round_['id']]
            record['stages']=[s for s in record.get('observed_stages',[]) if s in participant['stages'] and s in round_['completed']]
            record['completed']=full and len(record['stages'])==3
            eligible|=record['completed'] and record['free']
            LaunchEvent.reward(ctx.for_user(user),event,enrollment)
        anchor_participant=round_['participants'].get(round_['anchor'],{})
        anchor_full=len(anchor_participant.get('stages',[]))==3 and anchor_participant.get('free_response',False)
        if full and anchor_full and eligible and round_['id'] not in event['complete_rounds']:
            event['complete_rounds'].append(round_['id'])
            for row in ctx.rules.tables['launch_guild_milestones.csv']:
                if len(event['complete_rounds'])>=row['complete_rounds'] and row['tier_id'] not in event['milestones']:
                    if event['permissions']['guild_numeric_rewards']:
                        ctx.money(round_['anchor'],'CUR_STONE',row['anchor_stone'],'launch',event['id']+':'+row['tier_id'])
                        Inventory.grant(ctx,'MAT_BREAK',row['anchor_break_material'],dict(id=event['id']+':'+row['tier_id']),round_['anchor'])
                    event['milestones'].append(row['tier_id'])
                    if row['grant_final_buff'] and event['permissions']['guild_numeric_buff']:
                        event['buff_start']=ctx.now;event['buff_end']=min(ctx.now+1200,ctx.window_end)
        ctx.put('launch',guild['anchor'],event)

    @staticmethod
    def reward(ctx,event,enrollment):
        rounds=list(enrollment['rounds'].values())
        complete=sum(r['completed'] for r in rounds)
        conditions={'join_and_free_response':sum(r['free'] for r in rounds),'complete_world_rounds':complete,
                    'correct_guard_response':sum(r['guard'] for r in rounds),'correct_break_response':sum(r['break_'] for r in rounds),
                    'route_vote_in_distinct_rounds':sum(r['route'] for r in rounds)}
        for row in ctx.rules.tables['launch_tasks.csv']:
            if row['task_id'] not in enrollment['tasks'] and conditions[row['condition_type']]>=row['target']:
                if event['permissions']['task_numeric_rewards']:Growth.add(ctx,ctx.user,row['reward_count'],dict(id=event['id']+':'+row['task_id'],kind='launch'))
                enrollment['tasks'].append(row['task_id'])
        buckets={}
        for id_,r in sorted(enrollment['rounds'].items(),key=lambda p:(p[1]['start'],p[0])):
            day=int((r['start']-event['start'])//86400)
            if day not in range(7):continue
            selected=buckets.setdefault(day,[])
            if len(selected)>=6:continue
            score=sum({1:2,2:3,3:5}[s] for s in r['stages'])+min(2,r['guard']+r['break_'])*3+int(r['route'])*2+int(r['free'])*2
            selected.append(score)
        enrollment['score']=sum(sum(scores) for scores in buckets.values());enrollment['complete_count']=complete
        if len(enrollment['tasks'])==7 and not enrollment['title'] and event['permissions']['title_reward']:
            enrollment['title']=dict(name='青云踏飞雁',title_id='QingyunFeiyan',expiry=min(ctx.now+2592000,ctx.window_end))
        ctx.put('enrollment',ctx.user,enrollment)

    @staticmethod
    def advance(ctx):
        for event in ctx.entities('launch'):
            if ctx.now>=event['end']+60:event['status']='Settled'
            elif ctx.now>=event['end']:event['status']='Closing'
            ctx.put('launch',event['anchor'],event)

    @staticmethod
    def bonus(ctx):
        guild=Guilds.current(ctx)
        if not guild:return 0
        event=ctx.entity('launch',guild['anchor'])
        if not event or event.get('buff_end',0)<=ctx.now:return 0
        host=ctx.db.execute('SELECT 1 FROM session WHERE scope=? AND user=? AND room=? AND active=1 AND revoked=0 AND heartbeat>=?',(ctx.scope,guild['anchor'],ctx.room,ctx.now-5)).fetchone()
        return .1 if host else 0

    @staticmethod
    def start(ctx,args):
        ctx.anchor_only()
        permissions=dict(ctx.rules.configs['launch_event.json']['profiles']['prototype_full'])
        require(permissions['activity_enabled'],'FEATURE_DISABLED')
        guild=Guilds.current(ctx)
        require(guild is not None,'GUILD_NOT_FOUND')
        require(ctx.entity('launch',ctx.user) is None,'ALREADY_SETTLED')
        ctx.put('launch',ctx.user,dict(id=uid(),anchor=ctx.user,guild_id=guild['id'],start=ctx.now,end=min(ctx.now+604800,ctx.window_end),
                                     rules_hash=ctx.rules.sha256,permissions=permissions,rank_numeric_rewards=False,status='Active',complete_rounds=[],milestones=[]))

    @staticmethod
    def snapshot(ctx):
        guild=Guilds.current(ctx)
        if not guild:return None
        event=ctx.entity('launch',guild['anchor'])
        if not event:return None
        enrolled=ctx.entity('enrollment',ctx.user)
        progress={}
        if enrolled:
            rounds=list(enrolled['rounds'].values())
            progress=dict(join_and_free_response=sum(r['free'] for r in rounds),complete_world_rounds=enrolled['complete_count'],
                          correct_guard_response=sum(r['guard'] for r in rounds),correct_break_response=sum(r['break_'] for r in rounds),route_vote_in_distinct_rounds=sum(r['route'] for r in rounds))
        tasks=[dict(r,progress=progress.get(r['condition_type'],0),confirmed=bool(enrolled and r['task_id'] in enrolled['tasks'])) for r in ctx.rules.tables['launch_tasks.csv']]
        return dict(event,personal=enrolled,leaderboard=LaunchEvent.ranks(ctx,event),tasks=tasks,guild_milestones=ctx.rules.tables['launch_guild_milestones.csv'])

    @staticmethod
    def ranks(ctx,event):
        enrolled=[r for r in ctx.entities('enrollment') if r['home_guild']==event['guild_id'] and r.get('score',0)>=30 and r.get('complete_count',0)>=2]
        enrolled.sort(key=lambda r:(-r['score'],r['user']))
        result=[];rank=0;last=None
        for i,row in enumerate(enrolled):
            if row['score']!=last: rank=i+1;last=row['score']
            if rank>30:break
            name=ctx.db.execute('SELECT name FROM identity WHERE scope=? AND user=?',(ctx.scope,row['user'])).fetchone()
            result.append(dict(user=row['user'],name=name['name'] if name else '剑灵',score=row['score'],rank=rank))
        return result
