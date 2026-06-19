# Distributing Oscilinea

This is how Oscilinea gets from your Mac to a friend's Mac as a clean,
double-click installer with no security warnings — and how you ship updates
later without it being complicated for them.

The whole release is one command:

```bash
./scripts/release.sh
```

That builds a universal Release, code-signs it with your **Developer ID**,
wraps it in a `.pkg` installer, **notarizes** it with Apple, staples the
result, and drops the finished installer in `dist/`. You hand that one `.pkg`
file to friends (AirDrop, email, a shared link — your call).

---

## The update loop (every time you ship a new version)

This is the routine you repeat after a friend gives feedback and you make
changes. Nothing to memorize beyond these three steps:

1. *(Optional, for a notable release)* bump the version in `CMakeLists.txt` —
   the line `project(Oscilinea VERSION x.y.z ...)`.
2. Run `./scripts/release.sh` and wait a few minutes.
3. Send the new `.pkg` from `dist/`. Friends **double-click it to update** — it
   replaces the old version in place and their DAW projects keep working.

You never redo certificates or notarization; that setup is one-time (see below)
and the certificates are valid until 2031. Friends need **nothing else** — no
Syphon, no extra downloads; everything is built into the installer. Full detail
and command options are in [Shipping a release](#shipping-a-release-every-time)
further down.

---

## Background: the two halves of "Apple Developer ID"

Joining the Apple Developer Program gives you a **membership**. It does *not*
automatically give you the certificates that distribution needs. There are
several certificate types and only two of them are for shipping to other people:

| Certificate | What it's for | You need it? |
|---|---|---|
| **Apple Development** | Run *your* apps on *your* registered devices | Already in your keychain; **not** usable for distribution |
| **Developer ID Application** | Sign apps/plug-ins shipped *outside* the App Store | **Yes** |
| **Developer ID Installer** | Sign `.pkg` installers shipped *outside* the App Store | **Yes** |

**Notarization** is a separate step: after signing, you upload the build to
Apple, an automated service scans it for malware, and returns a "ticket". You
"staple" that ticket onto the installer. Once notarized, Gatekeeper on any Mac
lets it open with no "unidentified developer" or "damaged" warnings.

Your distribution Team ID is `8GPYZ947VY` (your paid membership, shown in the
Developer portal). Note: an older `Apple Development` certificate in your keychain
belongs to a different, free "personal team" (`J2BHY3YDKL`) created by Xcode — that
one cannot sign for distribution, so always use `8GPYZ947VY`.

---

## One-time setup

You do this once. After that, every release is just `./scripts/release.sh`.

### 1. Create the two Developer ID certificates

1. Go to <https://developer.apple.com/account/resources/certificates/list>.
2. Click the **+** (Create a Certificate).
3. Choose **Developer ID Application** → Continue.
4. It asks for a **Certificate Signing Request (CSR)**. Create one with
   Keychain Access:
   - Open **Keychain Access** → menu **Certificate Assistant** →
     **Request a Certificate from a Certificate Authority…**
   - Enter your email, leave **CA Email** blank, choose **Saved to disk**, save
     the `.certSigningRequest` file.
5. Upload that CSR, then **Download** the resulting `.cer` and double-click it to
   install it into your login keychain.
6. Repeat steps 2–5, this time choosing **Developer ID Installer**.

Verify both are present:

```bash
security find-identity -v | grep "Developer ID"
```

You should see one `Developer ID Application:` and one `Developer ID Installer:`
line. The release script auto-detects these — no config needed.

### 2. Store notarization credentials

1. Create an **app-specific password** for your Apple ID:
   - Go to <https://account.apple.com> → **Sign-In and Security** →
     **App-Specific Passwords** → **+**, name it `oscilinea-notary`, copy the
     generated password (looks like `abcd-efgh-ijkl-mnop`).
2. Store it in your keychain so the script can use it without prompting:

```bash
xcrun notarytool store-credentials oscilinea-notary \
    --apple-id "hazael.anaya@gmail.com" \
    --team-id "8GPYZ947VY" \
    --password "abcd-efgh-ijkl-mnop"
```

The name `oscilinea-notary` is what `release.sh` expects (override via
`scripts/release.config` if you ever change it).

That's it. Setup is done.

---

## Shipping a release (every time)

```bash
./scripts/release.sh
```

What happens:

1. Clean universal Release build (arm64 + x86_64).
2. Each format is signed with Developer ID Application + Hardened Runtime
   (the standalone also gets the audio-input entitlement).
3. The three formats are packaged into one installer and signed with
   Developer ID Installer.
4. The installer is uploaded to Apple, notarized, and stapled.
5. Gatekeeper is checked locally, a checksum is written.

Result in `dist/`:

```
Oscilinea-0.0.1-beta.YYYYMMDD-<commit>.pkg
Oscilinea-0.0.1-beta.YYYYMMDD-<commit>.pkg.sha256
```

Send the `.pkg`. Notarization typically takes a few minutes; the script waits
and reports the outcome.

### Useful variations

```bash
./scripts/release.sh --dry-run        # build an UNSIGNED .pkg to test layout (no certs needed)
./scripts/release.sh --skip-build     # reuse the current Release build
./scripts/release.sh --skip-notarize  # sign but don't upload to Apple (offline test)
```

---

## What your friends do

**Install:** double-click the `.pkg`, click through, enter their Mac password
once (installers always ask). No Control-clicking, no Terminal, no warnings.

The installer puts files in the standard system locations:

```
/Library/Audio/Plug-Ins/Components/Oscilinea.component   (AU)
/Library/Audio/Plug-Ins/VST3/Oscilinea.vst3             (VST3)
/Applications/Oscilinea.app                              (standalone)
```

Then they rescan plug-ins (or restart) in their DAW.

**Update:** you send a newer `.pkg`; they double-click it. It replaces the old
version in place. Because Oscilinea's plug-in identity codes never change
(`Sshp` / `Hzny`, bundle id `com.hazaelanaya.oscilinea`), their DAW treats it as
the same plug-in and **existing projects keep working** with the new version.

There is no separate uninstaller step before updating — installing over the top
is the supported path.

---

## Versioning and the update contract

- Bump `project(Oscilinea VERSION x.y.z ...)` in `CMakeLists.txt` for meaningful
  releases. The installer's internal version is `VERSION.YYYYMMDD`, which always
  increases, so macOS treats each build as newer than the last.
- **Never change** `PLUGIN_CODE` (`Sshp`), `PLUGIN_MANUFACTURER_CODE` (`Hzny`),
  the bundle id, or parameter IDs once a build is in a friend's hands. Those are
  the contract that lets updates load old projects. (See
  [docs/decisions/0002-plugin-identity.md](decisions/0002-plugin-identity.md).)

---

## Troubleshooting

**"No Developer ID Application certificate found."**
You haven't created the Developer ID certs yet, or only the Apple Development
cert is installed. Do the one-time setup above.

**Notarization fails.**
Get the details:

```bash
xcrun notarytool log <submission-id> --keychain-profile oscilinea-notary
```

Common causes: a binary wasn't signed with the Hardened Runtime, or the
secure timestamp couldn't be reached (notarization needs internet).

**A friend still sees a warning.**
Confirm the installer is stapled:

```bash
xcrun stapler validate "dist/Oscilinea-….pkg"
spctl --assess --type install -vv "dist/Oscilinea-….pkg"   # expect: "accepted, source=Notarized Developer ID"
```

**Standalone crashes on launch on their Mac.**
That was the pre-distribution bug we fixed: the standalone needs the microphone
usage string (CMake `MICROPHONE_PERMISSION_*`) and the audio-input entitlement
(`packaging/entitlements/standalone.entitlements`). A clean release build has
both.
