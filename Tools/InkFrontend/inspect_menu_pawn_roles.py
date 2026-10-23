"""Read-only serialized role evidence; no Unreal load, edits or asset saves."""
import base64
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MAP = ROOT / 'Content/Isometric_World/Sky_Temple/Maps/Level_Sky_Temple_Demo_01.umap'
MODE = ROOT / 'Content/Isometric_World/Demo/Blueprints/BP_TopDownGameMode.uasset'
map_bytes = MAP.read_bytes()
text = map_bytes.decode('ascii', errors='replace')
entry = re.search(r'([^;\x00]*:PersistentLevel\.BP_TopDownCharacter_C_2),/Script/Engine\.Character,([A-Za-z0-9+/=]+),(/Game/1/BP_TopDown\.BP_TopDown_C)', text)
if not entry:
    raise RuntimeError('Expected placed presentation metadata was not found; inspect runtime roles')
metadata = base64.b64decode(entry.group(2))
mode_text = MODE.read_bytes().decode('ascii', errors='replace')
gameplay_reference = '/Game/Isometric_World/Demo/Blueprints/BP_TopDownCharacter'
report = {
    'boundary': 'Read-only serialized actor metadata and GameMode asset name references; runtime binding is checked separately in PIE',
    'presentation': {'object': entry.group(1), 'class': entry.group(3),
                     'label_in_actor_metadata': b'BP_TopDownCharacter2\x00' in metadata},
    'map_references_game_mode': b'/Game/Isometric_World/Demo/Blueprints/BP_TopDownGameMode\x00' in map_bytes,
    'game_mode_has_default_pawn_property': 'DefaultPawnClass' in mode_text,
    'game_mode_references_gameplay_asset': gameplay_reference in mode_text,
    'expected_gameplay_class_for_pie': gameplay_reference + '.BP_TopDownCharacter_C',
}
OUT = ROOT / 'Saved/Codex/NiagaraUiRevision20261022/PawnRoles'
OUT.mkdir(parents=True, exist_ok=True)
(OUT / 'serialized-role-evidence.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps(report, ensure_ascii=False, indent=2))
