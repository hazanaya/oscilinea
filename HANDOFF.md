# Handoff

Last updated: 2026-05-21

## Where Things Stand

This project is an independent Sound Shape plug-in folder. It uses BasicOsc only as reference material and should not write into that repo.

Current scaffold includes:

- JUCE 8 CMake plug-in target: AU, VST3, and Standalone.
- Pass-through `PluginProcessor` with `ScopeFeed` audio-to-UI handoff.
- Focused Sound Shape display with mono period detection, cycle averaging, smoothing, and fallback drawing.
- System-style bottom panel for display controls, recording, and an OBS source window.
- macOS AVFoundation `.mov` recorder.
- Docs, git metadata, license, and project handoff files.

Verification:

- Release build succeeds for AU, VST3, and Standalone.
- Build copies AU and VST3 to the user plug-in folders.
- `auval -v aufx Sshp Hzny` passes.
- `pluginval` was not available in `PATH`.
- Repository scan contains no forbidden hardware-reference terms.

## Next Steps

1. Smoke test Standalone: confirm audio pass-through, Record writes a playable `.mov`, and OBS Window opens and updates.
2. Install `pluginval` if needed and validate the VST3.
3. Decide whether v0.2 should add Syphon/NDI/virtual-camera output beyond OBS Window Capture.

## Gotchas

- Do not change `PLUGIN_CODE=Sshp`, `PLUGIN_MANUFACTURER_CODE=Hzny`, or `BUNDLE_ID=com.hazaelanaya.soundshape` after release.
- Do not rename parameter IDs once sessions may exist in user hosts.
- Recording runs on the UI thread. Keep it out of `processBlock`.
- The OBS source window is a practical capture surface, not a true system video device.
