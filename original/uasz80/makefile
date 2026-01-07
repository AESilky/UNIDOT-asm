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

#pdp-11 defines
CFLAGS=-DBIGMEM -DNOPD
# 3b2 defines 
#CFLAGS=-DNOBSS -DALIGN=4 -DVMADR='unsigned long'

#pdp - 23 fix version
#LDFLAGS=-Z
#3b2 or split id machines
LDFLAGS=-i
# 32 bit machines
#LDFLAGS=

CC = cc
LD = cc

BIN = /usr/bin

LIB = /usr/lib

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
	../uascom/uasvm.o \
	../uascom/uasxref.o

TGT = uasz80

OBJ = uasz80.o uaz80gr.o

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
	cd $(BIN); strip $?; chown bin $?; chgrp bin $?; chmod 771 $?
	rm -f Installbin

Installlib:	uz80s.pd
	cp $? $(LIB)
	cd $(LIB); chown bin $?; chgrp bin $?; chmod 664 $?
	rm -f Installlib


# the following should be run by hand if needed to make a new grammar

uaz80gr.c:
	usp -ieO uaz80gr
	mv uaz80gr.c xxgram.c
	sed -e '/tsym/,$$d' xxgram.c >uaz80gr.c
	rm xxgram.c
	@echo new uaz80 grammar tables now made

uasz80.h: uasz80.z
	mktable uasz80.z
	mkpdtab uz80s.pd >uz80pd.h

uasz80.o:	uasz80.c uasz80.h

.c.o:
	$(CC) -c $(CFLAGS) $*.c
