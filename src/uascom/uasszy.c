/************************************************************************
*									*
*	Copyright (C) 1987, by Unidot, Inc.				*
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
*			uasszym.c - szymanski module			*
*									*
************************************************************************/

static char rcsid[] =
"@(#)$Header: uasszy.c,v 6.10 88/10/09 16:46:41 rmm Rel $ uas szymanski module";


#include "uas.h"
#include "../incl/uobj.h"
#include "funcdefs.h"		/* Forward defines for GCC */

/* Declarations (local) */

int szbump(ushort rel, long val);
int szecheck(SZE* szep);
void szesyinc();
void szyient(ushort inc, ushort rel, long val);


#define SZEDEF 0x8000

/* description of sze_flg contents

	+-+-----+-------+---------------+
	|D| VX  | VLTB  |    SECTNO     |
	+-+-----+-------+---------------+
	 |  |       |         |
	 |  |       |         +-- section containing varlen instr
	 |  |       +-- varlen table to use (see below)
	 |  +-- instruction index chosen
	 +-- set to 1 when proper entry is determined
*/

/* note: in order to choose a particular entry, the target section must
   be the same as the vli section and the target lc must be such that
   lc >= vlilc-vl_neg && lc <= vlilc+vl_pos.  The last entry will have
   vl_neg == vl_pos == 0 as a special case.
*/

static short	szenxt;
static VLSIZ	**szevlpt;

/* Now, to avoid going through the symbol table too often we collect
   the increments in the following table and when it is full we process
   the symbol table
*/

#define SZISIZ	32
static struct szyincrs {
	ushort	szincval;
	ushort	szincsect;
	long	szincloc;
}
		szyitab[SZISIZ],
		szyitmp;
static short	szyix;
/*		Szymanski routines			*/

/* Discussion:  For microprocessors that have span dependent instructions
   we collect a table in pass1 that contains the address of the sdi and
   the target.  In order to keep it manageable, we restrict such targets
   to be simple labels.  These are kept in blocks for efficiency.

   Each entry is made by means of a call of the form:

   szyentry( sdirel, sdilc, tgt, vltabx, vldefined );

   where 
	sdirel == section number of sdi
	sdilc == location counter (pass1) of sdi
	tgt == symbol table virtual pointer for target
	vltabx == index of variable length selection table if vldefined == 0
	vltabx == index of instruction selection if vldefined == 1;

   At interlude time, if there have been any entries made, the
   symbol table is adjusted by means of a call to the routine

   szyprocess();

   During pass2, upon the appearance of each variable length item, we
   call the routine

   n = szynext();

   which returns the appropriate choice of instruction selection for
   actual output.

   NOTE: In order to avoid the requirement of having to include a
   vltab pointer set with all assemblers, we require those that will
   use vdi to initialize a pointer with the call

   szyinit( vltp ) VLSIZ **vltp;

*/

void szyinit( vltp ) VLSIZ **vltp; {
	szevlpt = vltp;
	BDEB(1,("szyinit: %o %o %o %o %o\n",
		vltp,vltp[0],vltp[1],vltp[2],vltp[3]));
}

void szyentry( sdirel, sdilc, tgt, vltabx, vldefined )
	short	sdirel;
	long sdilc;
	VMADR	tgt;
	short	vltabx;
	short	vldefined;
{

	/* this routine makes a new entry in the chain of vli's -
	   the value of the location counter on entry is in bits.
	   It is stored in adu's to simplify calculations later */

	reg SZY		*szyp;
	reg SZE		*szep;

	BDEB(1,("szentry( %d, %ld, %ld, %d, %d )\n",sdirel,
		(long)sdilc,(long)tgt,vltabx,vldefined));
	if( szyhead == 0 ) szyhead = szycur = (SZY *)palloc( BUFSIZ );
	szyp = szycur;
	if( szyp->szy_cnt >= SZENO ){
		szyp->szy_lnk = szycur = (SZY *)palloc( BUFSIZ );
		szyp = szycur;
	}
	szep = &szyp->szy_sze[szyp->szy_cnt];
	szyp->szy_cnt++;
	if( vldefined )
		szep->sze_flg = (sdirel & 0xff) | (vltabx<<12) | SZEDEF;
	else
		szep->sze_flg = (sdirel & 0xff) | (vltabx<<8);
	szep->sze_lc = sdilc/curadu;
	szep->sze_tgt = tgt;
}

