#!/bin/sh
# Commodities - build build/Commodities.lha: the two commodities with icons
# (tool types filled in), a ReadMe and the source.
# SPDX-License-Identifier: GPL-2.0-only
set -e
top=$(cd "$(dirname "$0")/.." && pwd)
stage=$(mktemp -d "$top/build/pkg.XXXXXX")
pkg=$stage/Commodities
mkdir -p "$pkg/Source"
icon="python3 $top/tools/mkicon.py"
cp "$top/build/ClickToFront" "$top/build/StickyMenus" "$pkg/"
$icon tool "$pkg/ClickToFront.info" --image ctf --stack 4096 --pos 10,10 \
	--tooltype "DONOTWAIT" --tooltype "CX_PRIORITY=0" --tooltype "(QUALIFIER=NONE)" --tooltype "(ONRELEASE=NO)"
$icon tool "$pkg/StickyMenus.info" --image sticky --stack 4096 --pos 90,10 \
	--tooltype "DONOTWAIT" --tooltype "CX_PRIORITY=0" --tooltype "(CLICKTIME=350)"
cp "$top/README.md" "$pkg/ReadMe"
$icon project "$pkg/ReadMe.info" --image doc --tool SYS:Utilities/MultiView --pos 170,10
cp "$top"/src/*.c "$top"/src/*.h "$top/LICENSE" "$pkg/Source/"
$icon drawer "$stage/Commodities.info" --image drawer --window 60,40,300,90
out=$top/build/Commodities.lha
[ -e "$out" ] && mv "$out" "$out.old"
# jlha (as in Lagoon's packaging): level-2 headers, every file listed
(cd "$stage" && jlha a2o5q "$out" Commodities.info $(find Commodities -type f | LC_ALL=C sort) >/dev/null 2>&1)
[ -s "$out" ] || { echo "mkpackage: jlha failed" >&2; exit 1; }
# never ship certificates or keys
if (cd "$stage" && grep -rlE "BEGIN CERTIFICATE|PRIVATE KEY|sandbox-test-only" .) ||
   find "$stage" -iname '*.crt' -o -iname '*.pem' -o -iname '*.key' | grep -q .; then
	echo "mkpackage: certificate or key material found, refusing" >&2
	mv "$out" "$out.refused"
	exit 1
fi
echo "mkpackage: $out"
