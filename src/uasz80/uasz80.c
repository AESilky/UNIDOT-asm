/************************************************************************
*									*
*	Copyright (C) 1982,1985,1987 by Unidot, Inc.			*
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
*			     UAS Assembler				*
*									*
*			uasz80.c - z80 specific code			*
*									*
************************************************************************/

static char rcsid[]=
"@(#)$Header: uasz80.c,v 3.6 87/12/01 13:30:05 rmm Rel $ z80 specific code";

#ifndef NOPD
#define NOPD	/* ES: Define this here to aid code highlighting (is in makefile) */
#endif

#ifdef vms
#include "[-.uascom]uas.h"
#include "[-.incl]uobj.h"
#else
#include "../uascom/uas.h"
#include "../incl/uobj.h"
#include "../uascom/funcdefs.h"
#endif
#include "uasz80.h"

#include <string.h>

#ifdef NOPD
#include "uz80pd.h"
#endif

/* Definitions (local) */

int opmatch(struct format* fmp);


/*
 * Global variable definitions and initializations specific to z80.
 */

char chclass[128] ={		/* Character class table */
	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	I,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	I,	0,
	D,	D,	D,	D,	D,	D,	D,	D,
	D,	D,	0,	0,	0,	0,	0,	0,
	0,	I,	I,	I,	I,	I,	I,	I,
	I,	I,	I,	I,	I,	I,	I,	I,
	I,	I,	I,	I,	I,	I,	I,	I,
	I,	I,	I,	0,	0,	0,	0,	I,
	0,	I,	I,	I,	I,	I,	I,	I,
	I,	I,	I,	I,	I,	I,	I,	I,
	I,	I,	I,	I,	I,	I,	I,	I,
	I,	I,	I,	0,	0,	0,	0,	0,
};
struct	chent	chtab[] ={	/* table of single-character tokens */
	{ '~', TKUNOP },{ '*', TKMULOP, TVMUL },
	{ '/', TKMULOP, TVDIV },{ '%', TKMULOP, TVMOD },
	{ '+', TKADDOP, TVADD },{ '-', TKADDOP, TVSUB },
	{ '=', TKRELOP, TVEQ },{ '&', TKANDOP },
	{ '^', TKXOROP },{ '|', TKOROP },
	{ '(', TKLPAR },{ ')', TKRPAR },
	{ ',', TKCOM },{ ':', TKCOLON },
	{ '\n', TKEOL },{ -1, TKEOF },
	{ 0, TKERR } };

int		extoff = 2;
char		hlflg = 0;
char		ixiy = 0;
char		ixiyi = 0;
uns		ixiyr = 0;
long		ixiyv = 0;

struct	operand	optab[OPMAX] = {NULLCA};
/*
 * direc - Processes assembler directives which are special to this
 * assembler.  Calls dircom to process other directives.
 */

void direc( dirnum ) int dirnum;{


	if( dirnum != ADMAC )			/* look up the label */
		label = *labstr ? sylook( labstr ): 0;
	dircom( dirnum );
}

/*
 * equ - Handles the .equ and .set directives.  The argument is the type
 * of symbol to define (label or variable).  Some assemblers have special
 * interest code here to allow assigning symbols to registers or other
 * keywords.
 */

void equ( symtype ) int symtype;{


	if( !label ) nolabel();
	if( toktyp != TKSPC ){
		error("103 .equ should be followed by a space and an operand");
		goto skip;
	}
	iilex();
	if( iiparse() != 0 || !( curop.op_cls&1L << OCEXP )){
		error("103 Syntax error in .equ operand");
skip:		skipeol();
		return;
	}
	stdequend(symtype);
}
/*
 * iilex - Lexical scanner for expression parsing.  Calls token and does
 * further processing as required by the isp parser.
 */

