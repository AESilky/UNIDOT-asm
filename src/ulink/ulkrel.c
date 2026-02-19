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
*			ulkrel.c - relocation activities		*
*									*
************************************************************************/


static char rcsid[] =
"@(#)$Header: ulkrel.c,v 4.20 92/04/26 14:41:10 rmm Rel $ relocation activities";

#include "ulink.h"
#ifdef vms
#include "[-.incl]uobj.h"
#include "[-.incl]urel.h"
#include "[-.incl]ulktab.h"
#else
#include "../incl/uobj.h"
#include "../incl/urel.h"
#include "../incl/ulktab.h"
#endif
#include "funcdefs.h"

#include <string.h>

static char	*relact[32];
static char	*relhsh[32];
static char	relalen[32];
static char	relhlen[32];
static char	relint[256];
static char	*relp = relint;
static char	*relmap[20];		/* for pass two mapping	*/
static short	relmx;			/* how many have we got	*/

/*	interpretive stack follows		*/

#define ISTKSIZ	8		/* should not need this many entries	*/
#define IACC long		/* if we ever have to deal with linking */
				/* for machines in which addru > long   */
				/* then this has to be reworked		*/

static IACC	istk[ISTKSIZ];
static IACC	itmp;
static IACC	itmp0;
static IACC	itmp1;

static long	itmaddr;	/* address of item to be relocated	*/
static char	*txtstt;	/* point to start of text		*/
static char	*txtlim;	/* point to end of text			*/
static uns	itmoff;		/* offset from txtstt in curadu's	*/
static uns	reloc;		/* relocation item			*/

/* Declarations (Local) */

void aldl(IACC* isp, int n);
void aldm(IACC* isp, int n);
void astl(IACC* isp, int n);
void astm(IACC* isp, int n);
void bitovr();
long getbau(int off);
void interp(char* ip);
int newrel();
void obrlt();
void putbau(long v, int off);
void relovr();
void rltxtovr();
void rngerr(long v, long lo, long hi);


/* new construction for relocation: an interpretive table is
   used.  There is a standard interpretive table, that is instantiated
   between every source module so that old stuff will work.  This
   is done by calling relinit().  In addition, the linker now maintains
   a global: curadu giving the current address unit in bits.

   In the new schema, all locations counters are kept in bits.  The
   conversion to final is done at the very tail end of everything.
*/

/* getrelitem - Fetches and sets up the next relocation item in
 * the record to use by the interpreter - if called it is fatal if
 * there is no item present
 */

void getrelitem() {

	itmoff = *objblk.ob_ptr++ & 0xff;
	if( (curadu < 8) && (itmoff & 0x80) )
	    itmoff = ((itmoff & 0x7f) << 8) | (*objblk.ob_ptr++ & 0xff);
	itmoff -= 6;
	itmaddr = curoff + itmoff + rbase( cursec ); /* data address */
	reloc = *objblk.ob_ptr++ & 0xff;
	reloc |=  (*objblk.ob_ptr++ & 0xff) << 8;
	if( objblk.ob_ptr > objblk.ob_top ) rltxtovr();
}

/* rbase - Given a relocation descriptor, returns the relocation base
 * address.  During pass 1, the base returned assumes that the containing
 * section starts at 0.  During pass 2, the actual starting address is
 * used.								*/

long
rbase( reloc ) uns reloc;{


	section_t	*sep;
	long	base;

	reloc &= URBMSK;
	if( reloc == URBABS ) return 0L;			/* absolute */
	if( URBSEC <= reloc && reloc < svct ){			/* section */
		sep = sectab[secvec[reloc]&0xff];
		if( sep->se_atr & USEABS ) return 0L;
		base = pass2 || OVLYFILE ? sep->se_val: 0L;
		if( !(sep->se_atr & USECOM) ) base = base + sep->se_cum;
		return base;
	}
	if( URBEXT <= reloc && reloc < URBEXT+evct )		/* external */
		return extvec[reloc-URBEXT]->sy_val;		/* JDP 06/86 */
	DEB(0,(" Error rbase %x URBSEC %x URBEXT %x svct %x evct %x\n",
		reloc, URBSEC, URBEXT, svct, evct));
	error( "F28 Rbase relocation error, relocation key = %d",reloc);
	return 0;
}

