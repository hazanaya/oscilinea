#!/bin/bash
set -euo pipefail

# =============================================================================
# Oscilinea release builder
#
# Produces a SIGNED + NOTARIZED macOS installer (.pkg) you can hand directly to
# friends. They double-click it; macOS shows no security warnings. To ship an
# update later, just run this again and send the new .pkg.
#
#   ./scripts/release.sh                 full release: build, sign, notarize, staple
#   ./scripts/release.sh --dry-run       build an UNSIGNED .pkg to test the layout
#   ./scripts/release.sh --skip-build    reuse the current Release build
#   ./scripts/release.sh --skip-notarize sign but don't notarize (offline test)
#
# One-time setup (certificates + notarization credentials) is described in
# docs/DISTRIBUTION.md. This script reads its identities from the keychain
# automatically, or from scripts/release.config if you want to override them.
# =============================================================================

REPO_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$REPO_DIR/build"
ARTEFACTS="$BUILD_DIR/Oscilinea_artefacts/Release"
DIST_DIR="$REPO_DIR/dist"
ENTITLEMENTS="$REPO_DIR/packaging/entitlements/standalone.entitlements"
PKG_RESOURCES_SRC="$REPO_DIR/packaging/resources"

DRY_RUN=0
SKIP_BUILD=0
SKIP_NOTARIZE=0
for arg in "$@"; do
    case "$arg" in
        --dry-run)       DRY_RUN=1 ;;
        --skip-build)    SKIP_BUILD=1 ;;
        --skip-notarize) SKIP_NOTARIZE=1 ;;
        *) echo "Unknown option: $arg" >&2; exit 2 ;;
    esac
done

# --- Configuration -----------------------------------------------------------
# Optional overrides live in scripts/release.config (gitignored). Anything not
# set there is auto-detected from the keychain below.
if [[ -f "$REPO_DIR/scripts/release.config" ]]; then
    # shellcheck disable=SC1091
    source "$REPO_DIR/scripts/release.config"
fi

BUNDLE_ID_BASE="com.hazaelanaya.oscilinea"
NOTARY_PROFILE="${NOTARY_PROFILE:-oscilinea-notary}"

