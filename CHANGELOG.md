# Changelog

All notable changes to this project will be documented in this file.

## Unreleased

### Changed

- License: the source is now released under the GNU Affero General Public
  License v3 (was All Rights Reserved). JUCE 8 is used under its AGPLv3 option;
  Syphon-Framework stays under its BSD-style license. See
  [ADR 0009](docs/decisions/0009-open-source-under-agplv3.md).

### Added

- `TRADEMARKS.md`: the Oscilinea name and icon and the "Haza" manufacturer name
  are not part of the open-source license.

## 0.0.1-beta.20260801 — First public beta

Released 2026-08-01 via GitHub Releases
([hazanaya/oscilinea](https://github.com/hazanaya/oscilinea/releases/tag/v0.0.1-beta.20260801))
and [hazaelanaya.com/oscilinea](https://www.hazaelanaya.com/oscilinea).

### Fixed

- Pinned installer components as non-relocatable, so the installer always
  installs to `/Applications` and `/Library/Audio/Plug-Ins` instead of
  silently updating a stray older copy found elsewhere on the disk.

### Changed

- The installer now always shows the component selection screen, so people can
  check or uncheck the Audio Unit, VST3, and Standalone app individually
  instead of the choices hiding behind a "Customize" button.
- Renamed the plug-in manufacturer display name from "Hazael Anaya" to "Haza",
  so Oscilinea and future plug-ins group under a Haza folder in DAW plug-in
  menus. Identity codes (`Hzny`, `Sshp`) and the bundle ID are unchanged.

### Added

- Added an app icon: a glowing wave-packet trace on a dark squircle, generated
  by `packaging/icon/make-icon.swift` and wired in via JUCE's `ICON_BIG`.
- Added `scripts/release.sh`, a one-command builder that produces a Developer
  ID-signed, notarized `.pkg` installer for direct hand-off, with `--dry-run`,
  `--skip-build`, and `--skip-notarize` modes.
- Added `docs/DISTRIBUTION.md` with one-time certificate/notarization setup, the
  per-release runbook, and the install/update experience for end users.
- Added Hardened Runtime entitlements for the standalone and friendly installer
  screens (`packaging/`), plus an ADR for signed `.pkg` distribution.
- Scaffolded the independent Oscilinea JUCE plug-in project.
- Added pass-through AU/VST3/Standalone build target with its own plug-in identity.
- Added focused Oscilinea display, system-style control panel, `.mov` capture, and source-window capture support.
- Added a post-recording save dialog, trace/background color pickers, transparent background controls, ProRes 4444 alpha recording, and line style controls.
- Refined the panel UX with capture-first defaults, neutral recording chrome, line-style presets, no rounded endpoint dots, dynamic advanced controls, and an autohiding panel toggle.
- Polished the panel into Signal, Look, and Record rows with quieter accent chrome, compact color dots, fixed disabled advanced controls, an Aspect label, two-decimal slider readouts, and recording timecode.
- Added section dividers, aligned the Record row control height with the rest of the panel, and moved the hover-only panel chevron farther from the panel edge.
- Smoothed large period changes so the Oscilinea view glides between cycle spans instead of snapping between zoom levels.
- Added a Cycle Glide parameter and Signal-row Glide control, now ranging from instant cycle-span changes to 12-second transitions.
- Fixed a host crash when starting transparent recordings by using ProRes-safe AVFoundation settings and converting encoder exceptions into normal recording errors.
- Added AAC audio muxing to `.mov` recordings for the audio passing through the plug-in.
- Replaced the panel chevron with a centered bottom handle and larger hover target.
- Added a light-mode trace default so transparent displays stay visible on light system backgrounds.
- Added Look-row actions to save or reset new-instance appearance defaults across plug-in instances.
- Made the panel handle hide after a short sustained hover.
- Fixed the panel handle hover timeout so the line fully disappears and the hover area tracks the line position.
- Forced the panel handle timeout to clear stale pixels by hiding the whole handle component and repainting its parent area.
- Moved the panel handle line into the editor overlay so the invisible hit target never leaves a stale line behind.
- Restored the panel handle to the chevron-era alpha visibility model with a centered line inside the wider hit target.
- Made recording temp files UUID-backed and prevented recorder start-up from deleting pre-existing destinations, so multiple instances can record concurrently without filename collisions.
- Matched control captions and slider value text to the section labels so they remain readable in light mode.
- Fixed the source window button text color so it remains readable in light mode.
- Added a per-instance Syphon External Source output for direct capture in compatible live visual tools.
- Added a Look-row Flip control for cheap X, Y, or X+Y display mirroring in the rendered and captured output.
- Expanded the Line menu with additional presets while preserving Clean and Ink, widened Cycle Glide and trace Weight ranges, and made the control panel initially hidden.

### Changed

- Distribution moves from an ad-hoc-signed zip to a signed, notarized `.pkg`.
  The standalone installs to `/Applications` and the plug-ins to the system
  `/Library/Audio/Plug-Ins` locations; end users install and update by
  double-clicking the `.pkg`, with no Gatekeeper warnings.
- Renamed project documentation and product copy to Oscilinea after lightweight name research.
- Renamed the build targets, bundle ID, and user-facing strings to Oscilinea. The plug-in identity codes (`Sshp`/`Hzny`) and parameter IDs are unchanged; internal C++ names deliberately keep the original working name.
- New-instance appearance defaults now live in the `Oscilinea` application support folder.

### Fixed

- The standalone app now declares a microphone usage string paired with the
  audio-input entitlement, so it no longer crashes on launch when opening the
  audio input on modern macOS.
- Widened the scope feed write positions to 64-bit so displays stay correct through multi-day continuous sessions.
- Stopping a recording now gives up with an error after ten seconds instead of hanging the editor if the encoder never finishes.
