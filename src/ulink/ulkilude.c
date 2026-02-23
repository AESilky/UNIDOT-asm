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
*			ulkilude.c - interlude processing		*
*									*
************************************************************************/


static char rcsid[] =
"@(#)$Header: ulkilude.c,v 4.26 92/07/31 07:58:30 rmm Rel $ interlude processing";

#include "ulink.h"
#ifdef vms
#include "[-.incl]uobj.h"
#else
#include "../incl/uobj.h"
#endif
#include "funcdefs.h"

#include <string.h>

/* Definitions (Local) */

void afmtstt();
void allocseg(section_t* secp, section_t* lsecp);
void asgncom();
void asgnsec();
void asgnsym();
void grpcheck(group_t* gp);
void grpout(group_t* gp);
void ovrlapch();
void splitinit();
void symtoaout(section_t* csep);
void symtoobj();
void ufmtstt();

	/* the following are the section location counters	*/

static long		dtop;		/* data top if split		*/
static long		ctop;		/* code top if split		*/
static section_t		*csep;		/* section ptr for common	*/
static short		csect;		/* section number for common	*/
static short		codsec;		/* section number for code out	*/
static short		datsec;		/* section number for data out	*/
static short		xtdfound;	/* found an extend		*/
long			secfsize();

/*
 * interlude - Performs interpass processing.
 */


void interlude(){

	section_t	*secp;
	sytab_t	*syp;
	int		i;
	uns		h;
	long		l;
	long		base;
	long		top;

	/* Find any global symbols which are still undefined.  */

DEBOUT(0,("INTERLUDE\n"));

	if( proctype[0] && strcmp(proctype,"nscbcp") == 0 )
		codadu = absadu = 16, datadu = 8;
	if( datadu == 0 ){
		if( codadu == 0 ) codadu = 8;
		datadu = codadu;
	}
	if( codadu == 0 ) codadu = 8;
	if( absadu == 0 ) absadu = codadu;
	if( codadu != datadu && !rflag ) split = 1;	/* force split	*/
	if( split ) splitinit();			/* set up split	*/
	asgncom();			/* assign common variables	*/

	for( i=0; i<grpx; i++ )		/* check for group consistency */
		grpcheck(grptab[i]);

	asgnsec();	/* Assign hard addresses to sections.		*/
	asgnsym();	/* Assign hard addresses to global symbols	*/
	ovrlapch();	/* Check for section overlap			*/
	secp = sectab[URBABS];		/* point to absolute section	*/
	secp->se_adu = absadu;		/* set abs section adu		*/
	secp->se_cum = secp->se_mod - secp->se_val;
	if( secp->se_cum && codsep && codsep->se_cum == 0 ){
		codsep->se_val = secp->se_val;
		codsep->se_cum = secp->se_cum;
	}

	/* Start off the object output file.  */

	if( afmt ){
		afmtstt();
		if( !sflag && csep ) symtoaout(csep); /* syms to a.out	*/
	} else {
		ufmtstt();
		if( !sflag ) symtoobj();	/* output symbols to uobj */
		if( csep ){
			base = csep->se_val;
			top = base + csep->se_cum;
			while( base < top ){
				objblk.ob_type = UOBBSZ;
				l = top - base;
				if( l > 16384 ) l = 16384;
				oputl( base );
				oputb( csep->se_sym->sy_rel & 0xff );
				oputb( (int)l );
				oputb( (int)l>>8 );
				oflush();
				base += l;
			}
		}
	}

	/* now reset sizes of sections for pass 2 */

	for( i=0; i<stct; i++ ){
		secp = sectab[i];
		if( secp == csep ) continue;	/* no reset	*/
		if( secp->se_atr & USEABS ) continue;
		secp->se_cum = 0;	/* reset size of this section	*/
		secp->se_mod = 0;	/* reset size of this section	*/
	}
DEBOUT(0,("INTERLUDE end\n"));
}

