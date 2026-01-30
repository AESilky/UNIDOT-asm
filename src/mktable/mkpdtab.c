/************************************************************************
*									*
*	Copyright (C) 1987, by Unidot, Inc.				*
*									*
*			Unidot, Inc.					*
*			602 Park Point Dr.				*
*			Golden, CO 80401				*
*									*
*			All Rights Reserved				*
*									*
* This software is furnished under license and may be used and copied	*
* only in accordance with the terms of such license and with the	*
* inclusion of the above copyright notice.  This software or any other	*
* copies thereof may not be provided or otherwise made available to any	*
* other person.								*
*									*
* No title to or ownership of the software is hereby transferred.	*
*									*
* The information in this software is subject to change without notice	*
* and should not be construed as a commitment by Unidot, Inc.		*
*									*
* Unidot assumes no responsibility for the use or reliability of its	*
* software on equipment configurations that are not directly supported	*
* by Unidot, Inc.							*
*									*
*************************************************************************/

/************************************************************************
*									*
*			    UAS Assembler				*
*									*
*			mkpdtab.c - built-in pd table maker		*
*									*
************************************************************************/


static char rcsid[] =
"@(#)$Header: mkpdtab.c,v 1.7 87/12/01 06:58:24 rmm Exp $";

#include <stdarg.h> 		/* For va_arg */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h> 		/* For 'open' */
#include <unistd.h> 		/* For 'close' and `sbrk` */

typedef unsigned short uns;
#define white(c)	((c)==' '||(c)=='\t')
#define TKEOF	0
#define TKEOL	1
#define TKSPC	2
#define TKSYM	3
#define TKCON	4

	/* BEWARE OF THE FOLLOWING - IF THE HEADER FOR UAS IS
	   CHANGE, THE FOLLOWING MAY NEED TO CHANGE TOO         */

#define OTINS	1
#define OTDIR	2

short	toktyp;			/* type of current token	*/
short	inch;			/* current input character	*/
short	lineno;			/* current line number		*/
short	debug;			/* debug flag			*/
uns	v;			/* value of hex number		*/
char	idbuf[24];		/* symbol collected here	*/
struct	fm {
	unsigned short fm_f[8];	/* max of 6 operands & 2 other	*/
}	f[1000];		/* formats built here		*/
struct	op {			/* opcode table			*/
	unsigned short	op_v;	/* value			*/
	short	op_t;		/* type				*/
	char	op_s[10];	/* symbol			*/
}	o[700];
struct	sy {			/* symbol table			*/
	unsigned short	sy_v;	/* value			*/
	char	sy_s[10];	/* symbol			*/
}	s[500];

short	fx;			/* next format			*/
short	ox = 1;			/* next opcode			*/
short	sx = 0;			/* next symbol			*/
short	fn = 9;			/* number of entries in table	*/
char	objsuf[6] = "obj";
char	lstsuf[6] = "lst";
char	srcsuf[6] = "";
char	*source = 0;

/* Function Declarations (Local) */

void doargs(char* s);
void fatal(char* s, ...);
void gettab();
int preget(int tkn);
void puttab();
int token();
void usage();



void main(int argc, char** argv) {

	int	i;

	if (argc < 2) usage();	/* a filename is required */
	for( i=1; i<argc; i++ ){
		if( argv[i][0] == '-' ) doargs(argv[i]+1); else
		if( !source ) source = argv[i]; else
		usage();
	}
	if( source ){
		close(0);
		if( open(source,0) != 0 ) fatal("cannot open %s",source);
	}
	inch = getchar();
	toktyp = TKEOL;
	gettab();
	puttab();
	exit(0);
}


void doargs(char* s) {

	for(;;)switch( *s++ ){
case 0:		return;
case 'd':	debug++; continue;
	}
}

void usage() {

	fprintf(stderr,"usage: mkpdtab [-d] filename\n");
	exit(0);
}

