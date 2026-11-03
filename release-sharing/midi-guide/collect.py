"""Create the language-bookmarked volume and the individual-PDF download after rendering."""
import json
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
from pypdf import PdfReader, PdfWriter

root = Path(__file__).resolve().parents[2]
folder = root / 'output/pdf'
languages = json.loads((root / 'app/assets/interface_languages.json').read_text())
writer = PdfWriter()
with ZipFile(folder / 'Cosmos-MIDI-Map-PDFs.zip', 'w', compression=ZIP_DEFLATED) as archive:
    for item in languages:
        doc = folder / 'MIDI Map' / f"Cosmos-MIDI-Map-{item['code']}.pdf"
        reader = PdfReader(doc)
        assert len(reader.pages) == 3 and reader.trailer['/Root']['/Lang'] == item['code']
        writer.append(reader, outline_item=item['name'])
        archive.write(doc, doc.name)
writer.add_metadata({'/Title':'Cosmos · MIDI Map · 15 languages', '/Author':'Cosmos'})
writer.write(folder / 'Cosmos-MIDI-Map-15-languages.pdf')
