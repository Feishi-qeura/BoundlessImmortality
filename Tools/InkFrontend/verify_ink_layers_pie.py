"""Focused rendered regression after fixing canvas layers and shaped-text scale."""
from pathlib import Path
import unreal

source=Path(unreal.Paths.project_dir())/'Tools/InkFrontend/verify_niagara_revision_pie.py'
exec(compile(source.read_text(encoding='utf-8'),str(source),'exec'),globals())
OUT=Path(unreal.Paths.project_dir())/'Saved/Codex/NiagaraUiRevision20261022/RenderedGold'
OUT.mkdir(parents=True,exist_ok=True)
STEPS.clear()
step('fixture setup',setup,1)
step('fixture profiles',profiles,2)
step('shield sample',lambda:F.get_editor_property('anchor_status_widget').set_shield_presentation(60,100),1)
step('HUD portrait',lambda:hud_check('hud-720x1280'))
step('compact open',lambda:click('HUDRankView'),1)
step('compact portrait',lambda:board_check('compact-720x1280',True))
step('full open',lambda:F.show_page('Leaderboard'),1)
step('full portrait',lambda:board_check('full-720x1280'))
step('HUD restore',lambda:F.show_page('HUD'),1)
step('small resize',lambda:L.set_pie_viewport_size(360,640),1)
step('small HUD',lambda:hud_check('hud-360x640'))
step('landscape resize',lambda:L.set_pie_viewport_size(1280,720),1)
step('landscape HUD',lambda:hud_check('hud-1280x720'))
step('full landscape open',lambda:F.show_page('Leaderboard'),1)
step('full landscape',lambda:board_check('full-1280x720'))
