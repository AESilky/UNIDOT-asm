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
*			uasmac.c - macro stuff and repeat		*
*									*
************************************************************************/


static char rcsid[]=
"@(#)$Header: uasmac.c,v 6.7 88/02/08 08:25:48 rmm Rel $ uas macro stuff";

#include "uas.h"
#ifdef vms
#include "[-.incl]uobj.h"
#else
#include "../incl/uobj.h"
#endif
#include "funcdefs.h"		/* Forward defines for GCC */

#include <string.h>

/*	def1 - Copies one statement into a macro definition.  */


void def1(){

	reg char	*p;
	reg char	*q;
	reg VMADR	v;
	reg int		c;
	reg int		argno;
	char		opbuf[18];

	/*	Check for an ADMAC or ADENDM directive.		*/

	token();
	mexprint();
	if( toktyp == TKEOF ) fatal("59 End of file in macro definition");
	q = sline;
	for( p = opbuf; (c = *q) != '\n' && !white(c) && c != ':'; q++ ){
		if( p >= &opbuf[16] ) continue;
		if( c >= 'A' && c <= 'Z' ) c += 'a' - 'A';
		*p++ = c;
	}
	if( c == ':' ){			/* label, don't bother	*/
		while( *q == ':' ) q++;
		p = opbuf;
	}
	*p = 0;
	argno = 0;
	if( opbuf[0] ){				/* better check */
		argno = opval( opbuf );
		if( argno == ADMAC || argno == ADENDM ) goto fixlev;
	}
	while( white(*q) && *q != '\n' ) q++;
	for( p = opbuf; (c = *q) != '\n' && !white(c); q++ ){
		if( p >= &opbuf[16] ) continue;
		if( c >= 'A' && c <= 'Z' ) c += 'a' - 'A';
		*p++ = c;
	}
	*p = 0;
	if( opbuf[0] ) argno = opval( opbuf );	/* better check this one */
fixlev:	if( argno == ADMAC ) deflev++; else
	if( argno == ADENDM ) deflev--;
	strcpy( llobj, " macro");	/* what we are doing		*/
	scanpt = sline;			/* reset scan to start of line	*/
	if( pass2 ) goto endy;		/* no more work in pass 2	*/
	if( deflev <= 0 ){		/* end of definition		*/
		v = VAL1;
		*((char*)(wfetch(v))) = '\0';
		((MCH *)wfetch(mctail))->mc_arg = curdef->oc_arg;
endy:		do scanc(); while( ch != '\n' );
		goto endx;
	}
	/* copy a line of the definition */

	p = q = 0;
	do{
		scanc();
		if( ch == 0 ) fatal("59 End of file in macro definition");
		if( ch == argchr ){		/* macro parameter	*/
			scanc();		/* get next character	*/
			if( ch == mctchr ) ch = 0x80; /* macro expansion ct */
			else
			if( ch == argchr ) ch = 0x81; /* extra args	*/
			else
			if( '0' <= ch && ch <= '9' ){
				argno = ch - '0';
				if( argno > curdef->oc_arg )
					curdef->oc_arg = argno;
				ch = argno + 0x82;
			} else {
				unscanc();
				ch = argchr;
			}
		}
		if( ch == escchr ){		/* escaped character	*/
			scanc();		/* get next character	*/
			if( ch == argchr ){	/* \?			*/
				v = VAL1;
				*((char*)(wfetch(v))) = ch;
				scanc();
			}
		}

		/* normal character - stash away */

		v = VAL1;
		*((char*)(wfetch(v))) = ch;
	} while( ch != '\n' );
endx:	toktyp = TKEOL;
	scanpt = sline;
	*scanpt++ = '\n';
	*scanpt = 0;
}
/*	macro - Processes a macro call.		*/



