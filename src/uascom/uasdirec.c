/************************************************************************
*									*
*	Copyright (C) 1982,1985, by Unidot, Inc.			*
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
*			uas.direc.c - process directives		*
*									*
************************************************************************/


static char rcsid[] =
"@(#)$Header: uasdirec.c,v 6.25 89/04/13 07:14:43 rmm Rel $ uas directives";

#include "uas.h"
#ifdef vms
#include "[-.incl]uobj.h"
#else
#include "../incl/uobj.h"
#endif
#include "funcdefs.h"		/* Forward defines for GCC */

#include <string.h>

int expression(char* s, int strok, int norel);
void labnotok();
void newloclabs();
int noextlab();
int nonrelex(char* s);
void noopnd();
void notexpr(char* s);
void notrel(char* s);
int scanstr(char* s);
void title(char* msg, char* s);
int usingreg(char* s);


static short	loclabskips;		/* useless .loclabs to skip	*/
/*
 * dircom - Performs the assembler directives common to all versions.
 */

void dircom( dirnum ) int dirnum;{


	sytab_t	*syp;
	char	*sp;
	int		i;
	reg VMADR	sym;
	int		j;
	int		k;
	long		l;
	int		datbit;
	uns		datrel;
	int		dataln;
	char		cond;
	char		llsave;
	reg VMADR	sym2;
	mchain_t		*mchp;
	grchain_t		*grpp;

	if( label ) nopend();
	switch( dirnum ){

	case ADABS:	/* .abs */

		setsec( 0 );
		lcassign();
		break;

	case ADALIGN:	/* .align */

		if( !nonrelex(".align") ) break;
		if( 0L <= curop.op_val && curop.op_val <= 16L ){
			i = (int) curop.op_val;
			if( i > curaln ) curaln = i;
			i = 1 << i;
			lcalign( i*curadu );
			if( pass2 ) setorg();
		} else
			notexpr( "alignment");
		lcassign();
		break;

	case ADBLOCK:	/* .block */

		lcalign( bytaln );
		lcassign();
		if( !nonrelex(".block") ) break;
		curloc += curop.op_val * curadu;
		if( pass2 ) setorg();
		break;

	case ADBYTE:	/* .byte */

		datbit = bytbit;
		datrel = urabyte;
		dataln = bytaln;
		goto datscan;

	case ADCLIST:	/* .clist */

		if( nonrelex( ".clist" ) ) condlst = curop.op_val != 0;
		labnotok();
		mexprint();
		break;

	case ADCOMM:	/* .comm */

		/* this implements the equivalent needed by most C compilers
		   for variables that are materialized by the linker,
		   --added RMM 11/9/85----				*/

		if( noextlab() ) break;
		if( !nonrelex( ".comm" ) ) break;
		l = curop.op_val;
		/*
		if( curadu > 8 ) l *= (curadu + 7)/8;  WRONG! RMM
		*/
		if( l < 1 || l > 0xffffffL ){
			error( "13 Value not in range 1-0ffffffh");
			l = 2;
		}
		delim();
		if( toktyp != TKEOL ){
			if( !expression(".comm",NOSTR,NOREL) ) goto skip;
			if( curop.op_val < 0 || curop.op_val > 16 ){
				error("13 Value not in range 0-16");
				curop.op_val = 1;
			}
			l |= curop.op_val << 24;	/* set align */
		} else {
			if( l > 1 ) l |= 0x1000000L;  /* alignment req */
		}
		/*
		if( curadu < 8 ) l |= (long)curadu << 28; ALSO WRONG RMM
		*/
		skipeol();
		if( pass2 ) break;
		syp = (sytab_t *) wfetch( label );
		if( syp->sy_typ != STUND || syp->sy_val != 0 )
			syp->sy_atr |= SAMUD;
		syp->sy_atr |= SAGLO;
		syp->sy_val = SYVAL(l);
		xref( sym, 0 );
		break;

	case ADCOMMON:	/* .common */

		if( noextlab() ) break;
		syp = (sytab_t *) rfetch( label );
		if( syp->sy_typ == STSEC && (!pass2||syp->sy_atr&SADP2))
			setsec( syp->sy_rel ); /* old section */
		else	
			newsec(0); /* start a new common section */
		sectab[cursec].se_atr = curatr |= USECOM;
		goto tryrwx;

	case ADDROP:	/* .drop	*/

		token();
		labnotok();
		if( toktyp != TKSYM ) ulx = 0;	/* clear all usings */
		while( toktyp == TKSYM ){
			if( (k = usingreg(tokstr)) == -1 ) goto skip;
			for( i=0; i<ulx && ulist[i].us_reg != k; i++ );
			if( i != ulx ) ulist[i] = ulist[--ulx];
			else warn("50 Register not currently in using list");
			token();
			delim();
		}
		break;

	case ADDSECT:	/* .dsect */

		if( label ){
			if( noextlab() ) break;
			syp = (sytab_t *)rfetch(label);
			if( syp->sy_typ == STSEC &&
			    (!pass2||syp->sy_atr&SADP2 )){
				setsec( syp->sy_rel );	/* continue old sect */
				goto tryrwx;
			}
		}
		newsec(1);		/* start new section */
		goto tryrwx;

	case ADEJECT:	/* .eject */

		labnotok();
		if( llfull ) llfull = linect = 0;
		break;

	case ADELSE:	/* .else */

		labnotok();
		if( truelev ) truelev--;
		else error("15 Out of place .else");
		mexprint();
		break;

	case ADEND:	/* .end */

		labnotok();
		if( toktyp == TKSPC ){ /* read transfer address */
			iilex();
			expression(".end",NOSTR,RELOK);
			if( pass2 ){ /* output transfer address */
				oflush();
				objtyp = UOBTRA;
				oputl(( long ) curop.op_val );
				oputw( curop.op_rel );
			}
		}
		reading = 0;
		mexprint();
		break;

	case ADENDIF:	/* .endif */

		labnotok();
		if( condlev ){
			condlev--;
			truelev--;
		} else
			error( "16 Out of place .endif");
		mexprint();
		break;

	case ADEQU:	/* .equ */

		equ( STLAB );
		break;

	case ADERROR:	/* .error */

		labnotok();
		if( pass2 && scanstr(".error") ){
			errct++;
			immmsg( "Error   ", savstr );
		}
		break;

	case ADEXIT:	/* .exit */

		labnotok();
		if( infp && (infp->in_typ == INMAC||infp->in_typ == INRPT) ){
			if( infp->in_typ == INMAC ) mexlev--;
			popin();
		} else
			error( "18 No repeat or macro in progress");
		mexprint();
		break;

	case ADGLOB:	/* .global */

		labnotok();
		if( toktyp != TKSPC ) goto noop;
		token();
		while( toktyp == TKSYM ){
			sym = sylook( tokstr );
			syp = (sytab_t *) wfetch( sym );
			syp->sy_atr |= SAGLO;
			xref( sym, 0 );
			token();
			delim();
		}
		break;

	case ADGROUP:	/* .group */

		if( noextlab() ) break;
		syp = (sytab_t *) wfetch( sym );
		if( syp->sy_typ == STUND ) syp->sy_typ = STGRP;
		if( syp->sy_typ != STGRP ){
			error("38 Symbol can't be used for group, it's in use");
			goto skip;
		}
		for( i=0; i<grpx; i++ )
			if( grptab[i].gr_sym == label ) break;
		if( i >= grpx ){
			if( i >= 8 ) fatal("79 Too many groups (limit is 8)");
			grptab[grpx++].gr_sym = label;
		}
		if( toktyp != TKSPC ) goto noop;
		token();
		while( toktyp == TKSYM ){
			sym = sylook( tokstr );
			token();
			if( (sym2 = grptab[i].gr_lnk) == 0 )
				grpp = &grptab[i];
			else for(;;){
				grpp = (grchain_t *)wfetch(sym2);
				if( grpp->gr_sym == sym || grpp->gr_lnk == 0 )
					break;
				sym2 = grpp->gr_lnk;
			}
			if( grpp->gr_sym != sym ){
				valign();
				grpp->gr_lnk = sym2 = VALN(sizeof(grchain_t));
				grpp = (grchain_t *)wfetch(sym2);
				grpp->gr_lnk = 0;
				grpp->gr_sym = sym;
			}
			delim();
		}
		break;

	case ADIF:	/* .if */

		labnotok();
		condlev++;
		if( nonrelex( ".if" ) && curop.op_val != 0 ) truelev++;
		mexprint();
		break;

	case ADINPUT:	/* .input */

		labnotok();
		if( scanstr(".input") && include( tokstr ) == -1 )
			error("20 The input file could not be found");
		mexprint();
		break;

	case ADLOCLAB:	/* .loclab	*/

		/* delimiter of scope for local labels */
		newloclabs();
		break;

	case ADLIST:	/* .list */

		labnotok();
		if( nonrelex( ".list" ) ){
			if( 0 <= curop.op_val && curop.op_val <= 255 )
				i = curop.op_val;
			else
				i = 255;
			if(( curlst = infp->in_lst&0xff ) != 0 ) curlst--;
			if( i < curlst ) curlst = i;
			/*
			 * We list the .list directive itself only if
			 * listing was on before and is still on now.
			 */
			if( !curlst ) llfull = 0;
		}
		mexprint();
		break;

	case ADLONG:	/* .long */

		datbit = lngbit;
		datrel = uralong;
		dataln = lngaln;
datscan:	lcalign( dataln );
		lcassign();
		if( toktyp != TKSPC ) goto noop;
		iilex();
		while( toktyp != TKEOL ){
			if( !expression(opcstr,STROK,RELOK) ) goto skip;
			j = curop.op_flg & 0x7fff;	/* repeat count */
			if( curop.op_rel < URBUND &&
			    sectab[curop.op_rel].se_atr & (USEFIX|SEATDUMY) )
				curop.op_rel = 0;
			if( !(curop.op_cls & (1 << OCSTR)) &&
			    curop.op_rel && !(curop.op_rel & URAMSK) ){
				if( datrel == 0 ) notrel( opcstr );
				curop.op_rel |= datrel;
			}
			do {
				if( curop.op_cls & (1 << OCSTR) )
					emitstr( savstr, savlen );
				else if( pass2 ){
					emitv((long)curop.op_val,
						curop.op_rel, datbit);
				} else {
					curloc += datbit;
				}
			} while( --j > 0 );
			delim();
		}
		break;

	case ADMAC:	/* .macro */

		if( *labstr && labtyp != STNLAB ){
			curdef = oclook( labstr );
			curdef->oc_typ = OTMAC;
			if( pass2 ){
				if( mchead == 0 )
					fatal("68 MACRO chain err");
				mchp = (mchain_t *)rfetch(mchead);
				mchead = mchp->mc_lnk;
				curdef->oc_val = mchp->mc_def;
				curdef->oc_arg = mchp->mc_arg;
			} else {
				valign();
				sym = VALN(sizeof(mchain_t));
				if( mchead )
					((mchain_t *)wfetch(mctail))->mc_lnk = sym;
				else
					mchead = sym;
				mctail = sym;
				mchp = (mchain_t *)wfetch(mctail);
				mchp->mc_lnk = 0;
				mchp->mc_def = curdef->oc_val = virtop;
				curdef->oc_arg = 0;
			}
			deflev++;
		} else
			noextlab();
		mexprint();
		break;

	case ADMLIST:	/* .mlist */

		labnotok();
		if( nonrelex( ".mlist" ) ){
			mlist = 0;
			if( curop.op_val ) mlist = 1;
		}
		mexprint();
		break;

	case ADORG:	/* .org */

		nopend();
		lcalign( curadu );
		if( toktyp != TKSPC ) goto noop;
		iilex();
		if( !expression(".org",NOSTR,RELOK) ) goto skip;
		if( curop.op_rel == cursec ||
		    curop.op_rel == 0 && (curatr & USEFIX) ){
			curop.op_val *= curadu;
			if( curop.op_rel && curop.op_val < curloc ){
				error("40 .org can't move locctr backwards");
				curop.op_val = curloc;
			}
			curloc = curop.op_val;
			lcassign();
			if( pass2 ) setorg();
			break;
		}
		if( curop.op_rel )
			error("21 Operand does not refer to current section");
		else
			error("39 Operand is absolute, section relocateable");
		break;

	case ADREPT:	/* .repeat */

		labnotok();
		rptlev++;
		rptline = curline;	/* save the line number		*/
		rptstr = virtop;	/* Set now in case of syntax error */
		rptct = 1;
		if( !nonrelex( ".repeat" ) ) break;
		/*
		 * The expression may have generated some new xref entries,
		 * changing virtop.  So we set rptstr again to be sure it is
		 * correct.
		 */

		rptstr = virtop;
		rptct = 0;
		if( curop.op_val > 256L )
			error("13 Value not in range 0-256");
		else
			rptct = curop.op_val >= 0 ? curop.op_val: 0;
		mexprint();
		break;

	case ADSECT:	/* .sect */

		if( noextlab() ) break;
		syp = (sytab_t *) rfetch( label );
		if( syp->sy_typ == STSEC && (!pass2||syp->sy_atr&SADP2 ))
			setsec( syp->sy_rel );	/* continue old section */
		else
			newsec(0);		/* start new section */
tryrwx:		if( toktyp == TKEOL ) break;

		/* added code 3/87 rmm */

		if( token() != TKSYM ) goto notrwx;

		/* addition:  the letters RWXA (or rwxa) set attributes
		   for the section.  By default sections are RWX.  The
		   appearance of a section with some subset has the
		   effect of turning on the NEGATION of the RWS bits
		   in the attribute word.  Hence, at one place if we
		   say: joe	.sect	r	and at another
			joe	.sect	x	the effect is that the
		   section is NOT readable, writeable, nor executable!
		   New feature: 'A' means section is absolute
		*/

		sp = tokstr;
		i = 0;
		while(*sp)switch( *sp++ ){
		default:
		notrwx:		error("33 Operand not from letters \"rwxa\"");
				goto skip;
		case 'a':
		case 'A':	curatr |= USEFIX; continue;
		case 'r':
		case 'R':	i |= USENOR;	continue;
		case 'w':
		case 'W':	i |= USENOW;	continue;
		case 'x':
		case 'X':	i |= USENOX;	continue;
		}
		curatr |= ~i & (USENOR|USENOW|USENOX);
		sectab[cursec].se_atr = curatr;
		token();	/* pass by the string */
		break;

	case ADSET:	/* .set */

		equ( STVAR );
		break;

	case ADSPACE:	/* .space */

		labnotok();
		mexprint();
		if( llfull == 0 ) goto skip;
		i = 1;
		if( toktyp == TKSPC && nonrelex( ".space" ) )
			i = curop.op_val;
		if( !llerx ) llsrc[0] = NULLCA;
		if( i >= linect ){
			linect = 0; /* eject page */
			break;
		}
		while( --i > 0 ){
			llfull = -1;
			putline();
		}
		llfull = -1;
		break;

	case ADSTITL:	/* .stitle */

		labnotok();
		title( ".stitle", titl2 );
		break;

	case ADTITLE:	/* .title */

		labnotok();
		title( ".title", titl1 );
		break;

	case ADUSING:	/* .using */

		labnotok();
		token();
		while( toktyp == TKSYM ){
			if( (k = usingreg(tokstr)) == -1 ) goto skip;
			scanc();
			if( ch != '=' ){
				error("22 An '=' is expected");
				goto skip;
			}
			iilex();	/* start an expression	*/
			secexpr++;	/* set section name ok */
			i = expression(".using",NOSTR,RELOK);
			secexpr--;	/* restore mode		*/
/*			if( !i || curop.op_rel == 0 || curop.op_flg & OFFOR ){
	above line commented out to allow absolute values in .using
	expressions since this is likely to be a good use of the feature
	20 mar 88, RMM */

			if( !i || curop.op_flg & OFFOR ){
				error("85 Illegal using_t expression");
				goto skip;
			}
			for( i=0; i<ulx && ulist[i].us_reg != k; i++ );
			if( i < ulx )
				warn("93 Register already in .using list");
			if( i == ulx && ++ulx > ULXSIZ ){
				error("47 Too many usings for table space");
				goto skip;
			}
			ulist[i].us_reg = k;
			ulist[i].us_sect = curop.op_rel;
			ulist[i].us_off = curop.op_val;
			delim();
		}
		break;

	case ADWARN:	/* .warn */

		labnotok();
		if( pass2 && scanstr(".warn") ){
			warnct++;
			immmsg( "Warning ", savstr );
		}
		break;

	case ADWITHN:	/* .within */

		labnotok();
		nopend();
		if( !nonrelex(".within") ) break;
		if( 0L <= curop.op_val && curop.op_val <= 32L ){
			i = (int) curop.op_val;
			if( i < curext ) curext = i;
		} else
			error("13 Value not in range 0-32");
		lcassign();
		break;

	case ADWORD:	/* .word */

		datbit = wrdbit;
		datrel = uraword;
		dataln = wrdaln;
		goto datscan;

	default:
		tokpt = opcptr;
		error( "23 Unrecognized directive");
		goto skip;
	}
	return;

skip:	skipeol();
	return;

noop:	noopnd();
}

