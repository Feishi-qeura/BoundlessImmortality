"""Authority-observed presence spans, used to integrate time-limited bonuses."""
from .guilds import Guilds


class OnlineIntervals:
    @staticmethod
    def record(ctx,active):
        history=ctx.entity('presence_history',ctx.user,dict(spans=[]))
        spans=history['spans']
        if active:
            if spans and spans[-1]['room']==ctx.room and spans[-1]['end']>=ctx.now:
                spans[-1]['end']=max(spans[-1]['end'],ctx.now+5)
            else:spans.append(dict(room=ctx.room,start=ctx.now,end=ctx.now+5))
        elif spans and spans[-1]['room']==ctx.room:spans[-1]['end']=min(spans[-1]['end'],ctx.now)
        ctx.put('presence_history',ctx.user,history)

    @staticmethod
    def bonus_seconds(ctx,start,end):
        guild=Guilds.current(ctx)
        if not guild or end<=start:return 0
        spans=ctx.entity('presence_history',guild['anchor'],dict(spans=[]))['spans']
        periods=[(r['start'],r['end'],.2) for r in ctx.entities('lease') if r['guild_id']==guild['id'] and r['node_id']=='NODE_QI' and r['room']==ctx.room]
        event=ctx.entity('launch',guild['anchor'])
        if event and event.get('buff_end',0)>start:
            periods.append((event.get('buff_start',event['buff_end']-1200),event['buff_end'],.1))
        return sum(max(0,min(end,p_end,s['end'])-max(start,p_start,s['start']))*bonus
                   for p_start,p_end,bonus in periods for s in spans if s['room']==ctx.room)