int iilex(){

	sytab_t	*syp;

	iilexeme.ps_sym = token();
	iilexeme.ps_val0 = tokval;
	if( iilexeme.ps_sym == TKSYM ){ /* symbol */
		iilexeme.ps_val0 = (long) sylook( tokstr );
		syp = (sytab_t *) rfetch(( VMADR ) iilexeme.ps_val0 );
		if( syp->sy_typ == STKEY ){ /* keyword */
			iilexeme.ps_val0 = (uns)syp->sy_val & 0xff;
			iilexeme.ps_sym = (uns)syp->sy_val >> 8&0xff;
			if( iilexeme.ps_sym == TKAF ){ /* check for af' */
				scanc();
				if( ch == '\'' ) iilexeme.ps_sym = TKAFP;
				else unscanc();
			}
		}
	} else
	if( iilexeme.ps_sym == TKCOM ||
	    iilexeme.ps_sym == TKSPC ||
	    iilexeme.ps_sym == TKEOL ) iilexeme.ps_sym = TKEOF;
	iilexeme.ps_val1 = PSVAL1(iilexeme.ps_val0);
	return( iilexeme.ps_sym );
}

/*
 * inops - Reads the operands for a machine instruction and leaves their
 * descriptions in optab.
 */

void inops(){
	struct operand	*opp;

	if( toktyp == TKSPC ) iilex();
	for( opp = optab; opp < optab+OPMAX; opp++ )
		opp->op_cls = 1L << OCNULL;
	for( opp = optab; opp < optab+OPMAX; opp++ ){
		if( toktyp == TKEOL ) continue;
		if( iiparse() != 0 ){
			error("101 syntax error in operand");
			skipeol();
			return;
		}
		*opp = curop;
		delim();
	}
}
/*
 * instr - Generates the specified machine instruction.
 */

void instr( fmpa ) VMADR fmpa;{


	uns		v;
	uns		r;
	int		i;
	char		skel;
	struct format*	fmp = (struct format*)fmpa;

	label = *labstr ? sylook( labstr ): 0;
	lcassign();
	hlflg = ixiy = ixiyi = 0;
	inops();			/* read the instruction operands */
	while(!opmatch( fmp )){	/* scan for matching format entry */
		if( fmp->fm_flg&FMLAST ){
			error("105 Invalid operands");
			return;
		}
		fmp++;
	}

	/* Check for some special cases involving the ix and iy registers.  */

	if( (hlflg||fmp->fm_flg&FMNIXIY) && ixiy ||
	    fmp->fm_flg&FMNDISP && ixiyi && ixiyv ){
		error("125 illegal use of ix or ix register");
		return;
	}

	/* Emit the required prefix bytes.  */

	if( ixiy ) emitb( ixiy, 0 );
	if( fmp->fm_flg&FMCB ) emitb( 0xcb, 0 );
	else if( fmp->fm_flg&FMED ) emitb( 0xed, 0 );

	/* Build up the opcode skeleton byte.  */

	skel = fmp->fm_skel;
	for( i = 0; i < OPMAX; i++ ){
		v = optab[i].op_val;
		switch( fmp->fm_op[i]&OAMSK ){

		case OAL3:	/* left 3-bit field */
			skel |= v << 3&070;
			break;

		case OAR3:	/* right 3-bit field */
			skel |= v&07;
			break;

		case OARST:	/* restart address */
			if( v&~070 )
				error("123 Restart address not legal");
			skel |= v&070;
			break;

		case OAINT:	/* interrupt mode */
			if( v == 1 ) skel |= 020;
			else if( v == 2 ) skel |= 030;
			else if( v != 0 ) error("124 Not legal interrupt mode");
			break;

		}
	}

	/*
	 * Emit the (ix+d) or (iy+d) offset byte if one is needed and we
	 * are ready for the third byte.
	 */

	oneed(12);
	if( ixiyi && fmp->fm_flg&( FMCB|FMED ) && !( fmp->fm_flg&FMNDISP ))
		emitb(( uns ) ixiyv, URAA8|ixiyr );

	/* Emit the opcode byte.  */

	emitb( skel, 0 );

	/* Try again for the (ix+d) or (iy+d) offset byte.  */

	if( ixiyi && !( fmp->fm_flg&( FMCB|FMED|FMNDISP )))
		emitb(( uns ) ixiyv, URAA8|ixiyr );

	/* Emit trailing operand bytes.  */

	for( i = 0; i < OPMAX; i++ ){
		v = optab[i].op_val;
		r = optab[i].op_rel;
		switch( fmp->fm_op[i]&OAMSK ){

		case OA8:	/* 8-bit field */
			emitb( v, URAA8|r );
			break;

		case OA16:	/* 16-bit field */
			emitw( v, URAA16|r );
			break;

		case OA8R:	/* 8-bit field, pc relative */
			v -= curloc/curadu+1;
			if((int) v < -128 || (int) v >= 128 )
				error("106 Target out of range of short jump");
			else if( r != cursec )
		error("118 Short jump to external label or another section");
			emitb( v, 0 );
			break;
		}
	}
	return;
}
/*
 * opmatch - Returns 1 if the specified format entry matches the operands
 * in optab, 0 otherwise.
 */