void mexprint(){	/* routine is called for directives that should not
		   necessarily be printed	*/

	if( mexlev && mlist == 0 ) llfull = 0;
}

void noopnd(){
	error("10 An operand is required");
	skipeol();
}

int nonrelex(s) char *s;{

	/* set up and get an operand that is not relocatable	*/

	if( toktyp != TKSPC ){ noopnd(); return 0; }
	iilex();
	return expression(s,NOSTR,NOREL);
}

int scanstr(s)char *s;{

	/* set up and get an operand that is supposed to be a string */

	if( toktyp != TKSPC ){ noopnd(); return 0; }
	iilex();
	if( !expression(s,STROK,RELOK) ) return 0;
	if( curop.op_cls & (1 << OCSTR) ) return 1;
	error("11 Operand must be a string");
	skipeol();
	return 0;
}

void notexpr(s)char *s;{
	error("09 Operand not a valid %s expression",s);
	skipeol();
}

void nolabel(){

	char	*toksv;

	toksv = tokpt;
	tokpt = sline;
	error("14 A label is required");
	tokpt = toksv;
}


void notrel(s)char *s;{
	error("12 Relocation not legal for %s expression",s);
	skipeol();
}


/*
 * expression - Parses an expression and checks for various error conditions.
 * the expression's value and relocation are left in curop.  At call time,
 * iilexeme should contain the first token of the expression.
 */


