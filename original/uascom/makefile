# makefile for the common part of the uas system

# @(#)$Header: Makefile,v 6.3 88/03/08 16:02:35 rmm Exp $ make for uas common part

SRC = uas.c uasdirec.c uasinit.c uasinput.c \
	uaslook.c uasmac.c uasmisc.c uasout.c uasparse.c \
	uastoken.c uasvm.c uasszy.c uasxref.c

OBJ = uas.o uasdirec.o uasinit.o uasinput.o \
	uaslook.o uasmac.o uasmisc.o uasout.o uasparse.o \
	uastoken.o uasvm.o uasszy.o uasxref.o

#CFLAGS= -DNOPD -DBIGDEBUG -DDEBUG -DONECASE
CFLAGS= -DBIGMEM -DNOPD -DBIGDEBUG -DDEBUG -DONECASE

CC = cc

all:	$(OBJ)

$(OBJ):	uas.h ../incl/uobj.h

uas.o:	../incl/urel.h
