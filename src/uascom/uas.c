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
*			uas.c - main and subroutines			*
*									*
************************************************************************/


static char rcsid[]=
"@(#)$Header: uas.c,v 6.33 90/08/16 10:21:32 rmm Rel $ uas main routine";

#include <signal.h>
#include <string.h>

#include "uas.h"

#ifdef vms
#include "[-.incl]uobj.h"
#include "[-.incl]urel.h"
#else
#include "../incl/uobj.h"
#include "../incl/urel.h"
#endif
#include "funcdefs.h"		/* Forward defines for GCC */

/* Forward definitions (local) */
void assem1();
void listsec();
void lotoken();
void secprint(reg int rel, int n);
void setlabel();


void intr(){
	fprintf(ERRFIL,"INTERRUPT!\n");
	quit(BADEXIT);
}

/* the assembler starts here		*/

void main( argc, argv, env ) int argc; char *argv[]; char *env[];{

	reg int		i;
	reg OCTAB	*oc;
	char		*ocstt;
	short		xcodaln;
	short		xwrdaln;
	short		xbytaln;
	short		xlngaln;


	static	char	errfmt[] = "%u errors, %u warnings\n";

/*
 * To enable profiling, put '-DPROFILE=nnnn' in the cc line, where nnnn
 * is the profiling buffer size desired.  You should use the biggest feasible
 * buffer to avoid quantization errors.  You must also link in countbas.o,
 * which defines the symbol _countbas so that a C program can get to it.
 */

#ifdef PROFILE
	static short buffer[PROFILE];
	extern etext();
	extern short *countbas;
	countbas = buffer+3;
	monitor( 2, etext, buffer, PROFILE, 150 );
#endif // PROFILE

	ERRFIL = stderr;
#ifdef vms
	if( signal( SIGINT, SIG_IGN ) == SIG_DFL )
		signal( SIGINT, intr );		/* interrupt		*/
	if( signal( SIGQUIT, SIG_IGN ) == SIG_DFL )
		signal( SIGQUIT, intr );	/* quit			*/
	signal( SIGTERM, intr );		/* terminate		*/
	signal( SIGHUP, SIG_IGN );		/* hangup		*/
#endif
#ifdef msdos
	signal( SIGINT, intr ); /* interrupt            */
#endif
#ifndef NOTUNIX
	if( signal( SIGINT, SIG_IGN ) == SIG_DFL )
		signal( SIGINT, intr );		/* interrupt		*/
	if( signal( SIGQUIT, SIG_IGN ) == SIG_DFL )
		signal( SIGQUIT, intr );	/* quit			*/
	signal( SIGTERM, intr );		/* terminate		*/
	signal( SIGHUP, SIG_IGN );		/* hangup		*/
#endif

	init( argc, argv );
	ocstt = (char *)palloc(4);	/* buffer for reset		*/
	xcodaln = codaln;
	xwrdaln = wrdaln;
	xbytaln = bytaln;
	xlngaln = lngaln;
	dopass();
	pass2 = 1;
	interlude();
	ulx = 0;		/* no usings or drops	*/

	/* the following was added because the assembler gives
	   strange results when an opcode is used prior to its
	   being defined as a macro (or otherwise)
	*/

	for( i=0; i<(1<<OHSHLOG); i++ )
		for( oc=ochtab[i]; oc && (char *)oc >= ocstt; oc = oc->oc_lnk )
			oc->oc_typ = OTUND;
	nctl = nchd;			/* reset local label list */
	codaln = xcodaln;
	wrdaln = xwrdaln;
	bytaln = xbytaln;
	lngaln = xlngaln;
	oflush();
	dopass();
	oflush();
	objtyp = UOBOND;
	oflush();
	if( lflag ){
		pgcheck();
		fputc( '\n', LIST );
		pgcheck();
		fprintf( LIST, errfmt, errct, warnct );
		if( secct > URBSEC ) listsec();
		if( xflag ) putxref();
		fprintf(LIST,"\f\n");
	}
#ifdef	STATS
	if( stats ) putstats();
#endif
	if(OBJECT){
		fflush( OBJECT );
		filchk( OBJECT, "object" );
		fclose( OBJECT );
		OBJECT = 0;
	}
	if(LIST){
		fflush( LIST );
		filchk( LIST, "listing" );
		fclose( LIST );
		LIST = 0;
	}
	if( errct ){
		fprintf( ERRFIL, errfmt, errct, warnct );
		quit( BADEXIT );
	}
	if( verbose ) fprintf( ERRFIL, warnct ?
			"no errors, %d warnings\n" :
			"no errors detected\n", warnct );
#ifdef msdos
	if( warnct ) quit( WRNEXIT );
#endif
	quit( GOODEXIT );
}

