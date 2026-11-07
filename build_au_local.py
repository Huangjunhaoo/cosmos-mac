#!/usr/bin/env python3
"""Build the Live-native Cocoa panel and the existing COSMOS audio unit in one component."""
from pathlib import Path
import plistlib
import shutil
import subprocess

root = Path(__file__).resolve().parent
bundle = root / 'dist' / 'Cosmos.component'
contents = bundle / 'Contents'
resources = contents / 'Resources'
binary = contents / 'MacOS' / 'Cosmos'
binary.parent.mkdir(parents=True, exist_ok=True)
resources.mkdir(exist_ok=True)
component = dict(name='Chicken Chukka: Cosmos', description='Cosmos Drifting Memory Station',
                 type='aumf', subtype='CsL3', manufacturer='Csms', version=0x10D,
                 factoryFunction='CosmosLocalFactory', sandboxSafe=True, tags=['Effects', 'Delay'])
plist = dict(CFBundleIdentifier='local.cosmos.live.component', CFBundleName='Cosmos', CFBundleDisplayName='Cosmos',
             CFBundleExecutable=binary.name, CFBundlePackageType='BNDL', CFBundleVersion='15',
             CFBundleShortVersionString='0.1.13', CFBundleInfoDictionaryVersion='6.0',
             LSMinimumSystemVersion='13.0', AudioComponents=[component])
(contents / 'Info.plist').write_bytes(plistlib.dumps(plist))
for path in (root / 'app/assets').iterdir():
    if path.is_file():
        shutil.copy2(path, resources / path.name)
from tools.bundle_guides import copy_midi_guides
copy_midi_guides(root, resources)
shutil.copy2(root / 'au' / 'MIDI踏板指南.txt', resources)
# Objective-C class names are process-wide. Scope all embedded controls to this component.
classes = ('CXLibrary CXDrawer CXProgramRow CosmosAUView CosmosAUUnit CosmosPanelResources CosmosParameterHover CosmosParameterHoverPanel '
           'CosmosParameterHoverBubble CosmosWordmark PaperView ModeLabel FirmwareMenu ProgramDial '
           'ParameterDial MomentaryButton').split()
subprocess.run(['clang++', '-arch', 'arm64', '-arch', 'x86_64', '-std=c++17', '-fobjc-arc', '-O2', '-fno-fast-math', '-ffp-contract=off',
                '-mmacosx-version-min=13.0', '-fvisibility=hidden', '-bundle', '-DCOSMOS_LIVE_GUIDE=1',
                *[f'-D{name}=COSMOSLive_{name}' for name in classes],
                str(root/'app/program_library.mm'), *[str(root/'au'/file) for file in ('Unit.mm', 'Editor.mm', 'LocalComponent.mm')],
                '-framework', 'Cocoa', '-framework', 'CoreAudioKit', '-framework', 'AudioToolbox',
                '-framework', 'AudioUnit', '-framework', 'AVFoundation', '-framework', 'CoreMIDI',
                '-o', str(binary)], check=True)
subprocess.run(['codesign', '-f', '-s', '-', str(bundle)], check=True)
subprocess.run(['codesign', '--verify', '--deep', '--strict', str(bundle)], check=True)
print(bundle)
