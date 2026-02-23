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
*			     UAS Linker					*
*									*
*			ulkobj.c - object block processing		*
*									*
************************************************************************/


static char rcsid[] =
"@(#)$Header: ulkobj.c,v 4.25 92/07/31 07:58:53 rmm Rel $ object block processing";

#include "ulink.h"
#ifdef vms
#include "[-.incl]uobj.h"
#else
#include "../incl/uobj.h"
#endif
#include "funcdefs.h"

#include <stdio.h>
#include <string.h>


/* Definitions (Local) */

void fixabs(section_t* secp);
void obbsz();
void obloc();
void obpro();
void obsec();
void obtxt();
void obtra();
void odump();


/*
 * obglo - Processes a global symbols block.
 */

void obglo(){

	sytab_t	*syp;
	uns		rel;
	long	val;
	int		i;
	int		j;

	DEBOUT(pass2 + 1, ("\tOBGLO pass %d \n", pass2 + 1));
	while( objblk.ob_ptr < objblk.ob_top ){
		val = ogetl(&objblk );
		rel = ogetb(&objblk );
		syp = sylook( ogets(&objblk ));

		if( rel == URBUND ){			/* external symbol */
			DEBOUT(pass2 + 1, ("\t\tExternal ('%s')\n", syp->sy_str));
			if( evct >= EXTSIZ ) error("F12 Too many externals" );	
			extvec[evct++] = syp;
			if( pass2 ){
				if( rflag && !afmt ) continue;
				if( val == 0 && !(syp->sy_atr & SADP2) )
					/* not defined */
					syp->sy_atr |= SAUP2;
				continue;
			}
			if( syp->sy_typ == STUND && val ){ /* C common */
				i = (val >> 24) & 0xff;
				j = (syp->sy_val >> 24) & 0xff;
				val &= 0xffffffL;
				syp->sy_val &= 0xffffffL;
				if( j > i ) i = j;
				if( syp->sy_val > val ) val = syp->sy_val;
				syp->sy_val = val | ((long) i << 24);
			}
			continue;
		}
		if( rel == URBABS ){			/* absolute symbol */
			if( pass2 ){
				if( syp->sy_atr & SADP2 ) continue;
				syp->sy_atr &= ~SAUP2;
				syp->sy_atr |= SADP2;
				if( afmt && !sflag )
					outsym( syp->sy_rel, syp->sy_val,
						syp->sy_str );
				continue;
			}
			if( syp->sy_typ == STUND ){
				syp->sy_typ = STGLO;
				syp->sy_val = val;
				syp->sy_rel = URBABS;
				continue;
			}
			if( syp->sy_typ != STGLO ||
			     syp->sy_val != val ||
			     syp->sy_rel != URBABS )
				syp->sy_atr |= SAMUD;
			continue;
		}
	
		DEBOUT(pass2 + 1, ("\t\tGlobal ('%s')\n", syp->sy_str));

		/* entry symbol */

		if( pass2 ){
			if( syp->sy_atr & SADP2 ) syp->sy_atr |= SAMUD;
			syp->sy_atr &= ~SAUP2;
			syp->sy_atr |= SADP2;
			if( afmt && !sflag )
				outsym( syp->sy_rel, syp->sy_val, syp->sy_str );
			continue;
		}

		/* pass1 actions for entries */


		if( syp->sy_typ == STUND ){
			syp->sy_typ = STGLO;
			syp->sy_val = val+rbase( rel );
			syp->sy_rel = secvec[rel];
			syp->sy_ovl = ovlnum;
		} else
			syp->sy_atr |= SAMUD;
	}
}
/*
 * obgrp - Processes a group block
 *
 * The first item is the symbol for the group - succeeding symbols are
 * section names for the group - They must have been pre-defined.
 *
 */

