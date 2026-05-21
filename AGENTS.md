# Agents - Instructions for AI Coding Tools

This is a standalone JUCE plug-in project. Read this file before editing code.

## Read Before Code Changes

1. [DESIGN.md](DESIGN.md) - product intent, scope, and non-goals.
2. [HANDOFF.md](HANDOFF.md) - current state and next steps.
3. [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) - modules, threading, build, and capture design.
4. [docs/decisions/](docs/decisions/) - decisions that should not be casually reversed.

## Working Conventions

- Keep this project independent from `BasicOsc`. You may read `BasicOsc` as reference, but do not write into it while working here.
- Preserve the plug-in identity codes once released. `PLUGIN_CODE`, `PLUGIN_MANUFACTURER_CODE`, `BUNDLE_ID`, and parameter IDs are host/session contracts.
- The audio thread is pass-through only. Do not allocate, lock, log, access files, record video, open windows, or trigger repaint from `processBlock`.
- The only audio-to-UI handoff is `SoundShape::ScopeFeed`, a lock-free latest-sample ring buffer.
- UI capture, video recording, and source-window updates run on the message thread from the editor vblank callback.
- Update [CHANGELOG.md](CHANGELOG.md) for user-visible changes and [HANDOFF.md](HANDOFF.md) at the end of a session.
- Add a short ADR in `docs/decisions/` for non-trivial architecture or product decisions.

## Build

```bash
git submodule update --init --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
```

## Validation

```bash
auval -v aufx Sshp Hzny
pluginval --strictness-level 5 build/SoundShape_artefacts/Release/VST3/Sound\\ Shape.vst3
```