int expression(s,strok,norel) char *s; int strok; int norel; {

	/* if strok != 0, a string is allowed			*/
	/* if norel != 0, relocatables are not allowed		*/

	if( iiparse() != 0 ){			/* error */
		curop.op_cls = 0L;
		while( toktyp!= TKCOM && toktyp!= TKEOL ) token();
	}
	if( curop.op_cls & (1 << OCEXP) ){
		if( norel ){
			if( curop.op_rel < URBUND &&
			    sectab[curop.op_rel].se_atr & (USEFIX|SEATDUMY) )
				curop.op_rel = 0;
			if( curop.op_rel ){
			   error("12 Relocation not legal for %s expression",s);
			   return 0;
			}
		}
		return 1;
	}
	if( strok && curop.op_cls & (1 << OCSTR) ) return 1;

		/* not expression */

	if( llerx == 0 ) error( "07 Operand is not a valid %s expression",s);
	curop.op_val = 0;
	curop.op_rel = URBUND;
	return 0;
}

/*
 * title - Puts a title into the specified string, and fixes up the listing.
 */


void title( msg, s ) char *msg,*s;{


	if( !pass2 ) return;
	if( scanstr(msg) ){
		savstr[TITSIZ] = '\0';
		strcpy( s, savstr );
	}
	mexprint();
	if( llfull ) llfull = linect = 0;
}

