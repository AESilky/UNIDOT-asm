/************************************************************************
*									*
*	Copyright (C) 1987 by Unidot, Inc.				*
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
*		General Purpose Table Maker				*
*									*
*************************************************************************/

static char rcsid[] =
"@(#)$Header: mktable.c,v 1.9 89/05/13 11:41:24 rmm Rel $ table maker";

#include <stdarg.h> 		/* For va_arg */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h> 		/* For 'open' */
#include <unistd.h> 		/* For 'close' and `sbrk` */

/*

	This program is an attempt at a generalization and improvement
	over prior efforts: mkregtab, etc

	The basic notion is that a series of symbolic tables will be
	read and internal formats constructed.  A source table is
	of the following style:

	#table <ucchar> <field>...

	Note:  instructions to mktable are flagged by something special
	in column 1, followed by a keyword

	<ucchar> is one upper case alphabet. (IE, max 26 tables)
	<field> is one of the following:

		E	expression
		S	string
		C	character

	wherein an expression is made up of

	decimal constants:		nnnnn
	hex constants:			0xhhhhh
	octal constants:		0nnnn
	binary constants:		_bbb_bbb_bbb
	character constants:		'c'
	symbolic names:			A<alphanumeric or _>...

	separated by operators:  +. -, *, /, %, <<, >>, |, &, ^, ~
	There is no precedence, parens are used for that.

	fields are separated by either <white>...<comma> <white>...
	or <white> <white>...

	A S if not quoted may not contain white or a comma

	A C if not quoted may not be a blank or a comma

	If the first field is an S type, then this is called an
	index field.  Successive S's will be assigned successive
	numberic values.  The default starting value is 0.  The
	default increment is 1.  This may be changed with a line
	of the form

	#nnn[,nnn]	where the first number is the next value to
			be assigned.  The second number if present
			is the increment.  If absent, the increment is 1.
			If the increment is negative, the ordinal is
			shifted left by the increment each step.


	Index strings must be unique across all tables.

	A new table can be started at any time.

	After all tables are read, we are ready for output.  There
	are several forms, generally started with something of the
	form:

	#output [filename]

	If filename is missing, output continues to the file currently
	open on the stdout.  If filename is present, stdout is closed
	and reopened with the filename specified (always truncated).

	Text following the #output line is copied line for line to
	the output with certain substitutions made:

	%x(<expr>)		means to output the value of an
				expression in hex format (0xnnnn)
	%s(<string desig>)	means to output in string format: "..."
	%c(<expr>)		means to output in char format: '.'
	%d(<expr>)		decimal
	%o(<expr>)		octal

	where an expression is composed of constants and or references
	of the following form:  id.nn[.mm]  which indicates to
	find the value in field nn of the line with index <id>.  If the optional
	[.mm] is present, the expression is repeated for each of the
	fields, separated by the token immediately following the terminating
	')'.  In repeated fields, the designator $ stands for the current
	line.  As an abbreviation, <id> is equivalent to <id>.0, and generally
	the appearance of <id> that is a line index is equivalent to %d(<id>)

	A string field output as a string will have an ordinal assigned
	if the %u form is used.  A string field used as a value will have
	that ordinal used.  (If no ordinal has been assigned, an error
	message will result.)

	If a line in an output section begins with '*', followed by
	a single upper case character, that line will be repeated for
	each source line in the referenced table.

	Example: the register table

	#table A S S S S E

	(the first S is the register name field, the second is the
	pair, the third is the lsh, the fourth the rsh, the first
	the "name" of the register, and E the "mask")

	NIL	NIL	NIL	NIL	nilreg	_0000_0000_0000_0000
	R0	R01	R0R	R0L	r0	_0000_0000_0000_0001
	R1	R01	R1R	R1L	r1	_0000_0000_0000_0010

	etc

	In the output section a line beginning with "*<char>" calls for
	iterating over the table.  If the "*<char>" is followed by a
	digit, then we select only one instance of the designated field
	and assign that field a value as we do so (should be also have
	a variable increment here?)

	we output the register names with output of the form:

	char rgnames[] = {
	*A4	%s($.4),
		0 };

	then we output the table with the form:

	struct rgt rgtab[] = {
	*A	%d($.1), %d($.2), %d($.3), %x($.5), %d($.4),
		0,0,0,0,0 };

	Finally, in the "free" zone, started with a line of the
	form

	#free [filename]

	we break apart all lines and any string that matches one of
	the table indexes will be replaced with its table number
*/


