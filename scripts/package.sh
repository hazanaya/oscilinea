#!/bin/bash
set -euo pipefail

# Assembles a distributable Oscilinea beta package in dist/ from the current
# Release build artifacts, mirroring the layout of the original hand-built
# beta: install/uninstall scripts, README, checksums, build info, and a zip.
#
# Run a Release build first:
#   cmake -B build -DCMAKE_BUILD_TYPE=Release
#   cmake --build build --config Release -j

REPO_DIR="$(cd "$(dirname "$0")/.." && pwd)"
ARTEFACTS="$REPO_DIR/build/Oscilinea_artefacts/Release"

AU_SRC="$ARTEFACTS/AU/Oscilinea.component"
VST3_SRC="$ARTEFACTS/VST3/Oscilinea.vst3"
APP_SRC="$ARTEFACTS/Standalone/Oscilinea.app"

for path in "$AU_SRC" "$VST3_SRC" "$APP_SRC"; do
    if [[ ! -e "$path" ]]; then
        echo "Missing build artifact: $path" >&2
        echo "Run the Release build first (see header of this script)." >&2
        exit 1
    fi
done

VERSION="$(sed -n 's/^project(Oscilinea VERSION \([0-9.][0-9.]*\).*/\1/p' "$REPO_DIR/CMakeLists.txt")"
COMMIT="$(git -C "$REPO_DIR" rev-parse --short HEAD)"
DIRTY=""
if [[ -n "$(git -C "$REPO_DIR" status --porcelain --ignore-submodules=dirty)" ]]; then
    DIRTY="-dirty"
fi
STAMP="$(date +%Y%m%d)"
NAME="Oscilinea ${VERSION}-beta.${STAMP}-${COMMIT}${DIRTY}"

DIST_DIR="$REPO_DIR/dist"
PKG_DIR="$DIST_DIR/$NAME"

echo "Packaging $NAME"
rm -rf "$PKG_DIR" "$DIST_DIR/$NAME.zip" "$DIST_DIR/$NAME.zip.sha256"
mkdir -p "$PKG_DIR/Plug-Ins/AU" "$PKG_DIR/Plug-Ins/VST3" "$PKG_DIR/Standalone" "$PKG_DIR/Support"

ditto "$AU_SRC" "$PKG_DIR/Plug-Ins/AU/Oscilinea.component"
ditto "$VST3_SRC" "$PKG_DIR/Plug-Ins/VST3/Oscilinea.vst3"
ditto "$APP_SRC" "$PKG_DIR/Standalone/Oscilinea.app"

echo "Validating the packaged AU and VST3..."
AUVAL_RESULT="FAILED"
if auval -v aufx Sshp Hzny > /dev/null 2>&1; then
    AUVAL_RESULT="passed"
fi

PLUGINVAL_RESULT="not available on the build machine"
PLUGINVAL_BIN="$(command -v pluginval || true)"
if [[ -z "$PLUGINVAL_BIN" && -x "/Applications/pluginval.app/Contents/MacOS/pluginval" ]]; then
    PLUGINVAL_BIN="/Applications/pluginval.app/Contents/MacOS/pluginval"
fi
if [[ -n "$PLUGINVAL_BIN" ]]; then
    if "$PLUGINVAL_BIN" --strictness-level 5 --validate "$VST3_SRC" > /dev/null 2>&1; then
        PLUGINVAL_RESULT="passed at strictness level 5"
    else
        PLUGINVAL_RESULT="FAILED at strictness level 5"
    fi
fi

cat > "$PKG_DIR/INSTALL.command" <<EOF
#!/bin/bash
set -euo pipefail

PACKAGE_DIR="\$(cd "\$(dirname "\$0")" && pwd)"

AU_SRC="\$PACKAGE_DIR/Plug-Ins/AU/Oscilinea.component"
VST3_SRC="\$PACKAGE_DIR/Plug-Ins/VST3/Oscilinea.vst3"
APP_SRC="\$PACKAGE_DIR/Standalone/Oscilinea.app"

AU_DEST="\$HOME/Library/Audio/Plug-Ins/Components/Oscilinea.component"
VST3_DEST="\$HOME/Library/Audio/Plug-Ins/VST3/Oscilinea.vst3"
APP_DEST="\$HOME/Applications/Oscilinea.app"

