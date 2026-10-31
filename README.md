# Oscilinea

![Oscilinea trace](assets/oscilinea-feature-shape.png)

A focused macOS audio visualizer plug-in that renders the living shape of a sound — and makes that display easy to record or capture in streaming and video tools.

**Downloads live in [Releases](../../releases).**

Load Oscilinea on a track and it draws one steady, period-locked contour that morphs with oscillator shape, filtering, PWM, distortion, and dynamics. The audio passes through untouched — no processing, no latency, no tonal change. The display is the product.

## Highlights

- **One focused display.** Period-seeking mono trace with cycle averaging and graceful fallback for noisy or polyphonic sources.
- **Capture-first workflow.** Built-in `.mov` recording with the plug-in's audio (H.264, or ProRes 4444 with alpha for transparent backgrounds), a dedicated source window for Window Capture in streaming tools, and per-instance [Syphon](https://syphon.github.io/) output for live visual apps — Syphon support is built in, nothing extra to install.
- **Quiet, system-style controls.** The panel stays hidden until you reveal it from a subtle bottom handle; the trace stands alone.
- **Your look, by default.** Save the current appearance as the default for new instances without touching existing sessions.
- **Three formats.** Audio Unit for Logic Pro and GarageBand, VST3, and a standalone app — pick any or all in the installer.

## Requirements

- macOS 11 (Big Sur) or later
- Apple silicon and Intel (universal binary)

## Install

Download the `.pkg` from [Releases](../../releases) and double-click it. The installer is signed and notarized by Apple — no security warnings. Choose which formats to install (AU, VST3, Standalone app); updates install the same way, right over the previous version, and existing DAW sessions keep working.

In your DAW, Oscilinea appears under the **Haza** manufacturer folder.

## Status

Oscilinea is in beta. If something misbehaves, [open an issue](../../issues) — bug reports and ideas are very welcome.

It is also free and open-source software: the full source is in this repository under the [AGPLv3](LICENSE). See [License](#license) below.

---

# For developers

## Building from source

Requires macOS 11+, Xcode command-line tools, and CMake 3.22+.

```bash
git clone https://github.com/hazanaya/oscilinea.git
cd oscilinea
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
`dist/`.

**Shipping an update** (the loop after each round of feedback):

1. Make changes; bump `project(Oscilinea VERSION x.y.z ...)` in `CMakeLists.txt` for a notable release.
2. Run `./scripts/release.sh`.
3. Send the new `.pkg` from `dist/`. Friends double-click it to install or update — no security warnings, and existing DAW projects keep working.

Certificates and notarization are set up once and never repeated. First-time
setup and the full runbook are in [docs/DISTRIBUTION.md](docs/DISTRIBUTION.md).

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
├── LICENSE            # AGPLv3
├── TRADEMARKS.md
├── AGENTS.md
├── DESIGN.md
├── HANDOFF.md
├── CHANGELOG.md
├── docs/
│   ├── ARCHITECTURE.md
│   ├── DISTRIBUTION.md
│   ├── ROADMAP.md
│   └── decisions/
├── assets/            # README images
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

Oscilinea is free and open-source software under the **GNU Affero General Public
License, version 3** ([LICENSE](LICENSE)). Copyright (c) 2026 Jorge Hazael Anaya.
Decision and reasoning: [ADR 0009](docs/decisions/0009-open-source-under-agplv3.md).

- You may use, study, modify, and redistribute it, including modified versions,
  as long as your version is also released under the AGPLv3 with its source.
- The name "Oscilinea", its icon, and the "Haza" manufacturer name are
  trademarks and are not licensed; see [TRADEMARKS.md](TRADEMARKS.md). Forks need
  their own name, icon, and plug-in identity codes.
- Third-party code: [JUCE 8](extern/JUCE/LICENSE.md) is used under its AGPLv3
  option; [Syphon-Framework](extern/Syphon-Framework/License.txt) is under a
  BSD-style license. Both ship as git submodules and keep their own notices.
- Contributions are welcome in principle but will be asked for a short
  contributor agreement before merging, so the author keeps the right to
  relicense his own work (ADR 0009).
