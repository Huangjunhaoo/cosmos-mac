"""Finalize the individual installation cards and their language-indexed volume."""
from pathlib import Path
import json
import logging
import re
from pypdf import PdfReader, PdfWriter
from pypdf.annotations import Link
from pypdf.generic import DecodedStreamObject, NameObject, TextStringObject

root = Path(__file__).resolve().parents[2]
src = root / 'tmp/pdfs/installation'
output = root / 'output/pdf'
cards = output / 'Installation'
cards.mkdir(parents=True, exist_ok=True)
languages = json.loads((root / 'app/assets/interface_languages.json').read_text())
apple = 'https://support.apple.com/en-us/102445'
ableton = 'https://help.ableton.com/hc/en-us/articles/209068929-Using-AU-and-VST-plug-ins-on-macOS'
logic = 'https://support.apple.com/en-ca/122179'
logging.getLogger('pypdf').setLevel(logging.ERROR)

def rect(x, y, w, h):
    return (x, 842-y-h, x+w, 842-y)

def finish(code):
    writer = PdfWriter(clone_from=PdfReader(src / f'{code}.pdf'))
    writer.root_object[NameObject('/Lang')]=TextStringObject('en' if code=='Index' else code)
    page = writer.pages[0]
    runs = json.loads((src / f'{code}.json').read_text())
    nodes = [n.get_object() for n in writer.root_object['/StructTreeRoot']['/K']['/K']]
    assert len(nodes)==len(runs)
    actual = {}
    for node, string in zip(nodes, runs):
        node[NameObject('/ActualText')]=TextStringObject(string)
        actual[int(node['/K'])]=string
    def replace(match):
        text = actual[int(match[1])]
        unicode_hex = (b'\xfe\xff' + text.encode('utf-16be')).hex().encode()
        return b'/Span << /MCID '+match[1]+b' /ActualText <'+unicode_hex+b'> >> BDC'
    data, count = re.subn(rb'/P\s*<<\s*/MCID\s+(\d+)\s*>>\s*BDC', replace, page.get_contents().get_data())
    assert count == len(runs)
    stream = DecodedStreamObject()
    stream.set_data(data)
    page.replace_contents(stream)
    page.compress_content_streams()
    if code!='Index':
        x=450 if code=='ar' else 41
        for area,url in [(rect(185,800,226,28),apple),(rect(x,594,105,27),ableton),(rect(x,649,105,27),logic)]:
            writer.add_annotation(0, Link(rect=area,url=url))
    return writer

index = src / 'finished-index.pdf'
finish('Index').write(index)
volume = PdfWriter()
volume.append(index)
volume.add_outline_item('Languages / 语言', 0)
for i, entry in enumerate(languages, 1):
    code = entry['code']
    card = cards / f'Cosmos-Install-{code}.pdf'
    finish(code).write(card)
    volume.append(card, outline_item=entry['name'])
for i, entry in enumerate(languages):
    x=41+(i%3)*174
    y=382+(i//3)*67
    jump=volume.add_annotation(0,Link(rect=rect(x,y,164,50),target_page_index=i+1))
    jump["/Dest"][0]=volume.pages[i+1].indirect_reference
volume.add_metadata({'/Title':'Cosmos · Mac installation guide · 15 languages','/Author':'Cosmos'})
volume.write(output / '00 - Install Cosmos.pdf')
