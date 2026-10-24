"""Loopback-only HTTPS transport fixture for production avatar retry code."""
from pathlib import Path
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from datetime import datetime,timezone,timedelta
import json,ssl,threading,time,os,struct,zlib,ipaddress
from cryptography import x509
from cryptography.x509.oid import NameOID
from cryptography.hazmat.primitives import hashes,serialization
from cryptography.hazmat.primitives.asymmetric import rsa
HERE=Path(__file__).resolve().parent;DIR=HERE/'avatar-tls-fixture-02';DIR.mkdir(exist_ok=False)
def write(path,obj):path.write_text(json.dumps(obj,ensure_ascii=False,indent=2),encoding='utf-8')
key=rsa.generate_private_key(public_exponent=65537,key_size=2048)
subject=x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'Declared local avatar test')]);now=datetime.now(timezone.utc)
cert=x509.CertificateBuilder().subject_name(subject).issuer_name(subject).public_key(key.public_key()).serial_number(x509.random_serial_number()).not_valid_before(now-timedelta(minutes=1)).not_valid_after(now+timedelta(hours=4)).add_extension(x509.SubjectAlternativeName([x509.IPAddress(ipaddress.ip_address('127.0.0.1'))]),critical=False).sign(key,hashes.SHA256())
(DIR/'fixture.key').write_bytes(key.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.PKCS8,serialization.NoEncryption()))
(DIR/'fixture.crt').write_bytes(cert.public_bytes(serialization.Encoding.PEM))
def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
PNG=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',1,1,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(b'\x00\x22\x88\x66\xff'))+chunk(b'IEND',b'')
COUNT=0;LOCK=threading.Lock()
class Handler(BaseHTTPRequestHandler):
 def log_message(self,*args):pass
 def do_GET(self):
  global COUNT
  if self.path!='/avatar.png':self.send_error(404);return
  with LOCK:
   COUNT+=1;n=COUNT
   with (DIR/'requests.jsonl').open('a',encoding='utf-8') as f:f.write(json.dumps(dict(number=n,status=503 if n==1 else 200,utc=datetime.now(timezone.utc).isoformat(),monotonic=time.monotonic()))+'\n')
  body=b'declared first request failure' if n==1 else PNG
  self.send_response(503 if n==1 else 200);self.send_header('Content-Type','text/plain' if n==1 else 'image/png');self.send_header('Content-Length',str(len(body)));self.end_headers();self.wfile.write(body)
SERVER=ThreadingHTTPServer(('127.0.0.1',0),Handler);SERVER.daemon_threads=True
ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);ctx.load_cert_chain(str(DIR/'fixture.crt'),str(DIR/'fixture.key'));SERVER.socket=ctx.wrap_socket(SERVER.socket,server_side=True)
meta=dict(pid=os.getpid(),port=SERVER.server_port,url='https://127.0.0.1:'+str(SERVER.server_port)+'/avatar.png',self_signed_declared_fixture=True,stopped=False)
write(HERE/'avatar-service-ready-02.json',meta)
def monitor():
 while True:
  time.sleep(.5);p=HERE/'service-control-02.json';control=json.loads(p.read_text()) if p.exists() else {}
  lease=json.loads((HERE.parent/'engine-lease.json').read_text(encoding='utf-8-sig'))
  if control.get('stop') or datetime.fromisoformat(lease['expires_at'])<=datetime.now(timezone.utc):SERVER.shutdown();return
threading.Thread(target=monitor,daemon=True).start()
try:SERVER.serve_forever()
finally:SERVER.server_close();meta.update(stopped=True,requests=COUNT);write(HERE/'avatar-service-result-02.json',meta)