/* sbase - Given a relocation descriptor, returns the base address of the
 * containing section.  If a member of a group, it returns the base address
 * of the group.  This is used only in pass2, and only for the 8086. */

long
sbase( reloc ) uns reloc;{

	section_t	*sep;

	reloc &= URBMSK;
	if( reloc == URBABS || rflag ) return 0L;		/* absolute */
	if( URBSEC <= reloc && reloc < svct )			/* section */
		reloc = secvec[reloc];
	else if( URBEXT <= reloc && reloc < URBEXT+evct )	/* external */
		reloc = extvec[ reloc-URBEXT ]->sy_rel;
	else
		error( "F23 Sbase relocation error, relocation key = %d",reloc);
	sep = sectab[reloc & 0xff];
	if( sep->se_grp ){
		reloc = grptab[sep->se_grp-1]->gr_sym->sy_rel;
		sep = sectab[reloc & 0xff];
	}
	return sep->se_val;
}

/*
 * routine to change old reloc value to new reloc value - use in the
 * -r option to keep the relocation information
 */

int newrel() {

	uns		trel;
	char	*p;
	int		iact;
	int		len;
	sytab_t	*syp;

	iact = reloc;
	trel = iact & URBMSK;
	if( URBSEC <= trel && trel < svct )
	 	trel = sectab[secvec[trel]&0xfff]->se_sym->sy_rel & 0xff;
	else
	if( URBEXT <= trel && trel < URBEXT+evct ){	/* external	*/
		syp = extvec[trel-URBEXT];
		trel = syp->sy_rel;
		if( syp->sy_typ == STUND ){
			trel = syp->sy_ord;
			if( trel == 0 ) error("199 no ordinal assigned to %s",
					syp->sy_str);
		}
	}
	iact >>= UR_SHF;
	iact &= 31;
	if( !afmt ){
		p = relact[iact];
		len = relalen[iact];
		for( iact=1; iact<=relmx; iact++ )
			if( p == relmap[iact] ) break;
		if( iact > relmx ){
			if( relmx >= 20 ) relovr();
			relmap[++relmx] = p;
			fputc( UOBRLT, OBJOUT );
			fputc( len + 1, OBJOUT );
			fputc( iact, OBJOUT );
			while( --len >= 0 ) fputc( *p++, OBJOUT );
		}
	}
	return (iact << UR_SHF) | trel;
}
/*
 * selook - Returns a pointer to the section table entry for the
 * specified section.  Creates a new entry in the section table if
 * necessary.
 */

section_t *
selook( s ) char *s;{


	section_t	*sep;
	sytab_t	*syp;

	syp = sylook( s );
	if( syp->sy_typ == STUND ){ /* create new section table entry */
		if( stct >= SECSIZ ) error( "F21 Too many sections" );
		syp->sy_typ = STSEC;
		syp->sy_val = 0L;
		if( strcmp(s, ".abs ") ){	/* not funny .abs section */
			syp->sy_rel = stct;
			sep = sectab[stct++] =
#ifdef STATS
				(section_t *)zpalloc(sizeof(section_t),SECUSE);
#else
				(section_t *)zpalloc(sizeof(section_t));
#endif
			sep->se_sym = syp;
			sep->se_ext = 32;
		}
	}
	if( syp->sy_typ != STSEC )
		error("F49 Sect %s in use as ordinary symbol",syp->sy_str);
	return sectab[syp->sy_rel&0xff];
}
/*
 * treloc - Performs all relocation possible on a text block.
 */

