#makefile for ULINK

#@(#)$Header: Makefile,v 4.2 88/10/09 15:56:19 rmm Rel $ makefile for ulink

#pdp-11 defines
CFLAGS= -DDEBUG
# 3b2 defines 
#CFLAGS=-DNOBSS -DALIGN=4 -DVMADR='unsigned long'

#pdp - 23 fix version
#LDFLAGS=-Z
#3b2 or split id machines
#LDFLAGS=-i
# 32 bit machines
#LDFLAGS=

#pdp-11 defines
LDFLAGS=-i
#3b2- split id machines
#LDFLAGS=
#32bit machines
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

print:
	pr -n ../incl/uobj.h $(SRC)|lpr