void macro( vp ) VMADR vp;{


	reg INPUT	*newfp;
	reg int		i;
	reg char	*sp;
	reg char	**avp;
	reg int		ocarg;
	reg int		brlev;
	char		nbuf[6];

	/*
	 * first copy the line into the sav str area to avoid having
	 * a frame put out from under us.
	 */
	
	ocarg = opcode->oc_arg;
	if( toktyp == TKSPC ) scanc();
	while( white(ch) ) scanc();
	if( toktyp == TKEOL ) ch = '\n';
	sp = savstr;
	while( sp < &savstr[STRSIZ-2] && ch != '\n' && ch != ';' ){
		*sp++ = ch;
		scanc();
	}

	if( sp >= &savstr[STRSIZ-2] ) error("87 macro arg too long");
	while( ch != '\n' ) scanc();
	while( sp > savstr && white(sp[-1]) ) sp--;
	*sp = 0;

	/*
	 * Create an input stack frame with room for argument pointers
	 * at the beginning of the variable area.
	 */

	newfp = (INPUT *) pushin();
	avp = (char **) insp;
	insp += (ocarg+3) * sizeof(char *);
	iovck();
	newfp->in_typ = INMAC;

	/*
	 * Push the macro expansion count (?#) string, and initialize the
	 * extra arguments (??) string to null.
	 */

	avp[0] = insp;
	sprintf( nbuf, "%u", ++mexct );
	sp = nbuf;
	while(*sp != '\0' ) pushc(*sp++ );
	avp[1] = insp;
	pushc( '\0' );

	/*		Push the label (?0) string.		*/

	avp[2] = insp;
	sp = labstr;
	i = SYMSIZ;
	while( --i >= 0 && *sp != '\0' ) pushc( *sp++ );
	pushc( '\0' );

	/*		Push the operand field arguments.	*/

	sp = savstr;
	ch = *sp++;
	for( i = 1; i <= ocarg && ch; i++ ){
		avp[i+2] = insp;
		if( ch == lbrchr ){	/* argument enclosed in braces */
			brlev = 1;
			for(;;){
				ch = *sp++;
				if( ch == lbrchr ) brlev++;
				else if( ch == rbrchr ) brlev--;
				if( brlev <= 0 || ch == 0 ) break;
				if( ch == escchr ) ch = *sp++;
				pushc( ch );
			}
			if( ch == rbrchr ) ch = *sp++;
		} else {		/* normal argument (not in braces) */
			while( ch && ch != ',' && !white(ch) ){
				pushc( ch );
				ch = *sp++;
			}
		}
		if( i > ocarg ) pushc( rbrchr );
		pushc( '\0' );
		while( white(ch) ) ch = *sp++;
		if( ch == ',' ) ch = *sp++;
		while( white(ch) ) ch = *sp++;
	}
	for( ; i <= ocarg; i++ ){
		avp[i+2] = insp;
		pushc( '\0' );
	}
	if( ch ){
		if( ch == lbrchr ) ch = *sp++;
		avp[1] = insp;
		while( ch ){ pushc(ch); ch = *sp++; }
		while( insp > avp[1] && white(insp[-1]) ) insp--;
		if( insp > avp[1] && rbrchr == insp[-1] ) insp--;
		while( insp > avp[1] && white(insp[-1]) ) insp--;
		pushc( '\0' );
	}
	toktyp = TKEOL;
	newfp->in_ptr = (char *)vp;
	infp = newfp;
	if(( curlst = infp->in_lst&0xff ) != 0 ) curlst--;
	mexlev++;
}
/*
 * rpt1 - Copies one statement into a repeat definition.
 */

void rpt1(){

	reg INPUT	*newfp;
	reg VMADR	vp;
	reg char	rch;

	mexprint();
	laboc();
	if( toktyp == TKEOF ) fatal("61 End of file in repeat");
	if(*opcstr ){ /* check opcode field for special cases */
		opcode = oclook( opcstr );
		if( opcode->oc_typ == OTDIR ){		/* directive */
			if( OCVAL_I(opcode->oc_val) == ADREPT )	/* nested repeat */
				rptlev++;
			else if( OCVAL_I(opcode->oc_val) == ADENDR )	/* end rpt */
				rptlev--;
		}
	}
	strcpy( llobj, " repeat" );
	if( rptlev > 0 ){	/* copy a line of the repeat definition */
		scanpt = sline;	/* reset scan to beginning of line */

		do {			/* copy the line */
			scanc();
			vp = VAL1;
			*((char*)(wfetch(vp))) = ch;
		} while( ch != '\n' );
		unscanc();
		token();
	} else { /* finish off the repeat definition and start the repeat */
		vp = VAL1;
		*((char*)(wfetch(vp))) = '\0';
		newfp = pushin();
		newfp->in_typ = INRPT;
		newfp->in_rpt = rptct;
		newfp->in_fd = rptline;		/* starting line	*/
		newfp->in_seq = rptline;	/* set for both		*/
		vp = rptstr;
		while( rch = *((char*)rfetch(vp)) ){	/* copy definition to stack */
			pushc( rch );
			vp++;
		}
		newfp->in_ptr = insp;	/* set up as though at end of frame */
		setvirtop(rptstr);	/* reset virtop			*/
		infp = newfp;		/* switch to new input stack frame */
		if(( curlst = infp->in_lst&0xff ) != 0 ) curlst--;
	}
	skipeol();
}
/*
 * skip1 - Skips one statement due to an unsatisfied conditional assembly.
 */
void skip1(){

	reg int	i;

	mexprint();
	laboc();
	if( toktyp == TKEOF ) fatal("62 End of file while skipping");
	opcode = *opcstr ? oclook( opcstr ): 0;
	if( opcode && opcode->oc_typ == OTDIR ){
		switch( (short)opcode->oc_val ){

		case ADIF:	/* .if */
			condlev++;
			break;

		case ADELSE:	/* .else */
			if( truelev == condlev-1 ) truelev++;
			break;

		case ADENDIF:	/* .endif */
			condlev--;
			break;

		}
	}
	if( condlev > truelev ){	/* mark listing line as skipped */
		if( pass2 ) strcpy( llobj, " skipped" );
	} else {

		/*
		 * End of the skip.  List the directive regardless of
		 * the condlst value.
		 */

		llfull = curlst;
	}
	mexprint();
	skipeol();
}

/*
 * skipeol - Skips to the next end of line or end of file.
 */

void skipeol(){

	while( toktyp!= TKEOL && toktyp!= TKEOF ) token();
}
