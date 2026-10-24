"""Generate a small actual-UE boundary suite using the same rendered entry helpers."""
from pathlib import Path
import re

here=Path(__file__).resolve().parent
source=(here/'verify_engine_pie.py').read_text(encoding='utf-8')
head=source.split("step('main start via actual button'")[0]
head=re.sub(r"OUT=HERE/'pie-\d+'","OUT=HERE/'pie-04-boundaries'",head)
head=re.sub(r'engine-qa-service-\d+/requests.jsonl','engine-qa-service-04/requests.jsonl',head)
steps='''
step('actual main entry',lambda:click('StartGame'),4)
step('authority connected',lambda:ck('real authority connected',S.is_connected()))
step('native shortage request',lambda:ck('native UE shortage command enqueued',S.prepare_breakthrough('[{"item_id":"MAT_BR04_C01_G1","count":1},{"item_id":"MAT_BR04_G01_G1","count":1}]')),2)
step('shortage response',lambda:(ck('actual authority INSUFFICIENT_FUNDS',last_reply('PrepareBreakthrough').get('error_code')=='INSUFFICIENT_FUNDS'),ck('shortage leaves no attempt',state().get('tribulation') is None),ck('shortage preserves core',state().get('inventory',{}).get('MAT_BR04_C01_G1')==1)))
step('response loss fixture',lambda:control(drop_next='SignIn'))
step('sign entry',lambda:click('OpenSignIn'))
step('actual sign button',lambda:ck('real Slate sign sends command',ordinary('JourneySignInClaimButton')),7)
step('idempotent recovery',lambda:(ck('signed after lost response recovery',state().get('activities',{}).get('signed') is True),ck('same-id committed command replay',len([v for v in audit() if v.get('command')=='SignIn'])>=2 and len({v['request']['request_id'] for v in audit() if v.get('command')=='SignIn'})==1),ck('same receipt returned on replay',len({v['response'].get('receipt_id') for v in audit() if v.get('command')=='SignIn'})==1),ck('one sign count after actual replay',state().get('activities',{}).get('month_sign_count')==4),capture('lost-response-recovered')))
step('close sign',lambda:click('Ext_Close'))
step('native launch setup guild',lambda:ck('native guild setup enqueued',S.create_guild('边界验证武灵')),2)
step('native launch setup event',lambda:ck('native launch setup enqueued',S.start_launch_event()),2)
step('capture active countdown',lambda:capture('active-countdown'))
step('actual Wuling accessible beside countdown',lambda:click('OpenWuling'))
step('Wuling route confirmed',lambda:page_is('Wuling'))
step('close Wuling for landscape',lambda:click('Ext_Close'))
step('landscape real viewport',lambda:ck('resize actual PIE to landscape',L.set_pie_viewport_size(1280,720)),2)
step('landscape countdown capture',lambda:capture('active-countdown-landscape'))
step('landscape actual Wuling entry',lambda:click('OpenWuling'))
step('landscape Wuling route',lambda:page_is('Wuling'))
step('close Wuling for narrow portrait',lambda:click('Ext_Close'))
step('narrow portrait real viewport',lambda:ck('resize actual PIE to narrow portrait',L.set_pie_viewport_size(680,1280)),2)
step('narrow portrait countdown capture',lambda:capture('active-countdown-narrow'))
step('narrow portrait actual Wuling entry',lambda:click('OpenWuling'))
step('narrow portrait Wuling route',lambda:page_is('Wuling'))
'''
tail='def finish():'+source.split('def finish():',1)[1]
body=head+steps+tail
compile(body,'verify_engine_boundaries.py','exec')
(here/'verify_engine_boundaries.py').write_text(body,encoding='utf-8')

viewer_head=head.replace("pie-04-boundaries","pie-05-viewer").replace("engine-qa-service-04/requests.jsonl","engine-qa-service-05/requests.jsonl")
viewer_steps='''
step('actual viewer entry',lambda:click('StartGame'),4)
step('viewer identity',lambda:(ck('real service connected',S.is_connected()),ck('authority issued viewer identity',state().get('identity',{}).get('role')=='viewer')))
step('viewer direct native request',lambda:ck('native UE request enqueued for server authorization check',S.prepare_breakthrough('[{"item_id":"MAT_BR04_C01_G1","count":1}]')),2)
step('actual viewer refusal',lambda:(ck('actual UE FORBIDDEN response',last_reply('PrepareBreakthrough').get('error_code')=='FORBIDDEN'),ck('viewer has no attempt',state().get('tribulation') is None)))
step('viewer launch entry',lambda:click('OpenLaunchEvent'))
step('viewer launch gate',lambda:(ck('viewer UI refuses launch click',not L.click_action(F,'Ext_LaunchPrimary')),ck('viewer launch request absent',not any(v.get('command')=='StartLaunchEvent' for v in audit())),capture('viewer-launch-gated')))
'''
viewer=viewer_head+viewer_steps+tail
compile(viewer,'verify_engine_viewer.py','exec')
(here/'verify_engine_viewer.py').write_text(viewer,encoding='utf-8')
print('Prepared two bounded rendered PIE suites; no UE execution')
