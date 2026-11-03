#!/usr/bin/env python3
"""Build the editable native Cocoa application using the existing local SDK.

No downloads, audio device operations, app launch, installation, or global
settings changes. The output starts audio only after its Start button is used.
"""
import argparse,json,plistlib,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parent

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,default=ROOT/'dist/COSMOS-0.16.2.app');parser.add_argument('--ui-only',action='store_true',help='Compile only the UI object; no audio implementation is required.');args=parser.parse_args()
    compiler=shutil.which('clang++')
    if not compiler:raise SystemExit('A locally installed Clang and macOS SDK are required.')
    if sys.platform!='darwin':raise SystemExit('The standalone Cocoa application builds on macOS.')
    flags=['-std=c++17','-O2','-fobjc-arc','-fno-fast-math','-ffp-contract=off','-mmacosx-version-min=13.0','-Wall','-Wextra','-Wno-unused-parameter']
    build=ROOT/'app/build';build.mkdir(parents=True,exist_ok=True)
    if args.ui_only:
        output=build/'main.o';subprocess.run([compiler,*flags,'-c',str(ROOT/'app/main.mm'),'-o',str(output)],check=True);print(output);return
    audio=ROOT/'native/desktop_audio.mm'
    if not audio.is_file():raise SystemExit('The desktop audio implementation is not present yet; use --ui-only to check the interface.')
    bundle=args.output.resolve();contents=bundle/'Contents';macos=contents/'MacOS';macos.mkdir(parents=True,exist_ok=True);(contents/'Resources').mkdir(exist_ok=True)
    # Validate the source plist before copying it into the bundle.
    plistlib.loads((ROOT/'app/Info.plist').read_bytes());shutil.copyfile(ROOT/'app/Info.plist',contents/'Info.plist')
    for asset in (ROOT/'app/assets').glob('*'):
        if asset.is_file():shutil.copyfile(asset,contents/'Resources'/asset.name)
    from tools.bundle_guides import copy_midi_guides
    copy_midi_guides(ROOT,contents/"Resources")
    privacy=plistlib.loads((ROOT/'app/Info.plist').read_bytes())['NSMicrophoneUsageDescription']
    translated=json.loads((ROOT/'app/assets/languages.json').read_text())[privacy]
    codes=[item['code'] for item in json.loads((ROOT/'app/assets/interface_languages.json').read_text())]
    if len(codes)!=1+len(translated):raise ValueError('Incomplete microphone translations')
    for code, message in zip(codes,[privacy,*translated]):
        folder=contents/'Resources'/f'{code}.lproj';folder.mkdir(exist_ok=True)
        (folder/'InfoPlist.strings').write_bytes(plistlib.dumps({'NSMicrophoneUsageDescription':message},fmt=plistlib.FMT_BINARY))
    command=[compiler,*flags,str(ROOT/'app/main.mm'),str(audio),'-framework','Cocoa','-framework','AVFoundation','-framework','UniformTypeIdentifiers','-framework','CoreMIDI','-framework','CoreAudio','-framework','AudioToolbox','-framework','AudioUnit','-o',str(macos/'COSMOS')]
    subprocess.run(command,check=True)
    subprocess.run(['/usr/bin/codesign','--force','--sign','-',str(bundle)],check=True)
    print(bundle)
if __name__=='__main__':main()