# Auto-detect Developer ID identities from the keychain unless overridden.
if [[ -z "${DEV_ID_APP:-}" ]]; then
    DEV_ID_APP="$(security find-identity -v -p codesigning 2>/dev/null \
        | grep -o '"Developer ID Application: [^"]*"' | head -1 | tr -d '"' || true)"
fi
if [[ -z "${DEV_ID_INSTALLER:-}" ]]; then
    DEV_ID_INSTALLER="$(security find-identity -v 2>/dev/null \
        | grep -o '"Developer ID Installer: [^"]*"' | head -1 | tr -d '"' || true)"
fi

# --- Helpers -----------------------------------------------------------------
fail() { echo "ERROR: $*" >&2; exit 1; }
step() { echo; echo "==> $*"; }

require_signing_identities() {
    if [[ -z "${DEV_ID_APP:-}" ]]; then
        cat >&2 <<'MSG'
ERROR: No "Developer ID Application" certificate found in your keychain.

You currently have only an "Apple Development" certificate, which cannot be used
to distribute to other people. Create the Developer ID certificates first; see
docs/DISTRIBUTION.md (section "One-time setup"). Then re-run this script.

To test the installer layout without certificates, use:
    ./scripts/release.sh --dry-run
MSG
        exit 1
    fi
    if [[ -z "${DEV_ID_INSTALLER:-}" ]]; then
        fail "No \"Developer ID Installer\" certificate found. See docs/DISTRIBUTION.md."
    fi
}

# --- Version / naming --------------------------------------------------------
VERSION="$(sed -n 's/^project(Oscilinea VERSION \([0-9.][0-9.]*\).*/\1/p' "$REPO_DIR/CMakeLists.txt")"
[[ -n "$VERSION" ]] || fail "Could not read version from CMakeLists.txt"
COMMIT="$(git -C "$REPO_DIR" rev-parse --short HEAD)"
DIRTY=""
if [[ -n "$(git -C "$REPO_DIR" status --porcelain --ignore-submodules=dirty)" ]]; then
    DIRTY="-dirty"
fi
STAMP="$(date +%Y%m%d)"
# Monotonic numeric version for the installer's upgrade comparison.
PKGVER="${VERSION}.${STAMP}"
NAME="Oscilinea-${VERSION}-beta.${STAMP}-${COMMIT}${DIRTY}"
FINAL_PKG="$DIST_DIR/${NAME}.pkg"

echo "Building release: $NAME"
if [[ $DRY_RUN -eq 1 ]]; then
    echo "Mode: DRY RUN (no signing, no notarization)"
else
    require_signing_identities
    echo "  Application cert: $DEV_ID_APP"
    echo "  Installer cert:   $DEV_ID_INSTALLER"
    [[ $SKIP_NOTARIZE -eq 1 ]] && echo "  Notarization: SKIPPED (--skip-notarize)" \
                               || echo "  Notarization profile: $NOTARY_PROFILE"
fi

# --- Build -------------------------------------------------------------------
if [[ $SKIP_BUILD -eq 0 ]]; then
    step "Building universal Release (arm64 + x86_64)"
    cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release >/dev/null
    cmake --build "$BUILD_DIR" --config Release -j
fi

AU_SRC="$ARTEFACTS/AU/Oscilinea.component"
VST3_SRC="$ARTEFACTS/VST3/Oscilinea.vst3"
APP_SRC="$ARTEFACTS/Standalone/Oscilinea.app"
for path in "$AU_SRC" "$VST3_SRC" "$APP_SRC"; do
    [[ -e "$path" ]] || fail "Missing build artifact: $path (run a Release build first)"
done

# --- Stage fresh copies so we never sign the live build tree -----------------
WORK="$(mktemp -d "${TMPDIR:-/tmp}/oscilinea-release.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT
STAGE="$WORK/stage"
mkdir -p "$STAGE"
AU="$STAGE/Oscilinea.component"
VST3="$STAGE/Oscilinea.vst3"
APP="$STAGE/Oscilinea.app"
ditto "$AU_SRC" "$AU"
ditto "$VST3_SRC" "$VST3"
ditto "$APP_SRC" "$APP"

# --- Code signing ------------------------------------------------------------
if [[ $DRY_RUN -eq 0 ]]; then
    step "Code signing with Developer ID Application + Hardened Runtime"
    # Plug-ins inherit the host's permissions, so they need no entitlements.
    codesign --force --timestamp --options runtime \
        --sign "$DEV_ID_APP" "$AU"
    codesign --force --timestamp --options runtime \
        --sign "$DEV_ID_APP" "$VST3"
    # The standalone needs the audio-input entitlement (see the file's comment).
    codesign --force --timestamp --options runtime \
        --entitlements "$ENTITLEMENTS" \
        --sign "$DEV_ID_APP" "$APP"

    step "Verifying signatures"
    for b in "$AU" "$VST3" "$APP"; do
        codesign --verify --strict --verbose=2 "$b"
    done
else
    echo
    echo "(dry run) skipping code signing"
fi

# --- Component packages ------------------------------------------------------
step "Building component packages"
build_component_pkg() { # <bundle> <install-location> <identifier-suffix> <out>
    local bundle="$1" location="$2" suffix="$3" out="$4"
    local root; root="$(mktemp -d "$WORK/root.XXXXXX")"
    ditto "$bundle" "$root/$(basename "$bundle")"
    # Pin the bundle to its install location. By default Installer "relocates":
    # it finds any existing copy by bundle ID via Spotlight (an old install in
    # ~/Applications, a build tree, an unzipped release) and updates that copy
    # in place instead of installing to $location.
    local cplist="$root-components.plist"
    pkgbuild --analyze --root "$root" "$cplist" >/dev/null
    /usr/libexec/PlistBuddy -c "Set :0:BundleIsRelocatable false" "$cplist"
    pkgbuild --quiet \
        --root "$root" \
        --component-plist "$cplist" \
        --install-location "$location" \
        --identifier "${BUNDLE_ID_BASE}.${suffix}" \
        --version "$PKGVER" \
        "$out"
}
build_component_pkg "$AU"   "/Library/Audio/Plug-Ins/Components" "au"   "$WORK/au.pkg"
build_component_pkg "$VST3" "/Library/Audio/Plug-Ins/VST3"       "vst3" "$WORK/vst3.pkg"
build_component_pkg "$APP"  "/Applications"                      "app"  "$WORK/app.pkg"

# --- Installer resources + distribution definition ---------------------------
RES="$WORK/resources"
mkdir -p "$RES"
cp "$PKG_RESOURCES_SRC/welcome.html" "$RES/welcome.html"
cp "$PKG_RESOURCES_SRC/conclusion.html" "$RES/conclusion.html"
cp "$REPO_DIR/LICENSE" "$RES/LICENSE.txt"

DIST_XML="$WORK/distribution.xml"
cat > "$DIST_XML" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>Oscilinea ${VERSION} beta</title>
    <welcome file="welcome.html" mime-type="text/html"/>
    <conclusion file="conclusion.html" mime-type="text/html"/>
    <license file="LICENSE.txt" mime-type="text/plain"/>
    <options customize="always" require-scripts="false" hostArchitectures="arm64,x86_64"/>
    <volume-check>
        <allowed-os-versions>
            <os-version min="11.0"/>
        </allowed-os-versions>
    </volume-check>
    <choices-outline>
        <line choice="au"/>
        <line choice="vst3"/>
        <line choice="app"/>
    </choices-outline>
    <choice id="au" title="Audio Unit (AU)" description="For Logic Pro, GarageBand, MainStage, and other AU hosts.">
        <pkg-ref id="${BUNDLE_ID_BASE}.au"/>
    </choice>
    <choice id="vst3" title="VST3" description="For VST3 hosts.">
        <pkg-ref id="${BUNDLE_ID_BASE}.vst3"/>
    </choice>
    <choice id="app" title="Standalone app" description="Runs on its own, no DAW required.">
        <pkg-ref id="${BUNDLE_ID_BASE}.app"/>
    </choice>
    <pkg-ref id="${BUNDLE_ID_BASE}.au" version="${PKGVER}" onConclusion="none">au.pkg</pkg-ref>
    <pkg-ref id="${BUNDLE_ID_BASE}.vst3" version="${PKGVER}" onConclusion="none">vst3.pkg</pkg-ref>
    <pkg-ref id="${BUNDLE_ID_BASE}.app" version="${PKGVER}" onConclusion="none">app.pkg</pkg-ref>
</installer-gui-script>
XML

# --- Product archive ---------------------------------------------------------
step "Assembling installer"
UNSIGNED_PKG="$WORK/oscilinea-unsigned.pkg"
productbuild \
    --distribution "$DIST_XML" \
    --package-path "$WORK" \
    --resources "$RES" \
    "$UNSIGNED_PKG"

mkdir -p "$DIST_DIR"
if [[ $DRY_RUN -eq 1 ]]; then
    cp "$UNSIGNED_PKG" "$FINAL_PKG"
    echo
    echo "(dry run) UNSIGNED installer written:"
    echo "  $FINAL_PKG"
    echo "This is for layout testing only. Gatekeeper will reject it on other Macs."
    exit 0
fi

step "Signing installer with Developer ID Installer"
productsign --sign "$DEV_ID_INSTALLER" "$UNSIGNED_PKG" "$FINAL_PKG"
pkgutil --check-signature "$FINAL_PKG" | sed 's/^/    /'

# --- Notarization ------------------------------------------------------------
if [[ $SKIP_NOTARIZE -eq 0 ]]; then
    step "Notarizing (uploads to Apple, waits for the result)"
    if ! xcrun notarytool submit "$FINAL_PKG" \
            --keychain-profile "$NOTARY_PROFILE" --wait; then
        cat >&2 <<MSG

Notarization failed or no stored credentials were found for profile
"$NOTARY_PROFILE". Set up credentials once with:

    xcrun notarytool store-credentials "$NOTARY_PROFILE" \\
        --apple-id "<your Apple ID email>" \\
        --team-id "<your 10-char Team ID>" \\
        --password "<app-specific password>"

See docs/DISTRIBUTION.md for how to create the app-specific password.
To inspect a rejected submission:
    xcrun notarytool log <submission-id> --keychain-profile "$NOTARY_PROFILE"
MSG
        exit 1
    fi

    step "Stapling the notarization ticket"
    xcrun stapler staple "$FINAL_PKG"

    step "Gatekeeper assessment"
    spctl --assess --type install --verbose=4 "$FINAL_PKG" || \
        echo "  (spctl reported an issue; review the output above)"
else
    echo
    echo "Notarization skipped (--skip-notarize). The installer is signed but"
    echo "Gatekeeper will still warn on other Macs until it is notarized."
fi

# --- Checksum ----------------------------------------------------------------
step "Writing checksum"
( cd "$DIST_DIR" && shasum -a 256 "$(basename "$FINAL_PKG")" > "$(basename "$FINAL_PKG").sha256" )

echo
echo "Done."
echo "  Installer: $FINAL_PKG"
echo "  Checksum:  $FINAL_PKG.sha256"
echo
echo "Hand the .pkg to your friends. They double-click it and follow the prompts."