void treloc() {

	uns		nrel;
	char	*ip;

	objblk.ob_ptr = txtlim = txttop;
	txtstt = objblk.ob_buf + 6;
	if( curadu < 8 && (txtstt[-1] & 0x80) )
		txtstt++;			/* point to data start */
	while( objblk.ob_ptr < objblk.ob_top ){
		getrelitem();
DEB(1,(" relocation(off %d, reloc %x)\n",itmoff,reloc));
		ip = relact[(reloc >> UR_SHF) & 31];
		if( rflag ){
			nrel = newrel();
			if( nrel ){
				if( !afmt ){
					if( curadu < 8 && itmoff > 121 )
					    *txttop++ = ((itmoff+6)>>8) | 0x80;
					*txttop++ = itmoff+6;
					*txttop++ = nrel;
					*txttop++ = nrel >> 8;
				} else {
					putc( outsec, RELFILE );
					along( itmaddr, RELFILE );
					aword( nrel, RELFILE );
					relsize += 7;
				}
			}
		}
		if( ip ) interp( ip );
	}
	objblk.ob_top = txttop;
}

#ifdef DEBUG
xx(n,isp) reg IACC	*isp; {
	if( debug > 2 ){
		fprintf(stderr," [%x]",n&0xff);
		for(;isp<&istk[ISTKSIZ];isp++)
			fprintf(stderr,"\t%lx",*isp);
		fprintf(stderr,"\n");
	}
	return n;
}
#endif

