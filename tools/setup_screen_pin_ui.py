"""Apply/remove only the owned screen-PIN UI additions around board overlays.

The pinned base/Heltec patches stay unchanged. Exact paired replacements make
switching build targets reversible without resetting unrelated source edits.
"""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / 'upstream/examples/companion_radio/ui-new/UITask.cpp'
PAIRS = [
    ('#include "LockRenderer.h"', '#include "LockRenderer.h"\n#include "ScreenPinRenderer.h"'),
    ('  _node_prefs = node_prefs;', '''  _node_prefs = node_prefs;
#if defined(MESHIS_V2)
  meshis::screenPin().bind(&_node_prefs->meshis_screen_pin);
  meshis::keyLock().restorePinLock(millis());
#endif'''),
    ('  const auto oldPrgPhase=meshis::keyLock().phase();', '''  const auto oldPrgPhase=meshis::keyLock().phase();
  const bool oldPrgPin=meshis::screenPin().entryActive();'''),
    ('  if(oldPrgPhase!=meshis::keyLock().phase())_next_refresh=0;', '''  if(oldPrgPhase!=meshis::keyLock().phase() || oldPrgPin!=meshis::screenPin().entryActive())_next_refresh=0;'''),
    ('  quickTx.poll(messageMesh,millis());', '''  if(meshis::screenPin().takeChanged()){
    const bool saved=the_mesh.saveScreenPinPrefs();meshis::screenPin().resolveSave(saved);
    showAlert(saved?(meshis::screenPin().enabled()?"PIN gespeichert":"PIN entfernt"):"PIN Speicherfehler",1200);_next_refresh=0;
  }
  quickTx.poll(messageMesh,millis());'''),
    ('  c=static_cast<char>(meshis::keyLock().route(static_cast<uint8_t>(c),millis()));', '''  if(c && meshis::screenPin().entryActive()){_next_refresh=0;_auto_off=millis()+AUTO_OFF_MILLIS;}
  c=static_cast<char>(meshis::keyLock().route(static_cast<uint8_t>(c),millis()));'''),
    ('      if(meshis::keyLock().overlay()){', '''      if(meshis::screenPin().entryActive()){
        delay_millis=meshis::renderScreenPin(*_display,meshis::screenPin(),millis());
      }else if(meshis::keyLock().overlay()){'''),
    ('          && !meshis::keyLock().overlay()', '          && !meshis::keyLock().blocks()'),
]

def main():
    mode = sys.argv[1] if len(sys.argv) == 2 else ''
    if mode not in ('strip', 'apply'):
        raise SystemExit('Choose strip or apply')
    if not TARGET.exists():
        if mode == 'strip': return
        raise SystemExit('Pinned UI missing')
    source = TARGET.read_text()
    # Migrate this task's earlier unchecked save addition, without changing base UI.
    legacy = '\n'.join([
        '  if(meshis::screenPin().takeChanged()){',
        '    the_mesh.savePrefs();showAlert(meshis::screenPin().enabled()?"PIN gespeichert":"PIN entfernt",900);_next_refresh=0;',
        '  }', '  quickTx.poll(messageMesh,millis());'])
    if legacy in source:
        assert source.count(legacy) == 1
        source = source.replace(legacy, '  quickTx.poll(messageMesh,millis());', 1)

    for old, new in PAIRS:
        # PRG pairs are present only after the Heltec overlay.
        optional = 'oldPrg' in old
        if mode == 'strip':
            if new in source:
                assert source.count(new) == 1, 'Ambiguous owned PIN addition'
                source = source.replace(new, old, 1)
        elif new not in source:
            if optional and old not in source: continue
            assert source.count(old) == 1, 'PIN UI source conflict: preserve source for review'
            source = source.replace(old, new, 1)
    if source != TARGET.read_text(): TARGET.write_text(source)

if __name__ == '__main__': main()