#define AT	struct at
#define OL	struct ol
#define LN	struct ln
#define ST	struct st
#define ORD	short		/* may be long if needed	*/
#define OLSIZ	1024
#define HASHSIZ	128		/* must be power of two		*/

ST {				/* string structure		*/
	ST	*st_prev;	/* hash link			*/
	ORD	st_ord;		/* ordinal			*/
	short	st_tln;		/* table and line number	*/
				/* packed 5 bits and 11 bits	*/
	char	st_stg[2];	/* actually variable length	*/
}
	*hash[HASHSIZ];		/* hash heads			*/
ST	*allocst();		/* allocate and copy string	*/
ST	*findst();		/* locate a string		*/
ST	*getfield();		/* scan a field			*/
ST	*nilst;			/* pointer to "-"		*/
ST	*nest;			/* non-existent string		*/
ST	*globst;		/* set by resolveid()		*/

LN {				/* structure for each line	*/
	short	ln_cnt;		/* number of entries following	*/
	ST	*ln_st[1];	/* actually variable length	*/
};

OL {				/* ordinal to line structure	*/
	LN	*ol_ln;
}
	ol[OLSIZ],		/* max number of table lines	*/
	*olp = ol;

	/* note - the LN entry preceding a table will have ln_cnt set to -1
	  and the LN entry following a table will have ln_cnt set to -1 */

AT {				/* alpha to table structure	*/
	OL	*at_ol;
	short	at_lcnt;
}
	at[26];



char	line[256];		/* input line read here		*/
char	oline[256];		/* output line built here	*/
char	arena[512];		/* split apart here		*/
char	field[64];		/* max single field		*/
char	*fp[64];		/* pointers to fields		*/
char	col[64];		/* column where this started	*/
char	idbuf[24];		/* identifiers collected here	*/
char	cvtbuf[16];		/* conversion buffer		*/
char	*lp;			/* line pointer			*/
char	*xlp;			/* output line pointer		*/
short	cvtx;			/* number of entries in buf	*/
short	debug;			/* debug flag			*/
short	fpx;			/* number of fields		*/
short	lineno;			/* current input line		*/
ORD	ordinal;		/* ordinal to assign		*/
short	increment;		/* increment to ordinal		*/
long	exprval;		/* result of expression eval	*/
char	*decfmt = "%ld";
char	*hexfmt = "0x%lx";
char	*octfmt = "0%lo";
char	hex[] = "0123456789abcdef";
char	decf[8];
char	hexf[8];
char	octf[8];
short	hexchg;
short	octchg;
short	eof;


/* Function Declarations (Local) */

ST* allocst(char* s);
int among(int ch, char* strings);
void cvt(long val, int radix);
void dbtrap();
void doarg(char* s);
void doline(char* s, LN* thisln);
void dooutput();
void err(char* s, ...);
char* expr(char* s, LN* thisln, int gflg);
ST* findst(char* s);
ST* getfield(int f);
int gethash(char* s);
int getaline();
void gettable();
char* idscan(char* s);
void iterate(char* s);
void ordset(char* s);
void outval(int let, long val);
char* prime(char* s, LN* thisln, int gflg);
void process();
void pts(char* s);
char* resolveid(char* s, LN* thisln, int gflg);
void setout(char* s);
char* string(char* s, LN* thisln);
LN* tlntoln(int n);
void warn(char* s, ...);
void usage();
char* xlate(char* s, LN* thisln);



