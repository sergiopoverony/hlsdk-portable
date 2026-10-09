#!/bin/sh
# Builds Half-Life game libraries for PlayStation 4 and converts them to .prx modules
#
# Usage: scripts/ps4/build.sh [PS4 IP address]
#   With IP address, uploads modules into /data/xash/<gamedir> through FTP (GoldHEN, port 2121)
#
# Environment:
#   OO_PS4_TOOLCHAIN - path to OpenOrbis PS4 toolchain (required)
#   WAF_CONFIGURE    - extra arguments for ./waf configure, e.g. "--gamedir=gearbox --server-library-name=opfor"
#   PS4_DATA_DIR     - game root on the console, /data/xash by default
set -e

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"

if [ -z "$OO_PS4_TOOLCHAIN" ] || [ ! -f "$OO_PS4_TOOLCHAIN/link.x" ]; then
	echo "Set OO_PS4_TOOLCHAIN to OpenOrbis toolchain directory" >&2
	exit 1
fi

echo "=== Configuring"
./waf configure --ps4 -T release $WAF_CONFIGURE > build-ps4-configure.log 2>&1 || {
	tail -n 30 build-ps4-configure.log
	echo "Configure failed, full log: build-ps4-configure.log" >&2
	exit 1
}

echo "=== Building"
./waf build

GAMEDIR=$(sed -n "s/^GAMEDIR = '\(.*\)'$/\1/p" build/c4che/_cache.py)
GAMEDIR=${GAMEDIR:-valve}
OUT="$ROOT/build/ps4/$GAMEDIR"
rm -rf "$ROOT/build/ps4"
mkdir -p "$OUT/cl_dlls" "$OUT/dlls"

echo "=== Creating modules"
for so in build/cl_dll/*.so build/dlls/*.so; do
	[ -f "$so" ] || continue
	dir=$(basename "$(dirname "$so")")
	[ "$dir" = "cl_dll" ] && dir=cl_dlls
	name=$(basename "$so" .so).prx
	"$OO_PS4_TOOLCHAIN/bin/linux/create-fself" -in="$so" -out="$OUT/$dir/$name.oelf" --lib="$OUT/$dir/$name" --paid 0x3800000000000011 > /dev/null
	rm -f "$OUT/$dir/$name.oelf"
	echo "  $GAMEDIR/$dir/$name"
done

if [ -n "$1" ]; then
	DATA=${PS4_DATA_DIR:-/data/xash}
	echo "=== Uploading to $1:$DATA/$GAMEDIR"
	for f in "$OUT"/*/*.prx; do
		dir=$(basename "$(dirname "$f")")
		curl -s --ftp-create-dirs -T "$f" "ftp://$1:2121$DATA/$GAMEDIR/$dir/"
	done
	echo "Uploaded"
fi
