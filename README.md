# Oscilinea

macOS audio plug-in (AU / VST3 / Standalone) that turns incoming audio into one focused, period-locked Oscilinea display. Built on JUCE 8.

## Status

Early standalone spin-off from the BasicOsc shape view. The audio path is pass-through; the UI is focused on display, capture, and streaming/video-tool use rather than a full oscilloscope feature set.

## Quick Start

Requires macOS 11+, Xcode command-line tools, and CMake 3.22+.

```bash
git clone <this-repo>
cd <repo-folder>
git submodule update --init --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
```

Artifacts land in `build/Oscilinea_artefacts/Release/`:

- `AU/Oscilinea.component`
- `VST3/Oscilinea.vst3`
- `Standalone/Oscilinea.app`

With `COPY_PLUGIN_AFTER_BUILD` enabled, the AU and VST3 are also copied into the user plug-in folders.

Validate the AU:

```bash
auval -v aufx Sshp Hzny
```

## Distribution

To produce a signed, notarized installer to share with others:

```bash
./scripts/release.sh
```

This builds a universal Release, signs every format with your Developer ID,
wraps them in a single `.pkg`, notarizes it with Apple, and writes the result to
`dist/`. Friends double-click the `.pkg` to install (no security warnings) and
double-click a newer one to update. First-time certificate setup and the full
runbook are in [docs/DISTRIBUTION.md](docs/DISTRIBUTION.md).

`scripts/package.sh` produces the older unsigned zip and is kept only for quick
local testing.

## Capture

- **Record** starts immediately and turns into a red running timecode while armed. Press it again and the completed `.mov` asks where it should be saved. If you cancel that save dialog, the file remains in `~/Movies/Oscilinea Captures`.
- Multiple plug-in instances can record at the same time; each recording uses its own temporary `.mov` before the save prompt.
- Recordings include AAC audio for the signal passing through Oscilinea. They do not capture the full host mix unless that mix is routed through the plug-in.
- **Source Window** opens a separate live window named `Oscilinea Source`, intended for Window Capture/source capture in streaming and video tools.
- **External Source** starts a per-instance Syphon output named `Oscilinea N`, intended for direct capture in Syphon-compatible live visual tools. It runs while that plug-in editor is open and the control is started.
- New instances start with a clean trace over a transparent background: white in dark mode, dark neutral in light mode.
- The Look row menu can save the current appearance as the default for future new plug-in instances, or reset to the built-in appearance defaults. Existing saved sessions keep their saved state.
- **Transparent** backgrounds are available for the display. Transparent recordings use a ProRes 4444 `.mov` so alpha can survive export; opaque recordings use H.264.
- The panel can be hidden and revealed with a subtle centered bottom handle.
- A true virtual camera/video-device output is not implemented in v0.1; Syphon is available as the direct live-output bridge.

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
│   ├── DISTRIBUTION.md
│   ├── ROADMAP.md
│   └── decisions/
├── packaging/
│   ├── entitlements/
│   └── resources/
├── scripts/
│   ├── release.sh        # signed + notarized .pkg installer
│   └── package.sh        # unsigned zip (local testing)
├── extern/JUCE/
├── extern/Syphon-Framework/
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
- [Distribution](docs/DISTRIBUTION.md)
- [Roadmap](docs/ROADMAP.md)
- [Handoff](HANDOFF.md)
- [Decisions](docs/decisions/)

## License

See [LICENSE](LICENSE). JUCE is linked under its own separate license.
