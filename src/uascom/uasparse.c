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
*			uas.parse.c - operand isp parser		*
*									*
************************************************************************/


static char rcsid[] =
"@(#)$Header: uasparse.c,v 6.10 89/03/17 08:19:28 rmm Rel $ uas parser";

#include "uas.h"
#ifdef vms
#include "[-.incl]uobj.h"
#else
#include "../incl/uobj.h"
#endif
#include "funcdefs.h"		/* Forward defines for GCC */

#include <string.h>

/* Declarations (local) */

void error2(char* s);
int rel1check(psframe_t* p);
int rel2check(psframe_t* pl, psframe_t* p);
void sem01(int sem);

/*
 * iiparse - Attempts to parse the input stream, and returns 0 if successful.
 * Expects iilexeme to contain the first symbol of the input at call time.
 * Returns -1 on failure to recover from a syntax error, -2 on parse stack
 * overflow.
 */


int iiparse(){

	char	*scp;			/* scntab pointer */
	psframe_t	*liipsp;		/* local for speed	*/
	int		i;			/* temporary		*/
	int		sym;			/* lookahead symbol */
	char	*scp2;			/* scntab pointer */
	short	*pp;			/* ptab pointer */
	short	*smp;			/* pointer into semtab	*/
	extern short	semtab[];		/* semantic table	*/
	extern short	ptab[];			/* parse table		*/
	extern short	ntdflt[];		/* non-terminal defaults */
	extern char	scntab[];		/* scanning table	*/

	eflg = prevsem = uflg = 0;
	savlen = sav2len = 0;
	iipsp = liipsp = &iips[IISIZ];
	i = ptab[0];
	parsing = 1;
	lastsym = 0;
	for(;;){

		/* Push the current state and lookahead symbol on the stack */

		if( --liipsp < iips ) return -2;
		liipsp->ps_val0 = iilexeme.ps_val0;
		liipsp->ps_val1 = iilexeme.ps_val1;
		pp = liipsp->ps_state = ptab + (i & (~IIXFLG & 0xffff));
		sym = iilexeme.ps_sym;
		if( sym == TKSTR ){
			sav2len = tokval;
			if( savlen + sav2len > STRSIZ-2 )
fatal("75 String table overflow (limit is %d characters)",STRSIZ-2);
			memcpy( savstr+savlen+1,tokstr,sav2len+1);
		} else
		if( sym == TKERR ){
			error2("41 Illegal token for expression");
			return -1;
		}

BDEB(5,("push: sym=%d, val0 = %lx, val1 = %x\n",
			sym,iipsp->ps_val0,liipsp->ps_val1));

		/* Scan the tables to determine the next action. */

		scp2 = scp = scntab + *pp;
BDEB(5,("terminal scan for %d %d ",sym,*scp & 0xff));
		while( (i = (*scp&0xff)) != sym && i < IIESYM ) scp++;
		if( i == IIESYM ) return -1;
		i = pp[scp-scp2+1];
BDEB(5,("-> %d taction yields %x\n",*scp & 0xff,i));

		/* Perform a transition or start a reduction loop, depending
		 * upon the action just found.			*/

		if( i & (IIXFLG|IIRFLG) ){	/* transition or read/reduce */
			iilex();
			if( i & IIXFLG ) continue;	/* transition only */
		} else				/* simple reduction */
			liipsp++;

		do {			/* reduce loop */
			smp = semtab + (i & 0xff);
			iipsp = liipsp;
			iipspl = (liipsp += ((i >> 8) & IILMSK) - 1);
			i = *smp;
			sym = (i >> 8) & 0xff;
#ifdef ALTSEM		/* needed if there are any alternate semantics */
			iilsym = sym;
			iilset = (i & IIAFLG) ? *(smp+1) : 0100000;
#endif
			if( (i &= 0xff) != 0 ){	/* semnum is non zero */
BDEB(2,("semantic # %d\n",i));
				if( i >= 51 )	sem51( i );
					else	sem01( i );
				if( parsing == 0 ){
BDEB(5,("iiparse returns %d\n",iilexeme.ps_sym == TKEOF ? 0 : -1));
					if( iilexeme.ps_sym == TKEOF )
						return 0;
					return -1;
				}
				prevsem = i;
#ifdef ALTSEM		/* used if there are any alternate semantics */
				sym = iilsym;
#endif
			}
			pp = liipsp->ps_state - 1;
BDEB(5,("non terminal scan for %d ",sym));
			scp2 = scp = scntab - 1 + *pp;
			do scp++; while( (i=(*scp&0xff)) != sym && i != IIDSYM);
			if( i == IIDSYM )	pp = ntdflt + sym;
				else		pp -= scp - scp2;
BDEB(5,("-> %d ntaction returns %x\n",*scp & 0xff,*pp));
			i = *pp;
		} while( !(i & IIXFLG) );
	}
}
/*
 * sem01 - Common semantic routines.
 */

