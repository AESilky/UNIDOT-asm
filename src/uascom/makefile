# makefile for the common part of the uas system

# @(#)$Header: Makefile,v 6.3 88/03/08 16:02:35 rmm Exp $ make for uas common part

SRC = uas.c uasdirec.c uasinit.c uasinput.c \
	uaslook.c uasmac.c uasmisc.c uasout.c uasparse.c \
	uastoken.c uasvars.c uasvm.c uasszy.c uasxref.c

OBJ = uas.o uasdirec.o uasinit.o uasinput.o \
	uaslook.o uasmac.o uasmisc.o uasout.o uasparse.o \
	uastoken.o uasvars.o uasvm.o uasszy.o uasxref.o

# LAX_CC - Define to ease warnings on GCC
LAX_CC=-Wno-pointer-to-int-cast
# DEBUG_CC - Define to add debug symbols to executable
DEBUG_CC=-g
# GCC on 64-bit Linux defines (ES)
CFLAGS=$(LAX_CC) $(DEBUG_CC) -DNOPD -DALIGN=4
#CFLAGS= -DNOPD -DBIGDEBUG -DDEBUG -DONECASE
#CFLAGS= -DBIGMEM -DNOPD -DBIGDEBUG -DDEBUG -DONECASE

CC = cc

all:	$(OBJ)

clean:
	rm -f $(OBJ)

$(OBJ):	uas.h ../incl/uobj.h

uas.o:	../incl/urel.h
