# Sound Shape - Design Document

## One-Line Pitch

A focused macOS audio visualizer plug-in that renders the living shape of a sound and makes that display easy to record or capture in OBS.

## Vision

Sound Shape is the display-only sibling of BasicOsc. It removes the full oscilloscope surface and keeps the thing that feels most useful while designing synth sounds: a steady, readable contour that morphs with oscillator shape, filtering, PWM, distortion, and dynamics.

The panel should feel like system UI, not a themed hardware panel. The display can be expressive; the controls should be quiet, familiar, and fast.

## Goals

- **Pass-through audio.** No processing, no latency, no tonal change.
- **Focused shape view.** One period-seeking mono display with robust fallback for noisy or polyphonic sources.
- **Capture-first workflow.** Built-in `.mov` recording and a separate window suitable for OBS Window Capture.
- **Independent project.** Own source tree, git history, docs, plug-in codes, and build artifacts.
- **Mac-first implementation.** AU, VST3, and Standalone from one JUCE 8 CMake project.

## Non-Goals

- No multi-mode oscilloscope, vectorscope, polar display, or engineering measurement UI.
- No virtual camera driver in v0.1.
- No audio effects, metering plug-in suite, or preset library.
- No Windows/Linux support in v0.1.

## Constraints

- macOS 11+ universal binary (`arm64;x86_64`).
- JUCE 8 pinned as a git submodule.
- Audio thread never records, paints, allocates, locks, or opens UI.
- Plug-in codes and parameter IDs are immutable after release.