void obgrp(){

	sytab_t	*syp;
	group_t	*grp;
	group_t	*gl;
	int		grno;
	int		i;

	DEBOUT(0,("\tOBGRP pass %d \n",pass2+1));
	if( pass2 ) return;

	syp = sylook( ogets(&objblk ));
	if( syp->sy_typ == STUND ){		/* undefined	*/
		syp->sy_typ = STGRP;
#ifdef STATS
		grptab[grpx] = (group_t *)zpalloc( sizeof(group_t), GRPUSE );
#else
		grptab[grpx] = (group_t *)zpalloc( sizeof(group_t) );
#endif
		grptab[grpx]->gr_sym = syp;
		syp->sy_rel = ++grpx;		/* set the group number */
	}
	if( syp->sy_typ != STGRP )
		error("F34 group sym not a group %s",syp->sy_str);
	grno = syp->sy_rel;
	grp = grptab[ grno - 1 ];
	while( objblk.ob_ptr < objblk.ob_top ){
		syp = sylook( ogets(&objblk ));
		if( syp->sy_typ != STSEC )
			error("F33 Symbol %s not a section of a group",
				syp->sy_str);
		if( i = sectab[syp->sy_rel & 0xff]->se_grp ){
			if( i != grno )
				error("F29 Sect %s member of another group",
					syp->sy_str);
			continue;	/* already in this group	*/
		}
		if( grp->gr_lnk == 0 ){
#ifdef STATS
			grp->gr_lnk = gl=(group_t *)zpalloc(sizeof(group_t),GRPUSE);
#else
			grp->gr_lnk = gl = (group_t *)zpalloc( sizeof(group_t) );
#endif
		} else {
			for( gl = grp->gr_lnk; gl->gr_lnk; gl = gl->gr_lnk );
#ifdef STATS
			gl->gr_lnk = (group_t *)zpalloc( sizeof(group_t), GRPUSE );
#else
			gl->gr_lnk = (group_t *)zpalloc( sizeof(group_t) );
#endif
			gl = gl->gr_lnk;
		}
		gl->gr_sym = syp;
		gl->gr_lnk = 0;
	}
}
/*
 * object - Process a single object module.  Expects objbuf to contain the
 * object start block for the module.
 */

void object() {

	section_t*	secp;
	int		i;

	DEBOUT(0,("Object file '%s'\n",curfile));
	if( verbose ) printf("processing %s\n",curfile);
	if( pass2 && LOCFILE )
		fprintf(LOCFILE,"========== %s ============\n\n",curfile);
	if( objblk.ob_type != UOBOST )
		error("F25 First byte not correct for object file");
	evct = 0;
	svct = URBSEC;
	relinit();		/* init default relocation actions */
	while( (i = ofill(&objblk, OBJIN )) != UOBOND ){

		/* process blocks according to type.  */

		if( i == -1 ) error("F13 Unexpected end of file in %s",curfile);
		switch( objblk.ob_type ){

		case UOBGLO: obglo(); break;

		case UOBSEC: obsec(); break;

		case UOBLOC: obloc(); break;

		case UOBTRA: obtra(); break;

		case UOBTXT: obtxt(); break;

		case UOBBSZ: obbsz(); break;

		case UOBMOD: if( pass2 && !afmt ) oflush(); /* Module name */
			     break;

		case UOBPRO: obpro(); break;

		case UOBCMT: break;		/* comment	*/

		case UOBRLT: obrlt(); break;

		case UOBGRP: obgrp(); break;

		default: odump(); break;

		}
	}
	if( pass2 && LOCFILE ){
		if( nflag != 1 ) fputc( '\n', LOCFILE );
		fputc( '\n', LOCFILE );
		nflag = 1;
	}
	DEBOUT(0,("\tSizes of sections for %s\n   mod	   cum\n",curfile));
	for( i = URBSEC; i<stct; i++ ){
		secp = sectab[i];
		if( secp->se_atr & USEABS ) continue;	/* absolute section */
		if( !pass2 && secp->se_fpos ){
			if( secp->se_fpos != secp->se_mod)
	error("W48 Section '%s' length supposed to be 0x%lx, was 0x%lx",
		secp->se_sym->sy_str,secp->se_fpos, secp->se_mod );
			secp->se_fpos = 0;
		}
		if( secp->se_atr & USECOM ){		/* common section */
			if( secp->se_mod > secp->se_cum )
				secp->se_cum = secp->se_mod;
		} else
			secp->se_cum += secp->se_mod;	/* normal section */
		DEBOUT(0,("%6lx\t%6lx\n",secp->se_mod,secp->se_cum));
		secp->se_mod = 0L;
	}
}
/* obloc - Processes a local symbols block.  */

void obloc() {

	char	*rpt,
		*vpt;
	uns	rel;
	long	val;

	if( !pass2 || sflag && !nflag ) return;
	DEBOUT(0,("\tOBLOC pass %d\n",pass2 + 1));
	while( objblk.ob_ptr < objblk.ob_top ){
		vpt = objblk.ob_ptr;
		val = ogetl(&objblk );
		rpt = objblk.ob_ptr;
		rel = ogetb(&objblk );
		val += rbase(rel);		/* relocate		*/
		rel = secvec[rel];
		if( !sflag ){
			if( afmt ){
				outsym( rel, val, objblk.ob_ptr );
			} else {
				ostol( val, vpt );	/* rewrite value */
				ostob( ovlnum, rpt );	/* overlay kludge */
			}
		}
		if( LOCFILE ){
			fprintf(LOCFILE,"%-8.8s %8lx  ",objblk.ob_ptr,val);
			if( ++nflag == 5 ) fputc( '\n', LOCFILE), nflag = 1;
		}
		ogets(&objblk );		/* skip symbol		*/
	}
	oflush();
}