void gettab() {

	struct	fm	*fmp;
	struct	op	*ocp;
	struct	sy	*syp;
	int		i;
	int		val;
	char	*p;


	val = fx;			/* start of formats */
	while( token() == TKCON ){		/* read machine instructions */
		i = 0;
		fmp = &f[fx++];
		fmp->fm_f[i++] = v;
		preget( TKSPC );
		while( token() == TKCON ){
			fmp->fm_f[i++] = v;
			if( token() == TKEOL ) break;
		}
		if( i < fn ){
			if( fn != 9 )
				fatal("varying number of fields %d %d",i,fn);
			fn = i;
		}
		if( toktyp == TKEOL ) continue;	/* no mnemonics */
		while( toktyp == TKSYM ){	/* read instruction mnemonics */
			ocp = &o[ox];
			ocp->op_t = OTINS;
			ocp->op_v = val;
			strcpy(ocp->op_s,idbuf);
			ox++;
			if( token() == TKEOL ) break;
			token();		/* next symbol */
		}
		val = fx;
		if( toktyp != TKEOL ) fatal("optab");
	}
	if( toktyp != TKEOL ) fatal("optab sep");
	while( token() == TKCON ){		/* read assembler directives */
		val = v;
		while( token() == TKSPC ){	/* read directive mnemonics */
			preget( TKSYM );
			ocp = &o[ox];
			ocp->op_t = OTDIR;
			ocp->op_v = val;
			strcpy(ocp->op_s,idbuf);
			ox++;
		}
		if( toktyp != TKEOL ) fatal("dir");
	}
	if( toktyp != TKEOL ) fatal("dir sep");
	while( token() == TKCON ){		/* read predefined symbols */
		val = v;
		while( token() == TKSPC ){	/* read symbol mnemonics */
			preget( TKSYM );
			syp = &s[sx++];
			syp->sy_v = val;
			strcpy( syp->sy_s, idbuf );
		}
		if( toktyp != TKEOL ) fatal("sym fmt");
	}
	if( toktyp != TKEOL ) return;		/* no objsuf or lstsuf */
	while( white(inch) || inch == '\n' ) inch = getchar();
	if( inch == ';' || inch == EOF ) return;
	p = objsuf;
	while( !white(inch) && inch != '\n' && inch != ';'){
		*p++ = inch;
		inch = getchar();
	}
	*p = 0;
	while( white(inch) ) inch = getchar();
	if( inch == ';' || inch == '\n' || inch == EOF ) return;
	p = lstsuf;
	while( !white(inch) && inch != '\n' && inch != ';' ){
		*p++ = inch;
		inch = getchar();
	}
	*p = 0;
	while( white(inch) ) inch = getchar();
	if( inch == ';' || inch == '\n' || inch == EOF ) return;
	p = srcsuf;
	while( !white(inch) && inch != '\n' && inch != ';' ){
		*p++ = inch;
		inch = getchar();
	}
	*p = 0;
}

void puttab() {

	/* first output the format table */

	int	i;
	int	j;
	unsigned v;

	printf("struct format fmt[] = {\n");
	for( i=0; i<fx; i++ ){
		if( i%10 == 0 ) printf("/*%3d*/",i);
		for( j=0; j<fn; j++ ){
			if( j ) putchar( ' ' ); else putchar( '\t' );
			v = f[i].fm_f[j];
			printf("0x%x,", v);
		}
		printf("\n");
	}
	printf(" 0};\n\n");

	/* now output the opcode table */

	printf("struct octab opctab[] = {\n");
	for( i=1; i<ox; i++ )
		printf("	0, 0x%x, %d, 0, \"%s\",\n",
			o[i].op_v,o[i].op_t,o[i].op_s);
	printf(" 0,0,0,0,0};\n\n");

	/* now print the reserved words */

	printf("struct resw { short rw_val; char *rw_str; } rsw[] = {\n");
	for( i=0; i<sx; i++ )
		printf("	0x%x, \"%s\",\n",s[i].sy_v,s[i].sy_s);
	printf("0,0};\n");

	printf("char xobjsuf[] = \"%s\";\n",objsuf);
	printf("char xlstsuf[] = \"%s\";\n",lstsuf);
	printf("char xsrcsuf[] = \"%s\";\n",srcsuf);
}


int preget(int tkn) {

	if( tkn != token() ) fatal( "bad format" );
	return v;
}


void fatal(char *s, ...) {

	va_list argptr;
	va_start(argptr, s);
	fprintf(stderr, "%3d FATAL: ", lineno);
	vfprintf(stderr, s, argptr);
	fprintf(stderr, "\n");
	va_end(argptr);
	exit(1);
}

int token() {

	char	*p;
	uns	h;
	int	dv;

	if( toktyp == TKEOL ) ++lineno;
	if( inch == ';' || inch == '\n' ){
		while( inch != '\n' ) inch = getchar();
		inch = getchar();
		return toktyp = TKEOL;
	}
	if( inch == EOF ) return toktyp = TKEOF;
	if( white(inch) ){
		do inch = getchar(); while( white(inch) );
		if( inch == ';' ){
			while( inch != '\n' && inch != EOF )
				inch = getchar();
		}
		if( inch == '\n' ){
			inch = getchar();
			return toktyp = TKEOL;
		}
		if( inch == EOF ) return toktyp = TKEOF;
		return toktyp = TKSPC;
	}
	if( isdigit(inch) ){
		v = 0;
		dv = 0;
		while( isxdigit(inch) ){
			v <<= 4;
			if( !isdigit(inch) ) inch += 9;
			inch &= 0xf;
			v |= inch;
			dv = dv*10 + inch;
			inch = getchar();
		}
		if( inch == 'h' || inch == 'H' )
			inch = getchar();
		else
			v = dv;
		return toktyp = TKCON;
	}
	h = 0;
	p = idbuf;
	while( !white(inch) && inch != ';' && inch != '\n' && inch != EOF ){
		*p++ = inch;
		inch = getchar();
	}
	*p = 0;
	return toktyp = TKSYM;
}
