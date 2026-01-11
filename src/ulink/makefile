#makefile for ULINK
#
#  (ES, 1/2026) The code contains a virtual memory allocation mechanism to
#    allow large assembly source files to be assembled on machines that
#    had very little memory (as was common in the '80s). However, the code
#    uses `valloc` and a number of file operations that make it relatively
#    complex. In addition, it uses `valloc` and the `valloc` mechanism is
#    now considered obsolete, and a suggested replacement is `aligned_alloc`
#    from <stdlib.h>. Since many systems now have more physical memory than
#    many hard disk drives of the '80s, a new compile flag 'USEVM' has been
#    added to control whether the original virtual memory approaches are
#    used or if it is left up to the system by using `aligned_alloc` for
#    each allocation request. If `USEVM` is defined, the Unidot authored
#    virtual memory code will be used. If `USEVM` is not defined the
#    operating system `aligned_alloc` will be used, and also `BIGMEM` will
#    be defined for use in most of the code (as that makes the rest of the
#    code not worry about memory management). No attempt has been made to
#    convert the Unidot authored virtual memory routines to compile (only
#    a version without `USEVM` defined has been tested).
#

#@(#)$Header: Makefile,v 4.2 88/10/09 15:56:19 rmm Rel $ makefile for ulink

# LAX_CC - Define to ease warnings on GCC
LAX_CC=-Wno-pointer-to-int-cast
# DEBUG_CC - Define to add debug symbols to executable
DEBUG_CC=-g
# GCC on 64-bit Linux defines (ES)
CFLAGS=$(LAX_CC) $(DEBUG_CC) -DNOPD -DALIGN=4
# pdp-11 defines
#CFLAGS= -DDEBUG
# 3b2 defines
#CFLAGS=-DNOBSS -DALIGN=4 -DVMADR='unsigned long'

#pdp - 23 fix version
#LDFLAGS=-Z
# 3b2 and split id machines
#LDFLAGS=-i
# 32 bit machines
#LDFLAGS=

# pdp-11 defines
#LDFLAGS=-i
# 3b2 and split id machines
#LDFLAGS=
# 32bit machines
#LDFLAGS=
# 64bit machines
LDFLAGS=

CC = cc

OBJ= ulink.o ulkpass.o ulkilude.o ulkobj.o ulkrel.o \
	ulkgetpt.o ulklook.o ulkmisc.o

SRC= ulink.c ulkpass.c ulkilude.c ulkobj.c ulkrel.c \
	ulkgetpt.c ulklook.c ulkmisc.c

BIN = /usr/bin

ulink:		$(OBJ)
	$(CC) $(LDFLAGS) -o ulink $(OBJ)

Install:	Installbin

Installbin:	ulink
	cp ulink $(BIN)
	cd $(BIN);strip ulink;chown bin ulink;chgrp bin ulink;chmod 771 ulink
	@echo ulink is installed in /usr/bin


$(OBJ):		ulink.h ../incl/uobj.h

.c.o:
	$(CC) -c $(CFLAGS) $*.c

clean:
	rm -f $(OBJ)

print:
	pr -n ../incl/uobj.h $(SRC)|lpr
