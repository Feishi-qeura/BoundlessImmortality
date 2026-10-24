"""Local screenshot index. Uses original formal screenshots without pixel edits."""
from pathlib import Path
import json,html
H=Path(__file__).resolve().parent;D=H/'default-root-pie-14';R=json.loads((D/'report.json').read_text());assert R.get('passed')
groups={}
for filename in R['captures']:
 p=Path(filename);s=json.loads(p.with_suffix('.json').read_text());orientation='landscape' if s['width']>s['height'] else 'portrait';groups.setdefault(s['page'],{}).setdefault(orientation,p)
sections=[]
for page,views in groups.items():
 figures=[]
 for orientation in ['portrait']:
  p=views.get(orientation)
  if not p:continue
  relative=p.relative_to(H).as_posix();proof=p.with_suffix('.json').relative_to(H).as_posix()
  figures.append(f'<figure><a href="{html.escape(relative)}"><img loading="lazy" src="{html.escape(relative)}" alt="{html.escape(page)} {orientation}"></a><figcaption>{orientation} · <a href="{html.escape(proof)}">runtime readback</a></figcaption></figure>')
 sections.append(f'<section data-name="{html.escape(page.lower())}"><h2>{html.escape(page)}</h2><div class="views">'+''.join(figures)+'</div></section>')
document='''<!doctype html><html lang="zh-CN"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>newUI 44页9:16竖屏真实 PIE 截图</title><style>body{margin:0 auto;padding:32px;max-width:1500px;font:16px/1.6 system-ui;background:#f6f3ed;color:#243a35}header{position:sticky;top:0;background:#f6f3edf5;padding:12px 0;z-index:1}h1{font-size:26px;margin:0}input{padding:12px;width:min(90%,420px);font:inherit;border:1px solid #a5b4ad;border-radius:6px}.views{display:grid;grid-template-columns:1fr;gap:20px}figure{margin:0;background:white;padding:10px;border-radius:8px}img{display:block;max-width:100%;height:650px;object-fit:contain;margin:auto}figcaption{padding:8px}section{margin:32px 0}a{color:#35665a}@media(max-width:800px){body{padding:16px}.views{grid-template-columns:1fr}img{height:auto;max-height:750px}}</style><header><h1>newUI · 44页9:16竖屏真实 PIE 截图</h1><p>仅手机竖屏9:16 · 当前源码构建26 · 默认新 WBP · 隔离真实服务。原始截图未编辑。候选视觉 approved:false，等待人工确认；此索引不代表设备或生产平台验收。</p><input id="filter" placeholder="按英文页面名筛选" aria-label="筛选页面"></header>'''+''.join(sections)+'''<script>document.getElementById('filter').addEventListener('input',e=>{const q=e.target.value.toLowerCase();document.querySelectorAll('section').forEach(s=>s.hidden=!s.dataset.name.includes(q));});</script></html>'''
(H/'current44-screenshot-gallery.html').write_text(document,encoding='utf-8');print('Gallery pages',len(groups))