/* start off the aformat (or binary) output file */
void afmtstt() {

	section_t	*secp;
	int		i;
	long		l;


	secp = sectab[0];		/* point to absolute sect	*/
	secp->se_fpos = 0;		/* 0 file pos			*/
	if (!binfmt) {
		fclean(0L, 16 + 8 * stct);
		aword( 0x0605, OBJOUT );	/* magic word			*/
		aword( stct, OBJOUT );		/* count of sections		*/
		along( 0L, OBJOUT );		/* transfer address		*/
		along( 0L, OBJOUT );		/* size of relocation section	*/
		along( 0L, OBJOUT );		/* size of symbol section	*/
		secp->se_fpos = 16 + 8 * stct;	/* file pos of abs section	*/
		along(secfsize(secp), OBJOUT);		/* write size abs sect	*/
	}
	fpos = secp->se_fpos + secfsize(secp);	/* set file locctr	*/
	fpos &= 0xffffffL;			/* trim adu info	*/
	if (!binfmt) {
		along(secp->se_val, OBJOUT);		/* address of abs section */
	}
	DEBOUT(0, ("Section Num     fpos      val    fsize name (rel)\n"));
	for( i=URBSEC; i<stct; i++ ){
		secp = sectab[i];
		secp->se_fpos = 0;
		l = secfsize(secp);
		if( secp->se_atr & USEINIT ){
			if (!binfmt)
				secp->se_fpos = fpos;	/* file pos for afmt	*/
			else
				secp->se_fpos = secp->se_val;	/* file pos for binfmt */
			fpos += l;		/* adj file position	*/
			fpos &= 0xffffffL;	/* trim adu info	*/
		} else {
			l |= 0x80000000L;
		}
		if (!binfmt) {
			along( l, OBJOUT );		/* size of section	*/
			along( secp->se_val, OBJOUT );	/* loc of sect		*/
		}
	DEBOUT(0,("Section: %2d %8lx %8lx %8lx %s (%d)\n",
		i,secp->se_fpos,secp->se_val,l,secp->se_sym->sy_str,
		secp->se_sym->sy_rel & 0xff));
	}
}

void ufmtstt() {

	section_t	*secp;
	int		i;
	uns		h;

	/* start off a u.obj format output file	*/

	objblk.ob_type = 0;
	oflush();
	objblk.ob_type = UOBOST;
	oflush();

	if( rflag || kflag ){

		/* the file is to be relinked, so we need to output
		   a section record for all sections in the output
		   file
		*/

		for( i=URBSEC; i<stct; i++ ){
			secp = sectab[i];
			objblk.ob_type = UOBSEC;
			oputb(secp->se_aln);
			oputb(secp->se_ext);
			h = secp->se_atr | USEMOR;
			if( secp->se_atr & USEABS ) h |= USEFIX;
			oputb(h);
			h = USMLEN;
			if( secp->se_adu != 8 ) h |= USMADU;
			if( secp->se_wth ) h |= USMWTH;
			oputb(h);
			if( h & USMADU ) oputb( secp->se_adu );
			if( h & USMWTH ) oputb( secp->se_wth );
			oputl(secp->se_cum);
			oputs(secp->se_sym->sy_str);
			oflush();
		}
		if( !split ){
			for( i=0; i<grpx; i++ ) grpout( grptab[i] );
			return;
		}
	}

	if( split ){

		/* the file is being prepared for a "split" I/D load -
		   that is, there are two sections being output, one
		   for code, and one for data, so section records
		   should be written
		*/

		objblk.ob_type = UOBSEC;
		oputb(0);
		oputb(0);
		i = USENOW;
		if( codadu != 8 ) i |= USEMOR;
		oputb(i);
		if( i & USEMOR ){
			oputb( USMADU );
			oputb( codadu );
		}
		oputs( "code" );
		oflush();
		objblk.ob_type = UOBSEC;
		oputb(0);
		oputb(0);
		i = USENOX;
		if( datadu != 8 ) i |= USEMOR;
		oputb(i);
		if( i & USEMOR ){
			oputb( USMADU );
			oputb( datadu );
		}
		oputs( "data" );
		oflush();
		return;
	}

	if( absadu == 8 ) return;

	/* the file is an ordinary link, but the addressing
	   unit for the absolute section is not 8 bits, so
	   here we output a dummy section just so a
	   subsequent operation will get the right adu for
	   an absolute section. Specifically, we got
	   the absadu from the first section encountered
	   so we will use it */

	objblk.ob_type = UOBSEC;
	oputb(0);
	oputb(32);
	oputb(USEMOR);
	oputb(USMADU);
	oputb(absadu);
	oputs( ".abs " );	/* NOTE BLANK */
	oflush();
}