uns szynext(){
#ifdef BIGDEBUG
	reg int	i;
	if(!debug) return xsznext();
	printf("szynext");
	i = xsznext();
	printf(" returns %d\n",i);
	return i;
}
xsznext(){
#endif

	reg SZY		*szyp;
	reg SZE		*szep;

	if( szycur == 0 ){
		if( szyhead == 0 ) fatal("77 Sznext with no entries");
		szycur = szyhead;
		szenxt = 0;
	}
	szyp = szycur;
	if( szenxt >= szyp->szy_cnt ){
		szycur = szyp->szy_lnk;
		if( szycur == 0 ) fatal("76 Sznext ran off end");
		szenxt = 0;
		szyp = szycur;
	}
	szep = &szyp->szy_sze[szenxt++];
	BDEB(1,(" flg %x lc %d tgt %d\n",szep->sze_flg,
		(int)szep->sze_lc, (int)szep->sze_tgt));
	return (szep->sze_flg >> 12) & 0x7;
}

void szyprocess(){

	/* this is the main processing routine for computing the
	   proper length for variable length (span dependent) instructions.
	   According to Szymanski, the proper ritual is to make each
	   instruction long, if and only if it is required, and to
	   iterate across the table until no instruction has been
	   turned into a longer one
	*/

	reg SZY		*szyp;
	reg SZE		*szep;
	reg int		i;
	reg int		j;
	short		inc;
	short		again;
	short		thissec;
#define SZX	6
	short		cx;
	short		sec[SZX+1];	/* section			*/
	long		cum[SZX];	/* cumulative adjustment	*/

top:	again = 0;
	cx = 0;
	BDEB(0,("szprocess\n"));
	for( szycur = szyhead; szycur; szycur = szyp->szy_lnk ){
	BDEB(0,("	szprocess szycur = %x\n",szycur));
		szyp = szycur;
		for( i=0; i<szyp->szy_cnt; i++ ){
			szep = &szyp->szy_sze[i];
			sec[cx] = thissec = szep->sze_flg & 0xff;	
			for( j=0; sec[j] != thissec; j++ );
			if( j == cx ){
				if( j >= SZX ) fatal("szproc");
				cx++;
				cum[j] = NULLCA;
			}
			szep->sze_lc += cum[j];

			/* remember - we are maintaining the value
			   of the location counter here in adu's
			*/

			if( !(szep->sze_flg & SZEDEF) &&
			    (inc = szecheck(szep))){
				again = 1;
				cum[j] += inc;
			}
		}
	}
	if( szyix ) szesyinc();
	BDEB(0,("szprocess end: again = %d\n",again));
	if( again ) goto top;
	szycur = 0;
}

int szecheck(SZE* szep) {
#ifdef BIGDEBUG

	reg int	i;
	i = xszecheck( szep );
	if( debug )printf("szecheck( %x, %ld, %ld ) returns %d\n",
			szep->sze_flg, (long)szep->sze_lc,
			(long)szep->sze_tgt, i );
	return i;
}
xszecheck( szep ) reg SZE *szep; {
#endif

	/* determine proper response to an sze entry, return 0 if
	   length did not change, return increment amount if it did */

	reg int		i;
	reg SYTAB	*syp;
	reg VLSIZ	*vlp;
	reg int		j;
	reg int		sect;
	reg short	oldinc;
	reg long	l;

	vlp = szevlpt[((szep->sze_flg >> 8) & 0xf)];
	syp = (SYTAB *)rfetch( szep->sze_tgt );
	sect = szep->sze_flg & 0xff;		/* section of sdi */
	l = (long)syp->sy_val;
	if( sect != syp->sy_rel ) l = 0x7fffffffL;
	for( i=0; vlp->vl_neg || vlp->vl_pos; i++, vlp++ ){
		if( l >= (szep->sze_lc-vlp->vl_neg) &&
		    l <= (szep->sze_lc+vlp->vl_pos) )
			break;
	}
	if( (j = ((szep->sze_flg >> 12) & 7)) >= i ) return 0;
	oldinc = szevlpt[j]->vl_inc;
	szep->sze_flg &= 0xfff;
	szep->sze_flg |= (i<<12);
	i = vlp->vl_inc - oldinc;
	if( i <= 0 ) return 0;
	if( vlp->vl_neg==0 && vlp->vl_pos==0 ) szep->sze_flg |= SZEDEF;
	if( szyix >= SZISIZ ) szesyinc();
	szyient( i, sect, (long)szep->sze_lc );
	sectab[sect].se_loc += i * sectab[sect].se_adu;
	return i;
}

