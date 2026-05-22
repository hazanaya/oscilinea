# Handoff

Last updated: 2026-05-21

## Where Things Stand

This project is an independent Sound Shape plug-in folder. It uses BasicOsc only as reference material and should not write into that repo.

Current scaffold includes:

- JUCE 8 CMake plug-in target: AU, VST3, and Standalone.
- Pass-through `PluginProcessor` with `ScopeFeed` audio-to-UI handoff and a recording-only audio FIFO.
- Focused Sound Shape display with mono period detection, cycle averaging, smoothing, and fallback drawing.
- Three-row system-style bottom panel: Signal, Look, and Record. It includes compact color dots, fixed-position advanced controls, line styling, cycle-span glide control, recording, and a generic source window. The panel can be hidden or revealed with a centered bottom handle and generous hover target.
- Default appearance is white clean trace, transparent background, no trail.
- macOS AVFoundation `.mov` recorder. Record starts immediately, shows a red running timecode while armed, and asks where to save after recording ends. Opaque captures use H.264; transparent-background captures request ProRes 4444 for alpha. Recordings mux AAC audio for the signal passing through the plug-in.
- Docs, git metadata, license, and project handoff files.

Verification:

- Release build succeeds for AU, VST3, and Standalone.
- Build copies AU and VST3 to the user plug-in folders.
- `auval -v aufx Sshp Hzny` passes.
- `pluginval` was not available in `PATH`.
- Repository scan contains no forbidden hardware-reference terms.

## Next Steps

1. Smoke test Standalone/Logic: confirm audio pass-through, the autohiding panel toggle is discoverable, Glide ranges from instant cycle-span changes to slow transitions, line-style changes update preset values without moving controls, Record starts instantly with a running timecode, ending recording prompts for a destination and writes a playable `.mov` with audio, Transparent records alpha as expected, and Source Window opens and updates.
2. Install `pluginval` if needed and validate the VST3.
3. Decide whether v0.2 should add Syphon/NDI/virtual-camera output beyond Window Capture/source capture.

## Gotchas

- Do not change `PLUGIN_CODE=Sshp`, `PLUGIN_MANUFACTURER_CODE=Hzny`, or `BUNDLE_ID=com.hazaelanaya.soundshape` after release.
- Do not rename parameter IDs once sessions may exist in user hosts.
- Recording runs on the UI thread. Keep it out of `processBlock`.
- The audio thread only copies into `AudioCaptureBuffer` while recording; keep AVFoundation and file work on the message thread.
- Transparent recording uses ProRes 4444. Do not attach H.264-only compression properties, such as average bitrate, to the ProRes writer input; AVFoundation throws an Objective-C exception for that.
- The source window is a practical capture surface, not a true system video device.
