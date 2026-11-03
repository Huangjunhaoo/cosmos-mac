"""Attach exact Unicode text to the rendered shapes, especially mixed RTL and MIDI notation."""
import json
import re
from pathlib import Path
from pypdf import PdfReader, PdfWriter
from pypdf.generic import DecodedStreamObject, NameObject, TextStringObject

root=Path(__file__).resolve().parents[2]
guides=root/'output/pdf/MIDI Map'
manifest=json.loads((root/'app/assets/interface_languages.json').read_text())
for item in manifest:
    code=item['code']
    path=guides/f'Cosmos-MIDI-Map-{code}.pdf'
    writer=PdfWriter(clone_from=PdfReader(path))
    writer.root_object[NameObject('/Lang')]=TextStringObject(code)
    tags=writer.root_object['/StructTreeRoot']['/K']['/K']
    for i,page in enumerate(writer.pages):
        runs=json.loads((root/f'tmp/pdfs/midi-map/{code}-{i}.json').read_text())
        nodes=[n.get_object() for n in tags if n.get_object().raw_get('/Pg').idnum==page.indirect_reference.idnum]
        assert len(runs)==len(nodes)
        table={}
        for node,s in zip(nodes,runs):
            node[NameObject('/ActualText')]=TextStringObject(s)
            table[int(node['/K'])]=s
        def marked(m):
            k=int(m[1])
            u=(b'\xfe\xff'+table[k].encode('utf-16be')).hex().encode()
            return b'/Span << /MCID '+m[1]+b' /ActualText <'+u+b'> >> BDC'
        data,count=re.subn(rb'/P\s+<<\s*/MCID\s+(\d+)\s*>>\s*BDC',marked,page.get_contents().get_data())
        assert count==len(runs)
        stream=DecodedStreamObject();stream.set_data(data);page.replace_contents(stream)
        page.compress_content_streams()
    fixed=path.with_suffix('.part')
    writer.write(fixed);fixed.replace(path)