void listsec() {

	reg SYTAB	*syp;
	reg int		rel;
	reg long	l;

	rel = URBSEC;
	if( defadu != 8 ) rel++;
	if( rel >= secct ) return;
	strcpy( titl2, "Section Listing" );
	linect = 0;
	pgcheck();
	fputc('\n',LIST);
	pgcheck();
	fprintf(LIST, "Section                         #    Length\n");
	pgcheck();
	fputc('\n',LIST);
	for( ; rel < secct; rel++ ) secprint( rel, rel ); /* list sections */
	if( dmysec != SECSIZ-1 ){	/* list dummy sections	*/
		pgcheck(); fputc( '\n', LIST );
		pgcheck(); fprintf(LIST,"Dummy Sections\n");
		pgcheck(); fputc( '\n', LIST );
		for( rel = SECSIZ-2; rel >= dmysec; rel-- )
			secprint( rel, SECSIZ-1-rel );
	}
}

void secprint( rel, n ) reg int rel; int n;{

	reg SYTAB	*syp;
	reg long	l;

	pgcheck();
	syp = (SYTAB *) rfetch( sectab[rel].se_sym );
	l = sectab[rel].se_loc/sectab[rel].se_adu;
	fprintf(LIST,"%-31.31s%2d  %8lx\n", syp->sy_str, n, l);
}
/*		dopass - Performs one pass of the source input.		*/



void dopass(){

	curlst = 255;		/* must be before include() */
 	if( include( srcfile ) != 0 ) fatal( "86 Cannot open %s", srcfile );

	/* initialize several very important pass variables */

	condlev = curloc =
	cursec = deflev = mexct =
	rptlev = sectab[0].se_loc = truelev = 0;
	condlst = 1;
	mlist = 1;
	scanpt = &sline[0];
	*scanpt = 0;
	pendbits = 0;
	mexlev = 0;
	inclev = -1;			/* opening source makes this 0	*/
	reading = secct = 1;		/* section counter starts at one */
	dmysec = SECSIZ-1;		/* dummy section counter at 254 */

	curadu = defadu;
	curaln = minaln;
	curext = 32;
	curatr = USEFIX;
	if( defadu != 8 ){

		/* we must create a fake section that has the attributes
		   that we would like to ascribe to the absolute section
		   in the event that the addressing unit is not 8 bits */

		label = sylook( ".abs ");	/* NOTE THE BLANK */
		newsec(0);			/* establish this section */
		curadu = defadu;
		curaln = minaln;
		curext = 32;
		curatr = USEFIX;
		setsec(0);			/* back to absolute sect */
	}
	if( dfltsec != 0 ){	/* set the default section */
		label = sylook( dfltsec );
		newsec(0);
	}
	while( reading ){ /* process statements one at a time */
		if( deflev > 0 ) def1();		/* defining a macro */
		else if( rptlev > 0 ) rpt1();		/* defining a repeat */
		else if( condlev > truelev ) skip1();	/* conditional skip */
		else assem1();		/* assembling statements normally */
		if( toktyp == TKEOF ) break;
		if( toktyp != TKEOL ){
			error("06 More operands are present than expected");
			skipeol();
		}
		putline();
	}
	setsec( 0 );		/* update sectab info for last section */
	while( toktyp != TKEOF ) token();
}
/*		assem1 - Assembles a single statement.		*/



