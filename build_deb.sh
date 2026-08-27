#!/bin/sh
# build_deb.sh - Build Debian package for calc (Trinity Desktop)

set -e

PACKAGE_NAME="tde-calc"
ARCH="amd64"

VERSION_ARG=""
NO_REBUILD=0
CLEAN_BUILD=0

for arg in "$@"; do
    if [ "$arg" = "--no-rebuild" ] || [ "$arg" = "-n" ]; then
        NO_REBUILD=1
    elif [ "$arg" = "--clean" ] || [ "$arg" = "-c" ]; then
        CLEAN_BUILD=1
    elif [ -z "$VERSION_ARG" ] && [ "${arg#-}" = "$arg" ]; then
        VERSION_ARG="$arg"
    fi
done

if [ -n "$VERSION_ARG" ]; then
    VERSION="$VERSION_ARG"
elif [ -n "$APP_VERSION" ]; then
    VERSION="$APP_VERSION"
elif [ -f "src/version.h" ]; then
    VERSION=$(grep '#define CALCVERSION' src/version.h 2>/dev/null | sed -E 's/.*"([^"]+)".*/\1/' || echo "1.0")
else
    VERSION="1.0"
fi

[ -z "$VERSION" ] && VERSION="1.0"

echo "=== Version Target: $VERSION ==="

# Update src/version.h
echo "=== Updating version header (src/version.h) ==="
cat <<EOF > "src/version.h"
#ifndef CALC_VERSION_H
#define CALC_VERSION_H

#define CALCVERSION "$VERSION"

#endif // CALC_VERSION_H
EOF

BUILD_DIR="${PACKAGE_NAME}_${VERSION}_${ARCH}"

echo "=== Preparing Debian Package Build Directory ==="
# Clean old build directory
rm -rf "$BUILD_DIR"
rm -f "${BUILD_DIR}.deb"

# Ensure binary is compiled (ZX0 mode)
if [ "$NO_REBUILD" -eq 1 ]; then
    echo "=== Skipping compilation as requested ==="
elif [ "$CLEAN_BUILD" -eq 1 ]; then
    echo "=== Compiling calc in Release mode (clean build) ==="
    ./build.sh clean
    ./build.sh
else
    echo "=== Compiling calc in Release mode (incremental build) ==="
    ./build.sh
fi

SRC_BIN="build/calc"
if [ ! -f "$SRC_BIN" ]; then
    echo "Error: 'build/calc' binary not found. Compilation failed?" >&2
    exit 1
fi

echo "Using binary: $SRC_BIN"

# Create package directory tree
mkdir -p "$BUILD_DIR/DEBIAN"
mkdir -p "$BUILD_DIR/usr/local/bin"
mkdir -p "$BUILD_DIR/usr/share/applications"
mkdir -p "$BUILD_DIR/usr/share/icons/hicolor"

# TDE resource paths
mkdir -p "$BUILD_DIR/opt/trinity/share/apps/calc"
mkdir -p "$BUILD_DIR/opt/trinity/share/config.kcfg"
mkdir -p "$BUILD_DIR/opt/trinity/share/apps/tdeconf_update"
mkdir -p "$BUILD_DIR/usr/share/apps/calc"
mkdir -p "$BUILD_DIR/usr/share/config.kcfg"
mkdir -p "$BUILD_DIR/usr/share/apps/tdeconf_update"

# Copy binary
cp "$SRC_BIN" "$BUILD_DIR/usr/local/bin/calc"
chmod 755 "$BUILD_DIR/usr/local/bin/calc"

# Strip binary
if command -v sstrip >/dev/null 2>&1; then
    echo "Stripping binary with sstrip..."
    sstrip "$BUILD_DIR/usr/local/bin/calc" || true
else
    echo "Stripping binary with strip..."
    strip --strip-all "$BUILD_DIR/usr/local/bin/calc" || true
fi

# Copy desktop entry
cp "build/calc.desktop" "$BUILD_DIR/usr/share/applications/calc.desktop"
chmod 644 "$BUILD_DIR/usr/share/applications/calc.desktop"

