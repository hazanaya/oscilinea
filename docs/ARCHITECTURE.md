# Architecture

## Stack

- **Language:** C++20, plus Objective-C++ for macOS video encoding.
- **Framework:** JUCE 8, pinned as `extern/JUCE`.
- **Build:** CMake 3.22+.
- **Formats:** AU, VST3, Standalone.
- **Platform:** macOS 11+ universal binary.
- **Capture:** AVFoundation `.mov` writer on macOS, with AAC audio for the plug-in input.
- **Live output:** Syphon server-side source, pinned as `extern/Syphon-Framework` and compiled into the plug-in on macOS.

## Modules

```text
src/
├── PluginProcessor.{h,cpp}   - pass-through AudioProcessor; pushes audio into UI/capture buffers
├── PluginEditor.{h,cpp}      - editor, vblank tick, recording/source-window orchestration
├── dsp/AudioCaptureBuffer.h  - recording-only audio FIFO drained by the editor
├── dsp/ScopeFeed.h           - lock-free latest-sample ring buffer
├── params/AppearanceDefaults.* - external new-instance appearance preferences
├── params/Parameters.{h,cpp} - AudioProcessorValueTreeState layout and serialization
├── scope/
│   ├── SoundShapeView.*      - period detection, cycle integration, phosphor image
│   └── SoundShapeRenderer.*  - background and trace drawing
├── ui/
│   ├── Controls.*            - system-style control strip and attachments
│   └── SystemLookAndFeel.*   - quiet macOS-like control drawing
└── video/
    ├── SourceWindow.*        - separate source-capture surface
    ├── SyphonPublisher.*     - per-editor Syphon external source publisher
    ├── VideoRecorder.h       - C++ recorder facade
    └── VideoRecorder_mac.mm  - AVFoundation writer
```

## Data Flow

```text
host audio buffer
  -> PluginProcessor::processBlock
      -> unchanged audio output
      -> ScopeFeed
      -> optional AudioCaptureBuffer
  -> PluginEditor vblank
      -> SoundShapeView::tick
      -> repaint editor
      -> optional SourceWindow frame
      -> optional Syphon frame
      -> optional AVFoundation video frame + audio samples
```

## Threading

- `processBlock` is wait-free and allocation-free after `prepareToPlay`.
- `ScopeFeed` writes on the audio thread and reads on the message thread.
- `AudioCaptureBuffer` writes bounded audio copies on the audio thread and is drained on the message thread while recording.
- Pitch detection, smoothing, rendering, source-window updates, Syphon publishing, and recording run on the message thread.
- AVFoundation encoding and file I/O are intentionally not real-time audio responsibilities.

## Capture Model

The v0.1 capture strategy has two layers:

- **Recording:** the editor snapshots the rendered display at the selected frame rate and writes `.mov` through AVFoundation. Opaque captures use H.264; transparent-background captures use ProRes 4444 so alpha can be preserved. Audio passing through the plug-in is muxed as AAC.
- **External capture use:** the source window mirrors the display in a normal native window. Streaming and video tools can capture it via Window Capture/source capture.
- **Direct live source:** each open editor can start its own Syphon output from the Record row's External Source control. The source uses the current capture FPS and mirrors the rendered display, including transparent-background alpha. Syphon publishing is editor-driven; closing the plug-in editor stops that editor's source.

A true video-device source is still deferred because it requires a macOS virtual camera or NDI-style system integration outside ordinary AU/VST plug-in behavior.

## Appearance Defaults

The Look row can save current appearance values as defaults for future new plug-in instances. These defaults are external preference data in the user application support folder, not automatable plug-in parameters and not part of host project state. They are applied when a processor is constructed; if a host later restores a saved session state, that saved state replaces them.

## External Dependencies

JUCE and Syphon are source dependencies:

```bash
git submodule update --init --recursive
```

The macOS SDK supplies AVFoundation, AudioToolbox, Cocoa, CoreMedia, CoreVideo, IOSurface, and OpenGL.