void assem1(){

	laboc();
	BDEB(1,("lc=%lx,ad=%d,pb=%d	%s\n",curloc,curadu,pendbits,sline));
	if( *opcstr ){ /* we have an opcode field */
		opcode = oclook( opcstr );
		switch( opcode->oc_typ ){

		case OTUND:	/* undefined mnemonic */
			tokpt = opcptr;
			error( "01 Unrecognized opcode or undefined macro" );
			skipeol();
			break;

		case OTINS:	/* machine instruction */
			nopend();
			lcalign( codaln );
			instr(opcode->oc_val);
			break;

		case OTDIR:	/* assembler directive */
			direc( (int)opcode->oc_val );
			break;

		case OTMAC:	/* macro call */
BDEB(0,("%d expanding: %s, (%d)  stack depth is %d\n",
	llseqval,opcstr,opcode->oc_arg,(char *)infp-(char *)instk));
			mexprint();	/* here before push	*/
			macro( opcode->oc_val );
			break;

		}
	} else
	if( *labstr ){ /* we have a label field standing alone */
		nopend();
		setlabel();
		lcassign();
	} else		/* empty line	*/
		mexprint();
}
/*
 * assign - If a label is present, this routine gives it the specified type,
 * value, and relocation, checking for multiple definition and phase errors.
 * It also records a cross reference entry.
 */

void assign( typ, val, rel ) uns typ; long val; uns rel;{


	reg SYTAB	*syp;
	reg NUMLAB	*nml;
	VMADR		vnml;
	char		*tokpt1;

	lllocsiz = lllocspec;
	lllocval = val & lllocmask;
	if( label == 0 ) return;		/* no label */
	if( rel && typ != STSEC && rel < URBUND &&
	    sectab[rel].se_atr & (SEATDUMY|USEFIX) )
			rel = 0;
	if( typ == STNLAB ){		/* label is numeric	*/
		vnml = numlab(labval);
		nml = (NUMLAB *)wfetch(vnml);
		if( pass2 ){
			nml->nm_atr |= SADP2;
			if( nml->nm_atr & SAMUD )
				error("02 The label is multiply defined");
			return;
		}
		if( nml->nm_typ == STUND ){
			nml->nm_typ = typ;
			nml->nm_rel = rel;
			nml->nm_val = val;
			return;
		}
		if( nml->nm_rel != rel || nml->nm_val != val )
			nml->nm_atr |= SAMUD;
		return;
	}
	if( xflag && pass2 ){
		tokpt1 = tokpt;
		tokpt = labptr;
		xref( label, XRDEF );
		tokpt = tokpt1;
	}
	syp = (SYTAB *) wfetch( label );
	BDEB(1,("assign( %d, %lx, %d ): %s\n",typ,val,rel,syp->sy_str));
	if( syp->sy_typ == STKEY || syp->sy_typ == STKEQ ){
		if( typ != STKEY )
			error("35 The label is in use as a keyword");
		if( pass2 ) syp->sy_atr |= SADP2;
		return;
	}
	if( syp->sy_typ == STSEC ){
		if( typ != STSEC )
			error("36 The label is in use as a section name");
		if( pass2 && !(syp->sy_atr & SADP2) ){
			syp->sy_atr |= SADP2;
			syp->sy_val = SYVAL(val);
			syp->sy_rel = rel;
		}
		return;
	}
	if( syp->sy_typ != STVAR &&
	    (pass2 ? syp->sy_atr & SADP2 : syp->sy_typ!= STUND) ){
		error("02 The label is multiply defined");
		syp->sy_atr |= SAMUD;
		return;
	}
	if( syp->sy_typ == STUND ){		/* assign a type	*/
		syp->sy_typ = typ;
		syp->sy_rel = URBUND;
	}
	if( syp->sy_typ != typ ){
		error("17 The label was formerly a different type");
		return;
	}
	if( syp->sy_rel == URBUND || syp->sy_typ == STVAR ){
		/* assign a value */
		syp->sy_val = SYVAL(val);
		syp->sy_rel = rel;
	} else
		if (syp->sy_val != SYVAL(val)) {
printf("pass1 value = %lx, pass2 value = %lx\n",syp->sy_val,val);
		error( "03 Phase error in assembler: %lx",syp->sy_val);
		return;
	}
	if( syp->sy_rel != rel ){
printf("pass1 reloc = %d, pass2 reloc = %d\n",syp->sy_rel,rel);
		error( "42 Phase error in assembler: %d",syp->sy_rel);
		return;
	}
	if( pass2 ) syp->sy_atr |= SADP2;
	if( globflg ) syp->sy_atr |= SAGLO;
	globflg = 0;
	return;
}