long secfsize(section_t* secp) {

	int	i;
	long	l;

	/* returns the section length in bytes (file length) and inserts
	   the adu information in the high byte */

	i = secp->se_adu;
	l = secp->se_cum;
	if( i < 8 ){
		/* if adu is less than 8 addressing units are packed solid
		   into bytes */
		l = ((l * i) + 7) >> 3;
		l += (long)i << 24;
	} else
	if( i > 8 ){
		/* if adu is more than 8 addressing units leave gaps
		   between bytes */
		l = l * ((i + 7) >> 3);
		if( i <= 64 ) l += (long)i << 24;
	}
	return l;
}


void grpout(group_t* gp) {

	group_t 	*gl;
	char	*p;
	char	*p2;
	int		n;

	objblk.ob_type = UOBGRP;
	oputs( p = gp->gr_sym->sy_str );
	n = strlen(p) + 1;
	for( gl = gp->gr_lnk; gl; gl = gl->gr_lnk ){
		if( n >= 220 ){
			oflush();
			oputs( p );
			n = strlen(p) + 1;
		}
		oputs( p2 = gl->gr_sym->sy_str );
		n += strlen(p2) + 1;
	}
	oflush();
}

void grpcheck(group_t* gp) {	/* check for group consistency		*/

	section_t 	*secp;
	group_t 	*gl;
	int		n;
	int		at;
	int		adu;

	at = 0;
	adu = 0;
	for( n=0, gl=gp->gr_lnk; gl; gl=gl->gr_lnk, n++ ){
		secp = sectab[ gl->gr_sym->sy_rel & 0xff ];
		at |= secp->se_atr;
		if( adu == 0 ) adu = secp->se_adu;
		if( adu != secp->se_adu )
			error("36 Group has different addressing units");
		if( n && secp->se_atr & USEFIX ){
			error("31 Only first section of group may be fixed");
			secp->se_atr &= ~USEFIX;
		}
	}
	at &= USENOX|USENOR|USENOW;
	if( at == (USENOX|USENOR|USENOW) )
		error("W37 Group is not executable, readable, or writeable");
	for( gl=gp->gr_lnk; gl; gl=gl->gr_lnk ){
		secp = sectab[ gl->gr_sym->sy_rel & 0xff ];
		secp->se_atr |= at;
	}
}

void asgncom() {		/* assign addresses to common variables	*/

	section_t	*secp;
	sytab_t	*syp;
	int		i;
	uns		h;
	long		l;

	/* Find any global symbols which are still undefined.  */

	for( h = 0; h < 1 << HSHLOG; h++ ){
		for( syp = syhtab[h]; syp; syp = syp->sy_lnk ){
			if( syp->sy_atr & SAMUD ){
				error("41 Multiply defined: %s", syp->sy_str);
				mulct++;
			}
			if( syp->sy_typ == STUND ){
				syp->sy_rel = 0xff;
				if( rflag && !afmt ) continue;
				if( syp->sy_val ){
					if( csep == 0 ){
						csep = selook(commvar);
						csep->se_adu = datadu;
						csect = csep->se_sym->sy_rel;
						csep->se_atr |= USEREF|USENOX;
					}
					secp = csep;
					l = syp->sy_val;
					i = (l >> 24) & 0xf;
					l &= 0xffffffL;	/* val in adus */
					if( i > commalign ) commalign = i;
					if( i ){
						i = (1 << i) - 1;
						secp->se_cum += i;
						secp->se_cum &= ~(long)i;
					}
					syp->sy_val = secp->se_cum;
					secp->se_cum += l;
					secp->se_aln = commalign;
					syp->sy_typ = STGLO;
					syp->sy_rel = csect;
				} else {
					error("40 Undefined: %s", syp->sy_str);
					undct++;
				}
			}
		}
	}

#ifdef OLDCODE
	if( mulct ){ /* Print global symbols that are multiply defined.  */
		printf( "Multiply Defined:\n");
		for( h = 0; h < 1 << HSHLOG; h++ ){
			for( syp = syhtab[h]; syp; syp = syp->sy_lnk ){
				if( syp->sy_atr & SAMUD ){
					printf( " %-8.8s\n", syp->sy_str );
					errct++;
				}
			}
		}
	}
#endif
}