void main(int argc, char** argv) {

	int	i;

	if (argc < 2) usage();	/* a filename is required */
	for( i=1; i<argc; i++ )
		if( argv[i][0] == '-' ) doarg( argv[i]+1 ); else {
			close( 0 );
			if( open( argv[i], 0 ) != 0 )
				err("cannot open %s",argv[i]);
		}
	nilst = allocst( "-" );		/* make up nil st	*/
	nilst->st_stg[0] = 0;		/* make it really nil	*/
	nest = allocst( "1" );		/* make up one st	*/
	nest->st_stg[0] = 0;		/* make it really nil	*/
/*DEB*/if(debug>1){
	fprintf(stderr,"nilst = %o st = <%s>",nilst,nilst->st_stg);
	fprintf(stderr,"nest = %o st = <%s>",nest,nest->st_stg);
}

	/* put in a null line to separate tables */

	olp->ol_ln = (LN *)malloc(sizeof(LN)-sizeof(ST *));
	olp->ol_ln->ln_cnt = -1;
	olp++;
	process();
}

void doarg(char *s) {

	for(;;)switch( *s++ ){
case 0:		return;
case 'd':	debug++; continue;
default:	err("bad flag: %s",--s);
	}
}


void process() {

	char	*p;

top:	while( getaline() && line[0] != '#' );
	for(;;){
		if( eof ) exit(0);
		if( strncmp( "table", &line[1], 5 ) == 0 ){
			gettable();	/* return at eof or next # command */
			continue;
		}
		if( strncmp( "output", &line[1], 6 ) == 0 ){
			dooutput();
			continue;
		}
		if( strncmp( "free", &line[1], 4 ) == 0 ){
			setout(line+5);
			while( getaline() ){
				if( line[0] == '#' ) break;
				doline( line, NULL );
			}
			continue;
		}
		if( strncmp( "dec", &line[1], 3 ) == 0 ){
			strcpy( decf, line+4 );
			decfmt = decf;
			goto top;
		}
		if( strncmp( "hex", &line[1], 3 ) == 0 ){
			strcpy( hexf, line+4 );
			hexfmt = hexf;
			hexchg++;
			goto top;
		}
		if( strncmp( "oct", &line[1], 3 ) == 0 ){
			strcpy( octf, line+4 );
			octfmt = octf;
			octchg++;
			goto top;
		}
		warn("not understood: %s",line);
		goto top;
	}
}

void gettable() {

	/* this procedure reads one table - on entry the line contains
	  the line with the table character and the format */

	char	*p;
	int		i;
	short		fx;
	short		tabchar;
	char		format[16];
	short		curfld;
	ST		*curst[16];
	LN		*lnp;
	OL		*olps;

	ordinal = 0;
	increment = 1;
	p = line+6;		/* space over table */
	while( isspace(*p) )p++;
	tabchar = *p++ - 'A';
	if( tabchar < 0 || tabchar >= 26 ) err("bad table id: %c",p[-1]);
	if( at[tabchar].at_ol ) err("table previously defined: %c",p[-1]);
	at[tabchar].at_ol = olps = olp;
	fx = 0;
	while( *p ){
		while( isspace(*p) ) p++;
		if( fx >= 15 ) err("too many formats");
		switch( *p++ ){
	case 'e':
	case 'E':	format[fx++] = 'E';
			continue;
	case 'c':
	case 'C':	format[fx++] = 'C';
			continue;
	case 's':
	case 'S':	format[fx++] = 'S';
			continue;
	case '*':	if( fx == 0 ) err("nothing to repeat");
			while( fx < 15 ) format[fx] = format[fx-1], fx++;
			*p = 0;
		}
	}
	format[fx] = 0;
/*DEB*/if(debug)fprintf(stderr,"format for %c is %s\n",tabchar+'A',format);
	for(;;){
		if( !getaline() )goto done;
		if( line[0] == 0 ) continue;
		if( line[0] == '#' ){
			lp = line+1;
			if( *lp != ',' && (*lp < '0' || *lp > '9') ) goto done;
			ordset( lp );
			continue;
		}
		curfld = 0;
		lp = line;
		while( curfld < fx ){
			curst[curfld] = getfield(format[curfld]);
			if( curst[curfld] == nest ) break;
			curfld++;
		}
		if( curfld ){
/*DEB*/if(debug>1)fprintf(stderr,"line %d, ord %d has %d fields\n",
/*DEB*/		lineno,(int)ordinal,curfld);
			lnp = (LN *)malloc(sizeof(LN)+(curfld-1)*sizeof(ST *));
			lnp->ln_cnt = curfld;
			for( i=0; i<curfld; i++ )
				lnp->ln_st[i] = curst[i];
			olp->ol_ln = lnp;
			if( format[0] == 'S' && curst[0] ){
				if( curst[0]->st_tln != -1 ){
					err("dup: %s",curst[0]->st_stg);
				} else {
					i = tabchar << 11;
					i += olp - olps;
					curst[0]->st_tln = i;
					curst[0]->st_ord = ordinal;
				}
			}
			if( increment < 0 )
				ordinal <<= -increment;
			else
				ordinal += increment;
			olp++;
		}
	}
done:	at[tabchar].at_lcnt = olp - olps;
/*DEB*/if(debug)fprintf(stderr,"table %c: %d lines\n",
/*DEB*/		tabchar+'A',at[tabchar].at_lcnt);
	lnp = (LN *)malloc(sizeof(LN)-sizeof(ST *));
	lnp->ln_cnt = -1;
	olp->ol_ln = lnp;
	olp++;
	return;
}