void setlabel(){		/* establish the label virtual address */
	label = 0;
	if( *labstr ){
		label = labtyp == STNLAB ?
				numlab( labval ) :
				sylook( labstr );
	
	}
}
/*
 * interlude - Performs processing between pass 1 and pass 2.
 */
void interlude(){

	reg SYTAB	*syp;
	reg VMADR	p;
	reg uns		h;
	reg uns		rel;
	reg uns		r;
	reg char	*pp;
	reg char	*pp2;
	reg VMADR	p2;
	char		type;
	extern char	*lastcomp();

	objtyp = UOBOST;
	oflush();
	objtyp = UOBMOD;
	oputs( lastcomp(srcfile) );
	oflush();
	if( proctype[0] ){
		objtyp = UOBPRO;
		oputs( proctype );
		oflush();
	}
	if( relmap ){
		for( h=0; pp = relmap[h]; h++ ){
			for( pp2 = pp; *pp2; pp2++ ){
				if( *pp2 == A_LI1 ){ pp2++; continue; }
				if( *pp2 == A_LI2 ){ pp2 += 2; continue; }
				if( *pp2 == A_LI4 ){ pp2 += 4; continue; }
			}
			objtyp = UOBRLT;
			while( pp < pp2 ) oputb( *pp++ );
			oputb( 0 );
			oflush();
		}
	}
	if( szyhead ) szyprocess();		/* for var length instrs */
	for( rel = URBSEC; rel < secct; rel++ ){ /* output sections blocks */
		if( objtyp!= UOBSEC || relbot-objtop < SYMSIZ+7 ){
			oflush();
			objtyp = UOBSEC;
		}
		sectab[rel].se_atr &= ~SEATRSEG;
		syp = (SYTAB *) rfetch( sectab[rel].se_sym );
		oputb( sectab[rel].se_aln );
		oputb( sectab[rel].se_ext );
		oputb( sectab[rel].se_atr | USEMOR );
		h = 0;
		if( !(sectab[rel].se_atr & USEFIX) ) h = SEATSLEN;
		if( sectab[rel].se_within ) h |= SEATWITH;
		if( sectab[rel].se_adu != 8 ) h |= SEATADDU;
		oputb( (h >> 8) & 0xf );
		if( h & SEATADDU ) oputb( sectab[rel].se_adu );
		if( h & SEATWITH ) oputb( sectab[rel].se_within );
		if( h & SEATSLEN ) oputl( sectab[rel].se_loc/sectab[rel].se_adu );
		oputs( syp->sy_str );
	}
	rel = URBEXT;
	for( h = 0; h < (1 << SHSHLOG); h++ ){
		for( p = syhtab[h]; p; p = p2 ){
			syp = (SYTAB *) rfetch( p );
			p2 = syp->sy_lnk;
			if( syp->sy_typ == STKEY ||
			    syp->sy_typ == STKEQ ||
			    syp->sy_typ == STSEC )
				continue;
			if( syp->sy_typ == STUND && syp->sy_val == 0 &&
			    (syp->sy_atr & (SAGLO|SAREF)) == SAGLO ){

				/*
				 * The symbol was declared external but never
				 * referenced, so we won't output it to the
				 * object file.  But we still must fill in the
				 * symbol table fields for the benefit of
				 * the xref lister.  */

				syp = (SYTAB *) wfetch( p );
				syp->sy_typ = STLAB;
				syp->sy_val = 0;
				syp->sy_rel = URBEXT;
				continue;
			}
			if( syp->sy_atr&SAGLO || uext&&syp->sy_typ == STUND )
				type = UOBGLO;
			else
				type = UOBLOC;
			if( objtyp != type ) oflush();
			oneed( SYMSIZ+7 );
			objtyp = type;
			if( syp->sy_typ == STUND ){
				oputl( (long)syp->sy_val );
				oputb( URBUND );
				if( syp->sy_atr&SAGLO || uext ){
					if( rel >= URBMSK )
			fatal("78 Too many externals (limit is 1792)");
					syp = (SYTAB *) wfetch( p );
					syp->sy_typ = STLAB;
					syp->sy_atr |= SAGLO;
					syp->sy_val = 0;
					syp->sy_rel = rel++;
				}
			} else {
				oputl( (long) syp->sy_val );
				r = syp->sy_rel & 0xff;
				if( r >= secct ) r = 0;
				oputb( r );
			}
			oputs( syp->sy_str );
		}
	}
}
/*
 * laboc - Scans the label and opcode fields from the next assembler
 * statement, and leaves their strings in global arrays labstr and
 * opcstr.  Each string is set to null if its field is missing.  Toktyp
 * is left on the token after the opcode (if it is present).
 */

