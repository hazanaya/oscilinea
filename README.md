# Sound Shape

macOS audio plug-in (AU / VST3 / Standalone) that turns incoming audio into one focused, period-locked Sound Shape display. Built on JUCE 8.

## Status

Early standalone spin-off from the BasicOsc Sound Shape view. The audio path is pass-through; the UI is focused on display, capture, and streaming/video-tool use rather than a full oscilloscope feature set.

## Quick Start

Requires macOS 11+, Xcode command-line tools, and CMake 3.22+.

```bash
git clone <this-repo>
cd "004 Sound Shape"
git submodule update --init --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
```

Artifacts land in `build/SoundShape_artefacts/Release/`:

- `AU/Sound Shape.component`
- `VST3/Sound Shape.vst3`
- `Standalone/Sound Shape.app`

With `COPY_PLUGIN_AFTER_BUILD` enabled, the AU and VST3 are also copied into the user plug-in folders.

Validate the AU:

```bash
auval -v aufx Sshp Hzny
```

## Capture

- **Record** starts immediately and turns into a red running timecode while armed. Press it again and the completed `.mov` asks where it should be saved. If you cancel that save dialog, the file remains in `~/Movies/Sound Shape Captures`.
- Recordings include AAC audio for the signal passing through Sound Shape. They do not capture the full host mix unless that mix is routed through the plug-in.
- **Source Window** opens a separate live window named `Sound Shape Source`, intended for Window Capture/source capture in streaming and video tools.
- New instances start with a white clean trace over a transparent background.
- **Transparent** backgrounds are available for the display. Transparent recordings use a ProRes 4444 `.mov` so alpha can survive export; opaque recordings use H.264.
- The panel can be hidden; its toggle is intentionally invisible until hovered.
- A true virtual camera/video-device output is not implemented in v0.1; that requires a separate macOS video-device, Syphon, or NDI-style integration.

## Project Layout

```text
.
├── CMakeLists.txt
├── README.md
├── AGENTS.md
├── DESIGN.md
├── HANDOFF.md
├── CHANGELOG.md
├── docs/
│   ├── ARCHITECTURE.md
│   ├── ROADMAP.md
│   └── decisions/
├── extern/JUCE/
└── src/
    ├── dsp/
    ├── params/
    ├── scope/
    ├── ui/
    └── video/
```

## Docs

- [Design](DESIGN.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Roadmap](docs/ROADMAP.md)
- [Handoff](HANDOFF.md)
- [Decisions](docs/decisions/)

## License

See [LICENSE](LICENSE). JUCE is linked under its own separate license.
