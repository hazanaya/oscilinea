# Handoff

Last updated: 2026-08-01

## Where Things Stand

This project is an independent Oscilinea plug-in folder. It uses BasicOsc only as reference material and should not write into that repo.

Current scaffold includes:

- JUCE 8 CMake plug-in target: AU, VST3, and Standalone.
- Pass-through `PluginProcessor` with `ScopeFeed` audio-to-UI handoff and a recording-only audio FIFO.
- Focused Oscilinea display with mono period detection, cycle averaging, smoothing, and fallback drawing.
- Three-row system-style bottom panel: Signal, Look, and Record. It includes compact color dots, fixed-position advanced controls, light/dark-safe captions, value text, and source controls, expanded line styling, X/Y display flipping, extended cycle-span glide control, recording, a generic source window, and a per-instance Syphon External Source output. The panel is initially hidden and can be revealed with a centered bottom handle and generous hover target.
- Default appearance is a clean trace over a transparent background with no trail. Dark mode defaults to a white trace; light mode defaults to a dark neutral trace for contrast.
- The Look row has a compact defaults menu. It can save the current appearance as the default for future new plug-in instances or reset back to built-in appearance defaults. Saved host sessions still reopen from their saved state.
- macOS AVFoundation `.mov` recorder. Record starts immediately, shows a red running timecode while armed, and asks where to save after recording ends. Opaque captures use H.264; transparent-background captures request ProRes 4444 for alpha. Recordings mux AAC audio for the signal passing through the plug-in.
- Each recording uses a UUID-backed temporary filename in `~/Movies/Oscilinea Captures`, so separate plug-in instances can record at the same time without colliding.
- Syphon is pinned as `extern/Syphon-Framework` and compiled into the macOS build as a server-side source publisher. External Source runs from the editor vblank loop and stops when that editor closes.
- Build, bundle, and user-facing identity all use Oscilinea. The plug-in identity codes stay `Sshp`/`Hzny`, and internal C++ names deliberately keep the original `SoundShape` working name.
- The manufacturer display name is "Haza" (ADR 0007) — the DAW menu folder Oscilinea and all future plug-ins group under. Hosts list it as "Haza: Oscilinea". Identity codes and bundle ID are unchanged.
- `scripts/release.sh` builds the real distributable: a Developer ID-signed, notarized `.pkg` installer in `dist/` (build → sign with Hardened Runtime → pkg → notarize → staple). The standalone carries a microphone usage string + audio-input entitlement so it survives the Hardened Runtime. See `docs/DISTRIBUTION.md`.
- `scripts/package.sh` remains as the older unsigned zip path, kept only for quick local testing.
- Docs, git metadata, license, and project handoff files.

Verification:

- Clean universal Release build succeeds for AU, VST3, and Standalone under the Oscilinea name.
- Build copies AU and VST3 to the user plug-in folders; stale Sound Shape copies were removed from this machine.
- `auval -v aufx Sshp Hzny` passes.
- `pluginval --strictness-level 5` passes on the VST3 (pluginval 1.0.4).
- Repository scan contains no forbidden hardware-reference terms.
- Developer ID signing is set up on the build machine (paid team `8GPYZ947VY`; both Developer ID certs valid to 2031; `notarytool` profile `oscilinea-notary` stored). Private signing keys are backed up at `~/oscilinea-signing/` (machine-local, not in git).
- The first signed + notarized installer was built end to end: `./scripts/release.sh` produced `dist/Oscilinea-…-a2fa8dd.pkg`, notarization was Accepted and stapled, and `spctl --assess --type install` reports `source=Notarized Developer ID`.
- `otool -L` confirms AU, VST3, and Standalone link only macOS system frameworks (Syphon is statically linked); end users need nothing beyond the installer.
- The `de765f9` installer was run end to end on this machine with all three components selected: AU, VST3, and Standalone landed in `/Library/Audio/Plug-Ins` and `/Applications`, receipts and signatures verified, `auval` and `pluginval` pass on the installed copies, and the AU registers under the "Haza" manufacturer folder. The installer's component selection screen and the non-relocatable pinning were both exercised for real (an earlier install had relocated the app onto a stale `~/Applications` copy; that copy is deleted).

## Next Steps

1. Smoke test Standalone/Logic: confirm audio pass-through, the initially hidden panel toggle is discoverable, Glide ranges from instant cycle-span changes to long transitions, line-style changes update preset values without moving controls, Clean and Ink retain their expected looks, the extra line presets and wider Weight range feel useful, Flip mirrors the editor/source/capture display as Off, X, Y, and X+Y, Record starts instantly with a running timecode, ending recording prompts for a destination and writes a playable `.mov` with audio, Transparent records alpha as expected, Source Window opens and updates, and External Source appears in Syphon-compatible tools for multiple plug-in instances. Also confirm the standalone's first-launch microphone prompt appears and is accepted.
2. Send the first Oscilinea beta installer (`dist/Oscilinea-0.0.1-beta.20260801-de765f9.pkg`) once smoke testing looks good. No beta has ever been distributed; identity values, parameter IDs, and the "Haza" manufacturer name freeze once one is.
3. Future releases are one command: `./scripts/release.sh` (see `docs/DISTRIBUTION.md` → "The update loop"). Bump `project(Oscilinea VERSION …)` for notable releases.
4. Decide whether v0.2 should add NDI or a true virtual-camera output beyond Syphon/source capture.

## Gotchas

- Do not change `PLUGIN_CODE=Sshp`, `PLUGIN_MANUFACTURER_CODE=Hzny`, or the Oscilinea bundle ID after release.
- `COMPANY_NAME "Haza"` freezes once the first beta ships; future plug-ins must use the identical string to share the DAW menu folder.
- Do not rename parameter IDs once sessions may exist in user hosts.
- New-instance appearance defaults are external preference data, not plug-in parameters. They are applied at construction and then host-saved state can override them.
- Recording runs on the UI thread. Keep it out of `processBlock`.
- The audio thread only copies into `AudioCaptureBuffer` while recording; keep AVFoundation and file work on the message thread.
- Transparent recording uses ProRes 4444. Do not attach H.264-only compression properties, such as average bitrate, to the ProRes writer input; AVFoundation throws an Objective-C exception for that.
- Do not delete an existing output path when starting a recording. Concurrent instances must never be able to remove another instance's active temp file.
- The source window and Syphon output are practical capture surfaces, not true system video devices.
- External Source is editor-driven. Closing the plug-in editor tears down that instance's Syphon server.
- CMake `target_sources` paths use the internal `SoundShape` file names; a blanket SoundShape-to-Oscilinea replace breaks the build.
- Recorder stop waits at most ten seconds for AVFoundation to finalize, then fails with a normal recording error instead of hanging the editor.
- "Apple Developer ID" is the membership; distribution needs separate **Developer ID Application** and **Developer ID Installer** certificates, which are not the auto-created "Apple Development" cert. Notarization needs Hardened Runtime on every binary plus a secure timestamp (internet required).
- The release `.pkg` installs system-wide, so it prompts once for an admin password. That is expected installer behavior, not a regression from the old per-user `INSTALL.command`.
- Keep `scripts/release.config` (Developer ID overrides) and any `.cer`/`.p12`/`.csr`/key files out of git; `.gitignore` already excludes them.
- Component pkgs must stay pinned with `BundleIsRelocatable=false`. Without it, Installer finds any existing copy by bundle ID (old `~/Applications` install, build tree, unzipped release) and updates that copy in place instead of installing to the intended location.
- macOS App Management (TCC) blocks command-line moves/deletes of installed app bundles; stray app copies have to be removed via Finder.
