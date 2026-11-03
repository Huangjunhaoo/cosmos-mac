"""Freeze the previously tested application and AU, adding only the installation PDFs."""
import hashlib
import json
import os
import plistlib
import re
import shutil
import subprocess
import tempfile
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED

from pypdf import PdfReader
from PIL import Image, ImageChops

root = Path(__file__).resolve().parents[2]
docs = root / 'output/pdf'
working = root / 'tmp/pdfs/installation'
manifest = json.loads((root / 'app/assets/interface_languages.json').read_text())
archive = root / 'dist/Cosmos-Mac-Final-0.16.2-0.1.10.zip'
previous = root / 'dist/Cosmos-Mac-Multilingual-0.16.2-0.1.10.zip'
release = root / 'release-sharing/2026-11-03-final'
release.mkdir(exist_ok=True)

def digest(b):
    return hashlib.sha256(b).hexdigest()

def check_install():
    book=PdfReader(docs/'00 - Install Cosmos.pdf', strict=True)
    assert len(book.pages)==1+len(manifest)==16 and len(book.outline)==16
    for n,(entry,annotation) in enumerate(zip(manifest,book.pages[0]['/Annots']),1):
        assert annotation.get_object()['/Dest'][0].idnum==book.pages[n].indirect_reference.idnum
        assert book.get_destination_page_number(book.outline[n])==n and book.outline[n].title==entry['name']
        code=entry['code'];p=docs/'Installation'/f'Cosmos-Install-{code}.pdf'
        r=PdfReader(p, strict=True);assert len(r.pages)==1 and r.trailer['/Root']['/Lang']==code
        texts=json.loads((working/f'{code}.json').read_text())
        nodes=r.trailer['/Root']['/StructTreeRoot']['/K']['/K']
        assert [x.get_object()['/ActualText'] for x in nodes]==texts
        for token in ('macOS 13','Apple silicon + Intel','Standalone / COSMOS-0.16.2.app',
                      'Audio Unit / Cosmos.component','~/Library/Audio/Plug-Ins/Components/',
                      'COSMOS Live.component','Logic Pro','Ableton Live','MISO','⇧⌘G','Manuals'):
            assert token in '\n'.join(texts),(code,token)
        assert len(book.pages[n]['/Annots'])==3 and len(r.pages[0]['/Annots'])==3
        subprocess.run(['pdftoppm','-singlefile','-r','86','-png',str(p),str(working/f'single-{code}')],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
        a=Image.open(working/f'single-{code}.png').convert('RGB')
        b=Image.open(working/f'volume-{n+1:02d}.png').convert('RGB')
        assert ImageChops.difference(a,b).getbbox() is None,code
    assert len(book.pages[0]['/Annots'])==15

def snapshot():
    files={root/p for p in ('README.md','build_app.py','build_au_local.py','tools/bundle_guides.py',
            'native/desktop_audio.mm','app/main.mm','app/Info.plist','au/Editor.mm','au/Unit.mm',
            'au/LocalComponent.mm','au/MIDI踏板指南.txt','au/本地面板安装说明.txt')}
    for pat in ('native/*.hpp','app/*.hpp','app/*.inc','au/*.hpp','app/assets/*',
                'output/pdf/MIDI Map/*.pdf','release-sharing/install-guide/*',
                'release-sharing/midi-guide/*'):
        files.update(p for p in root.glob(pat) if p.is_file() and not p.name.startswith('.'))
    for p in files:
        if p.suffix in ('.mm','.hpp','.inc'):
            for inc in re.findall(r'^\s*#(?:import|include)\s+"([^"]+)"',p.read_text(),re.M):
                assert (p.parent/inc).resolve() in files, (p,inc)
    path=release/'Cosmos-Final-source.zip'
    with ZipFile(path,'w',compression=ZIP_DEFLATED,compresslevel=9) as z:
        for p in sorted(files): z.write(p,p.relative_to(root))
    with ZipFile(path) as z:
        assert z.testzip() is None and all(z.read(str(p.relative_to(root)))==p.read_bytes() for p in files)
    return path

check_install()
assert digest(previous.read_bytes())=='cc2e536ba133ffa2eb0d14ad5838afd9b197aae6a62fa66ea5f9cbb2c7efba7c'
added={'Cosmos Mac/00 - Install Cosmos.pdf':docs/'00 - Install Cosmos.pdf'}
for entry in manifest:
    f=docs/'Installation'/f"Cosmos-Install-{entry['code']}.pdf"
    added['Cosmos Mac/Manuals/Installation/'+f.name]=f
scratch=archive.with_suffix('.building')
shutil.copyfile(previous,scratch)
with ZipFile(scratch,'a',compression=ZIP_DEFLATED,compresslevel=9) as z:
    assert not set(added).intersection(z.namelist())
    for name,p in added.items():z.write(p,name)
with ZipFile(previous) as old, ZipFile(scratch) as z:
    assert len(z.namelist())==len(set(z.namelist()))
    assert set(z.namelist())==set(old.namelist())|set(added) and z.testzip() is None
    for i in old.infolist():
        assert z.read(i.filename)==old.read(i.filename) and z.getinfo(i.filename).external_attr==i.external_attr
    for name,p in added.items():assert z.read(name)==p.read_bytes()
    prior_entries=len(old.namelist())

products=[]
with tempfile.TemporaryDirectory() as dest:
    subprocess.run(['ditto','-x','-k',str(scratch),dest],check=True)
    bundle_root=Path(dest)/'Cosmos Mac'
    for name,want,build,arch in [('Standalone/COSMOS-0.16.2.app','0.16.2','29',{'arm64'}),('Audio Unit/Cosmos.component','0.1.10','12',{'arm64','x86_64'})]:
        p=bundle_root/name
        subprocess.run(['codesign','--verify','--deep','--strict',str(p)],check=True)
        info=plistlib.loads((p/'Contents/Info.plist').read_bytes())
        binary=p/'Contents/MacOS'/info['CFBundleExecutable']
        installed=Path.home()/('Applications/'+p.name if p.suffix=='.app' else 'Library/Audio/Plug-Ins/Components/'+p.name)
        assert binary.read_bytes()==(installed/'Contents/MacOS'/info['CFBundleExecutable']).read_bytes()
        assert info['CFBundleShortVersionString']==want and info['CFBundleVersion']==build and info['LSMinimumSystemVersion']=='13.0'
        assert os.access(binary,os.X_OK) and set(subprocess.check_output(['lipo','-archs',str(binary)],text=True).split())==arch
        products.append(dict(path=name,version=want,build=build,minimum_macos=info['LSMinimumSystemVersion'],architecture=sorted(arch),executable_sha256=digest(binary.read_bytes())))
    assert len(PdfReader(bundle_root/'Manuals/Cosmos-MIDI-Map-15-languages.pdf',strict=True).pages)==45
    for entry in manifest:
        code=entry['code'];midi=bundle_root/'Manuals'/f'Cosmos-MIDI-Map-{code}.pdf'
        assert len(PdfReader(midi,strict=True).pages)==3
        for product in products: assert (bundle_root/product['path']/'Contents/Resources/MIDI Map'/midi.name).read_bytes()==midi.read_bytes()
scratch.replace(archive)
source=snapshot()
record={'status':'最终定档，开发结束','date':'2026-11-03','products':products,'local_signing':True,'apple_notarized':False,
        'share_package':{'filename':archive.name,'bytes':archive.stat().st_size,'sha256':digest(archive.read_bytes())},
        'source_snapshot':{'filename':source.name,'sha256':digest(source.read_bytes())},
        'installation_guide':{'filename':'Cosmos Mac/00 - Install Cosmos.pdf','pages':16,'sha256':digest((docs/'00 - Install Cosmos.pdf').read_bytes())},
        'languages':[e['code'] for e in manifest],
        'verified':{'unchanged_entries_from_previous_share':prior_entries,'new_installation_pdfs':16,
          'installed_executables_identical':True,'strict_signatures_and_executable_permissions_after_unpack':True,
          'original_midi_volume_pages':45,'installation_language_links':15,'installation_cards_match_volume_render':True}}
(release/'定档记录.json').write_text(json.dumps(record,ensure_ascii=False,indent=2)+'\n')
print(json.dumps(record['share_package'],ensure_ascii=False))
