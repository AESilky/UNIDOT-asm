# Makefile for the Z80 edition of UAS

#@(#)$Header: Makefile,v 3.7 87/12/01 13:29:48 rmm Rel $ z80 makefile

#		Notes on Setup
#
#  There are two independent choices to be made:  whether the symbol
#  table is to be memory resident or virtual.  This is controlled by
#  the BIGMEM switch.  If either a lot of memory is available (or programs
#  to be assembled are not large, then define BIGMEM.  Otherwise, leave
#  it undefined.
#
#  The second choice is whether the opcode and directive table is to
#  be compiled in or read when the assembler is executed.  It is somewhat
#  faster getting into execution if the compiled in option is selected.
#  This is done by defining NOPD.  More flexibility results if the file
#  is read upon execution since changes (although delicate) may be more
#  easily made.  For production use, defining NOPD is recommended.
#
#  Lastly, be SURE that the SAME choices are made in the uascom directory!

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
CFLAGS=$(LAX_CC) $(DEBUG_CC) -DNOPD -DALIGN=4 -DUSEVM -DBIGMEM
#pdp-11 defines
#CFLAGS=-DBIGMEM -DNOPD
# 3b2 defines 
#CFLAGS=-DNOBSS -DALIGN=4 -DVMADR='unsigned long'

#pdp - 23 fix version
#LDFLAGS=-Z
#3b2 or split id machines
#LDFLAGS=-i
# 32 bit machines
LDFLAGS=

CC = cc
LD = cc

ifeq ($(OS),Windows_NT)
    HOME_DIR := $(USERPROFILE)
else
    HOME_DIR := $(HOME)
endif
BIN := $(HOME_DIR)/.local/bin

LIB := $(HOME_DIR)/.local/lib

UASCOM=  ../uascom/uas.o \
	../uascom/uasdirec.o \
	../uascom/uasinit.o \
	../uascom/uasinput.o \
	../uascom/uaslook.o \
	../uascom/uasmac.o \
	../uascom/uasmisc.o \
	../uascom/uasout.o \
	../uascom/uasparse.o \
	../uascom/uasszy.o \
	../uascom/uastoken.o \
	../uascom/uasvars.o \
	../uascom/uasvm.o \
	../uascom/uasxref.o

TGT = uasz80

OBJ = uasz80.o uaz80gr.o easrgchk.o

.SUFFIXES:	.pd .pdc

all:
	cd ../uascom;make
	make all2

all2:	$(TGT)

$(TGT):		$(OBJ) $(UASCOM)
	$(LD) $(LDFLAGS) -o $(TGT) $(OBJ) $(UASCOM)

Install:	Installbin Installlib
	rm -f Install

Installbin:	$(TGT)
	cp $? $(BIN)
	cd $(BIN); strip $?; chmod 771 $?
	rm -f Installbin

Installlib:	uz80s.pd
	cp $? $(LIB)
	cd $(LIB); chmod 664 $?
	rm -f Installlib

clean:
	rm -f $(OBJ)
	cd ../uascom;make clean

# the following should be run by hand if needed to make a new grammar

uaz80gr.c:
	usp -ieO uaz80gr
	mv uaz80gr.c xxgram.c
	sed -e '/tsym/,$$d' xxgram.c >uaz80gr.c
	rm xxgram.c
	@echo new uaz80 grammar tables now made

# uasz80.h has been 'adjusted' to allow it to compile with the current
# 	version of GCC, so the following build step has been removed to
#	remove the risk of overwriting the adjusted version of the file.
#	Since uasz80 was adjusted, the source for mktable has been
#	acquired and has been 'adjusted' to compile. Therefore, it is
#	possible that uasz80.h could actually be generated from uasz80.z
#	but mktable probably needs to be updated somewhat to generate
#	an output file that will compile correctly with current GCC.
#
#uasz80.h: uasz80.z
#	mktable uasz80.z
#	mkpdtab uz80s.pd >uz80pd.h

uasz80.o:	uasz80.c uasz80.h

.c.o:
	$(CC) -c $(CFLAGS) $*.c
