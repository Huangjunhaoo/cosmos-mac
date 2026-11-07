# Cosmos for macOS

Cosmos is an unofficial macOS implementation of the SOMA COSMOS drifting-memory looper. Record a phrase, then let it drift through stereo delays, granular textures or rhythmic repeats. Use the standalone app with an audio interface, or insert the Audio Unit on a track in Ableton Live or Logic Pro.

**[Download Cosmos for Mac](https://github.com/Huangjunhaoo/cosmos-mac/releases/latest/download/Cosmos-Final-Standalone-0.16.3-AU-0.1.12-Mac.zip)** · **[Installation guide in 15 languages](https://github.com/Huangjunhaoo/cosmos-mac/releases/latest/download/Cosmos-Installation-15-languages.pdf)**

**中文**　下载完整 ZIP，解压后打开 `00 - Install Cosmos.pdf`，点选语言即可查看安装步骤。完整包还包含独立版、AU 插件和 MIDI 踏板说明书。项目目录中的源码可供构建和修改。

| Edition | Frozen version | Mac requirements |
| --- | --- | --- |
| Standalone | 0.16.3 | macOS 13+ on Apple silicon or Intel |
| Audio Unit (AUv2) | 0.1.12 | macOS 13+ on Apple silicon or Intel |

Both editions share 128 named preset slots. Open the right sidebar, select a slot, then click Load. Use MIDI Program Change 0–127 to recall a saved sound with the sidebar closed. PC 0 is the default. In Live, route the MIDI track to the Cosmos AU and match its preset input channel. Find the AU under **Chicken Chukka → Cosmos**.

### Play

- Select a firmware family and one of twelve programs. The front/rear mode has separate controls and recording for each engine, with stereo mixdown or separate outputs.
- For a single microphone or guitar, choose MISO in the standalone app for stereo output.
- Learn a pedal in the standalone app, use MIDI Map in Ableton Live, or use Controller Assignments in Logic Pro. The installer ZIP includes the corresponding guides.

### Build from source

Install the Xcode Command Line Tools (Clang and the macOS SDK) and Python 3 on a Mac. From the repository root run:

```sh
python3 build_app.py         # dist/COSMOS-0.16.3.app
python3 build_au_local.py    # dist/Cosmos.component
```

Both builds target Apple silicon and Intel. The scripts use local signatures. Apple has not notarized the downloadable release. The bundled MIDI PDFs are already in the source tree, so building the app or plug-in does not require regenerating them, a firmware download, or connecting hardware.

### License and provenance

The project's original software is offered under **GNU GPL version 3 only** (`GPL-3.0-only`). See [LICENSE](LICENSE) for the terms and [THIRD-PARTY.md](THIRD-PARTY.md) for the firmware-derived work and graphic assets.

[Development notes](docs/DEVELOPMENT-HISTORY.md) · [Release manifest](release-manifest.json)
