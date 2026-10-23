from dataclasses import dataclass
from datetime import datetime, timedelta, timezone
import json
import uuid


class RuleError(ValueError):
    pass


def require(condition, code):
    if not condition:
        raise RuleError(code)


def integer(value, minimum=0, maximum=2**53-1):
    require(type(value) is int and minimum <= value <= maximum, 'INVALID_ARGUMENT')
    return value


def encode(value):
    return json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(',', ':'), allow_nan=False)


def uid():
    return uuid.uuid4().hex


@dataclass
class CommandContext:
    db: object
    authority: object
    session: dict
    now: float
    request_id: str = ''

    @property
    def scope(self): return self.session['scope']
    @property
    def user(self): return self.session['user']
    @property
    def room(self): return self.session['room']
    @property
    def role(self): return self.session['role']
    @property
    def rules(self): return self.authority.rules
    @property
    def day(self): return datetime.fromtimestamp(self.now, timezone(timedelta(hours=8))).strftime('%Y-%m-%d')
    @property
    def window_end(self): return self.db.execute('SELECT end FROM namespace WHERE id=?', (self.scope,)).fetchone()[0]

    def anchor_only(self): require(self.role == 'anchor', 'FORBIDDEN')

    @property
    def reservation_key(self):
        import hashlib
        return 'command:'+hashlib.sha256(encode([self.scope,self.user,self.request_id]).encode()).hexdigest()

    def for_user(self, user):
        row = self.db.execute('SELECT s.*,i.role,i.name FROM session s JOIN identity i USING(scope,user) WHERE s.scope=? AND s.user=? ORDER BY generation DESC LIMIT 1', (self.scope,user)).fetchone()
        require(row is not None, 'IDENTITY_NOT_FOUND')
        return CommandContext(self.db,self.authority,dict(row),self.now,self.request_id)

    def entity(self, kind, id_, default=None):
        row = self.db.execute('SELECT data FROM entity WHERE scope=? AND kind=? AND id=?', (self.scope, kind, id_)).fetchone()
        return json.loads(row[0]) if row else default

    def put(self, kind, id_, data):
        self.db.execute('INSERT INTO entity VALUES(?,?,?,?) ON CONFLICT(scope,kind,id) DO UPDATE SET data=excluded.data',
                        (self.scope, kind, id_, encode(data)))

    def entities(self, kind):
        return [json.loads(r[0]) for r in self.db.execute('SELECT data FROM entity WHERE scope=? AND kind=?', (self.scope, kind))]

    def count(self, owner, key, delta=0):
        if delta:
            self.db.execute('INSERT INTO counter VALUES(?,?,?,?) ON CONFLICT(scope,owner,key) DO UPDATE SET value=value+excluded.value',
                            (self.scope, owner, key, delta))
        row = self.db.execute('SELECT value FROM counter WHERE scope=? AND owner=? AND key=?', (self.scope, owner, key)).fetchone()
        return row[0] if row else 0

    def money(self, owner, currency, delta=0, reason='', source=''):
        row = self.db.execute('SELECT amount FROM wallet WHERE scope=? AND owner=? AND currency=?', (self.scope, owner, currency)).fetchone()
        old = row[0] if row else 0
        require(old + delta >= 0, 'INSUFFICIENT_FUNDS')
        if delta:
            self.db.execute('INSERT INTO wallet VALUES(?,?,?,?) ON CONFLICT(scope,owner,currency) DO UPDATE SET amount=excluded.amount',
                            (self.scope, owner, currency, old + delta))
            self.db.execute('INSERT INTO wallet_ledger VALUES(?,?,?,?,?,?,?,?)',
                            (uid(), self.scope, owner, currency, delta, reason, source or self.request_id, self.now))
        return old + delta

    def fact(self, key, kind, data, user=None):
        cursor = self.db.execute('INSERT OR IGNORE INTO fact VALUES(?,?,?,?,?,?)',
                                (self.scope, key, kind, user or self.user, self.now, encode(data)))
        return bool(cursor.rowcount)
