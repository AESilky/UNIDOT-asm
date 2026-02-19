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

#include <string.h>


/* Definitions (Local) */

void fixabs(section_t* sep);
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

	DEB(0,("\tOBGLO pass %d \n",pass2+1));
	while( objblk.ob_ptr < objblk.ob_top ){
		val = ogetl(&objblk );
		rel = ogetb(&objblk );
		syp = sylook( ogets(&objblk ));

		if( rel == URBUND ){			/* external symbol */
			DEB(0,("\t\tExternal ('%s')\n",syp->sy_str));
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
	
		DEB(0,("\t\tGlobal ('%s')\n",syp->sy_str));

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
	reg GROUP	*grp;
	reg GROUP	*gl;
	int		grno;
	int		i;

	DEB(0,("\tOBGRP pass %d \n",pass2+1));
	if( pass2 ) return;

	syp = sylook( ogets(&objblk ));
	if( syp->sy_typ == STUND ){		/* undefined	*/
		syp->sy_typ = STGRP;
#ifdef STATS
		grptab[grpx] = (GROUP *)zpalloc( sizeof(GROUP), GRPUSE );
#else
		grptab[grpx] = (GROUP *)zpalloc( sizeof(GROUP) );
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
			grp->gr_lnk = gl=(GROUP *)zpalloc(sizeof(GROUP),GRPUSE);
#else
			grp->gr_lnk = gl = (GROUP *)zpalloc( sizeof(GROUP) );
#endif
		} else {
			for( gl = grp->gr_lnk; gl->gr_lnk; gl = gl->gr_lnk );
#ifdef STATS
			gl->gr_lnk = (GROUP *)zpalloc( sizeof(GROUP), GRPUSE );
#else
			gl->gr_lnk = (GROUP *)zpalloc( sizeof(GROUP) );
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

	section_t*	sep;
	int		i;

	DEB(0,("Object file '%s'\n",curfile));
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
	DEB(0,("\tSizes of sections for %s\n   mod	   cum\n",curfile));
	for( i = URBSEC; i<stct; i++ ){
		sep = sectab[i];
		if( sep->se_atr & USEABS ) continue;	/* absolute section */
		if( !pass2 && sep->se_fpos ){
			if( sep->se_fpos != sep->se_mod)
	error("W48 Section '%s' length supposed to be 0x%lx, was 0x%lx",
		sep->se_sym->sy_str,sep->se_fpos, sep->se_mod );
			sep->se_fpos = 0;
		}
		if( sep->se_atr & USECOM ){		/* common section */
			if( sep->se_mod > sep->se_cum )
				sep->se_cum = sep->se_mod;
		} else
			sep->se_cum += sep->se_mod;	/* normal section */
		DEB(0,("%6lx\t%6lx\n",sep->se_mod,sep->se_cum));
		sep->se_mod = 0L;
	}
}
/* obloc - Processes a local symbols block.  */

void obloc() {

	char	*rpt,
			*vpt;
	uns		rel;
	long	val;

	if( !pass2 || sflag && !nflag ) return;
	DEB(0,("\tOBLOC pass %d\n",pass2 + 1));
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

	section_t	*sep;
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

	DEB(0,("\tOBSEC pass %d\n",pass2+1));
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
			if( datadu == 0 ) datadu = adu;
			i = adu - datadu;
		} else {			/* code section		*/
			if( codadu == 0 ) codadu = adu;
			i = adu - codadu;
		}
		sep = selook( ogets(&objblk) );
		syp = sep->se_sym;
		if( i ) error( "F15 Mix of address units, sect %s",syp->sy_str);
		if( sep->se_adu == 0 ){
			sep->se_adu = adu;
			if( atr & USEFIX ){
				fixabs( sep );
				sep->se_atr = USEABS;
			}
		}
		if( absadu == 0 ) absadu = sep->se_adu;
		if( sep->se_adu != adu )
		     error( "16 Section %s has different address units",
			    syp->sy_str);
		if( within >= 32 ) within = 0;
		if( sep->se_wth == 0 ) sep->se_wth = within;
		if( within && sep->se_wth != within )
			error("W50 Within conflict in sect %s",syp->sy_str);
		osec = syp->sy_rel & 0xff;
		i = sep->se_wth;
		mask = (1L << i) - 1;
wthchk:		ltmp = sep->se_cum + slen;
		if( i && (sep->se_cum ^ ltmp) & ~mask){

			/* here the contribution of this module would
			   cause up to span a 2**within boundary so we
			   need to round up the current se_cum to be
			   on a proper boundary */

			if( sep->se_atr & USEXTD1 ){
				osec = sep->se_xtd & 0xff;
				sep = sectab[osec];
				goto wthchk;
			}
			if( strcmp(proctype,"z8000") )
				sep->se_cum = (sep->se_cum | mask) + 1;
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
					sep2->se_sym = sep->se_sym;
					sep2->se_adu = sep->se_adu;
					sep2->se_atr = sep->se_atr;
					sep2->se_atr2 = sep->se_atr2;
					sep2->se_ext = sep->se_ext;
					sep2->se_grp = sep->se_grp;
					sep2->se_wth = sep->se_wth;
					sep2->se_atr &= ~(USEFIX|USEABS);
					sep->se_atr |= USEXTD1;
					sep->se_xtd = osec = stct++;
					sep = sep2;
					sep->se_mod = sep->se_cum = 0;
				}
			}
		}
		if( !pass2 ) sep->se_fpos = slen;
		DEB(0,("\t\tSECT '%s'\n",syp->sy_str));
		if( aln ){
			if( aln > sep->se_aln ) sep->se_aln = aln;
			i = (1 << aln) - 1;
			if( sep->se_cum & i ) sep->se_cum = (sep->se_cum|i) + 1;
		}
		if( ext < sep->se_ext ) sep->se_ext = ext;
		sep->se_atr |= atr & (USENOX|USENOR|USENOW|USECOM);
		sep->se_atr |= USEREF;		/* actually referenced	*/
		if( atr &= USEFIX ){
			atr = USEABS;
			fixabs( sep );
		}
		if( (sep->se_atr ^ atr) & USEABS )
		    error("52 Mix of relocatable and absolute sections in %s",
			syp->sy_str);
		secvec[svct++] = osec;		/* move to keep count right */
