from pathlib import Path
here=Path(__file__).resolve().parent
source=(here/'verify_engine_pie.py').read_text(encoding='utf-8')
head=source.split("step('main start via actual button'")[0].replace("OUT=HERE/'pie-07'","OUT=HERE/'pie-10-frame-final'").replace('engine-qa-service-07/requests.jsonl','engine-qa-service-04/requests.jsonl')
steps='''
step('real main entry',lambda:click('StartGame'),4)
step('active authority event',lambda:ck('actual launch active',state().get('launch',{}).get('status')=='Active'))
step('final frame portrait capture',lambda:capture('frame-portrait'))
step('native frame actual Slate click',lambda:ck('native countdown Slate opens launch',ordinary('LaunchCountdownHitTarget')))
step('actual countdown route',lambda:page_is('LaunchEvent'))
step('return to HUD',lambda:click('Ext_Close'))
step('Wuling remains accessible',lambda:click('OpenWuling'))
step('actual Wuling route',lambda:page_is('Wuling'))
step('close Wuling',lambda:click('Ext_Close'))
step('landscape actual viewport',lambda:ck('actual landscape resize',L.set_pie_viewport_size(1280,720)),2)
step('final frame landscape capture',lambda:capture('frame-landscape'))
step('landscape countdown Slate click',lambda:ck('landscape countdown opens launch',ordinary('LaunchCountdownHitTarget')))
step('landscape actual route',lambda:page_is('LaunchEvent'))
step('close launch for narrow',lambda:click('Ext_Close'))
step('narrow actual viewport',lambda:ck('actual narrow portrait resize',L.set_pie_viewport_size(680,1280)),2)
step('final frame narrow capture',lambda:capture('frame-narrow'))
step('narrow countdown Slate click',lambda:ck('narrow countdown opens launch',ordinary('LaunchCountdownHitTarget')))
step('narrow actual route',lambda:page_is('LaunchEvent'))
'''
tail='def finish():'+source.split('def finish():',1)[1]
body=head+steps+tail
compile(body,'verify_final_frame_smoke.py','exec')
(here/'verify_final_frame_smoke.py').write_text(body,encoding='utf-8')
