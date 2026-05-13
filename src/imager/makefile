#makefile for IMAGER

# LAX_CC - Define to ease warnings on GCC
LAX_CC=
# DEBUG_CC - Define to add debug symbols to executable and enable debugging output
DEBUG_CC=-g -DDEBUG -DBIGDEBUG
# GCC on 64-bit Linux defines (ES)
CFLAGS=$(LAX_CC) $(DEBUG_CC) -DNOPD -DALIGN=4 -DBIGMEM

# 64bit machines
LDFLAGS=

CC = cc

OBJ = imager.o

SRC = imager.c

ifeq ($(OS),Windows_NT)
    HOME_DIR := $(USERPROFILE)
else
    HOME_DIR := $(HOME)
endif
BIN := $(HOME_DIR)/.local/bin

all:	imager


imager:		$(OBJ)
	$(CC) $(LDFLAGS) -o imager $(OBJ)

Install:	Installbin

Installbin:	imager
	cp imager $(BIN)
	cd $(BIN);strip imager;chmod 771 imager
	@echo imager is installed in $(BIN)


$(OBJ):		imager.h

.c.o:
	$(CC) -c $(CFLAGS) $*.c

clean:
	rm -f $(OBJ)

print:
	pr -n $(SRC)|lpr