void stdequend(symtype)int symtype;{		/* finish of standard .equ processing */

	int	i;
	long	l;

	if( eflg || uflg || llerx ) return;
	if( curop.op_rel >= URBEXT ){
		error("24 External references of .equ not allowed");
		return;
	}
	if( curop.op_flg & OFFOR ){
		error("19 Forward references of .equ not allowed");
		return;
	}
	if( !(curop.op_cls & (1L << OCEXP)) ){
		error("49 Operand of .equ not an expression");
		skipeol();
		return;
	}
	assign( symtype, curop.op_val, curop.op_rel );
	lllocsiz = 0;		/* put into the object code field */
	i = 0x11;		/* init field width		*/
	l = ~0xfL;
	while( l && (curop.op_val & l) ) i += 0x11, l <<= 4;
	if( equfld ) i = (i & 0xf0) | equfld;
	hexit( llobj, i, curop.op_val );
}
/*		delim - Skips over a delimiter string if one is present.  */



void delim(){

	if( toktyp == TKSPC || toktyp == TKCOM ){
		iilex();
		if( toktyp == TKEOL ) error("05 Another operand is expected");
	}
}

void lcassign(){
	lcalign( curadu );
	assign( labtyp, curloc/curadu, cursec );
	if( labtyp != STNLAB && labtyp != STUND ) newloclabs();
}

void newloclabs(){

	/* routines sets up new scope for local labels */

	if( pass2 ){
		if( --loclabskips >= 0 ) return;
	} else {
		if( nchd == 0 ){ loclabskips++; return; }
		nctl->nc_lnk = (numchn_t *)palloc(sizeof(numchn_t));
	}
	nctl = nctl->nc_lnk;
}

void labnotok(){
	if( label ) error("88 a label is not allowed");
}

int noextlab(){
	if( !label ){
		nolabel();
		return 1;
	}
	if( labtyp == STNLAB ){
		error("89 numeric label not allowed");
		return 1;
	}
	return 0;
}

int usingreg(s)char *s; {

	VMADR		sym;
	sytab_t	*syp;
	int		k;

	sym = sylook( s );
	syp = (sytab_t *) rfetch( sym );
	if( syp->sy_typ == STKEQ ){
		xref( sym, 0 );
		syp = (sytab_t *)rfetch(syp->sy_val);
	}
	if( (k = regcheck(syp)) == -1 )
		error("48 Symbol is not a suitable register");
	return k;
}