DEB(0,("sect %s is output section %d, atr is %x\n",
	syp->sy_str,osec,sep->se_atr&0xffff));
	}
}

void fixabs(section_t* sep) {

	if( sep->se_atr & USEFIX ){
		error("W55 Absolute sections (%s) cannot be placed",
			sep->se_sym->sy_str);
		sep->se_atr &= ~USEFIX;
	}
}
/* obtra - Processes a transfer address block.  */

void obtra() {

	DEB(0,("\tOBTRA pass %d\n",pass2 + 1));
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

	section_t	*sep;
	int		count;
	long	top;
	int		gi;
	long	l;
	char	*s;
	char	*r;

	curoff = ogetl(&objblk );		/* in address units	*/
	cursec = ogetb(&objblk );		/* section number	*/
	count = ogetb(&objblk );		/* maybe more needed	*/
	DEB(0,("\tOBTXT  (%lx, %x, %x) \n",curoff,cursec,count));
	if( cursec >= svct ) error("F30 Specified section not valid");
	outsec = secvec[cursec];
	sep = sectab[outsec];
	curadu = sep->se_adu;			/* the the address unit	*/
	if( curadu == 0 ) sep->se_adu = curadu = 8;
	if( curadu < 8 && count & 0x80 )
		count = ((count & 0x7f) << 8 ) | ogetb(&objblk);
	top = curoff+count;			/* new end of data	*/
	if( sep->se_atr & USEABS ){
		if( sep->se_mod == 0 ) sep->se_val = curoff;
		if( curoff < sep->se_val ) sep->se_val = curoff;
	}
	if( top > sep->se_mod ){
		sep->se_mod = top;
		if( count ) sep->se_atr |= USEINIT; /* has been written into */
	}
	DEB(1,("obtxt: [%d] mod: %lx\n",outsec, sep->se_mod));
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
	if( split ) gi = sep->se_atr & USENOX ? 2 : 1; else
	if( sep->se_grp ) gi = grptab[sep->se_grp-1]->gr_lnk->gr_sym->sy_rel;
	ostob( gi, objblk.ob_buf+4 );
	treloc();
	if( afmt ){
DEB(0,("obtxt: curadu = %d curoff = %ld, fpos = %ld cum = %ld\n",curadu,
	curoff,sep->se_fpos,sep->se_cum));
		if( sep->se_atr & USEABS )
			curoff -= sep->se_val;
		else
			curoff += sep->se_cum;
		if( curadu < 8 ){
			curoff *= curadu;
			if( curoff & 7 )
				error("F32 Afmt write not byte aligned");
		} else
			curoff *= (curadu+7) & ~7;
		l = sep->se_fpos + (curoff >> 3);
		fseek( OBJOUT, l, 0 );
		fclean( l, r-s );
		while( s < r ) putc( *s, OBJOUT ), s++;
	} else {
		oflush();
	}
}
/* obbsz - Processes a bss block.  */

