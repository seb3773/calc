#!/usr/bin/env bash
set -euo pipefail

SRC_ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SRC_ROOT/build"
APPDIR="$BUILD_DIR/AppDir"

need_cmd() {
	command -v "$1" >/dev/null 2>&1 || {
		echo "error: missing required command: $1" >&2
		exit 1
	}
}

need_cmd cmake
need_cmd pkg-config
need_cmd strip
need_cmd sed
need_cmd awk
need_cmd wget
need_cmd cp
need_cmd chmod
need_cmd mkdir

# Make sure build dir exists
mkdir -p -- "$BUILD_DIR"

# Clean and Build the binary using the existing build.sh script
if [ "${NO_BUILD:-0}" -eq 0 ]; then
	echo "info: building kcalc in release mode..."
	"$SRC_ROOT/build.sh" clean
	"$SRC_ROOT/build.sh" release
else
	echo "info: skipping build (using existing binary)"
fi

BIN_PATH="$BUILD_DIR/calc"
if test ! -x "$BIN_PATH"; then
	echo "error: missing built binary: $BIN_PATH" >&2
	exit 1
fi

# Clean and create AppDir structure
echo "info: preparing AppDir..."
rm -rf -- "$APPDIR"
mkdir -p -- \
	"$APPDIR/usr/bin" \
	"$APPDIR/usr/lib" \
	"$APPDIR/usr/share/applications" \
	"$APPDIR/usr/share/icons/hicolor/48x48/apps"

# Copy binary to staged name tdecalc
cp -a "$BIN_PATH" "$APPDIR/usr/bin/tdecalc"

# Strip staged binary
if command -v sstrip >/dev/null 2>&1; then
	echo "info: stripping staged binary with sstrip"
	sstrip "$APPDIR/usr/bin/tdecalc" >/dev/null 2>&1 || true
else
	echo "info: using strip --strip-all"
	strip --strip-all "$APPDIR/usr/bin/tdecalc" >/dev/null 2>&1 || true
fi

# Resolve and copy TQt3, TDE, art, GMP and other dependencies from ldd output
echo "info: copying library dependencies..."
while read -r libname libpath; do
	if [[ -n "$libpath" && -f "$libpath" ]]; then
		echo "  -> bundling: $libname ($libpath)"
		cp -L "$libpath" "$APPDIR/usr/lib/"
	else
		echo "  warning: library $libname not resolved to a valid file ($libpath)"
	fi
done < <(ldd "$BIN_PATH" | awk '/=>/ {print $1, $3}' | grep -E '^lib(tqt|tde|DCOP|art|gmp)')

# Copy icon
ICON_SRC="$SRC_ROOT/icons/calc.png"
if test -f "$ICON_SRC"; then
	cp -a "$ICON_SRC" "$APPDIR/tdecalc.png"
	cp -a "$ICON_SRC" "$APPDIR/usr/share/icons/hicolor/48x48/apps/tdecalc.png"
else
	echo "error: missing icon: $ICON_SRC" >&2
	exit 1
fi

# Create Desktop entry at root of AppDir and usr/share/applications
cat > "$APPDIR/tdecalc.desktop" <<EOF
[Desktop Entry]
Version=1.0
Name=tdecalc
GenericName=Scientific Calculator
Comment=Customizable Windows 10/11 Calculator Clone
Exec=tdecalc
Icon=tdecalc
Terminal=false
Type=Application
X-TDE-StartupNotify=true
Categories=Qt;Utility;Calculator;
EOF
chmod 0644 "$APPDIR/tdecalc.desktop"
cp -a "$APPDIR/tdecalc.desktop" "$APPDIR/usr/share/applications/tdecalc.desktop"

# Create AppRun entry script
cat > "$APPDIR/AppRun" <<'EOF'
#!/bin/sh
SELF=$(readlink -f "$0")
HERE=${SELF%/*}
export LD_LIBRARY_PATH="$HERE/usr/lib:$LD_LIBRARY_PATH"
exec "$HERE/usr/bin/tdecalc" "$@"
EOF
chmod 0755 "$APPDIR/AppRun"

# Download appimagetool if not present
APPIMAGETOOL="$BUILD_DIR/appimagetool"
if [ ! -s "$APPIMAGETOOL" ]; then
	echo "info: downloading appimagetool..."
	wget -q --show-progress -O "$APPIMAGETOOL" "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage"
	chmod +x "$APPIMAGETOOL"
fi

# Build AppImage using --appimage-extract-and-run to bypass FUSE requirements
OUT_APPIMAGE="$SRC_ROOT/tdecalc-x86_64.AppImage"
rm -f -- "$OUT_APPIMAGE"

echo "info: generating AppImage..."
# Set ARCH environment variable so appimagetool knows what architecture we are packaging
export ARCH=x86_64
"$APPIMAGETOOL" --appimage-extract-and-run "$APPDIR" "$OUT_APPIMAGE"

echo "AppImage successfully built: $OUT_APPIMAGE"
exit 0