int opmatch(struct format* fmp) {
	int	i;

	for( i = 0; i < OPMAX; i++ )
		if( !( 1L <<( fmp->fm_op[i]&OCMSK )&optab[i].op_cls ))
			return 0;
	return 1;
}
/*
 * predef - Reads the predefined symbols into the symbol table.
 */

void predef(){

	struct	format	*fmp;
	struct	octab	*ocp;
	struct	sytab	*syp;
	int		i;
	VMADR	val;
	extern char	objsuf[], lstsuf[];
	extern char	*lstfmt;

	val = VALN(4);				/* don't use virtual zero */
	errnum = 1;				/* number errors	*/
	strcpy( proctype, "z80" );		/* set processor type	*/

#ifndef NOPD

	while( token() == TKCON ){ /* read machine instructions */

		if( (uns) phytop & (ALIGN-1) ) phytop++; /* force alignment */
		val = (int) phytop;
		for(;;){			/* read format table entries */

			fmp = (struct format *)palloc(sizeof(struct format));
			for( i = 0; i < OPMAX; i++ ){ /* operand descriptors */
				fmp->fm_op[i] = tokval;
				preget( TKSPC );
				preget( TKCON );
			}
			fmp->fm_skel = tokval;
			preget( TKSPC );
			fmp->fm_flg = preget( TKCON );
			if( token() != TKEOL ) break;
			preget( TKCON );
		}
		fmp->fm_flg |= FMLAST;
		while( toktyp == TKSPC ){ /* read instruction mnemonics */
			preget( TKSYM );
			ocp = oclook( tokstr );
			ocp->oc_typ = OTINS;
			ocp->oc_val = val;
			token();
		}
		if( toktyp != TKEOL ) badpre();
	}
	if( toktyp != TKEOL ) badpre();
	while( token() == TKCON ){		/* read assembler directives */
		val = tokval;
		while( token() == TKSPC ){	/* read directive mnemonics */
			preget( TKSYM );
			ocp = oclook( tokstr );
			ocp->oc_typ = OTDIR;
			ocp->oc_val = val;
		}
		if( toktyp != TKEOL ) badpre();
	}
	if( toktyp != TKEOL ) badpre();
	while( token() == TKCON ){		/* read predefined symbols */
		val = tokval;
		while( token() == TKSPC ){	/* read symbol mnemonics */
			preget( TKSYM );
			syp = (sytab_t *) wfetch( sylook( tokstr ));
			syp->sy_typ = STKEY;
			syp->sy_val = val;
			syp->sy_atr = SADP2;
		}
		if( toktyp != TKEOL ) badpre();
	}
	if( toktyp != TKEOL ) return;		/* no objsuf or lstsuf */
	scanc();
	while( white(ch) || ch == '\n' ) scanc();
	if( ch == ';' || eof(ch) ) goto out;
	p = objsuf;
	while( !white(ch) && ch != '\n' && ch != ';') *p++ = ch, scanc();
	*p = 0;
	while( white(ch) ) scanc();
	if( ch == ';' ){
		while( ch != '\n' ) scanc();
		scanc();
	}
	if( eof(ch) ) return;
	while( white(ch) ) scanc();
	p = lstsuf;
	while( !white(ch) && ch != '\n' && ch != ';' ) *p++ = ch, scanc();
	*p = 0;
#else			/* install the compiled tables */
	/* first the opcodes */
	for( i=0; opctab[i].oc_str[0]; i++ ){
		if( opctab[i].oc_typ == OTINS )
			opctab[i].oc_val = (VMADR)&fmt[(uns)(opctab[i].oc_val)];
		opcinsert( &opctab[i] );
	}
	/* now the reserved words */
	for( i=0; rsw[i].rw_str; i++ ){
		val = sylook(rsw[i].rw_str);
		syp = (sytab_t *)wfetch(val);
		syp->sy_typ = STKEY;
		syp->sy_val = SYVAL((unsigned long)rsw[i].rw_val);
		syp->sy_atr = SADP2;
	}
	/* now the suffixes */
	strcpy( objsuf, xobjsuf );
	strcpy( lstsuf, xlstsuf );
#endif			/* of installing the compiled tables	*/

}
/*
 * sem51 - Parser semantic routines specific to asz80.
 */