void dooutput() {

	setout(line+7);
	ordinal = 0;
	increment = 1;
	while( getaline() ){
		if( line[0] == '#' ) break;
		if( line[0] != '*' ){
			doline(line, NULL);
		} else {
			iterate( line+1 );
		}
	}
}

void ordset(char* s) {

	/* set up a new ordinal and increment */

	short	flag;

	if( *s != ',' ){
		ordinal = 0;
		while( *s >= '0' && *s <= '9' )
			ordinal = ordinal*10 + *s++ - '0';
	}
	if( *s == ',' ){
		increment = 0;
		s++;
		flag = 0;
		if( *s == '-' ) flag++, s++;
		while( *s >= '0' && *s <= '9' )
			increment = increment*10 + *s++ - '0';
		if( flag ) increment = -increment;
	}
}



void iterate(char* s) {

	int	i;
	short	table;
	short	field;
	OL	*thisol;
	LN	*thisln;
	ST	*thisst;
	short	lcnt;

	field = -1;
	table = *s++ - 'A';
	if( table < 0 || table >= 26 ) err("bad iteration %s",--s);
	if( *s >= '0' && *s <= '9' ) field = *s++ - '0';
	thisol = at[table].at_ol;
	lcnt = at[table].at_lcnt;
	for( i=0; i<lcnt; i++, thisol++ ){
		thisln = thisol->ol_ln;
		if( field >= 0 ){		/* select one field only */
			if( field >= thisln->ln_cnt )
				continue;	/* no such field	*/
			thisst = thisln->ln_st[field];
			if( thisst == nilst )
				continue;	/* don't output the '-'	*/
			if( thisst->st_ord != -1 )
				continue;	/* already output	*/
			thisst->st_ord = ordinal;
			if( increment < 0 )
				ordinal <<= -increment;
			else
				ordinal += increment;
		}
		doline( s, thisln );
	}
}

char *string(char* s, LN* thisln) {

	int	n;
	ST	*thisst;

	if( *s == '(' ){
		s = string( s+1, thisln );
		if( *s != ')' ) err("mismatched ')'"); else s++;
		return s;
	}
	if( *s == '$' ){
		s++;
		if( thisln == NULL ) err("no context");
		s = resolveid(s, thisln, 1);
		if( globst != nest && globst != nilst ) pts( globst->st_stg );
		goto retnow;
	}
	if( isalpha( *s ) ){
		s = idscan( s );
		thisst = allocst( idbuf );
		thisln = NULL;
		if( *s == '.' ){	/* lookfor ID.n */
			thisln = tlntoln( thisst->st_tln );
			if( thisln == NULL ){
				warn("%s not an index",idbuf);
				goto retnow;
			}
			s++;
			s = resolveid( s, thisln, 1);
			if( globst == nest || globst == nilst  )
				goto retnow;
			thisst = globst;
		}
		pts( thisst->st_stg );
		goto retnow;
	}
	err("neither $ nor ID");
retnow:	return s;
}