/* obsec - Processes a sections block.  */

void obsec() {

	section_t	*secp;
	sytab_t	*syp;
	int		i;
	int		aln;
	int		ext;
	int		atr;
	int		adu;
	int		atr2;
	int		within;
	long	slen;
	long	mask;
	int		osec;
	long		ltmp;

	DEBOUT(0,("\tOBSEC pass %d\n",pass2+1));
	while( objblk.ob_ptr < objblk.ob_top ){
		if( svct >= SECSIZ ) error("F14 Too many sections" );
		aln = ogetb(&objblk );
		ext = ogetb(&objblk );
		atr = ogetb(&objblk );
		atr2 = 0;
		slen = 0;
		within = 0;
		adu = 8;
		if( atr & USEMOR ) atr2 = ogetb( &objblk );
		if( atr2 & USMADU ) adu = ogetb( &objblk );
		if( atr2 & USMWTH ) within = ogetb( &objblk );
		if( atr2 & USMLEN ) slen = ogetl( &objblk );
		if( atr & USENOX ){		/* data section		*/
			if (debug > 1 && pass2 && *syp->sy_str) printf("\t\tsect %s is a DATA section\n", syp->sy_str);
			if( datadu == 0 ) datadu = adu;
			i = adu - datadu;
		} else {			/* code section		*/
			if (debug > 1 && pass2 && *syp->sy_str) printf("\t\tsect %s is a CODE section\n", syp->sy_str);
			if( codadu == 0 ) codadu = adu;
			i = adu - codadu;
		}
		secp = selook( ogets(&objblk) );
		syp = secp->se_sym;
		if( i ) error( "F15 Mix of address units, sect %s",syp->sy_str);
		if( secp->se_adu == 0 ){
			secp->se_adu = adu;
			if( atr & USEFIX ){
				fixabs( secp );
				secp->se_atr = USEABS;
			}
		}
		if( absadu == 0 ) absadu = secp->se_adu;
		if( secp->se_adu != adu )
		     error( "16 Section %s has different address units",
			    syp->sy_str);
		if( within >= 32 ) within = 0;
		if( secp->se_wth == 0 ) secp->se_wth = within;
		if( within && secp->se_wth != within )
			error("W50 Within conflict in sect %s",syp->sy_str);
		osec = syp->sy_rel & 0xff;
		i = secp->se_wth;
		mask = (1L << i) - 1;
wthchk:		ltmp = secp->se_cum + slen;
		if( i && (secp->se_cum ^ ltmp) & ~mask){

			/* here the contribution of this module would
			   cause up to span a 2**within boundary so we
			   need to round up the current se_cum to be
			   on a proper boundary */

			if( secp->se_atr & USEXTD1 ){
				osec = secp->se_xtd & 0xff;
				secp = sectab[osec];
				goto wthchk;
			}
			if( strcmp(proctype,"z8000") )
				secp->se_cum = (secp->se_cum | mask) + 1;
			else {
				/* here the contribution of this module to
				   this section will violate the within
				   constraints - we solve this problem by
				   making up another section with the
				   same name! */

				if( slen > (1L << 16) )
error("F17 sect bigger than allowed (%lx)",slen);
				if( rflag ) error("F17 can't relink overflowed sects");
				if( !pass2 ){
					section_t *sep2;
					if( stct >= SECSIZ )
						error("F21 Too many sections");
					sectab[stct] = sep2 = 
#ifdef STATS
				    (section_t *)zpalloc(sizeof(section_t),SECUSE);
#else
					(section_t *)zpalloc(sizeof(section_t));
#endif
					sep2->se_sym = secp->se_sym;
					sep2->se_adu = secp->se_adu;
					sep2->se_atr = secp->se_atr;
					sep2->se_atr2 = secp->se_atr2;
					sep2->se_ext = secp->se_ext;
					sep2->se_grp = secp->se_grp;
					sep2->se_wth = secp->se_wth;
					sep2->se_atr &= ~(USEFIX|USEABS);
					secp->se_atr |= USEXTD1;
					secp->se_xtd = osec = stct++;
					secp = sep2;
					secp->se_mod = secp->se_cum = 0;
				}
			}
		}
		if( !pass2 ) secp->se_fpos = slen;
		DEBOUT(0,("\t\tSECT '%s'\n",syp->sy_str));
		if( aln ){
			if( aln > secp->se_aln ) secp->se_aln = aln;
			i = (1 << aln) - 1;
			if( secp->se_cum & i ) secp->se_cum = (secp->se_cum|i) + 1;
		}
		if( ext < secp->se_ext ) secp->se_ext = ext;
		secp->se_atr |= atr & (USENOX|USENOR|USENOW|USECOM);
		secp->se_atr |= USEREF;		/* actually referenced	*/
		if( atr &= USEFIX ){
			atr = USEABS;
			fixabs( secp );
		}
		if( (secp->se_atr ^ atr) & USEABS )
		    error("52 Mix of relocatable and absolute sections in %s",
			syp->sy_str);
		secvec[svct++] = osec;		/* move to keep count right */
DEBOUT(0,("\t\tsect %s is output section %d, atr is %x\n",
	syp->sy_str,osec,secp->se_atr&0xffff));
	}
}