#ifdef DEBUG
void semprint(int (*ff)(), int sem) {
	psframe_t *pl;
	pl = iipspl;
	printf("%2d   [%8lx %4x %4x %4x]", sem,
		(long)curop.op_val,curop.op_rel,curop.op_flg,(int)curop.op_cls);
	for( pl = iipspl; pl >= iipsp; pl-- )
		printf(" <%lx|%x|%x>", (long)pl->ps_val0,
			pl->ps_val1, pl->ps_flg);
	printf("\n");
	(*ff)(sem);
	printf("%2d==>[%8lx %4x %4x %4x]", sem,
		(long)curop.op_val,curop.op_rel,curop.op_flg,(int)curop.op_cls);
	pl = iipspl;
	printf(" <%lx|%x|%x>\n", (long)pl->ps_val0, pl->ps_val1, pl->ps_flg);
}
#endif


void sem01(int sem) {
#ifdef DEBUG
	if(debug<=2) xsem01( sem ); else semprint( xsem01, sem );
}
xsem01( sem ) int sem; {
#endif


	psframe_t	*p,
			*pl;
	sytab_t	*syp;
	char	*dp,
			*sp;
	int		c,
			i,
			j;
	numlab_t	*nml;

	p = iipsp;
	pl = iipspl;
	switch( sem ){

	case 1:		/* <s> ::= <operand> */
		parsing = 0;
		break;

	case 2:		/* <expr1> ::= addop <expr1> */
		if( PSVAL1_I(pl->ps_val1) == TVADD ){	/* unary plus */
			pl->ps_val0 = p->ps_val0;
			pl->ps_val1 = p->ps_val1;
		} else {				/* unary minus */
			if( rel1check(p) ) goto relerr;
			pl->ps_val0 = -p->ps_val0;
			pl->ps_val1 = 0;
		}
		pl->ps_flg = p->ps_flg&OFFOR;
		break;

	case 3:		/* <expr1> ::= unop <expr1> */
		if( rel1check(p) ) goto relerr;
		pl->ps_val0 = ~p->ps_val0;
		pl->ps_val1 = 0;
		pl->ps_flg = p->ps_flg&OFFOR;
		break;

	case 4:		/* <expr6> ::= <expr6> xorop <expr5> */
		if( rel2check(pl,p) ) goto relerr;
		pl->ps_val0 ^= p->ps_val0;
		pl->ps_flg |= p->ps_flg&OFFOR;
		break;

	case 5:		/* <expr2> ::= <expr2> mulop <expr1> */
		if( rel2check(pl,p) ) goto relerr;
		switch( PSVAL1_I(p[1].ps_val1) ){
		case TVMUL: pl->ps_val0 *= p->ps_val0; break;
		case TVDIV: if( p->ps_val0 == 0 ){
				error2("44 Divide by zero");
				break;
			    }
			    pl->ps_val0 /= p->ps_val0;
			    break;
		case TVMOD: if( p->ps_val0 == 0 ){
				error2("45 Modulo 0");
				break;
			    }
			    pl->ps_val0 %= p->ps_val0;
			    break;
		case TVSHL: if( p->ps_val0 < 0 ){
				error2("46 Shift value negative");
				break;
			    }
			    if( p->ps_val0 >= 32 ) pl->ps_val0 = 0;
			    pl->ps_val0 <<= p->ps_val0;
			    break;
		case TVSHR: if( p->ps_val0 < 0 ){
				error2("46 Shift value negative");
				break;
			    }
			    if( p->ps_val0 >= 32 ) p->ps_val0 = 31;
			    if( pl->ps_val0 < 0 )
				pl->ps_val0 = ~(~pl->ps_val0 >> p->ps_val0);
			    else
				pl->ps_val0 >>= p->ps_val0;
			    break;
		}
		pl->ps_flg |= p->ps_flg&OFFOR;
		break;

	case 6:		/* <expr3> ::= <expr3> addop <expr2> */
		if( PSVAL1_UI(p[1].ps_val1) == TVADD ){		/* addition */
			if( pl->ps_val1 && p->ps_val1 && rel1check(p) )
				rel1check( pl );
			if( pl->ps_val1 && p->ps_val1 ){
		error2("28 Both operands of addition are relocateable");
				goto errex;
			}
			pl->ps_val0 += p->ps_val0;
			pl->ps_val1 = PSVAL1(PSVAL1_UL(pl->ps_val1) + PSVAL1_UL(p->ps_val1));
		} else {				/* subtraction */
			if( p->ps_val1 ) rel1check( p );
			if( pl->ps_val1 != p->ps_val1 ) rel1check( pl );
			if( p->ps_val1 ){
				if( pl->ps_val1 != p->ps_val1 )
error2("29 Both operands of subtraction must belong to the same section");
				pl->ps_val1 = p->ps_val1 = 0;
			}
			pl->ps_val0 -= p->ps_val0;
		}
		pl->ps_flg |= p->ps_flg&OFFOR;
		break;

	case 7:		/* <expr5> ::= <expr5> andop <expr4> */
		if( rel2check(pl,p) ) goto relerr;
		pl->ps_val0 &= p->ps_val0;
		pl->ps_flg |= p->ps_flg&OFFOR;
		break;

	case 8:		/* <expr> ::= <expr> orop <expr6> */
		if( rel2check(pl,p) ) goto relerr;
		pl->ps_val0 |= p->ps_val0;
		pl->ps_flg |= p->ps_flg&OFFOR;
		break;

	case 9:		/* <expr4> ::= <expr3> relop <expr3> */
		rel2check(pl,p);	/* kill dummy sections only */
		switch( PSVAL1_I(p[1].ps_val1) ){
		case TVEQ: pl->ps_val0 = pl->ps_val1 == p->ps_val1 &&
				      pl->ps_val0 == p->ps_val0;
			   break;
		case TVNE: pl->ps_val0 = pl->ps_val1 != p->ps_val1 ||
				      pl->ps_val0 != p->ps_val0;
			   break;
		default:   if( pl->ps_val1 != p->ps_val1 ) goto cmperr;
		}
		switch( PSVAL1_I(p[1].ps_val1) ){
		case TVLT: pl->ps_val0 = pl->ps_val0 <  p->ps_val0; break;
		case TVGT: pl->ps_val0 = pl->ps_val0 >  p->ps_val0; break;
		case TVLE: pl->ps_val0 = pl->ps_val0 <= p->ps_val0; break;
		case TVGE: pl->ps_val0 = pl->ps_val0 >= p->ps_val0; break;
		}
		if( pl->ps_val0 ) pl->ps_val0 = -1L;
		pl->ps_val1 = 0;
		pl->ps_flg |= p->ps_flg & OFFOR;
		break;

	case 10:	/* <primary> ::= ( <expr7> ) */
		pl->ps_val0 = p[1].ps_val0;
		pl->ps_val1 = p[1].ps_val1;
		pl->ps_flg = p[1].ps_flg;
		break;

	case 11:	/* <primary> ::= constant */
		pl->ps_val1 = PSVAL1(0);
		pl->ps_flg = 0;
		break;

	case 12:	/* <primary> ::= symbol */
		lastsym = (VMADR)((unsigned long)p->ps_val1);
		xref( lastsym, 0 );
		syp = (sytab_t *)rfetch( lastsym );
		if( syp->sy_typ == STSEC ){
			if( !secexpr ){
			  error2("31Symbol has been defined as a section name");
			  goto errex;
			}
			pl->ps_val0 = 0;
			pl->ps_val1 = PSVAL1(syp->sy_rel | URAMSK);
			break;
		}
		if(!( syp->sy_atr & SAREF )){	/* Must set referenced flag */
			syp = (sytab_t *)wfetch( lastsym );
			syp->sy_atr |= SAREF;
		}
		if( syp->sy_typ == STUND || syp->sy_rel == URBUND ||
		    syp->sy_typ == STVAR && pass2 && !(syp->sy_atr&SADP2) ){
			error2("25 Undefined symbol");
			uflg = 1;
			pl->ps_val0 = 0;
			pl->ps_val1 = PSVAL1(0);
			pl->ps_flg = OFFOR;
		} else {
			pl->ps_val0 = (long)syp->sy_val;
			pl->ps_val1 = PSVAL1(syp->sy_rel);
			pl->ps_flg = pass2 && !(syp->sy_atr&SADP2) ? OFFOR : 0;
		}
		break;

	case 13:	/* <primary> ::= $ */
		pl->ps_val0 = curloc/curadu;
		pl->ps_val1 = PSVAL1(cursec);
		pl->ps_flg = 0;
		break;

	case 14:	/* <string1> ::= string */
		memcpy( savstr, savstr+savlen+1, sav2len+1 );
		savlen = sav2len;
		break;

	case 15:	/* <operand> ::= <expr> */
		curop.op_cls = (1L<<OCEXP)|(1L<<(prevsem == 10 ?OCPEX:OCNEX));
		if( prevsem != 12 && prevsem != 21 ) lastsym = 0;
		if( eflg ){
			curop.op_val = curop.op_rel = curop.op_flg = 0;
		} else {
			curop.op_val = p->ps_val0;
			curop.op_rel = PSVAL1_UI(p->ps_val1);
			curop.op_flg = p->ps_flg;
		}
		break;

	case 16:	/* <operand> ::= <expr> ( <expr> )	*/
		curop.op_cls = 1 << OCEXP;
		curop.op_val = curop.op_rel = curop.op_flg = 0;
		rel1check( pl );		/* kill dummy rel */
		if( eflg || uflg || pl->ps_val1 ){
			error2( "37 repeat expression invalid" );
			break;
		}
		curop.op_val = p[1].ps_val0;		/* value	*/
		curop.op_rel = PSVAL1_UI(p[1].ps_val1);	/* relocation	*/
		curop.op_flg = pl->ps_val0;		/* repeat	*/
		break;

	case 17:	/* <expr4> ::= <string1> relop string */
		sp = savstr;
		i = savlen;
		dp = savstr+i+1;
		j = sav2len;
		c = 0;
		while( i > 0 && j > 0 && ( c = *sp++ - *dp++ )== 0 ) i--, j--;
		if( c == 0 ) c = i-j; /* c reflects the comparison outcome */
		switch( PSVAL1_I(p[1].ps_val1) ){
		case TVEQ: pl->ps_val0 = c == 0; break;
		case TVNE: pl->ps_val0 = c!= 0; break;
		case TVLT: pl->ps_val0 = c < 0; break;
		case TVGT: pl->ps_val0 = c > 0; break;
		case TVLE: pl->ps_val0 = c <= 0; break;
		case TVGE: pl->ps_val0 = c >= 0; break;
		}
		if( pl->ps_val0 ) pl->ps_val0 = -1;
		pl->ps_val1 = PSVAL1(0);
		pl->ps_flg = 0;
		break;

	case 18:	/* <operand> ::= <string1>		*/

		curop.op_cls = 1L << OCSTR;
		curop.op_val = savlen;		/* string length */
		curop.op_flg = 1;		/* repeat	*/
		break;

	case 19:	/* <operand> ::= <expr> ( <string1> )	*/

		curop.op_cls = 1L << OCSTR;
		rel1check( pl );			/* kill dummy sect */
		if( eflg || uflg || pl->ps_val1 ){
			error2( "37 repeat expression invalid" );
			break;
		}
		curop.op_val = savlen;			/* string length */
		curop.op_flg = pl->ps_val0;		/* repeat	*/
		break;

	case 20:	/* <string1> ::= <string1> string	*/

		memcpy( savstr+savlen, savstr+savlen+1, sav2len+1);
		savlen += sav2len;
		return;

	case 21:	/* <primary> ::= numlab			*/

		lastsym = numlab((int)tokval);
		nml = (numlab_t *)rfetch(lastsym);
		pl->ps_val0 = nml->nm_val;
		pl->ps_val1 = PSVAL1(nml->nm_rel);
		pl->ps_flg = OFFOR;
		if( nml->nm_typ == STUND ){
			error2("92 Undefined local label");
			uflg = 1;
		} else
		if( !pass2 || (nml->nm_atr&SADP2) ) pl->ps_flg = 0;
		return;
	}
	return;
relerr:	error2( "27 Operation not permitted on relocateable value");
	goto errex;
cmperr:	error2( "30 Relationals must refer to the same section");
	goto errex;
experr: error2( "26 Error in expression" );
errex:	eflg = 1;
	pl->ps_val0 = 0;
	pl->ps_val1 = PSVAL1(0);
}

int rel1check(psframe_t* p) {

	if( PSVAL1_I(p->ps_val1) < URBUND &&
	    sectab[PSVAL1_I(p->ps_val1)].se_atr & (USEFIX|SEATDUMY) ) p->ps_val1 = PSVAL1(0);
	if( p->ps_val1 ) return 1;
	return 0;
}

int rel2check(psframe_t* pl, psframe_t* p) {

	if( PSVAL1_I(p->ps_val1) < URBUND &&
	    sectab[PSVAL1_I(p->ps_val1)].se_atr & (USEFIX|SEATDUMY) ) p->ps_val1 = PSVAL1(0);
	if( PSVAL1_I(pl->ps_val1) < URBUND &&
	    sectab[PSVAL1_I(pl->ps_val1)].se_atr & (USEFIX|SEATDUMY) ) pl->ps_val1 = PSVAL1(0);
	if( pl->ps_val1 || p->ps_val1 ) return 1;
	return 0;
}

void error2(char* s) {
	char	*toksv;
	toksv = tokpt;
	tokpt = tokpt2;
	if( llerx == 0 ) error(s);
	tokpt = toksv;
}