LN *tlntoln(int n) {		/* map n to a line entry */

	OL	*thisol;

	if( n == -1 ) return NULL;
	thisol = at[ (n>>11) & 0x1f ].at_ol + (n & 0x7ff);
	return thisol->ol_ln;
}

char* resolveid(char* s, LN* thisln, int gflg) {

	/* on entry s points to the '.' after the
	  identifier or the '$'.  The chain is followed as far as
	  possible and finally the ST found will be put in globst.

	  RMM: now it is permissible to modify the line number with
	  an expression of the form +nn or -nn.

	  Later we will permit the form: ID.ID.ID as well as ID.n.n

	  if gflg is 1, then we don't want a value, only the globst
	*/

	int	i;
	int	j;

/*DEB*/if(debug>4)fprintf(stderr,"resolveid( %s, %n )\n",s,thisln);
	globst = nest;
	for(;;){

		if( *s == '.' ) s++;
		i = j = 0;
		if( *s == '+' || *s == '-' ) j = *s++;
		while( *s >= '0' && *s <= '9' ) i = i*10 + *s++ - '0';
		if( j ){
			if( j == '-' ) i = -i;
			if( thisln ){
				while( i < 0 && thisln->ln_cnt != -1 ) thisln--;
				while( i > 0 && thisln->ln_cnt != -1 ) thisln++;
			}
			i = 0;
			if( *s == '.' ) s++;
			while( *s >= '0' && *s <= '9' ) i = i*10 + *s++ - '0';
		}
		if( i < thisln->ln_cnt ) globst = thisln->ln_st[i];
		if( *s != '.' ) break;
		thisln = tlntoln(globst->st_tln);
		if( gflg && thisln == NULL && globst != nest )
			warn("not an index: %s",globst->st_stg);
	}
	return s;
}

char* prime(char* s, LN* thisln, int gflg) {

	int	i;
	short	op;
	short	radix;
	ST	*thisst;
	OL	*thisol;
	char	*s2;

	/* primes are unaries followed by primes,
	  ( <expr> ), constants of hex, decimal, or octal, or
	  bitmaps, or symbols that have an ordinal assigned, or
	  references to a current field */

	/* if gflg is 1, then we only want the globst set */

	globst = nest;
	exprval = 0;
	if( *s == '(' ){
		s2 = expr( s+1, thisln, gflg );
		if( *s2 != ')' ) err("missing ')' in %s at ^%s",s,s2 );
		return s2+1;
	}
	if( among( *s, "+-~!" ) ){
		op = *s;
		s = prime( s+1, thisln, gflg );
		if( op == '-' ) exprval = -exprval; else
		if( op == '~' ) exprval = ~exprval; else
		if( op == '!' ) exprval = !exprval;
		goto retnow;
	}
	if( *s == '_' ){		/* scan a bit map */
		while( *s == '_' || *s == '0' || *s == '1' ){
			if( *s == '0' || *s == '1' ){
				exprval <<= 1;
				if( *s == '1' ) exprval++;
			}
			s++;
		}
		goto retnow;
	}
	if( isalpha( *s ) ){
		s = idscan( s );
		if( *s == '(' && strcmp( idbuf, "P" ) == 0 ){
			s2 = prime(s+1,thisln,1);
			if( *s2 != ')' ) err("missing ')' in %s at |%s",s,s2 );
			s = s2+1;
			exprval = 0;
			if( globst == nilst ) goto retnow;
			if( globst != nest ) exprval = 1;
			goto retnow;
		}
		globst = thisst = allocst( idbuf );
		thisln = NULL;
		if( *s == '.' ){

			/* lookfor ID.[+-nn.]n */

			thisln = tlntoln( thisst->st_tln );
			if( gflg ) goto retnow;
			if( thisln == NULL ){
				warn("%s has no value",idbuf);
				goto retnow;
			}
			goto resit;
		}
		if( gflg ) goto retnow;
		if( thisst->st_ord == -1 ){
			warn("value of 0 assumed for %s",idbuf);
			thisst->st_ord = 0;
		}
		exprval = thisst->st_ord;
		goto retnow;
	}
	if( isdigit( *s ) ){
		radix = 10;
		if( *s == '0' ){
			radix = 8;
			s++;
			if( *s == 'x' || *s == 'X' ){
				radix = 16;
				s++;
			}
		}
		while( isdigit(*s) || radix == 16 && isxdigit(*s) ){
			i = *s++;
			exprval *= radix;
			if( i >= '0' && i <= '9' ) i -= '0'; else
			if( i >= 'a' && i <= 'f' ) i -= 'a' - 10; else
			if( i >= 'A' && i <= 'F' ) i -= 'A' - 10;
			exprval += i;
		}
		goto retnow;
	}
	if( *s == '$' ){
		if( thisln == NULL ) err("no context");
		s++;
resit:		s = resolveid( s, thisln, gflg );
		if( gflg || globst == nilst || globst == nest ) goto retnow;
		exprval = globst->st_ord;
		if( globst->st_ord == -1 ) expr( globst->st_stg, thisln, gflg );
		goto retnow;
	}
	err("what?? %s",s);
retnow:	return s;
}