void fixabs(section_t* secp) {

	if( secp->se_atr & USEFIX ){
		error("W55 Absolute sections (%s) cannot be placed",
			secp->se_sym->sy_str);
		secp->se_atr &= ~USEFIX;
	}
}
/* obtra - Processes a transfer address block.  */

void obtra() {

	DEBOUT(0,("\tOBTRA pass %d\n",pass2 + 1));
	if( pass2 && !traflg ){
		tranad = olodl(objblk.ob_buf) + rbase(olodw(objblk.ob_buf+4));
		traflg = 1;
		if( !afmt ){
			ostol( tranad, objblk.ob_buf );
			ostow( URBABS, objblk.ob_buf+4 );
			oflush();
		}
	}
}

/* obtxt - Processes a text block.  */

void obtxt() {

	section_t	*secp;
	int		count;
	long		top;
	int		gi;
	long		l;
	char		*s;
	char		*r;

	curoff = ogetl(&objblk );		/* in address units	*/
	cursec = ogetb(&objblk );		/* section number	*/
	count = ogetb(&objblk );		/* maybe more needed	*/
	DEBOUT(0,("\tOBTXT  (curoff:%lx, cursec:%x, count:%x) \n",curoff,cursec,count));
	if( cursec >= svct ) error("F30 Specified section not valid");
	outsec = secvec[cursec];
	secp = sectab[outsec];
	curadu = secp->se_adu;			/* the address unit	*/
	if( curadu == 0 ) secp->se_adu = curadu = 8;
	if( curadu < 8 && count & 0x80 )
		count = ((count & 0x7f) << 8 ) | ogetb(&objblk);
	top = curoff+count;			/* new end of data	*/
	if( secp->se_atr & USEABS ){
		if( secp->se_mod == 0 ) secp->se_val = curoff;
		if( curoff < secp->se_val ) secp->se_val = curoff;
	}
	if( top > secp->se_mod ){
		secp->se_mod = top;
		if( count ) secp->se_atr |= USEINIT; /* has been written into */
	}
	DEBOUT(1,("\t\tobtxt(P%d): outsec:%d mod:%lx\n", pass2+1, outsec, secp->se_mod));
	if( !pass2 ) return;			/* no more work		*/
	if( !count && !rflag ) return;		/* no more work		*/

	if( curadu != 8 ) count = curadu > 8 ?
				count * ((curadu+7) >> 3) :
				((count * curadu) + 7) >> 3;
	r = txttop = objblk.ob_ptr + count;
	s = objblk.ob_ptr;			/* for copy out		*/
	l = rbase(cursec);
	ostol( l + curoff, objblk.ob_buf );
	gi = URBABS;
	if( rflag || kflag ) gi = outsec; else
	if( split ) gi = secp->se_atr & USENOX ? 2 : 1; else
	if( secp->se_grp ) gi = grptab[secp->se_grp-1]->gr_lnk->gr_sym->sy_rel;
	ostob( gi, objblk.ob_buf+4 );
	treloc();
	if( afmt ){
DEBOUT(0,("obtxt-afmt: curadu = %d curoff = %ld, fpos = %ld cum = %ld\n",curadu,
	curoff,secp->se_fpos,secp->se_cum));
		if( secp->se_atr & USEABS )
			curoff -= secp->se_val;
		else
			curoff += secp->se_cum;
		if( curadu < 8 ){
			curoff *= curadu;
			if( curoff & 7 )
				error("F32 Afmt write not byte aligned");
		} else
			curoff *= (curadu+7) & ~7;
		l = secp->se_fpos + (curoff >> 3);
		fseek(OBJOUT, l, SEEK_SET);
		fclean( l, r-s );
		while( s < r ) putc( *s, OBJOUT ), s++;
	} else {
		oflush();
	}
}
/* obbsz - Processes a bss block.  */