void laboc(){

	reg int		col1;
	reg int		colfound;
	reg char	*scansv;

	/* the syntax of an opcode line depends upon whether the colreqd
	   flag is set:  if it is then we allow:

		[space] <symbol> : [:] [[space] <opcode>] ...
	   or	[space] <opcode> ...

	   if it is not then we allow:

		[space] <symbol> : [:] [[space] <opcode>] ...
	   or	<space> <opcode> ...
	*/

	labstr[0] = opcstr[0] = NULLCA;
	scansv = scanpt;
	colfound = 0;
	labtyp = 0;
	col1 = token();
	if( toktyp == TKSPC ) lotoken();
	for(;;){
		if( toktyp == TKNLAB && labstr[0] == 0 ){
			/* found numeric label	*/
			strcpy( labstr, tokstr );
			labtyp = STNLAB;
			labval = tokval;
			lotoken();
			continue;
		}
		if( toktyp == TKSYM ){		/* found symbol		*/
			tokstr[SYMSIZ] = NULLCA;	/* trim to size		*/
			if( !colfound ){	/* no colon yet		*/
				if( labstr[0] == 0 ){	/* no label yet	*/
					scansv = scanpt;
					labptr = scanpt-1;
					strcpy( labstr, tokstr );
					labtyp = STLAB;
					lotoken();
					continue;
				}

				/* we already have a label - but it is
				   really a label, or is it an opcode?	*/

				if( colreqd || col1 == TKSPC )
					goto movelab;

				/* if we get here we believe that the
				   second symbol is really the opcode */
			}

			/* the second symbol is really the opcode */

			opcptr = scanpt-1;
			strcpy( opcstr, tokstr );
			token();
			return;
		}
		if( toktyp == TKCOLON ){
			if( ++colfound >= 2 ){
				if( labstr[0] ) globflg++;
				if( colfound == 3 ) error("43 Too many colons");
			}
			lotoken();
			continue;
		}
		break;
	}

	/* if we have found a colon, then we either have a label,
	   or it is null.  We permit this case to simplify macro work
	   If we have found no symbol, then similarly we are through */

	if( colfound || labstr[0] == 0 ) return;

	/* we did not find a colon so we have to decide if the
	   one symbol that we found was a label or an opcode	*/

	if( labtyp == STLAB && (colreqd || col1 == TKSPC) ){
		/* the label WAS the opcode */
movelab:	opcptr = labptr;
		strcpy( opcstr, labstr );
		labstr[0] = NULLCA;
		labtyp = 0;
	}
	scanpt = scansv;
	token();
}