void sem51(int sem) {


	struct psframe	*p,
			*pl;
	struct sytab	*syp;

	p = iipsp;
	pl = iipspl;
	switch( sem ){

	case 51:	/* <operand> ::= a */
		curop.op_cls = ( 1L << OCR8M )|( 1L << OCR8 )|( 1L << OCA );
		curop.op_val = 7;
		break;

	case 52:	/* <operand> ::= c */
		curop.op_cls = ( 1L << OCR8M )|( 1L << OCR8 )|( 1L << OCC );
		curop.op_val = 1;
		break;

	case 53:	/* <operand> ::= ( hl ) */
		curop.op_cls = ( 1L << OCR8M )|( 1L << OCIHL );
		curop.op_val = 6;
		hlflg = 1;
		break;

	case 54:	/* <operand> ::= scc */
		curop.op_cls = ( 1L << OCSCC )|( 1L << OCLCC );
		curop.op_val = p->ps_val0;
		break;

	case 55:	/* <operand> ::= lcc */
		curop.op_cls = ( 1L << OCLCC );
		curop.op_val = p->ps_val0;
		break;

	case 56:	/* <operand> ::= ( <ixiy> ) */
		curop.op_cls = ( 1L << OCR8M )|( 1L << OCIHL );
		curop.op_val = 6;
		ixiyv = p[1].ps_val0;
		ixiyr = PSVAL1_UI(p[1].ps_val1);
		break;

	case 57:	/* <ixiy> ::= zr16 */
		ixiy = ixiyi = p->ps_val0;
		pl->ps_val0 = pl->ps_flg = 0;
		pl->ps_val1 = PSVAL1(0);
		break;

	case 58:	/* <operand> ::= zr8 */
		curop.op_cls = ( 1L << OCZR8 );
		curop.op_val = p->ps_val0;
		break;

	case 59:	/* <operand> ::= ( c ) */
		curop.op_cls = ( 1L << OCIC );
		break;

	case 60:	/* <operand> ::= hl */
		curop.op_cls = ( 1L << OCSSDD )|( 1L << OCQQ )|( 1L << OCHL );
		curop.op_val = 4;
		hlflg = 1;
		break;

	case 61:	/* <operand> ::= sp */
		curop.op_cls = ( 1L << OCSSDD )|( 1L << OCSP );
		curop.op_val = 6;
		break;

	case 62:	/* <operand> ::= ( sp ) */
		curop.op_cls = ( 1L << OCISP );
		break;

	case 63:	/* <operand> ::= bc */
		curop.op_cls = ( 1L << OCSSDD )|( 1L << OCQQ );
		curop.op_val = 0;
		break;

	case 64:	/* <operand> ::= ( bc ) */
		curop.op_cls = ( 1L << OCIBCDE );
		curop.op_val = 0;
		break;

	case 65:	/* <operand> ::= de */
		curop.op_cls = ( 1L << OCSSDD )|( 1L << OCQQ )|( 1L << OCDE );
		curop.op_val = 2;
		break;

	case 66:	/* <operand> ::= ( de ) */
		curop.op_cls = ( 1L << OCIBCDE );
		curop.op_val = 2;
		break;

	case 67:	/* <operand> ::= af */
		curop.op_cls = ( 1L << OCQQ )|( 1L << OCAF );
		curop.op_val = 6;
		break;

	case 68:	/* <operand> ::= af' */
		curop.op_cls = ( 1L << OCAFP );
		break;

	case 69:	/* <operand> ::= zr16 */
		curop.op_cls = ( 1L << OCSSDD )|( 1L << OCQQ )|( 1L << OCHL );
		curop.op_val = 4;
		if( ixiy && ( ixiy&0xff )!= p->ps_val0 ) /* ix,iy both used */
			curop.op_cls = 0;
		ixiy = p->ps_val0;
		break;

	case 70:	/* <operand> ::= r8 */
		curop.op_cls = ( 1L << OCR8M )|( 1L << OCR8 );
		curop.op_val = p->ps_val0;
		break;

	}
}
