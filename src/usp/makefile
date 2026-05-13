# makefile for USP
#
# (ES) USP source was acquired from RCC, but it didn't include a makefile,
# so this one was created using the makefile for uascom as a starting point.
#

# LAX_CC - Define to ease warnings on GCC (ES)
LAX_CC=-Wno-pointer-to-int-cast
# DEBUG_CC - Define to add debug symbols to executable (ES)
DEBUG_CC=-g -DDEBUG
# GCC on 64-bit Linux defines (ES)
CFLAGS=$(LAX_CC) $(DEBUG_CC)

# 64bit machines
LDFLAGS=

CC = cc

APPS= usp usp2 usp3 usp4 usp5 uspset

OBJ= usp.o usp2.o usp3.o usp4.o usp5.o uspset.o

SRC= usp.c usp2.c usp3.c usp4.c usp5.c uspset.c

ifeq ($(OS),Windows_NT)
    HOME_DIR := $(USERPROFILE)
else
    HOME_DIR := $(HOME)
endif
BIN := $(HOME_DIR)/.local/bin

LIB := $(HOME_DIR)/.local/lib

all:	$(APPS)

usp:	usp.o
	$(CC) $(LDFLAGS) -o usp usp.o

usp2:	usp2.o
	$(CC) $(LDFLAGS) -o usp2 usp2.o

usp3:	usp3.o
	$(CC) $(LDFLAGS) -o usp3 usp3.o

usp4:	usp4.o
	$(CC) $(LDFLAGS) -o usp4 usp4.o

usp5:	usp5.o
	$(CC) $(LDFLAGS) -o usp5 usp5.o

uspset:	uspset.o
	$(CC) $(LDFLAGS) -o uspset uspset.o

$(OBJ):		$(SRC)

.c.o:
	$(CC) -c $(CFLAGS) $*.c

clean:
	rm -f $(OBJ)
	rm -f $(APPS)

test:
	usp -ieO test_gr
	mv test_gr.c xxgram.c
	sed -e '/tsym/,$$d' xxgram.c >test_gr2.c
	rm xxgram.c
	@echo new test_gr2 grammar tables now made

