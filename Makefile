# Commodities for AmigaOS 3.x (vbcc + NDK 3.2)
#   make            build/ClickToFront, build/StickyMenus
#   make package    build/Commodities.lha (with icons and docs)
VBCC ?= /opt/amiga/vbcc
NDK  ?= /opt/amiga/ndk/Include_H
VC   := PATH=$(VBCC)/bin:$$PATH VBCC=$(VBCC) $(VBCC)/bin/vc +aos68k
CFLAGS := -c99 -O2 -cpu=68000 -dontwarn=153 -I$(NDK)

all: build/ClickToFront build/StickyMenus

build/ClickToFront: src/clicktofront.c src/cxcommon.c src/cxcommon.h
	@mkdir -p build
	$(VC) $(CFLAGS) -o $@ src/clicktofront.c src/cxcommon.c -lamiga
build/StickyMenus: src/stickymenus.c src/cxcommon.c src/cxcommon.h
	@mkdir -p build
	$(VC) $(CFLAGS) -o $@ src/stickymenus.c src/cxcommon.c -lamiga

package: all
	tools/mkpackage.sh

.PHONY: all package