char* expr(char* s, LN* thisln, int gflg) {

	long	eval1;
	short	op;

	/* if gflag is set, then we only want the globst set */

	op = *s;
	s = prime( s, thisln, gflg );
	eval1 = exprval;
	for(;;){
		exprval = eval1;
		if( !among( *s, "+-*/%|&^<>" ) ) return s;
		op = *s++;
		if( op == '<' ){
			if( *s != '<' ) return s-1;
			s++;
		} else
		if( op == '>' ){
			if( *s != '>' ) return s-1;
			s++;
		}
		s = prime( s, thisln, 0 );
		switch( op ){
	case '+':	eval1 += exprval;	continue;
	case '-':	eval1 -= exprval;	continue;
	case '*':	eval1 *= exprval;	continue;
	case '/':	if( exprval != 0 ) eval1 /= exprval; continue;
	case '%':	if( exprval != 0 ) eval1 %= exprval; continue;
	case '&':	eval1 &= exprval;	continue;
	case '|':	eval1 |= exprval;	continue;
	case '^':	eval1 ^= exprval;	continue;
	case '<':	eval1 <<= exprval;	continue;
	case '>':	eval1 >>= exprval;	continue;
		}
		err("op = %c",op);
	}
}

void cvt(long val, int radix) {

	int	i;
	cvtx = 0;
	switch( radix ){

default:	err("bad radix: %d",radix);
		return;

case 16:	while( cvtx < 8 ){
			cvtbuf[cvtx++] = hex[val & 0xf];
			val >>= 4;
			if( val == 0 ) return;
		}
		return;
case 8:		while( cvtx < 10 ){
			cvtbuf[cvtx++] = hex[val & 0x7];
			val >>= 3;
			if( val == 0 ) return;
		}
		cvtbuf[cvtx++] = hex[val & 0x3];
		return;
case 10:	if( val < 0 ){
			val = -val;
			if( val < 0 ){
				strcpy( cvtbuf, "-4000000000" );
				cvtx = 11;
				return;
			}
			cvt( val, radix );
			cvtbuf[cvtx++] = '-';
			return;
		}
		do {
			cvtbuf[cvtx++] = (int)(val % 10) + '0';
			val /= 10;
		} while( val );
		return;
	}
}

void outval(int let, long val) {

	char	*fmt;

	switch( let ){

case 'h':	if( val <= 9 ){
			*xlp++ = val + '0';
			return;
		}
		cvt( val, 16 );
		if( cvtbuf[cvtx-1] > '9' ) cvtbuf[cvtx++] = '0';
		while( --cvtx >= 0 ) *xlp++ = cvtbuf[cvtx];
		*xlp++ = 'h';
		return;

case 'd':
deccvt:		cvt( val, 10 );
cpycvt:		while( --cvtx >= 0 ) *xlp++ = cvtbuf[cvtx];
		return;

case 'x':	if( hexchg ){
			fmt = hexfmt;
			goto spr;
		}
		cvt( val, 16 );
		if( cvtx > 1 || cvtbuf[cvtx-1] > '9' ){
			cvtbuf[cvtx++] = 'x';
			cvtbuf[cvtx++] = '0';
		}
		goto cpycvt;
case 'o':	if( !octchg ){
			cvt( val, 8 );
			goto cpycvt;
		}
		fmt = octfmt;
		if( val >= 0 && fmt != decfmt ){
			if( fmt == octfmt ){
				if( !octchg && val < 8 ) fmt = decfmt;
			} else {
				if( !hexchg && val < 11 ) fmt = decfmt;
			}
		}
spr:		sprintf(xlp,fmt,val);
		xlp += strlen(xlp);
		return;

	}
	err("odd conversion: <%c>",let);
}

