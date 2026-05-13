#makefile for MKTABLE and MKPDTAB
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
#  (ES, 2/2026) Having problems when not using the UNIDOT virtual memory,
#    so now trying to get it working (but just with the BIGMEM flag). All
#    occurrences of the UNIDOT 'valloc' were changed to 'uavalloc' to
#    differentiate from the system 'valloc'.
#

# LAX_CC - Define to ease warnings on GCC
LAX_CC=-Wno-pointer-to-int-cast
# DEBUG_CC - Define to add debug symbols to executable
DEBUG_CC=-g
# GCC on 64-bit Linux defines (ES)
CFLAGS=$(LAX_CC) $(DEBUG_CC) -DALIGN=4

# 64bit machines
LDFLAGS=

CC = cc

OBJ= mkpdtab.o mktable.o

SRC= mkpdtab.c mktable.c

ifeq ($(OS),Windows_NT)
    HOME_DIR := $(USERPROFILE)
else
    HOME_DIR := $(HOME)
endif
BIN := $(HOME_DIR)/.local/bin

LIB := $(HOME_DIR)/.local/lib

all:	mkpdtab mktable

mkpdtab:	$(OBJ)
	$(CC) $(LDFLAGS) -o mkpdtab mkpdtab.o

mktable:	$(OBJ)
	$(CC) $(LDFLAGS) -o mktable mktable.o


$(OBJ):		$(SRC)

.c.o:
	$(CC) -c $(CFLAGS) $*.c

clean:
	rm -f $(OBJ)

test:
	mktable test.z
	mkpdtab test.pd >test.h