void splitinit(){		/* called immediately after init	*/

#ifdef STATS
	codsep = (section_t *)zpalloc(sizeof(section_t),SECUSE);
	datsep = (section_t *)zpalloc(sizeof(section_t),SECUSE);
#else
	codsep = (section_t *)zpalloc(sizeof(section_t));
	datsep = (section_t *)zpalloc(sizeof(section_t));
#endif
	codsep->se_adu = codadu;
	datsep->se_adu = datadu;
	codsep->se_atr = USEFIX|USENOW;		/* not writeable */
	datsep->se_atr = USEFIX|USENOX;		/* not executable */
	codsep->se_sym = sylook( "code " );	/* NOTE BLANK	*/
	datsep->se_sym = sylook( "data " );	/* NOTE BLANK	*/
	codsep->se_sym->sy_typ = STSEC;
	codsep->se_sym->sy_rel = codsec = 1;
	datsep->se_sym->sy_typ = STSEC;
	datsep->se_sym->sy_rel = datsec = 2;
}

void asgnsec() {		/* assign addresses to sections		*/

	section_t	*secp;
	group_t	*gl;
	int		i;
	int		n;
	group_t	*grp;
	section_t	*lsecp;

	lsecp = 0;
	for( i = URBSEC; i<stct; i++ ){
		secp = sectab[i];

		if( secp->se_atr & USEALLO ) continue;	/* allocated	*/
		if( secp->se_atr & USEABS ){
			secp->se_cum = secp->se_mod - secp->se_val;
			secp->se_atr |= USEALLO;
			continue;
		}
		if( !(secp->se_atr & USEREF) )
			error("W11 Section %s not encountered in any module",
				secp->se_sym->sy_str );
		if( rflag ) secp->se_grp = 0;	/* no groups assigned	*/
		if( n = secp->se_grp ){
			if( secp->se_sym != (grp=grptab[n-1])->gr_lnk->gr_sym)
				continue;
			for( gl = grp->gr_lnk; gl; gl = gl->gr_lnk ){
				allocseg(sectab[gl->gr_sym->sy_rel&0xff],lsecp);
				lsecp = sectab[gl->gr_sym->sy_rel&0xff];
			}
			continue;
		}
		allocseg( secp, lsecp );
		lsecp = secp;
	}
	if( split ){
		codsep->se_cum = ctop;
		datsep->se_cum = dtop;
	}
}

void allocseg(section_t* secp, section_t* lsecp) {

	int	i;
	long	base;		/* general location counter	*/
	long	cbase;		/* code loc counter if split	*/
	long	xbase;
	long	top;		/* top of section		*/
	long	l;

tiptop:

	secp->se_atr |= USEALLO;		/* mark as allocated		*/
	if( secp->se_atr & USEFIX ){
		if( split && !(secp->se_atr & USENOX) )
			ctop = secp->se_val;
		else
			dtop = secp->se_val;
	}
	if( !afmt && !split && !rflag && dtop < sectab[URBABS]->se_val )
		sectab[URBABS]->se_val = dtop;

	xbase = base = dtop;
	cbase = ctop;
	if( split && !(secp->se_atr & USENOX) ) xbase = base = cbase;

	/* Take care of alignment and extent constraints.  */

	DEBOUT(0,("%d Section('%s') base is %lx\n",
		secp->se_sym->sy_rel & 0xff, secp->se_sym->sy_str, base));

	i = secp->se_ext;
	if( i < 31 && (secfsize(secp) & 0xffffffL) > (1L << i))
		error( "43 Section %s too big for extent",
			secp->se_sym->sy_str );
	i = secp->se_aln;
#ifdef OLDCODE
	if( i < 1 ) i = 1;		/* RMM - force minaln to 2**1 */
#endif
	l = 0;
	if( i < 32 ) l = -1L << i;
	base = (base - l - 1) & l;	/* alignment constraint */
	top = base + secp->se_cum;
	if( !strcmp(proctype,"z8000") ){
		if( (top ^ base) & 0xffff0000 ||
		    lsecp && nomix &&
		    (lsecp->se_atr ^ secp->se_atr) & (USENOX|USENOR|USENOW)){
			base = (base + 0xffffL) & 0xffff0000L;
			top = base + secp->se_cum;
		}
	}
	i = secp->se_ext;
	l = 0;
	if( i < 32 ) l = -1L << i;
	if( (base & l) != ((top - 1L) & l) ){ /* adjust for extent */
		if( secp->se_aln > i ) i = secp->se_aln;
		l = 0;
		if( i < 32 ) l = -1L << i;
		base = base - l - 1 & l;
		top = base + secp->se_cum;
	}
	if( split && !(secp->se_atr & USENOX) ) ctop = top; else dtop = top;
	DEBOUT(0,("ctop is %lx, dtop is %lx\n",ctop,dtop));
	if( secp->se_val == xbase && xbase != base )
error("47 Location for sect %s does not meet alignment/extent constraints",
			secp->se_sym->sy_str);
	secp->se_val = base;
	if( afmt && base > xbase && lsecp ){
		if ( binfmt || base < xbase + 16 ) {

			/* here we have aligned the section and therefore
			may have left a gap between this section and
			the previous section.  If this in binary format
			make these file contiguous. 
			If afmt it is convenient if these can be made file
			contiguous if possible - We do this if the gap is
			no bigger than 15 bytes.
			*/
			lsecp->se_cum += base-xbase;
		}
	}
	if( rflag && !afmt )	secp->se_val = 0; /* start at zero */
		else		secp->se_atr |= USEFIX;

	DEBOUT(0,("\tInterlude SEC base %lx cum %lx\n",base,secp->se_cum));
	if( secp->se_atr & USEXTD1 ){
		lsecp = secp;
		secp = sectab[secp->se_xtd & 0xff];
		goto tiptop;
	}
	if( !afmt && !split && !rflag && dtop > sectab[URBABS]->se_mod )
		sectab[URBABS]->se_mod = dtop;
}


