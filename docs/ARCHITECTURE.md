# Architecture

## Stack

- **Language:** C++20, plus Objective-C++ for macOS video encoding.
- **Framework:** JUCE 8, pinned as `extern/JUCE`.
- **Build:** CMake 3.22+.
- **Formats:** AU, VST3, Standalone.
- **Platform:** macOS 11+ universal binary.
- **Capture:** AVFoundation `.mov` writer on macOS.

## Modules

```text
src/
├── PluginProcessor.{h,cpp}   - pass-through AudioProcessor; pushes audio into ScopeFeed
├── PluginEditor.{h,cpp}      - editor, vblank tick, recording/source-window orchestration
├── dsp/ScopeFeed.h           - lock-free latest-sample ring buffer
├── params/Parameters.{h,cpp} - AudioProcessorValueTreeState layout and serialization
├── scope/
│   ├── SoundShapeView.*      - period detection, cycle integration, phosphor image
│   └── SoundShapeRenderer.*  - background and trace drawing
├── ui/
│   ├── Controls.*            - system-style control strip and attachments
│   └── SystemLookAndFeel.*   - quiet macOS-like control drawing
└── video/
    ├── SourceWindow.*        - separate OBS Window Capture surface
    ├── VideoRecorder.h       - C++ recorder facade
    └── VideoRecorder_mac.mm  - AVFoundation writer
```

## Data Flow

```text
host audio buffer
  -> PluginProcessor::processBlock
      -> unchanged audio output
      -> ScopeFeed
  -> PluginEditor vblank
      -> SoundShapeView::tick
      -> repaint editor
      -> optional SourceWindow frame
      -> optional AVFoundation frame
```

## Threading

- `processBlock` is wait-free and allocation-free after `prepareToPlay`.
- `ScopeFeed` writes on the audio thread and reads on the message thread.
- Pitch detection, smoothing, rendering, recording, and OBS-window updates run on the message thread.
- AVFoundation recording is intentionally not a real-time audio responsibility.

## Capture Model

The v0.1 capture strategy has two layers:

- **Recording:** the editor snapshots the rendered display at the selected frame rate and writes `.mov` through AVFoundation. Opaque captures use H.264; transparent-background captures use ProRes 4444 so alpha can be preserved.
- **OBS use:** the source window mirrors the display in a normal native window. OBS and similar tools can capture it via Window Capture.

A true video-device source is deferred because it requires a macOS virtual camera, Syphon, NDI, or similar IPC layer outside ordinary AU/VST plug-in behavior.

## External Dependencies

JUCE is the only source dependency:

```bash
git submodule update --init --recursive
```

The macOS SDK supplies AVFoundation, CoreMedia, and CoreVideo.