# Copy TDE config/UI files (to both opt/trinity and usr for maximum TDE compat)
cp "src/calcui.rc" "$BUILD_DIR/opt/trinity/share/apps/calc/calcui.rc"
cp "src/calcui.rc" "$BUILD_DIR/usr/share/apps/calc/calcui.rc"
chmod 644 "$BUILD_DIR/opt/trinity/share/apps/calc/calcui.rc" "$BUILD_DIR/usr/share/apps/calc/calcui.rc"

cp "src/calc.kcfg" "$BUILD_DIR/opt/trinity/share/config.kcfg/calc.kcfg"
cp "src/calc.kcfg" "$BUILD_DIR/usr/share/config.kcfg/calc.kcfg"
chmod 644 "$BUILD_DIR/opt/trinity/share/config.kcfg/calc.kcfg" "$BUILD_DIR/usr/share/config.kcfg/calc.kcfg"

cp "src/calcupd.upd" "$BUILD_DIR/opt/trinity/share/apps/tdeconf_update/calcupd.upd"
cp "src/calcupd.upd" "$BUILD_DIR/usr/share/apps/tdeconf_update/calcupd.upd"
chmod 644 "$BUILD_DIR/opt/trinity/share/apps/tdeconf_update/calcupd.upd" "$BUILD_DIR/usr/share/apps/tdeconf_update/calcupd.upd"

# Package application icons
ICON_SRC="icons/calc.png"
if [ -f "$ICON_SRC" ]; then
    echo "Packaging icons..."
    REAL_SZ="64x64"
    REAL_DIR="$BUILD_DIR/usr/share/icons/hicolor/$REAL_SZ/apps"
    mkdir -p "$REAL_DIR"
    cp "$ICON_SRC" "$REAL_DIR/tdecalc.png"
    chmod 644 "$REAL_DIR/tdecalc.png"

    # Create symlinks for standard sizes
    for sz in 16x16 22x22 24x24 32x32 48x48; do
        DST_DIR="$BUILD_DIR/usr/share/icons/hicolor/$sz/apps"
        mkdir -p "$DST_DIR"
        ln -sf "../../$REAL_SZ/apps/tdecalc.png" "$DST_DIR/tdecalc.png"
    done
fi

# Generate control file
cat << EOF > "$BUILD_DIR/DEBIAN/control"
Package: $PACKAGE_NAME
Version: $VERSION
Section: utils
Priority: optional
Architecture: $ARCH
Maintainer: seb3773
Depends: libgmp10, libfontconfig1, libx11-6, libtqt-mt | libtqt3-mt
Description: A customizable Windows 10/11 calculator clone for Trinity Desktop (TDE)
 A highly configurable, modern, and lightweight scientific calculator
 designed specifically for the Trinity Desktop Environment (TDE).
 Replicates all mathematical functions and layout behaviors of the
 Windows 10/11 calculator, with deep customizability.
EOF

# Generate postinst script to refresh icon cache and desktop database
cat << 'EOF' > "$BUILD_DIR/DEBIAN/postinst"
#!/bin/sh
set -e

if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -f -t /usr/share/icons/hicolor >/dev/null 2>&1 || true
fi
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database -q /usr/share/applications >/dev/null 2>&1 || true
fi
EOF

# Generate prerm script (refresh database on removal)
cat << 'EOF' > "$BUILD_DIR/DEBIAN/prerm"
#!/bin/sh
set -e

if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -f -t /usr/share/icons/hicolor >/dev/null 2>&1 || true
fi
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database -q /usr/share/applications >/dev/null 2>&1 || true
fi
EOF

# Set permissions of DEBIAN scripts
chmod 755 "$BUILD_DIR/DEBIAN/postinst"
chmod 755 "$BUILD_DIR/DEBIAN/prerm"

echo "=== Building Debian Package ==="
if which dpkg-deb >/dev/null 2>&1; then
    if dpkg-deb --help | grep -q -- "--root-owner-group"; then
        dpkg-deb --root-owner-group --build "$BUILD_DIR"
    elif which fakeroot >/dev/null 2>&1; then
        fakeroot dpkg-deb --build "$BUILD_DIR"
    else
        dpkg-deb --build "$BUILD_DIR"
    fi
    echo "=== Package Built successfully: ${BUILD_DIR}.deb ==="
    # Cleanup directory
    rm -rf "$BUILD_DIR"
else
    echo "Error: dpkg-deb command not found. Cannot build .deb package." >&2
    echo "Ensure you are on a Debian-based system to run this script." >&2
    exit 1
fi
