"""Shared build/package inventory; canonical pixel art lives in the catalog."""
from pathlib import Path
import json
ROOT=Path(__file__).resolve().parents[1]
CATALOG=json.loads((ROOT/'assets/meshi_board/catalog.json').read_text())
PETS=CATALOG['pets']
NEW_PETS=PETS[CATALOG['original_count']:]
assert [p['id'] for p in PETS]==list(range(len(PETS)))
assert len({p['name'] for p in PETS})==len(PETS)
