from .contracts import require


class Activities:
    @staticmethod
    def start(ctx,args):
        ctx.anchor_only();require(not ctx.entity('activity',ctx.user),'ACTIVITY_BUSY')
        activity=args.get('activity_id')
        if activity=='ACT_SHARED':
            ctx.put('activity',ctx.user,dict(kind='meditation',start=ctx.now))
        elif activity in ('WEEK_GUARD','WEEK_IRON'):
            from .world import World
            World.start(ctx,args.get('preset','ENC_SOLO'),args.get('route','guard'))
            current=ctx.entity('activity',ctx.user);round_=ctx.entity('world',current['id'])
            round_['theme']=activity;round_['boundaries']=[60,220,380,540,600];ctx.put('world',round_['id'],round_)
        else:require(False,'FEATURE_DISABLED')

    @staticmethod
    def end(ctx,args):
        ctx.anchor_only();activity=ctx.entity('activity',ctx.user)
        require(activity and activity['kind']=='meditation','ACTIVITY_BUSY')
        ctx.put('activity',ctx.user,None)

    @staticmethod
    def sign_in(ctx,args):
        key='sign:'+ctx.day
        require(ctx.count(ctx.user,key)==0,'ALREADY_SETTLED')
        ctx.count(ctx.user,key,1)
        ctx.count(ctx.user,'sign-month:'+ctx.day[:7],1)
        # Seven daily rewards are explicit placeholders; sign-in records no economy grant.

    @staticmethod
    def claim_chest(ctx,args):
        threshold=args.get('milestone')
        require(type(threshold) is int and threshold in (3,7,14,21),'INVALID_ARGUMENT')
        key=ctx.user+':month:'+ctx.day[:7]+':'+str(threshold)
        # Existing auto-granted entitlements are already claimed. Never grant twice.
        require(not ctx.entity('cosmetic',key),'ALREADY_SETTLED')
        require(ctx.count(ctx.user,'sign-month:'+ctx.day[:7])>=threshold,'MILESTONE_LOCKED')
        ctx.put('cosmetic',key,dict(owner=ctx.user,month=ctx.day[:7],milestone=threshold,expiry=ctx.window_end))

    @staticmethod
    def snapshot(ctx):
        verses=('心静如水，剑意自明','护阵同心，破障同行','修为有序，问道不息','云起青山，剑随心动')
        from datetime import datetime,timedelta
        index=datetime.strptime(ctx.day,'%Y-%m-%d').toordinal()%len(verses)
        today=datetime.strptime(ctx.day,'%Y-%m-%d').date()
        monday=today-timedelta(days=today.weekday())
        weekly=[]
        placeholders=('灵石（占位）','灵草（占位）','玄铁（占位）','丹药（占位）','残卷（占位）','玉简（占位）','锦囊（占位）')
        for i in range(7):
            date=(monday+timedelta(days=i)).isoformat()
            claimed=bool(ctx.count(ctx.user,'sign:'+date))
            weekly.append(dict(day=date,milestone=i+1,claimed=claimed,eligible=date==ctx.day and not claimed,reward=placeholders[i],reward_id='PREVIEW_SIGN_DAY_'+str(i+1),placeholder=True))
        count=ctx.count(ctx.user,'sign-month:'+ctx.day[:7])
        monthly=[]
        for threshold in (3,7,14,21):
            claimed=bool(ctx.entity('cosmetic',ctx.user+':month:'+ctx.day[:7]+':'+str(threshold)))
            monthly.append(dict(milestone=threshold,claimed=claimed,eligible=count>=threshold and not claimed,reward=str(threshold)+'日签到纪念外观'))
        return dict(sign_in_reward_mode='placeholder',sign_in_qi_reward=0,day=ctx.day,signed=bool(ctx.count(ctx.user,'sign:'+ctx.day)),month_sign_count=ctx.count(ctx.user,'sign-month:'+ctx.day[:7]),
                    signed_days=[r['key'][5:] for r in ctx.db.execute("SELECT key FROM counter WHERE scope=? AND owner=? AND substr(key,1,5)='sign:' AND value>0 ORDER BY key",(ctx.scope,ctx.user))],
                    monthly_milestones=[3,7,14,21],weekly_chests=weekly,monthly_chests=monthly,verse=verses[index],templates=[dict(row,reward_budget='seven_daily_placeholders_no_grant') if row['activity_id']=='ACT_SIGN' else dict(row) for row in ctx.rules.tables['activities.csv']])
