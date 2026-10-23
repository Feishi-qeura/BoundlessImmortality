from statistics import median
from .contracts import require, uid
from .guilds import Guilds
from .leases import Leases
from .world import World
from .growth import Growth


class Races:
    @staticmethod
    def participants(ctx,guild,room):
        return [r['user'] for r in ctx.db.execute('SELECT m.user FROM membership m JOIN session s USING(scope,user) WHERE m.scope=? AND m.guild_id=? AND s.room=? AND s.revoked=0 AND s.active=1 AND s.heartbeat>=? ORDER BY m.user',(ctx.scope,guild,room,ctx.now-5))]

    @staticmethod
    def queue(ctx,args):
        ctx.anchor_only()
        guild=Guilds.current(ctx)
        require(guild is not None,'GUILD_NOT_FOUND')
        require(not ctx.entity('activity',ctx.user),'ACTIVITY_BUSY')
        require(not any(l['guild_id']==guild['id'] and l['status']=='Active' and l['end']>ctx.now for l in ctx.entities('lease')),'LEASE_ACTIVE')
        current=Races.participants(ctx,guild['id'],ctx.room)
        require(current,'FORMAL_MEMBER_REQUIRED')
        require(args.get('node_id') in {r['node_id'] for r in ctx.rules.tables['nodes.csv']},'UNKNOWN_NODE')
        old=ctx.entity('queue',ctx.user)
        require(not old or old['status']!='Queued','ALREADY_QUEUED')
        rounds=[r for r in ctx.entities('world') if r['anchor']==ctx.user and r['status']=='Completed']
        rounds.sort(key=lambda r:r['start'])
        size=median([len(r['participants'])-1 for r in rounds[-3:]]) if len(rounds)>=3 else len(current)
        band=0 if size<10 else (1 if size<50 else 2)
        queue=dict(id=uid(),anchor=ctx.user,guild_id=guild['id'],room=ctx.room,start=ctx.now,node_id=args['node_id'],band=band,status='Queued',new_team=len(rounds)<3)
        ctx.put('queue',ctx.user,queue)
        ctx.put('activity',ctx.user,dict(kind='queue',id=queue['id']))
        Races.advance(ctx)

    @staticmethod
    def cancel(ctx,args):
        ctx.anchor_only()
        queue=ctx.entity('queue',ctx.user)
        require(queue and queue['status']=='Queued','QUEUE_CLOSED')
        queue['status']='Cancelled';ctx.put('queue',ctx.user,queue);ctx.put('activity',ctx.user,None)

    @staticmethod
    def advance(ctx):
        queued=[r for r in ctx.entities('queue') if r['status']=='Queued']
        groups={}
        for q in sorted(queued,key=lambda q:(q['start'],q['anchor'])):groups.setdefault((q['node_id'],q['band']),[]).append(q)
        for candidates in groups.values():
            if len(candidates)<2:continue
            teams=[];id_=uid()
            for q in candidates[:4]:
                team=dict(anchor=q['anchor'],guild_id=q['guild_id'],room=q['room'],representatives=[],common=[],anchor_actions=[],responses={},score=0)
                teams.append(team);q['status']='Matched';q['race_id']=id_;ctx.put('queue',q['anchor'],q)
                ctx.put('activity',q['anchor'],dict(kind='race',id=id_));ctx.put('last_race',q['anchor'],dict(id=id_))
            ctx.put('race',id_,dict(id=id_,node_id=candidates[0]['node_id'],start=ctx.now,status='Ready',teams=teams,rules_hash=ctx.rules.sha256))
        for race in ctx.entities('race'):
            if race['status']=='Settled':continue
            elapsed=ctx.now-race['start']
            for team in race['teams']:
                actor=ctx.db.execute('SELECT active,heartbeat FROM session WHERE scope=? AND user=? AND room=? AND revoked=0',(ctx.scope,team['anchor'],team['room'])).fetchone()
                if actor and actor['active'] and actor['heartbeat']>=ctx.now-5:
                    team['last_online']=ctx.now
                else:
                    last=team.get('last_online',race['start'])
                    if actor:last=max(last,actor['heartbeat'])
                    if ctx.now-last>=65:team['rank_forfeited']=True
            if elapsed>=120 and race['status']=='Ready':
                for team in race['teams']:
                    people=Races.participants(ctx,team['guild_id'],team['room'])
                    people.sort(key=lambda user:(ctx.count(user,'race-selected:'+ctx.day),user))
                    team['representatives']=people[:10]
                    team['eligible']=bool(team['representatives'])
                    for user in team['representatives']:ctx.count(user,'race-selected:'+ctx.day,1)
                race['status']='Running'
            if elapsed>=480:race['status']='Frozen'
            if elapsed>=600:
                Races.settle(ctx,race)
            ctx.put('race',race['id'],race)

    @staticmethod
    def submit(ctx,args):
        race=ctx.entity('race',args.get('race_id'))
        require(race and race['status']=='Running' and race['start']+120<=ctx.now<race['start']+480,'RACE_CLOSED')
        team=next((t for t in race['teams'] if t['room']==ctx.room and (t['anchor']==ctx.user or ctx.user in t['representatives'])),None)
        require(team and team.get('eligible'),'FORBIDDEN')
        require(ctx.session['active'] and ctx.session['heartbeat']>=ctx.now-5,'PARTICIPATION_REQUIRED')
        elapsed=ctx.now-race['start']-120
        kind=args.get('kind')
        if kind=='common':
            slot=min(5,int(elapsed//60))
            require(args.get('action')==('护阵' if slot%2==0 else '破障'),'WRONG_RESPONSE')
            if slot not in team['common']:team['common'].append(slot)
        elif kind=='anchor':
            require(ctx.user==team['anchor'],'FORBIDDEN')
            slot=next((i for i,t in enumerate((90,270)) if abs(elapsed-t)<=10),None)
            require(slot is not None,'TIMING_CLOSED')
            if slot not in team['anchor_actions']:team['anchor_actions'].append(slot)
        elif kind=='representative':
            require(ctx.user in team['representatives'],'FORBIDDEN')
            slot=next((i for i,t in enumerate((60,240)) if t<=elapsed<t+20),None)
            require(slot is not None,'TIMING_CLOSED')
            slots=team['responses'].setdefault(ctx.user,[])
            if slot not in slots:slots.append(slot)
        else:require(False,'INVALID_ACTION')
        stage=min(3,int(elapsed//120)+1)
        participant=team.setdefault('participants',{}).setdefault(ctx.user,dict(stages=[],role=ctx.role,guild_id=(Guilds.current(ctx) or {}).get('id'),free_response=True))
        if stage not in participant['stages']:participant['stages'].append(stage)
        if ctx.role=='viewer' and ctx.count(ctx.user,'race-response:'+race['id'])<2:
            ctx.count(ctx.user,'race-response:'+race['id'],1)
            World.contribution(ctx,ctx.user,5,dict(id='race-'+race['id'],start=race['start']))
        team['score']=len(team['common'])*100+len(team['anchor_actions'])*100+(200*sum(len(v) for v in team['responses'].values())//(2*len(team['representatives'])))
        ctx.fact('race-action:'+ctx.reservation_key,'race_action',dict(race_id=race['id'],kind=kind,stage=stage))
        ctx.put('race',race['id'],race)

    @staticmethod
    def settle(ctx,race):
        if not ctx.fact('race-settled:'+race['id'],'race_settled',dict(race_id=race['id'])):return
        scores=sorted({t['score'] for t in race['teams'] if t.get('eligible') and not t.get('rank_forfeited') and t['score']>=600},reverse=True)
        for team in race['teams']:
            owner=ctx.for_user(team['anchor'])
            if team['score'] in scores and team.get('eligible') and not team.get('rank_forfeited'):
                group=scores.index(team['score']);duration=(1800,1200,600)[min(2,group)]
                Leases.activate(owner,race['id'],race['node_id'],duration,race['start']+600)
            round_=dict(id='race-'+race['id']+':'+team['anchor'],anchor=team['anchor'],start=race['start'],participants=team.get('participants',{}),completed=[],reward_kind='race')
            for stage in (1,2,3):
                if 2*(stage-1) in team['common'] and 2*(stage-1)+1 in team['common']:
                    round_['completed'].append(stage);World.stage_reward(owner,round_,stage)
            World.finish(owner,round_,include_focus=False,launch=False)
            ctx.put('activity',team['anchor'],None)
        race['status']='Settled'

    @staticmethod
    def snapshot(ctx):
        last=ctx.entity('last_race',ctx.user)
        return ctx.entity('race',last['id']) if last else None
