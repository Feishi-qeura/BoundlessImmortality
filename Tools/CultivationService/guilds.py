import math
from .contracts import require, uid


class Guilds:
    @staticmethod
    def current(ctx, user=None):
        user = user or ctx.user
        row = ctx.db.execute('SELECT * FROM guild WHERE scope=? AND anchor=? AND dissolved=0', (ctx.scope, user)).fetchone()
        if not row:
            row = ctx.db.execute('SELECT g.* FROM guild g JOIN membership m ON m.scope=g.scope AND m.guild_id=g.id WHERE m.scope=? AND m.user=? AND g.dissolved=0', (ctx.scope, user)).fetchone()
        return dict(row) if row else None

    @staticmethod
    def apply(ctx, operation, args):
        if operation == 'CreateGuild':
            ctx.anchor_only()
            name = args.get('name', '')
            require(isinstance(name, str) and not any(ord(c) < 32 for c in name), 'NAME_REJECTED')
            length = math.ceil(sum(.5 if ord(c) < 128 else 1 for c in name.strip()))
            require(2 <= length <= 12 and len(name) <= 24, 'NAME_REJECTED')
            require(not Guilds.current(ctx), 'ALREADY_CREATED')
            ctx.db.execute('INSERT INTO guild VALUES(?,?,?,?,?,?,0)',
                           (ctx.scope, uid(), ctx.user, name.strip(), ctx.now, ctx.now,))
        elif operation == 'JoinGuild':
            require(ctx.role == 'viewer', 'FORBIDDEN')
            guild = ctx.db.execute('SELECT * FROM guild WHERE scope=? AND id=? AND dissolved=0', (ctx.scope, args.get('guild_id'))).fetchone()
            require(guild is not None, 'GUILD_NOT_FOUND')
            host = ctx.db.execute('SELECT 1 FROM session WHERE scope=? AND user=? AND room=? AND revoked=0', (ctx.scope, guild['anchor'], ctx.room)).fetchone()
            require(host is not None, 'WRONG_ROOM')
            member = ctx.db.execute('SELECT * FROM membership WHERE scope=? AND user=?', (ctx.scope, ctx.user)).fetchone()
            require(not member or not member['guild_id'], 'ALREADY_MEMBER')
            require(not member or ctx.now >= member['rejoin_after'], 'REJOIN_WAIT')
            ctx.db.execute('INSERT INTO membership VALUES(?,?,?,?,0) ON CONFLICT(scope,user) DO UPDATE SET guild_id=excluded.guild_id,joined=excluded.joined',
                           (ctx.scope, ctx.user, guild['id'], ctx.now))
        elif operation in ('LeaveGuild', 'RemoveMember'):
            user = ctx.user
            if operation == 'RemoveMember':
                ctx.anchor_only()
                user = args.get('user_id')
                own = Guilds.current(ctx)
                member_guild = Guilds.current(ctx, user)
                require(own and member_guild and own['id'] == member_guild['id'], 'FORBIDDEN')
            ctx.db.execute('UPDATE membership SET guild_id=NULL,rejoin_after=? WHERE scope=? AND user=?',
                           (ctx.now + 86400, ctx.scope, user))
        elif operation == 'RenameGuild':
            ctx.anchor_only()
            guild = Guilds.current(ctx)
            require(guild is not None, 'GUILD_NOT_FOUND')
            require(ctx.now - guild['renamed'] >= 604800, 'RENAME_WAIT')
            name = args.get('name', '')
            require(isinstance(name, str) and 2 <= math.ceil(sum(.5 if ord(c) < 128 else 1 for c in name)) <= 12, 'NAME_REJECTED')
            ctx.db.execute('UPDATE guild SET name=?,renamed=? WHERE scope=? AND id=?', (name, ctx.now, ctx.scope, guild['id']))
        elif operation == 'DissolveGuild':
            ctx.anchor_only()
            guild = Guilds.current(ctx)
            require(guild is not None, 'GUILD_NOT_FOUND')
            require(args.get('confirmed') is True, 'CONFIRMATION_REQUIRED')
            require(not ctx.entity('activity', ctx.user), 'ACTIVITY_BUSY')
            ctx.db.execute('UPDATE guild SET dissolved=1 WHERE scope=? AND id=?', (ctx.scope, guild['id']))
            ctx.db.execute('UPDATE membership SET guild_id=NULL,rejoin_after=? WHERE scope=? AND guild_id=?', (ctx.now + 86400, ctx.scope, guild['id']))
            for lease in ctx.entities('lease'):
                if lease['guild_id']==guild['id'] and lease['status']=='Active':
                    lease['status']='Cancelled';ctx.put('lease',lease['id'],lease)
        else:
            require(False, 'UNKNOWN_COMMAND')