void ovrlapch() {

	section_t	*secp;
	section_t	*xsep;
	int		i;
	int		j;
	long		base;
	long		top;
	long		xbase;
	long		xtop;

	if( rflag || OVLYFILE ) return;
	for( i = URBSEC; i < stct; i++ ){
		secp = sectab[i];
		top = secp->se_cum;
		if( top == 0 ) continue;		/* empty section */
		base = secp->se_val;
		top += base;
		for( j = URBSEC; j < i; j++ ){
			xsep = sectab[j];

			/* if we are linking for a split id machine, then
			   we don't need to check if the executability
			   attributes are not the same */

			if( split && ((xsep->se_atr^secp->se_atr)&USENOX) )
				continue;

			/* if a section is empty it cannot overlap */

			xtop = xsep->se_cum;
			if( xtop == 0L ) continue; /* empty section */

			xbase = xsep->se_val;
			xtop += xbase;
			if( xbase <= base && base < xtop ||
			    base <= xbase && xbase < top ){
				error( "W42 Sections %s and %s overlap",
					secp->se_sym->sy_str,
					xsep->se_sym->sy_str );
				ovct++;
			}
		}
	}
}

void asgnsym() {		/* assign addresses to symbols		*/

	section_t	*secp;
	sytab_t	*syp;
	uns		h;
	int		i;

	for( h = 0; h < 1 << HSHLOG; h++ ){
		for( syp = syhtab[h]; syp; syp = syp->sy_lnk ){
			i = syp->sy_rel & 0xff;
			if( i == URBABS || i == URBUND ) continue;
			secp = sectab[i];
			if( OVLYFILE == NULL && secp != NULL)
				syp->sy_val += secp->se_val;
		}
	}
}


void symtoobj() {		/* output symbols */

	sytab_t	*syp;
	uns		h;
	int		i;

	objblk.ob_type = UOBGLO;
	for( h = 0; h < 1 << HSHLOG; h++ ){
		for( syp = syhtab[h]; syp; syp = syp->sy_lnk ){
			if( syp->sy_typ != STGLO && syp->sy_typ != STUND )
				continue;
			if( objblk.ob_top+7+strlen(syp->sy_str) >=
			    objblk.ob_buf+OBJSIZ ){
				oflush();
				objblk.ob_type = UOBGLO;
			}
			oputl( syp->sy_val );
			i = URBABS;
			if( rflag ) i = syp->sy_rel & 0xff;
			if( OVLYFILE ){

				/* the overlay stuff is kludged in here
				   because there is not really a byte
				   for it anywhere else */

				oputb( syp->sy_ovl );
			} else {
				oputb( i );
			}
			oputs( syp->sy_str );
			if( i == 0xff || i == URBABS)
				syp->sy_ord = nxtord++;/* set ordinal */
		}
	}
	oflush();
}


void symtoaout(section_t* csep) {

	/* output common symbols to a.out */

	sytab_t	*syp;
	uns		h;
	char	csrel;

	csrel = csep->se_sym->sy_rel;
	for( h = 0; h < 1 << HSHLOG; h++ ){
		for( syp = syhtab[h]; syp; syp = syp->sy_lnk ){
			if( syp->sy_typ != STGLO ) continue;
			if( syp->sy_rel != csrel ) continue;
			outsym( csrel, syp->sy_val, syp->sy_str );
		}
	}
}