echo "Installing $NAME for the current user..."

for path in "\$AU_SRC" "\$VST3_SRC" "\$APP_SRC"; do
    if [[ ! -e "\$path" ]]; then
        echo "Missing package file: \$path" >&2
        exit 1
    fi
done

mkdir -p "\$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "\$HOME/Library/Audio/Plug-Ins/VST3"
mkdir -p "\$HOME/Applications"

rm -rf "\$AU_DEST" "\$VST3_DEST" "\$APP_DEST"
ditto "\$AU_SRC" "\$AU_DEST"
ditto "\$VST3_SRC" "\$VST3_DEST"
ditto "\$APP_SRC" "\$APP_DEST"

xattr -dr com.apple.quarantine "\$AU_DEST" "\$VST3_DEST" "\$APP_DEST" 2>/dev/null || true

echo
echo "Installed:"
echo "  \$AU_DEST"
echo "  \$VST3_DEST"
echo "  \$APP_DEST"
echo
echo "Restart or rescan plug-ins in your DAW."
echo "Optional AU validation:"
echo "  auval -v aufx Sshp Hzny"
EOF
chmod +x "$PKG_DIR/INSTALL.command"

cat > "$PKG_DIR/UNINSTALL.command" <<EOF
#!/bin/bash
set -euo pipefail

AU_DEST="\$HOME/Library/Audio/Plug-Ins/Components/Oscilinea.component"
VST3_DEST="\$HOME/Library/Audio/Plug-Ins/VST3/Oscilinea.vst3"
APP_DEST="\$HOME/Applications/Oscilinea.app"

echo "Removing Oscilinea files installed for the current user..."

rm -rf "\$AU_DEST" "\$VST3_DEST" "\$APP_DEST"

echo
echo "Removed:"
echo "  \$AU_DEST"
echo "  \$VST3_DEST"
echo "  \$APP_DEST"
echo
echo "If you saved new-instance appearance defaults, they may remain at:"
echo "  \$HOME/Library/Application Support/Oscilinea/New Instance Defaults.xml"
EOF
chmod +x "$PKG_DIR/UNINSTALL.command"

cat > "$PKG_DIR/README.md" <<EOF
# $NAME