char *xlate(char *s, LN *thisln ) {

	/* a '%' has been scanned, continue with the translation */

	int	i;

/*DEB*/if(debug>5)fprintf(stderr,"xlate( %s, - )\n",s);
	switch( i = *s++ ){
case 'c':	s = expr( s, thisln, 0 );
		i = exprval & 0xff;
		if( i < ' ' || i > 0176 ){
			sprintf(xlp,"'\\%o'",i);
			xlp += strlen(xlp);
			break;
		}
		*xlp++ = '\'';
		if( i == '\\' || i == '\'' ) *xlp++ = '\\';
		*xlp++ =  i;
		*xlp++ =  '\'';
		break;

case 'h':
case 'd':
case 'x':
case 'o':	s = expr( s, thisln, 0 );
		outval( i, exprval );
		break;

case 's':	s = string( s, thisln );
		break;

default:	*xlp++ = '%';
		*xlp++ = i;
		break;
	}
	return s;
}

void doline(char* s, LN* thisln) {

	/* translate one line in the context of a specific line */

	int		i;
	ST		*thisst;


	xlp = oline;			/* set up the output line ptr */
	while( *s ){
		if( isalnum(*s) ){
			s = idscan(s);
			thisst = findst(idbuf);
			if( thisst == NULL ||
			   thisst->st_tln == -1 ||
			   thisst->st_ord == -1 ){
				pts( idbuf );
				continue;
			}
			/* now resolve an affix chain */
			if( *s == '.' ){
				thisln = tlntoln( thisst->st_tln );
				if( thisln == NULL ){
					warn("not index %s",idbuf);
					*xlp++ = '0';
					continue;
				}
				s = resolveid( s+1, thisln, 1 );
				thisst = globst;
				if( thisst == nest ){
					warn("no field");
					*xlp++ = '0';
					continue;
				}
				if( thisst->st_ord == -1 ){
					expr(thisst->st_stg,NULL,0);
					sprintf(xlp,"%ld",exprval);
					xlp += strlen(xlp);
					continue;
				}
			}
			sprintf(xlp,"%d",thisst->st_ord);
			xlp += strlen(xlp);
			continue;
		}
		if( isdigit(*s) ){
			while( isalnum(*s) ) *xlp++ = *s++;
			continue;
		}
		switch( i = *s++ ){
	case '`':	continue;
	case '\\':	i = *s++;
	default:	*xlp++ = i;
			continue;
	case '%':	s = xlate( s, thisln );
			continue;
		}
	}
	while( xlp > oline && xlp[-1] == ' ' ) xlp--;
	*xlp = 0;
	s = oline;
	while( i = *s++ ) putchar( i );
	putchar( '\n' );
}


int gethash(char* s) {

	int	i;

	for( i=0; *s; s++ ) i = (i << 3) + *s;
	return i & (HASHSIZ-1);
}

char* idscan(char* s) {

	char	*p;

	p = idbuf;
	while( isalnum(*s) || *s == '_' ) *p++ = *s++;
	*p = 0;
	return s;
}

ST* findst(char* s) {

	ST	*x;

	for( x = hash[gethash(s)]; x; x = x->st_prev )
		if( strcmp(s,x->st_stg) == 0 ) break;
	return x;
}


ST* allocst(char* s) {

	/* make up a ST entry for a string */

	int		i;
	char	*r;
	ST		*x;


	if( s == NULL || *s == 0 ) return NULL;
	x = findst( s );
	if( x ) return x;
	x = (ST *)malloc( sizeof(ST) + strlen(s) - 1 );
	strcpy( x->st_stg, s );
	x->st_tln = x->st_ord = -1;
	i = gethash(s);
	x->st_prev = hash[i];
	hash[i] = x;
	return x;
}



