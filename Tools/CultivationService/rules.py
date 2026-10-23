"""Typed design import. No asset or legacy-save writes."""
from dataclasses import dataclass
from pathlib import Path
import csv
import hashlib
import json
import re

VERSION = 'wuling-prototype-v1.4.0'


def typed(value):
    if value.lower() in ('true', 'false'):
        return value.lower() == 'true'
    if re.fullmatch(r'-?\d+', value):
        return int(value)
    if re.fullmatch(r'-?\d+\.\d+', value):
        return float(value)
    return value


@dataclass(frozen=True)
class RuleBundle:
    version: str
    sha256: str
    tables: dict
    configs: dict
    realms: list
    items: dict

    @classmethod
    def load(cls, path: Path):
        root = Path(path) / 'tables'
        required = ('realm_costs.csv', 'world_rewards.csv', 'encounters.csv', 'balance.json',
                    'items-v1.4/item_master.csv', 'items-v1.4/recipes.csv',
                    'items-v1.4/recipe_inputs.csv', 'items-v1.4/item_system.json',
                    'items-v1.4/tribulation.json', 'launch_event.json')
        if any(not (root / name).is_file() for name in required):
            raise ValueError('RULE_TABLE_MISSING')
        tables, configs, hash_ = {}, {}, hashlib.sha256()
        for file in sorted(root.rglob('*')):
            if file.suffix not in ('.json', '.csv') or file.name.startswith('ui-'):
                continue
            relative = file.relative_to(root).as_posix()
            hash_.update(relative.encode() + b'\0' + file.read_bytes() + b'\0')
            if file.suffix == '.json':
                configs[relative] = json.loads(file.read_text(encoding='utf-8-sig'))
            else:
                with file.open(encoding='utf-8-sig', newline='') as handle:
                    rows = list(csv.DictReader(handle))
                if any(None in row for row in rows):
                    raise ValueError('INVALID_CSV')
                tables[relative] = [{key: typed(value) for key, value in row.items()} for row in rows]
        items = {row['item_id']: row for row in tables['items-v1.4/item_master.csv']}
        realms = tables['realm_costs.csv']
        if len(items) != 730 or len(items) != len(tables['items-v1.4/item_master.csv']):
            raise ValueError('DUPLICATE_OR_MISSING_ITEM')
        if len(realms) != 34 or [r['realm_index'] for r in realms] != list(range(34)):
            raise ValueError('INVALID_REALM_SEQUENCE')
        if any(r['item_id'] not in items for r in tables['world_rewards.csv']):
            raise ValueError('INVALID_REWARD_REFERENCE')
        recipes=tables['items-v1.4/recipes.csv']
        recipe_ids={r['recipe_id'] for r in recipes}
        if len(recipe_ids)!=len(recipes) or any(r['output_item'] not in items for r in recipes):
            raise ValueError('INVALID_RECIPE_INDEX')
        if any(r['recipe_id'] not in recipe_ids or r['item_id'] not in items or type(r['count']) is not int or r['count']<=0 for r in tables['items-v1.4/recipe_inputs.csv']):
            raise ValueError('INVALID_RECIPE_INPUT')
        for item in items.values():
            if type(item['tradeable']) is not bool or type(item['paid_origin']) is not bool:
                raise ValueError('INVALID_BOOLEAN')
        runtime=Path(__file__).with_name('runtime_balance.json')
        raw=runtime.read_bytes();calibration=json.loads(raw)
        factor=calibration.get('flight_health_factor')
        if type(factor) not in (int,float) or not 0<factor<=1:raise ValueError('INVALID_RUNTIME_CALIBRATION')
        regions=calibration.get('region_mechanics',{})
        if not isinstance(regions,dict) or set(regions)!= {'qingyun','luoxia','xuantie'} or not all(isinstance(regions[k],dict) for k in regions) or [regions[k].get('kind') for k in ('qingyun','luoxia','xuantie')]!=['guard','gather_route','weakpoint']:
            raise ValueError('INVALID_REGION_MECHANICS')
        seconds=regions['xuantie'].get('weakpoint_seconds')
        if type(seconds) is not int or not 1<=seconds<=30:raise ValueError('INVALID_REGION_WINDOW')
        configs['runtime_balance.json']=calibration;hash_.update(b'runtime_balance.json\0'+raw+b'\0')
        return cls(VERSION, hash_.hexdigest(), tables, configs, realms, items)