void interp(char* ip) {

	reg IACC	*isp;
	int		i;
	long	v;

	isp = &istk[ISTKSIZ];			/* empty stack		*/
	itmp0 = itmp1 = 0;			/* init temporaries	*/
#ifdef DEBUG
	for(;;)switch( xx(i = *ip++,isp) ){
#else
	for(;;)switch( i = *ip++ ){
#endif

case A_HALT:		/* zero had always better be halt		*/
#ifdef DEBUG
		if( debug > 2 ) fprintf(stderr," ::\n");
#endif
		return;

case A_SWAP:		/* swap bottom items on stack			*/

		itmp = *isp; *isp = isp[1]; isp[1] = itmp; continue;

case A_DUP:		/* duplicate bottom item			*/

		--isp; *isp = isp[1]; continue;

case A_POP:		/* discard bottom item				*/

		isp++; continue;

case A_INCAP:		/* increment addru pointer by tos		*/

		itmoff += *isp++;
		continue;

case A_ADD:		/* tos1 + tos ==> tos				*/
		isp[1] += isp[0];
		isp++;
		continue;

case A_SUB:		/* tos1 - tos ==> tos				*/
		isp[1] -= isp[0];
		isp++;
		continue;

case A_AND:		/* tos1 & tos ==> tos				*/

		isp[1] &= isp[0];
		isp++;
		continue;

case A_OR:		/* tos1 | tos ==> tos				*/

		isp[1] |= isp[0];
		isp++;
		continue;

case A_XOR:		/* tos1 ^ tos ==> tos				*/

		isp[1] ^= isp[0];
		isp++;
		continue;

case A_SHR:		/* tos1 >> tos ==> tos				*/
		i = *isp++ & 31;
		if( *isp < 0 )	*isp = ~(~*isp >> i);
			else	*isp >>= i;
		continue;


case A_SHL:		/* tos1 << tos ==> tos				*/
		i = *isp++ & 31;
		*isp <<= i;
		continue;

case A_NEG:		/* -tos ==> tos					*/


		*isp = -*isp;
		continue;

case A_CMP:		/* ~tos ==> tos					*/

		*isp = ~*isp;
		continue;

case A_MUL:		/* tos1 * tos ==> tos				*/

		isp[1] *= isp[0];
		isp++;
		continue;

case A_DIV:		/* tos1 / tos ==> tos				*/

		if( isp[0] ) isp[1] /= isp[0];
		isp++;
		continue;

case A_MOD:		/* tos1 % tos ==> tos				*/

		if( isp[0] ) isp[1] %= isp[0];
		isp++;
		continue;

case A_RNG:		/* if tos2 >= tos1 && tos2 <=tos, ok, tos2 ==> tos */

rngtst:		if( isp[2] < isp[1] || isp[2] > isp[0] )
			rngerr(isp[2],isp[1],isp[0]);
		isp += 2;
		continue;

case A_SEXT:		/* if( tos1 & (1<<tos) ) sign extend, else mask	*/
		i = *isp++;
		if( i >= 31 || i < 0 ) continue;
		v = 1L << i;
		if( *isp & v )
			*isp |= ~--v;
		else
			*isp &= --v;
		continue;


case A_MERGE:		/* (tos2 & ~tos) | (tos1 & tos) ==> tos		*/

		isp[2] = (isp[2] & ~isp[0]) | (isp[1] & isp[0]);
		isp += 2;
		continue;

case A_LI1:		/* load an immediate byte			*/

		*--isp = *ip++ & 0xff;
		continue;

case A_LI2:		/* load an immediate word (lsb first)		*/

		*--isp = *ip++ & 0xff;
		*isp |= (long)(*ip++ & 0xff) << 8;
		continue;

case A_LI4:		/* load an immediate long (lsb first)		*/

		*--isp = *ip++ & 0xff;
		*isp |= (long)(*ip++ & 0xff) << 8;
		*isp |= (long)(*ip++ & 0xff) << 16;
		*isp |= (long)(*ip++ & 0xff) << 24;
		continue;

case A_LLC:		/* load address of addru loaded			*/

		*--isp = itmaddr;
		continue;

case A_LTU:		/* load address of target addru			*/

		*--isp = rbase( reloc );
		continue;

case A_LTG:		/* load address of target group			*/

		*--isp = sbase( reloc );
		continue;

case A_RNG8:		/* range must be in range -128 to 127		*/
		*--isp = -128L;
		*--isp = 127L;
		goto rngtst;

case A_RNG8U:		/* range must be in range 0 to 255		*/
		*--isp = 0L;
		*--isp = 255L;
		goto rngtst;

case A_RNG16:		/* range must be in range -32768 to 32767	*/
		*--isp = -32768L;
		*--isp = 32767L;
		goto rngtst;

case A_RNG16U:		/* range must be in range 0 to 65535		*/
		*--isp = 0L;
		*--isp = 65535L;
		goto rngtst;

case A_LRI:		/* load bottom eight bits of reloc item		*/
		*--isp = reloc & 0xff;
		continue;

case A_LRIL:		/* load next 32 bits (8bits here+24)		*/
		*--isp = reloc & 0xff;
		*isp |= (*objblk.ob_ptr++ & 0xff) << 8;
		*isp |= (long)(*objblk.ob_ptr++ & 0xff) << 16;
		*isp |= (long)(*objblk.ob_ptr++ & 0xff) << 24;
		continue;

case A_LT0:		/* load temporary 0				*/
		*--isp = itmp0;
		continue;

case A_LT1:		/* load temporary 1				*/
		*--isp = itmp1;
		continue;

case A_ST0:		/* store temporary 0				*/
		itmp0 = *isp++;
		continue;

case A_ST1:		/* store temporary 1				*/
		itmp1 = *isp++;
		continue;

case A_LNK:		/* link to next relocation item			*/
		getrelitem();
		ip = relact[(reloc >> UR_SHF) & 31];
		continue;

default:	switch( i & 0xe0 ){

	case A_LDK:	/* load bottom 5 bits (sign extended) into stack */
			v = i & 0x1f;
			if( v & 0x10 ) v |= ~0x1f;
			*--isp = v;
			continue;

	case A_LDL:	/* load items from least significant first	*/
			aldl( --isp, i & 0x1f );
			continue;

	case A_LDM:	/* load items from most significant first	*/
			aldm( --isp, i & 0x1f );
			continue;

	case A_STL:	/* store items from least significant first	*/
			astl( isp++, i & 0x1f );
			continue;

	case A_STM:	/* store items from most significant first	*/
			astm( isp++, i & 0x1f );
			continue;

	default:	error("F46 Internal reloc error: %x\n",ip[-1] & 0xff);
		}
	}
}

long getbau(int off) {

	/* fetch one addr unit (byte or bigger) starting at off */

	char	*p;
	int		n;
	long	v;

	n = (curadu + 7) >> 3;
	p = txtstt + off * n;
	v = 0;
	for( --n; n >= 0; --n ) v = (v << 8) | (p[n] & 0xff);
	if( curadu < 32 ) v &= (1L << curadu) - 1;
	return v;
}

void putbau( long v, int off ) {

	/* store one addr unit (byte or bigger) starting at off */

	char	*p;
	int		i;
	int		n;

	n = (curadu + 7) >> 3;
	p = txtstt + off * n;
	if( curadu < 32 ) v &= (1L << curadu) - 1;
	for( i=0; i<n; i++ ) *p++ = v, v >>= 8;
}

void aldl(IACC* isp, int n) {

	/* load n+1 curadu's from least significant first */
	long	v;
	int		i;
	int		bitlen;
	int		bitoff;
	char	*bytptr;

	v = 0;
	n++;
	if( curadu == 8 ){
		bytptr = txtstt + itmoff + n;
		if( bytptr > txtlim ) rltxtovr();
		for( i=0; i<n; i++ ) v = (v << 8) | (*--bytptr & 0xff);
	} else
	if( curadu < 8 ){
		bitlen = curadu * n;
		bitoff = itmoff * curadu;
		bytptr = txtstt + (bitoff >> 3);
		bitoff &= 7;
		if( bitoff + bitlen > 32 ) bitovr();
		v = bytptr[3] & 0xff;
		v = (v << 8) | (bytptr[2] & 0xff);
		v = (v << 8) | (bytptr[1] & 0xff);
		v = (v << 8) | (bytptr[0] & 0xff);
		v >>= bitoff;
		if( bitlen < 32 ) v &= (1L << bitlen) - 1;
	} else
		while( --n >= 0 ) v = (v << curadu) | getbau( itmoff+n );
	*isp = v;
}

void aldm(IACC* isp, int n) {

	/* load n+1 curadu's from most significant first */
	long	v;
	int		i;
	int		bitlen;
	int		bitoff;
	char	*bytptr;

	v = 0;
	n++;
	if( curadu == 8 ){
		bytptr = txtstt + itmoff;
		for( i=0; i<n; i++ ) v = (v << 8) | (*bytptr++ & 0xff);
		if( bytptr > txtlim ) rltxtovr();
	} else
	if( curadu < 8 ){
		bitlen = curadu * n;
		bitoff = itmoff * curadu;
		bytptr = txtstt + (bitoff >> 3);
		bitoff &= 7;
		if( bitoff + bitlen > 32 ) bitovr();
		v = bytptr[0] & 0xff;
		v = (v << 8) | (bytptr[1] & 0xff);
		v = (v << 8) | (bytptr[2] & 0xff);
		v = (v << 8) | (bytptr[3] & 0xff);
		v <<= bitoff;				/* left align	*/
		if( bitlen < 32 ){
			v >>= 32 - bitlen;		/* right align	*/
			v &= (1L << bitlen) - 1;	/* trim		*/
		}
	} else
		for( i=0; i<n; i++ ) v = (v << curadu) | getbau( itmoff+i );
	*isp = v;
}

void bitovr() {
	error("F22 Bit field overflow");
}

void rltxtovr() {
	error("F99 Rel Txt Error (Internal)");
}

void astl(IACC* isp, int n) {

	/* store n+1 curadu's from least significant first */

	long	v;
	char	*bytptr;
	int		i;
	int		bitlen;
	int		bitoff;

	v = *isp;
	n++;
	if( curadu == 8 ){
		bytptr = txtstt + itmoff;
		for( i=0; i<n; i++ ) *bytptr++ = v, v >>= curadu;
		return;
	}
	if( curadu < 8 ){
		bitlen = curadu * n;
		bitoff = itmoff * curadu;
		bytptr = txtstt + (bitoff >> 3);
		bitoff &= 7;
		if( bitoff + bitlen > 32 ) bitovr();
		while( bitoff + bitlen >= 8 ){
			if( bitoff ){
				i = (1 << bitoff) - 1;
				*bytptr &= i;
				*bytptr |= ((int)v << bitoff) & ~i;
			} else
				*bytptr = v;
			v >>= (8 - bitoff);
			bitlen -= (8 - bitoff);
			bitoff = 0;
			bytptr++;;
		}
		if( bitlen ){		/* residue	*/
			i = ((1 << bitlen) - 1) << bitoff;
			*bytptr &= ~i;
			*bytptr |= ((int)v << bitoff) & i;
		}
		return;
	}
	for( i=0; i<n; i++ ) putbau( v, itmoff+i ), v >>= curadu;
}

void astm(IACC* isp, int n) {

	/* store n+1 curadu's from most significant first */

	long	v;
	int		i;
	int		bitlen;
	int		bitoff;
	char	*bytptr;

	v = *isp++;
	n++;
	if( curadu == 8 ){
		bytptr = txtstt + itmoff;
		for( i=n-1; i>=0; i-- ) bytptr[i] = v, v >>= curadu;
		return;
	}
	if( curadu < 8 ){
		bitlen = curadu * n;
		bitoff = itmoff * curadu;
		bytptr = txtstt + (bitoff >> 3);
		bitoff &= 7;
		if( bitoff + bitlen > 32 ) bitovr();
		if( bitlen < 32 ) v <<= 32 - bitlen;
		while( bitoff + bitlen >= 8 ){
			n = v >> 24;		/* get left byte	*/
			if( bitoff ){
				i = ((1 << bitoff) - 1) << (8 - bitoff);
				*bytptr &= i;
				*bytptr |= (n >> bitoff) & ~i;
			} else
				*bytptr = v;
			v <<= (8 - bitoff);
			bitlen -= (8 - bitoff);
			bitoff = 0;
			bytptr++;;
		}
		if( bitlen ){		/* residue	*/
			n = v >> 24;		/* get left byte	*/
			i = ((1 << bitlen) - 1) << (8 - (bitoff+bitlen));
			*bytptr &= ~i;
			*bytptr |= (n >> bitoff) & i;
		}
		return;
	}
	for( i=n-1; i>=0; i-- ) putbau( v, itmoff+i ), v >>= curadu;
}


void rngerr(long v, long lo, long hi) {

	char	rgsym[34];

	strcpy( rgsym, "absolute section" );
	if( cursec ){
		strcpy( rgsym, "section '" );
		strcat( rgsym, sectab[secvec[cursec] & 0xff]->se_sym->sy_str );
		strcat( rgsym, "'" );
	}
	if( lo == hi )
		error("51 %s 0x%lx: relocation range error",
			rgsym,itmaddr);
	else
		error("44 %s 0x%lx: range test 0x%lx <= 0x%lx <= 0x%lx failed",
			rgsym,itmaddr,lo,v,hi);
}

void relinit() {

	/* routine called at the start of each object module to
	   reinstantiate the default table
	*/

	int	i;

	for( i=0; i<32; i++ )  relact[i] = uractions[i];
}

/*
 * obrlt - Builds a relocation action map
 */

void obrlt() {		/* table of local action - global action pairs */

	int		i;
	char	*p;
	int		j;
	int		k;
	int		len;

	i = *objblk.ob_ptr++ & 0xff;
	if( i < 0 || i >= 32 ) error("F38 Bad relocation tag");
	if( relmx >= 18 ) relovr();
	p = relp;
	j = 0;
	len = objblk.ob_top - objblk.ob_ptr;
	if( p + len >= &relint[256] ) relovr();
	while( objblk.ob_ptr < objblk.ob_top ){
		*p++ = k = *objblk.ob_ptr++;
		j = j*5 + k;
	}
	for( j &= 31; relhsh[j]; j = (j+1) & 31 )
		if( len == relhlen[j] && strcmp(relhsh[j],relp) == 0 ){
			relact[i] = relhsh[j];
			relalen[i] = len;
			return;
		}
	relhsh[j] = relact[i] = relp;
	relhlen[j] = relalen[i] = len;
	relp = p;
}

void relovr() {
	error("F35 Relocation action table overflow");
}