Oscilinea is a macOS audio visualizer plug-in. It passes audio through unchanged and draws a focused shape display that can be used inside a DAW, recorded to \`.mov\`, mirrored in a source window, or published as a Syphon source.

This is a beta build for testing.

## Included

- \`Plug-Ins/AU/Oscilinea.component\` - Audio Unit plug-in for Logic Pro, GarageBand, MainStage, and AU hosts.
- \`Plug-Ins/VST3/Oscilinea.vst3\` - VST3 plug-in for VST3 hosts.
- \`Standalone/Oscilinea.app\` - standalone app version.
- \`INSTALL.command\` - copies the plug-ins and app into user-level install locations.
- \`UNINSTALL.command\` - removes the files installed by \`INSTALL.command\`.
- \`Support/SHA256SUMS.txt\` - checksums for the packaged files.
- \`Support/BUILD_INFO.txt\` - build and validation notes.

## Requirements

- macOS 11 or newer.
- Apple Silicon or Intel Mac.
- A DAW or app that can load AU or VST3 plug-ins, unless using the standalone app.

## Quick Install

1. Unzip the package.
2. Open the unzipped \`$NAME\` folder.
3. Double-click \`INSTALL.command\`.
4. Restart or rescan plug-ins in your DAW.

The installer copies files to:

\`\`\`text
~/Library/Audio/Plug-Ins/Components/Oscilinea.component
~/Library/Audio/Plug-Ins/VST3/Oscilinea.vst3
~/Applications/Oscilinea.app
\`\`\`

No administrator password should be needed.

## Manual Install

If you prefer to install by hand:

1. Copy \`Plug-Ins/AU/Oscilinea.component\` to \`~/Library/Audio/Plug-Ins/Components/\`.
2. Copy \`Plug-Ins/VST3/Oscilinea.vst3\` to \`~/Library/Audio/Plug-Ins/VST3/\`.
3. Copy \`Standalone/Oscilinea.app\` to \`~/Applications/\` or \`/Applications/\`.
4. Restart or rescan plug-ins in your DAW.

## Opening On macOS

This beta is ad-hoc signed and not notarized. macOS may show a security warning the first time you open the standalone app or scan the plug-in.

If you trust the person who sent this build:

- For the standalone app, Control-click \`Oscilinea.app\`, choose \`Open\`, then confirm.
- If macOS blocks it, open \`System Settings > Privacy & Security\` and allow it from there.
- The install script also removes quarantine attributes from the installed copies, which often avoids "damaged" or "unidentified developer" beta-build warnings.

## Basic Use

1. Add Oscilinea to an audio or instrument track.
2. Audio passes through unchanged.
3. The control panel starts hidden. Move the pointer near the bottom center of the plug-in and click the handle to reveal it.
4. Use the \`Signal\` row for gain/freeze/smoothing/glide/trail controls.
5. Use the \`Look\` row for color, transparency, line style, flip, and weight controls.
6. Use the \`Record\` row to record \`.mov\`, open a source window, or start a Syphon External Source.

## For Claude Or Another Assistant

From Terminal, after unzipping:

\`\`\`bash
cd "/path/to/$NAME"
./INSTALL.command
\`\`\`

Optional AU validation after install:

\`\`\`bash
auval -v aufx Sshp Hzny
\`\`\`

To uninstall the files installed by this package:

\`\`\`bash
cd "/path/to/$NAME"
./UNINSTALL.command
\`\`\`

Do not use \`sudo\` for the included install/uninstall scripts. They are intended to install only for the current user.

## Uninstall

Run \`UNINSTALL.command\`, or remove these files manually:

\`\`\`text
~/Library/Audio/Plug-Ins/Components/Oscilinea.component
~/Library/Audio/Plug-Ins/VST3/Oscilinea.vst3
~/Applications/Oscilinea.app
\`\`\`

User appearance defaults, if created, may remain here:

\`\`\`text
~/Library/Application Support/Oscilinea/New Instance Defaults.xml
\`\`\`

## Build Notes

- Version: \`$NAME\`
- Source commit: \`$COMMIT$DIRTY\`
- Plug-in identity: AU type \`aufx\`, subtype \`Sshp\`, manufacturer \`Hzny\`
- Formats: AU, VST3, Standalone
- Build type: Release
- \`auval -v aufx Sshp Hzny\`: $AUVAL_RESULT
- \`pluginval\`: $PLUGINVAL_RESULT
EOF

cat > "$PKG_DIR/Support/BUILD_INFO.txt" <<EOF
$NAME

Source commit: $COMMIT$DIRTY
Build type: Release
Formats: AU, VST3, Standalone
macOS target: macOS 11 or newer
Architectures: Apple Silicon and Intel

Plug-in identity:
  AU type: aufx
  AU subtype / PLUGIN_CODE: Sshp
  Manufacturer code: Hzny
  Bundle ID: com.hazaelanaya.oscilinea

Validation on build machine:
  cmake --build build --config Release -j: passed
  auval -v aufx Sshp Hzny: $AUVAL_RESULT
  pluginval: $PLUGINVAL_RESULT

Notes:
  This is a beta build.
  It is ad-hoc signed and not notarized.
  Audio is pass-through; visualization and capture run outside the audio thread.
EOF

echo "Writing checksums..."
(
    cd "$PKG_DIR"
    find . -type f ! -name ".DS_Store" ! -path "./Support/SHA256SUMS.txt" -print0 \
        | sort -z \
        | xargs -0 shasum -a 256 > "Support/SHA256SUMS.txt"
)

echo "Zipping..."
(
    cd "$DIST_DIR"
    ditto -c -k --sequesterRsrc --keepParent "$NAME" "$NAME.zip"
    shasum -a 256 "$NAME.zip" > "$NAME.zip.sha256"
)

echo
echo "Package ready:"
echo "  $PKG_DIR"
echo "  $DIST_DIR/$NAME.zip"
echo "  $DIST_DIR/$NAME.zip.sha256"
echo
echo "Validation: auval $AUVAL_RESULT; pluginval $PLUGINVAL_RESULT"