void szyient( ushort inc, ushort rel, long val ) {

	reg struct szyincrs *szp;

	/* this maintains the table in sorted order */
	for( szp = &szyitab[szyix-1]; szp >= szyitab; szp-- ){
		if( szp->szincsect > rel ||
		    szp->szincsect == rel && szp->szincloc > val ){
			szp[1] = szp[0];
			continue;
		}
		break;
	}
	/* here we point just below the proper place */
	szp++;
	szp->szincval = inc;
	szp->szincsect = rel;
	szp->szincloc = val;
	szyix++;
/*DEB*/	for( szp = szyitab; szp < &szyitab[szyix-1]; szp++ ){
		if( szp[0].szincsect > szp[1].szincsect ||
		    szp[0].szincsect == szp[1].szincsect &&
		    szp[0].szincloc > szp[1].szincloc ){
			fatal("szyitab out of sort");
		}
	}
}


int szbump(ushort rel, long val) {
#ifdef BIGDEBUG
	int n;
	n = xszbump(rel,val);
/*DEB*/if(debug>0)printf("szbump( %d, %lx ) returns %d\n",rel,val,n);
	return n;
}
xszbump(rel,val) long val; {
#endif
	reg struct szyincrs *szp;
	reg struct szyincrs *szp2;
	long	val2;

	szp2 = &szyitab[szyix];
	if( szp2[-1].szincsect < rel ) return 0;
	for(szp = szyitab; szp < szp2 && szp->szincsect != rel; szp++);
	val2 = val;
	while( szp < szp2 && szp->szincsect == rel && val > szp->szincloc ){
		val += szp->szincval;
		szp++;
	}
	return (int)(val - val2);
}

void szesyinc(){

	/* hard work - got through the symbol table, adjusting all
	   entries with addresses greater than val (which is in adu's) */

	reg SYTAB	*syp;
	reg VMADR	p;
	reg int		h;
	reg int		n;
	reg VMADR	p2;
	reg NUMCHN	*nmc;

	BDEB(5,("szesyinc() %d entries\n",szyix));

	/* convert the increments into absolute deltas */
	for( h = 0; h < (1 << SHSHLOG); h++ ){
	BDEB(0,("szesyinc() h = %d\n",h));
		for( p = syhtab[h]; p; p = p2 ){
			syp = (SYTAB *) rfetch( p );
			p2 = syp->sy_lnk;
			if( syp->sy_typ == STKEY ||
			    syp->sy_typ == STKEQ ||
			    syp->sy_typ == STUND ||
			    syp->sy_rel == 0 ||
			    syp->sy_rel >= URBUND ||
			    syp->sy_typ == STSEC )
				continue;
			n = szbump( syp->sy_rel, (long)syp->sy_val );
			if( n == 0 ) continue;
			syp = (SYTAB *) wfetch( p );
BDEB(1,("symbol %s value increase from %ld to %ld\n",syp->sy_str,
	syp->sy_val, syp->sy_val+n));
			syp->sy_val += n;
		}
	}
	/* now the numeric labels	*/
	for( nmc = nchd; nmc; nmc = nmc->nc_lnk ){
		for( h=0; h<NMCCNT; h++ ){
			for( p = nmc->nc_nm[h]; p; p = p2 ){
				syp = (SYTAB *)rfetch(p);
				p2 = syp->sy_lnk;
				n = szbump( syp->sy_rel, (long)syp->sy_val );
				if( n == 0 ) continue;
				syp = (SYTAB *) wfetch( p );
				syp->sy_val += n;
			}
		}
	}
	szyix = 0;
}