void lotoken() {

	/* routine guaranteed not to read a token that is not a label,
	   opcode or a colon */

	extern int notokerr;
	scanc();
	while( white(ch) ) scanc();
	scanpt--;
	toktyp = TKSPC;
	if( chclass[ch] & (I|D) ||
	    ch == '.' ||
	    ch == ':' ||
	    ch == ';' ||
	    ch == '\n' ){ notokerr = 1; token(); notokerr = 0; };
}
/*
 * newsec - Creates a new section and makes it the current section.
 */


void newsec(int f) {	/* f is 0 for normal sections, 1 for dummy sections */

	reg int	se;

	/* for ordinary sections, we start from the bottom
	   0 is the absolute section
	   1... are normal relocatable sections

	   dummy sections are started at the top
	   SECSIZ-1 is an unnamed dummy section
	   SECSIZ-1... are named dummy sections
	*/

	se = f ? (label ? --dmysec : SECSIZ-1) : secct++;
	if( secct >= dmysec ) fatal( "80 Too many sections (limit is 254)" );
	setsec( se );
	assign( STSEC, 0L, se );
	sectab[se].se_sym = label;
	curatr = 0;
	curloc = 0;
	if( f ) curatr |= SEATDUMY, sectab[se].se_atr = curatr;
	curaln = minaln;
	curext = 32;
	curadu = defadu;
}

#ifdef	STATS

/*
 * putstats - Outputs various statistics for performance evaluation.
 */

void putstats() {

	fprintf( stdout, "%5u dynamic physical bytes used\n", phytop-&end );
	fprintf( stdout, "%5u total physical bytes used\n",( uns ) phytop );
	fprintf( stdout, "%5u virtual bytes used\n", virtop );
	fprintf( stdout, "%5u symbols\n", symct );
	fprintf( stdout, "%5u symbol lookups\n", sylct );
	fprintf( stdout, "%5u symbol lookaside hits\n", ashct );
	fprintf( stdout, "%5u hash chain links traversed\n", chnct );
	fprintf( stdout, "%5u vm accesses\n", vmgct );
	fprintf( stdout, "%5u vm disk reads\n", vmrct );
	fprintf( stdout, "%5u vm disk writes\n", vmwct );
}
#endif
/*
 * setorg - Sets up the next address for text output.
 */
void setorg() {

	if( curatr & SEATDUMY ) return;
	if( objtyp != UOBTXT || nxtloc != curloc || nxtsec != cursec ){
		oflush();
		objtyp = UOBTXT;
		oputl( curloc / curadu );
		oputb( cursec );
		if( curadu < 8 ) oputb( 0x80 );
		oputb( 0 );
		nxtloc = curloc;
		nxtsec = cursec;
	}
}

/*
 * setsec - Changes to the specified section for code generation.
 */

void setsec( sec ) uns sec;{

	reg SECTION	*sep;

	if( sec == cursec ) return;
	lcalign(curadu);
	sep = &sectab[cursec];
	sep->se_aln = curaln;
	sep->se_ext = curext;
	sep->se_atr = curatr;
	sep->se_loc = curloc;
	sep->se_adu = curadu;
	sep = &sectab[cursec = sec];
	curaln = sep->se_aln;
	curext = sep->se_ext;
	curatr = sep->se_atr;
	curloc = sep->se_loc;
	curadu = sep->se_adu;
}

/*
dmpblk(p) char *p; {
	char *p2;
	p2 = p + 16;
	while( p < p2 ){ if( *p ) break; p++; }
	if( p == p2 ) return;
	p = p2 - 16;
	printf("%4x ",p);
	while( p < p2 ) printf(" %2x",*p& 0xff), p++;
	printf(" |");
	p -= 16;
	while( p < p2 ) printf("%c",*p < ' ' || *p > 0176 ? '.' : *p), p++;
	printf("|\n");
}
*/
