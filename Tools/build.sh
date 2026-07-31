#!/bin/sh
# Builds BOTH .3gx variants and drops them in releases/ inside the project.
#
# Why this script exists: the devkitPro toolchain refuses to build from a path containing a
# space, and this project normally lives in one ("D:\Claude Code\..."). So the compile has to
# happen in a mirror on a space-free path. That is an accident of the toolchain, not something
# the project should leak - so the artifacts come straight back here to releases/.
#
# Usage, from the project root:
#     sh Tools/build.sh
#
# Override the mirror location with MIRROR=/some/path if C:\ctrc is taken.

set -eu

PROJ=$(pwd)
MIRROR=${MIRROR:-/c/ctrc}
OUT="$PROJ/releases"
TARGET=CTRComposer-BlankTemplate

# The flag that selects which of the two builds we get.
IDENTITY=Sources/plugin/identity.inc.c

if [ ! -f "$PROJ/$IDENTITY" ]; then
    echo "run this from the project root (did not find $IDENTITY)" >&2
    exit 1
fi

# ---- locate the toolchain -----------------------------------------------------------------
# $DEVKITARM is *tried*, not trusted: some shells here ship it pre-set to the Linux-style
# /opt/devkitpro path even on Windows, where nothing is installed. Probe for the actual
# compiler and fall through to the usual install locations.
has_gcc() { [ -x "$1/bin/arm-none-eabi-gcc.exe" ] || [ -x "$1/bin/arm-none-eabi-gcc" ]; }

DKA=""
for cand in "${DEVKITARM:-}" "${DEVKITPRO:-}/devkitARM" /c/devkitPro/devkitARM /opt/devkitpro/devkitARM; do
    [ -n "$cand" ] || continue
    if has_gcc "$cand"; then DKA=$cand; break; fi
done

if [ -z "$DKA" ]; then
    echo "devkitARM not found. Tried \$DEVKITARM, \$DEVKITPRO/devkitARM, /c/devkitPro/devkitARM," >&2
    echo "and /opt/devkitpro/devkitARM. Install devkitPro or set DEVKITARM to a real path." >&2
    exit 1
fi

# Always derived from the compiler we actually found - never from $DEVKITPRO, which can point
# at an empty directory that exists but holds no toolchain (so a -d test passes and lies).
DKP=$(dirname "$DKA")

MAKE=$DKP/msys2/usr/bin/make.exe
[ -x "$MAKE" ] || MAKE=$(command -v make)

# GCC writes temporaries to TMP; the default can be a directory we cannot write to.
mkdir -p "$MIRROR/tmp"
MIRROR_WIN=$(cd "$MIRROR" && pwd -W 2>/dev/null | sed 's|/|\\|g') || MIRROR_WIN=""

# devkitPro's makefiles read DEVKITARM/DEVKITPRO as *make variables*. Passing them on the
# command line rather than exporting them works regardless of how the parent shell propagates
# (or fails to propagate) its environment to child processes.
# PATH has to be passed too. The devkitPro make.exe reports its own location using the msys2
# view of the filesystem (/opt/devkitpro/...), which only resolves inside the devkitPro msys2
# shell - from git-bash the recursive $(MAKE) then fails with "No such file or directory".
# Handing it a PATH that contains a real, resolvable make sidesteps that entirely.
VARS="DEVKITARM=$DKA DEVKITPRO=$DKP PATH=$DKA/bin:$DKP/tools/bin:$DKP/msys2/usr/bin:/usr/bin:/bin"
[ -n "$MIRROR_WIN" ] && VARS="$VARS TMP=$MIRROR_WIN\\tmp TEMP=$MIRROR_WIN\\tmp"

# NOT parallel: the devkitPro template races its own link rule under -j.
build() { (cd "$MIRROR" && "$MAKE" -j1 $VARS "$@"); }

# ---- mirror the sources -------------------------------------------------------------------
echo "==> syncing to $MIRROR"
mkdir -p "$MIRROR"
for item in Sources Includes Assets Tools Makefile 3ds.ld CTRComposer.plgInfo; do
    [ -e "$PROJ/$item" ] || continue
    rm -rf "$MIRROR/$(basename "$item")"
    cp -r "$PROJ/$item" "$MIRROR/"
done

mkdir -p "$OUT"

# ---- build 1: the template (TOOLS_ONLY 0, as committed) ------------------------------------
echo "==> building the template"
build clean >/dev/null 2>&1 || true
build >/dev/null
cp "$MIRROR/$TARGET.3gx" "$OUT/$TARGET.3gx"

# ---- build 2: the universal toolkit (TOOLS_ONLY 1) -----------------------------------------
# Flipped in the MIRROR only, so the working tree is never touched.
echo "==> building the universal build (default.3gx)"
sed -i 's/^#define TOOLS_ONLY 0$/#define TOOLS_ONLY 1/' "$MIRROR/$IDENTITY"
grep -q '^#define TOOLS_ONLY 1' "$MIRROR/$IDENTITY"   # fail loudly if the sed missed its target
build clean >/dev/null 2>&1 || true
build >/dev/null
cp "$MIRROR/$TARGET.3gx" "$OUT/default.3gx"

# ---- the same checks CI runs ---------------------------------------------------------------
echo "==> verifying"
for f in "$OUT/$TARGET.3gx" "$OUT/default.3gx"; do
    # Luma3DS only loads the 3GX$0002 container; 3GX$0001 builds fine and then silently refuses.
    head -c 8 "$f" | grep -q '3GX\$0002' || { echo "not a 3GX\$0002 container: $f" >&2; exit 1; }
done
for s in Tracker 'Game Guide' 'Example:'; do
    if grep -qa "$s" "$OUT/default.3gx"; then
        echo "default.3gx still mentions '$s' - the TOOLS_ONLY build is wrong" >&2
        exit 1
    fi
done

VER=$(grep -oa 'v[0-9]\.[0-9]\.[0-9]' "$OUT/$TARGET.3gx" | sort -u | head -n1)
echo
echo "  $OUT/$TARGET.3gx   ->  luma/plugins/<TitleID>/"
echo "  $OUT/default.3gx                  ->  luma/plugins/default.3gx"
echo
echo "  version on screen: $VER   (check it matches on hardware)"
