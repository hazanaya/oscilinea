# 0006 - Signed, notarized .pkg for distribution

## Status

Accepted (2026-06-19)

## Context

The first betas were distributed (or prepared) as an ad-hoc-signed zip with a
`.command` install script that stripped quarantine and asked testers to
Control-click to bypass Gatekeeper. With an Apple Developer Program membership
now available, we can sign and notarize properly. We also need an update path
that is simple for non-technical friends and survives indefinitely as feedback
leads to new builds.

## Decision

Distribute Oscilinea as a single **Developer ID-signed, notarized `.pkg`
installer**, produced by `scripts/release.sh`.

- All three formats (AU, VST3, Standalone) are signed with **Developer ID
  Application** and the **Hardened Runtime**, then bundled into one product
  `.pkg` signed with **Developer ID Installer** and notarized with `notarytool`,
  ticket stapled.
- The installer targets **system locations** (`/Library/Audio/Plug-Ins/...`,
  `/Applications`). Asking for an admin password once is the expected installer
  experience and is more reliable for DAW scanning than per-user installs.
- Updates are delivered by handing over a newer `.pkg`; the user double-clicks
  it and it replaces the previous version in place. No auto-updater (Sparkle)
  was added: it only helps the standalone, not the plug-ins inside a DAW, and
  adds hosting + key-management overhead not justified for a small test group.
- Distribution channel is direct hand-off (AirDrop / email / shared link); no
  hosting infrastructure or appcast is maintained.

## Consequences

- Friends get a no-warning, double-click install and update experience.
- The plug-in identity codes (`Sshp` / `Hzny`, bundle id, parameter IDs) must
  stay frozen so updates load existing projects — see
  [0002-plugin-identity.md](0002-plugin-identity.md).
- Two new pre-distribution requirements are codified: the Hardened Runtime and,
  for the standalone, the microphone usage string + audio-input entitlement
  (without which it crashes on launch on modern macOS).
- One-time setup (Developer ID certificates + a stored notarization profile) is
  required on the build machine; documented in
  [docs/DISTRIBUTION.md](../DISTRIBUTION.md).
- The older `scripts/package.sh` (unsigned zip) remains only as a quick local
  packaging aid; `scripts/release.sh` is the real distribution path.