ST *getfield(int f){

	/* get the next field: format letter is S, C, E */

	/* on entry, the scanning pointer, lp, is positioned at the
	  first character of the next field,  after a field is processed
	  trailing white space is skipped */

	/* if the format code is S, a string is expected.  If surrounded
	  by quote marks, they will be deleted.

	  If the format code is C, a single character is expected.  If
	  surrounded by a single quote (apostrophe), they will be
	  deleted.  In this case an apostrophe is represented as '\''.
	  The character is converted to a hex string for future use.

	  If the format code is E, all non-blanks will be scanned with
	  the possibility of a comma.

	*/

	char	*fp;
	char	*cp;
	int		stch;

	fp = field;
	cp = lp;

	stch = 0;
	if( *cp == 0 || *cp == ';' ) goto trail;
	switch( f ){

case 'E':	while( *cp && !among(*cp," \t,;") ) *fp++ = *cp++;
		break;

case 'S':	if( *cp == '"' ) stch = *cp++;
		while( *cp ){
			if( stch != '"' && among(*cp," \t,;") ) break;
			if( *cp == '"' && stch == '"' ){ cp++; break; }
			if( *cp == '\\' ) *fp++ = *cp++;
			*fp++ = *cp++;
		}
		break;

case 'C':	if( *cp == '\'' ) stch = *cp++;
		if( *cp == '\\' ) cp++;
		sprintf(field,"0x%x",*cp & 0xff );
		cp++;
		if( stch == '\'' ){
			if( *cp != stch ) warn("missing '"); else cp++;
		}
		while( *fp ) fp++;
		break;
	}

trail:	*fp = 0;
	while( isspace(*cp) ) cp++;
	if( *cp == ',' ){
		stch = 1;
		cp++;
		while( isspace(*cp) ) cp++;
	}
	lp = cp;
	if( fp == field ) return stch ? nilst : nest;

		/* note: preceding is to distinguish between fields not
		  on line and those explicitly null */

/*DEB*/if(debug > 4)fprintf(stderr,"field returns: <%s>\n",field);

	if( field[0] == '-' && field[1] == 0 ){
		if( f == 'E' ||
		   f == 'S' && stch != '"' ||
		   f == 'C' && stch != '\'' )
			return nilst;
	}
	return allocst(field);
}

void usage() {

	fprintf(stderr, "usage: mktable [-d] filename\n");
	exit(0);
}

void err(char *s, ...) {

	va_list argptr;
	va_start(argptr, s);
	fprintf(stderr, "%3d error:	", lineno);
	vfprintf(stderr, s, argptr);
	fprintf(stderr,"\n");
	va_end(argptr);
	exit(1);
}

void warn(char *s, ...) {

	va_list argptr;
	va_start(argptr, s);
	fprintf(stderr, "%3d warn:	", lineno);
	vfprintf(stderr, s, argptr);
	fprintf(stderr, "\n");
	va_end(argptr);
}

int getaline() {

	int		i;
	char	*p;


	lineno++;
	fpx = 0;
	p = line;
	arena[0] = 0;
	while( (i = getchar()) != '\n' ){
		if( i == EOF ){
			line[0] = 0;
			eof++;
			return 0;
		}
		*p++ = i;
	}
	*p = 0;
/*DEB*/if(debug>3)fprintf(stderr,"%d	%s\n",lineno,line);
	if( line[0] == ';' ) line[0] = 0;
	return 1;
}


void pts(char* s) {

	char	*p;

	p = xlp;
	while( *p++ = *s++ );
	xlp = p-1;
}


int among(int ch, char* strings) {

	int	i;

	while( i = *strings++ )
		if( i == ch ) return 1;
	return 0;
}

void setout(char *s) {		/* reset output */

	char	*r;
	if( s == NULL ) return;
	while( isspace(*s) ) s++;
	if( *s == 0 ) return;
	for( r=s; *r && !isspace(*r); r++);
	*r = 0;
	freopen( s, "w", stdout );
}

void dbtrap() { }
