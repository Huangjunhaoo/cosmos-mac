"""Include the two-page MIDI guide for each language offered by the panel."""
import json
import shutil

def copy_midi_guides(root, resources):
    languages = json.loads((root / 'app/assets/interface_languages.json').read_text())
    target = resources / 'MIDI Map'
    target.mkdir(exist_ok=True)
    for language in languages:
        name = f"Cosmos-MIDI-Map-{language['code']}.pdf"
        shutil.copyfile(root / 'output/pdf/MIDI Map' / name, target / name)