void obbsz() {		/* init bss section to zeroes		*/

	section_t	*sep;
	int		count;
	long	top;
	int		gi;
	long	l;

	curoff = ogetl(&objblk );		/* in address units	*/
	cursec = ogetb(&objblk );		/* section number	*/
	count = ogetb(&objblk );		/* get count of adu's	*/
	count = (count << 8) | ogetb(&objblk);
	DEB(0,("\tOBBSZ  (%lx, %x, %x) \n",curoff,cursec,count));
	if( cursec >= svct ) error("F30 Specified section not valid");
	outsec = secvec[cursec];
	sep = sectab[outsec];
	curadu = sep->se_adu;			/* the the address unit	*/
	top = curoff+count;			/* new end of data	*/
	if( !count ) return;			/* empty		*/
	sep->se_atr |= USEINIT;			/* has been written into */
	if( sep->se_atr & USEABS ){
		if( sep->se_mod == 0 ) sep->se_val = curoff;
		if( curoff < sep->se_val ) sep->se_val = curoff;
	}
	if( top > sep->se_mod ) sep->se_mod = top;
	DEB(1,("bsz: [%d] mod: %lx\n",outsec, sep->se_mod));

	if( !pass2 ) return;
	if( curadu != 8 ) count = curadu > 8 ?
				count * ((curadu+7) >> 3) :
				((count * curadu) + 7) >> 3;
	l = rbase(cursec);
	ostol( l + curoff, objblk.ob_buf );
	gi = URBABS;
	if( rflag ) gi = outsec; else
	if( split ) gi = sep->se_atr & USENOX ? 2 : 1; else
	if( sep->se_grp ) gi = grptab[sep->se_grp-1]->gr_lnk->gr_sym->sy_rel;
	ostob( gi, objblk.ob_buf+4 );
	if( afmt ){
DEB(0,("obbsz: curadu = %d curoff = %ld, fpos = %ld cum = %ld\n",curadu,
	curoff,sep->se_fpos,sep->se_cum));
		if( cursec == URBABS )
			curoff -= sep->se_val;
		else
			curoff += sep->se_cum;
		if( curadu < 8 ){
			curoff *= curadu;
			if( curoff & 7 )
				error("F32 Afmt write not byte aligned");
		} else
			curoff *= (curadu+7) & ~7;
		l = sep->se_fpos + (curoff >> 3);
		fseek( OBJOUT, l, 0 );
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
