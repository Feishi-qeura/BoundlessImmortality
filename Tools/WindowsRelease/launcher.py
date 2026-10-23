"""Portable local gameplay launcher. Never enables simulated identities in Shipping."""
import argparse
from contextlib import nullcontext
import ctypes
import json
import logging
import os
from pathlib import Path
import re
import subprocess
import sys
import time
import urllib.request
import uuid

LOG = logging.getLogger('wuling-launcher')
HIDDEN = subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0


def display_arguments(arguments):
    # The distributable opens in portrait by default. Explicit QA/user resolution
    # and fullscreen options remain authoritative instead of receiving duplicates.
    if any(a.lower().startswith(('-resx=', '-resy=', '-fullscreen', '-windowedfullscreen')) for a in arguments):
        return list(arguments)
    return ['-windowed', '-ResX=540', '-ResY=960', *arguments]


class ProfileLock:
    def __init__(self, path):
        self.path, self.file = Path(path), None

    def __enter__(self):
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.file = self.path.open('a+b')
        if self.path.stat().st_size == 0:
            self.file.write(b'0')
        self.file.flush()
        self.file.seek(0)
        try:
            if os.name == 'nt':
                import msvcrt
                msvcrt.locking(self.file.fileno(), msvcrt.LK_NBLCK, 1)
            else:
                import fcntl
                fcntl.flock(self.file, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except OSError:
            self.file.close()
            self.file = None
            raise RuntimeError('PROFILE_ALREADY_RUNNING') from None
        return self

    def __exit__(self, *_):
        if self.file:
            if os.name == 'nt':
                import msvcrt
                self.file.seek(0)
                msvcrt.locking(self.file.fileno(), msvcrt.LK_UNLCK, 1)
            self.file.close()


def read_health(endpoint):
    if not re.fullmatch(r'http://127\.0\.0\.1:([0-9]{1,5})', endpoint):
        raise RuntimeError('INVALID_LOCAL_ENDPOINT')
    port = int(endpoint.rsplit(':', 1)[1])
    if not 1024 <= port <= 65535:
        raise RuntimeError('INVALID_LOCAL_ENDPOINT')
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
    with opener.open(endpoint + '/health', timeout=2) as reply:
        return json.load(reply)


def validate_health(health, expected_hash):
    if (health.get('service') != 'wuling-local-authority' or health.get('protocol') != 1
            or health.get('profile') != 'prototype_full' or health.get('rule_version') != 'wuling-prototype-v1.4.0'):
        raise RuntimeError('AUTHORITY_PROTOCOL_MISMATCH')
    if health.get('rules_hash') != expected_hash:
        raise RuntimeError('RULE_HASH_MISMATCH')


class LocalAuthority:
    def __init__(self, python, root, rules, data, expected_hash, timeout=25):
        self.python, self.root, self.rules, self.data = str(python), Path(root).resolve(), Path(rules).resolve(), Path(data).resolve()
        self.expected_hash, self.timeout = expected_hash, timeout
        self.process = self.log = None
        self.endpoint = ''
        self.ready_file = self.data / ('ready-' + uuid.uuid4().hex + '.json')

    def __enter__(self):
        self.data.mkdir(parents=True, exist_ok=True)
        self.log = (self.data / 'authority.log').open('ab')
        # Explicit -c import path works under isolated Python and from any cwd.
        bootstrap = 'import sys;sys.path.insert(0,sys.argv.pop(1));from Tools.CultivationService.__main__ import main;main()'
        command = [self.python, '-I', '-B', '-u', '-c', bootstrap, str(self.root),
            '--port', '0', '--rules', str(self.rules), '--db', str(self.data / 'prototype.sqlite3'),
            '--ready-file', str(self.ready_file), '--parent-stdin']
        try:
            self.process = subprocess.Popen(command, cwd=self.data, stdin=subprocess.PIPE,
                stdout=self.log, stderr=self.log, creationflags=HIDDEN)
            deadline = time.monotonic() + self.timeout
            while time.monotonic() < deadline:
                if self.process.poll() is not None:
                    raise RuntimeError('AUTHORITY_EXITED: see authority.log')
                if self.ready_file.is_file():
                    ready = json.loads(self.ready_file.read_text(encoding='utf-8'))
                    if ready.get('pid') != self.process.pid:
                        raise RuntimeError('AUTHORITY_PROCESS_MISMATCH')
                    self.endpoint = f'http://127.0.0.1:{ready["port"]}'
                    validate_health(self.health(), self.expected_hash)
                    LOG.info('Local authority ready at %s', self.endpoint)
                    return self
                time.sleep(.1)
            raise RuntimeError('AUTHORITY_STARTUP_TIMEOUT')
        except BaseException:
            self.__exit__(*sys.exc_info())
            raise

    def health(self):
        return read_health(self.endpoint)

    def __exit__(self, *_):
        if self.process:
            if self.process.stdin:
                self.process.stdin.close()
            try:
                self.process.wait(timeout=8)
            except subprocess.TimeoutExpired:
                self.process.terminate()
                self.process.wait(timeout=5)
        if self.log:
            self.log.close()
        self.ready_file.unlink(missing_ok=True)


def local_identity(data):
    path = data / 'identity.json'
    if path.exists():
        identity = json.loads(path.read_text(encoding='utf-8'))
        if not re.fullmatch(r'[a-f0-9]{32}', identity.get('id', '')):
            raise RuntimeError('INVALID_LOCAL_IDENTITY: preserve identity.json and restore its backup')
        return identity['id']
    identity = uuid.uuid4().hex
    pending = path.with_suffix('.pending')
    pending.write_text(json.dumps(dict(id=identity)), encoding='utf-8')
    pending.replace(path)
    return identity


def main():
    parser = argparse.ArgumentParser(description='Windows local full-gameplay package launcher')
    parser.add_argument('--package', type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument('--profile', default='default')
    parser.add_argument('--connect', help='Join another local launcher authority (loopback only).')
    parser.add_argument('--smoke-check', action='store_true')
    parser.add_argument('--game-arg', action='append', default=[])
    args = parser.parse_args()
    if not re.fullmatch(r'[A-Za-z0-9_-]{1,40}', args.profile):
        parser.error('profile must contain 1..40 ASCII letters, digits, underscore or hyphen')
    package = args.package.resolve()
    manifest = json.loads((package / 'release-manifest.json').read_text(encoding='utf-8'))
    if manifest.get('configuration') != 'Development' or manifest.get('profile') != 'prototype_full':
        raise RuntimeError('FULL_LOCAL_GAMEPLAY_REQUIRES_DEVELOPMENT_PACKAGE')
    data = Path(os.environ.get('LOCALAPPDATA', str(Path.home()))) / 'BoundlessImmortality/LocalV14' / args.profile
    data.mkdir(parents=True, exist_ok=True)
    # Windows packaged-app redirection can put LocalAppData on another volume.
    # Use the final directory for both sides of atomic rename and for SQLite sidecars.
    data = data.resolve()
    logging.basicConfig(filename=data / 'launcher.log', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
    with ProfileLock(data / 'profile.lock'):
        identity = local_identity(data)
        runtime = package / 'Runtime/python.exe'
        authority = LocalAuthority(runtime, package / 'LocalService', package / 'Rules', data, manifest['rules_hash'])
        if args.connect:
            validate_health(read_health(args.connect), manifest['rules_hash'])
        with nullcontext(None) if args.connect else authority as owned:
            endpoint = args.connect if args.connect else owned.endpoint
            if args.smoke_check:
                LOG.info('Package authority smoke passed')
                return 0
            game = package / manifest['game_executable']
            if not game.is_file():
                raise RuntimeError('GAME_EXECUTABLE_MISSING')
            command = [str(game), '-WulingPrototype', '-GWLocalTest', '-WulingApp=windows-local-v14',
                '-WulingRoom=room-' + identity, '-WulingUser=anchor-' + identity, '-WulingUrl=' + endpoint,
                '-UserDir=' + str(data / 'Game'), '-log', *display_arguments(args.game_arg)]
            LOG.info('Starting packaged Development game')
            game_log = (data / 'game-console.log').open('ab')
            try:
                with subprocess.Popen(command, cwd=game.parent, stdout=game_log, stderr=game_log) as process:
                    while process.poll() is None:
                        if owned and owned.process.poll() is not None:
                            # Keep the game open to show disconnect state and preserve its outbox.
                            LOG.error('Authority exited during gameplay; transactions remain pending')
                            return process.wait()
                        time.sleep(.5)
                    return process.returncode
            finally:
                game_log.close()


if __name__ == '__main__':
    try:
        sys.exit(main())
    except Exception as error:
        LOG.exception('Local package startup failed')
        message = '启动失败：' + str(error) + '\n日志：%LOCALAPPDATA%\\BoundlessImmortality\\LocalV14'
        if os.name == 'nt' and not sys.stderr:
            ctypes.windll.user32.MessageBoxW(None, message, '无极仙道', 0x10)
        else:
            print(message, file=sys.stderr)
        sys.exit(1)