void obbsz() {		/* init bss section to zeroes		*/

	section_t	*secp;
	int		count;
	long	top;
	int		gi;
	long	l;

	curoff = ogetl(&objblk );		/* in address units	*/
	cursec = ogetb(&objblk );		/* section number	*/
	count = ogetb(&objblk );		/* get count of adu's	*/
	count = (count << 8) | ogetb(&objblk);
	DEBOUT(0,("\tOBBSZ  (%lx, %x, %x) \n",curoff,cursec,count));
	if( cursec >= svct ) error("F30 Specified section not valid");
	outsec = secvec[cursec];
	secp = sectab[outsec];
	curadu = secp->se_adu;			/* the the address unit	*/
	top = curoff+count;			/* new end of data	*/
	if( !count ) return;			/* empty		*/
	secp->se_atr |= USEINIT;			/* has been written into */
	if( secp->se_atr & USEABS ){
		if( secp->se_mod == 0 ) secp->se_val = curoff;
		if( curoff < secp->se_val ) secp->se_val = curoff;
	}
	if( top > secp->se_mod ) secp->se_mod = top;
	DEBOUT(1,("bsz: [%d] mod: %lx\n",outsec, secp->se_mod));

	if( !pass2 ) return;
	if( curadu != 8 ) count = curadu > 8 ?
				count * ((curadu+7) >> 3) :
				((count * curadu) + 7) >> 3;
	l = rbase(cursec);
	ostol( l + curoff, objblk.ob_buf );
	gi = URBABS;
	if( rflag ) gi = outsec; else
	if( split ) gi = secp->se_atr & USENOX ? 2 : 1; else
	if( secp->se_grp ) gi = grptab[secp->se_grp-1]->gr_lnk->gr_sym->sy_rel;
	ostob( gi, objblk.ob_buf+4 );
	if( afmt ){
DEBOUT(0,("obbsz: curadu = %d curoff = %ld, fpos = %ld cum = %ld\n",curadu,
	curoff,secp->se_fpos,secp->se_cum));
		if( cursec == URBABS )
			curoff -= secp->se_val;
		else
			curoff += secp->se_cum;
		if( curadu < 8 ){
			curoff *= curadu;
			if( curoff & 7 )
				error("F32 Afmt write not byte aligned");
		} else
			curoff *= (curadu+7) & ~7;
		l = secp->se_fpos + (curoff >> 3);
		fseek(OBJOUT, l, SEEK_SET);
		fclean( l, count );
		while( --count >= 0 ) putc( 0, OBJOUT );
	} else {
		oflush();
	}
}

/*
 * obpro - Handles a processor name declaration
 */

void obpro() {

	static int once;

	if( pass2 ){
		if( !afmt && once == 0 ){
			oflush();
			once = 1;
		}
		return;
	}
	if( proctype[0] == 0 ) strcpy( proctype, objblk.ob_ptr );
	if( strcmp( proctype, objblk.ob_ptr ) )
		error("W27 Modules for processors %s and %s linked",
			proctype, objblk.ob_ptr );
}

/*
 * odump - Dumps an object block for debugging.
 */

void odump() {

	char	*optsave;

	optsave = objblk.ob_ptr;
	objblk.ob_ptr = objblk.ob_buf;
	printf( "Type = %d, length = %d:", objblk.ob_type&0xff,
	objblk.ob_top-objblk.ob_buf );
	while( objblk.ob_ptr < objblk.ob_top ){
		if( ((objblk.ob_ptr-objblk.ob_buf) & 0xf) == 0 ) putchar('\n');
		printf( " %02x", ogetb( &objblk ));
	}
	printf( "\n\n" );
	objblk.ob_ptr = optsave;
}

void outsym(int sect, long addr, char* str) {

	/* writes a symbol in afmt style - the address is the relocated
	   address - it should be adjusted to be section relative if
	   the rflag is set */

	int	i;

	if( rflag && sect ) addr -= sectab[sect & 0xff]->se_val;
	putc( sect, SYMFILE );
	along( addr, SYMFILE );
	symsize += 6;
	while( i = *str++ ){
		putc( i, SYMFILE );
		symsize++;
	}
	putc( 0, SYMFILE );
}
