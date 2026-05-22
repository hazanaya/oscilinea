# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Added

- Scaffolded the independent Sound Shape JUCE plug-in project.
- Added pass-through AU/VST3/Standalone build target with its own plug-in identity.
- Added focused Sound Shape display, system-style control panel, `.mov` capture, and source-window capture support.
- Added a post-recording save dialog, trace/background color pickers, transparent background controls, ProRes 4444 alpha recording, and line style controls.
- Refined the panel UX with capture-first defaults, neutral recording chrome, line-style presets, no rounded endpoint dots, dynamic advanced controls, and an autohiding panel toggle.
- Polished the panel into Signal, Look, and Record rows with quieter accent chrome, compact color dots, fixed disabled advanced controls, an Aspect label, two-decimal slider readouts, and recording timecode.
- Added section dividers, aligned the Record row control height with the rest of the panel, and moved the hover-only panel chevron farther from the panel edge.
- Smoothed large period changes so the Sound Shape view glides between cycle spans instead of snapping between zoom levels.
- Added a Cycle Glide parameter and Signal-row Glide control, ranging from instant cycle-span changes to 4-second transitions.
- Fixed a host crash when starting transparent recordings by using ProRes-safe AVFoundation settings and converting encoder exceptions into normal recording errors.
- Added AAC audio muxing to `.mov` recordings for the audio passing through the plug-in.
- Replaced the panel chevron with a centered bottom handle and larger hover target.
- Added a light-mode trace default so transparent displays stay visible on light system backgrounds.
- Added Look-row actions to save or reset new-instance appearance defaults across plug-in instances.
- Made the panel handle hide after a short sustained hover.
- Fixed the panel handle hover timeout so the line fully disappears and the hover area tracks the line position.
- Forced the panel handle timeout to clear stale pixels by hiding the whole handle component and repainting its parent area.
