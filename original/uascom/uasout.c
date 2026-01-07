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
*			uasout.c - emitters and file writers		*
*									*
************************************************************************/


static char rcsid[]=
"@(#)$Header: uasout.c,v 6.15 88/07/22 10:49:52 rmm Rel $ uas output routines";

#include "uas.h"
#ifdef vms
#include "[-.incl]uobj.h"
#else
#include "../incl/uobj.h"
#endif

/*		emitb - Emits a byte of object code.		*/



emitb( value, reloc ) uns value, reloc;{

	reg int	i;

	/* we start by saving the values in the pending table just for
	   being systematic */

	if( pass2 )
		emitv( (long)value, reloc, bytbit );
	else
		curloc += bytbit;
}

nopend(){			/* clear the pending stuff		*/

	/* routine must be called only during pass2 */

	reg int		j;
	reg int		ca;
	reg int		objf;

	if( pendbits == 0 ) return;
	if( !pass2 ){
		curloc += pendbits;
		pendbits = 0;
		return;
	}
	ca = curadu;
	if( ca < 8 ) ca = 8;
	j = 0;
	objf = (ca + 3)/4;		/* nibble count		*/
	while( pendbits ){
		j = pendbits;
		if( pendbits > ca ) j = ca;
		pendbits -= j;
		if( msbord ){
			emitau( pendv >> (32-ca), pendrel, j );
			pendv <<= j;
		} else {
			emitau( pendv, pendrel, j );
			pendv >>= j;
			pendv &= (1L << pendbits) - 1;
		}
		pendrel = 0;
	}

	/* this routine is called to flush out the buffer for
	   alignment purposes.  We have already put into the list
	   buffer the pending stuff which was listed with the
	   previous line.  So we can simply discard what we did
	   and start again
	*/

	lllocval = (curloc/curadu) & lllocmask;
	lllocsiz = lllocspec;
	llobt = llobj;
	*llobt = 0;
}

/* the following routine actually puts out the code or data	*/

emitau( value, reloc, bits ) long value; uns reloc; int bits;{

	/* routine is now called only during pass2	*/

	/* this routine actually puts out data into the object file.
	   The number of bits supplied may be less than one current
	   address unit, but will never be more if curadu is 8 or more.
	   It may be more in the case of curadu < 8.  Note that the count
	   field is now in multiples of the current address unit.  Only
	   the last byte in an addru may be less than full.  That is, an
	   18 bit addru will result in three bytes in the record.  For bit
	   and nibble addressed machines we put out at least one byte.
	   If there is a residue, we force the next record to start over
	   so that we don't have to pack bits.
	*/

	reg int		i;
	reg long	l;

	/* Output to the object file without breaking up a relocatable item.  */

	BDEB(1,("emitau(0x%lx,0x%x,%d)\n",value,reloc,bits));
	if( bits < 32 ) value &= (1L << bits) - 1;
	if( !(curatr & SEATDUMY) ){
		i = (curadu+7) >> 3;
		if( i < 4 ) i = 4;
		if( reloc ) i += 8;			/* paranoia	*/
		if( relbot-objtop < i ) nxtloc = -1;	/* equiv to oneed */
		setorg();
		nxtloc = curloc + (bits < curadu ? curadu : bits);
		i = objbuf[5] & 0xff;
		if( curadu < 8 ) i = ((i & 0x7f) << 8) | (objbuf[6] & 0xff);
		if( reloc ){

			/* note: the mystery about the +6 in the following
			   code is that in the original assembler, the
			   relocation action item pointed to a byte in
			   the text block and there was a 6 byte header.
			   Now the count refers to address units in
			   the text.  Hence, if we bias this by six it
			   comes out the same in the common byte case */

			/* NOTE: the following order is BACKWARDS from
			   the way it will really be put out.
			   RMM 21 Dec 87			*/

			if( curadu < 8 && i > 121 )
				*--relbot = ((i+6) >> 8) | 0x80;
			*--relbot = i+6;
			*--relbot = reloc;
			*--relbot = reloc >> 8;
		}
		if( curadu < 8 ){
			if( bits & 7 ) nxtloc = -1L;
			i += (bits + curadu - 1) / curadu;
			objbuf[5] = (i >> 8) | 0x80;
			objbuf[6] = i;
		} else {
			objbuf[5]++;
		}
		l = value;
		i = bits;
		if( i < curadu ) i = curadu;
		while( i > 0 ){
			if( objtop >= relbot ) oputb(0);
			*objtop++ = l;
			l >>= 8;
			i -= 8;
		}
	}
	/* now put in the listing field */
	i = curadu;
	if( i < 8 ) i = 8;
	i = (i + 3)/4;			/* nibble count		*/
	listau( i, value, bits );
	llobt += i;
	if( aduspace && ++aduspc2 >= aduspace ) llobt++, aduspc2 = 0;
	curloc += bits < curadu ? curadu : bits;
}

/*
 * emitstr - Emits a string
 */


emitstr( s, n ) reg char *s; {

	reg int	i;
	int	m;
	int	msk;

	if( !pass2 ){
		curloc += n * bytbit;
		return;
	}
	msk = (1 << bytbit) - 1;	/* byte mask			*/
	while( --n >= 0 ) emitv( (long)(*s++ & msk), 0, bytbit );
}

emitv( value, reloc, bits ) long value; uns reloc; int bits; {

	reg int	i;
	long	mask;
	long	v;
	int	ca;
	int	mca;
	int	objf;

	/* Check whether relocation is needed.  */

	if( (reloc&URAMSK) == URANOP ||
	    (i = (reloc&URBMSK)) == URBABS ||
	    i < URBUND && sectab[i].se_atr & (USEFIX|SEATDUMY) )
		reloc = 0;

BDEB(1,("emitv(0x%lx,0x%x,%d)pv:%lx,pb:%d\n",value,reloc,bits,pendv,pendbits));
	ca = curadu;
	if( ca < 8 ) ca = 8;
	mca = 32 - ca;
	objf = (ca + 3)/4;		/* nibble count		*/
	mask = ~0L;
	if( bits < 32 ) mask = (1L << bits) - 1;
	value &= mask;
	if( reloc ){
		if( pendbits || pendrel )
			error("82 Relocation error");
		pendrel = reloc;
	}
	if( msbord ){		/* stash high order units first */
		
		value <<= (32 - bits);		/* left align in long	*/
		if( pendbits ){
			if( value < 0 )	v = ~(~value >> pendbits);
				else	v = value >> pendbits;
			pendv |= v;
		} else
			pendv = value;
		pendbits += bits;
		v = 0;
		if( pendbits > 32 ) v = value << (64 - pendbits);
		while( pendbits >= ca ){
			emitau( pendv >> mca, pendrel, ca );
			pendrel = 0;
			pendv <<= ca;
			pendv |= v < 0 ? ~(~v >> mca) : v >> mca;
			v <<= ca;
			pendbits -= ca;
		}
		if( pendbits ) listau( objf, pendv >> mca, ca );
	} else {		/* stash low order units first	*/
		pendv |= value << pendbits;
		pendbits += bits;
		if( pendbits < 32 ) pendv &= (1L << pendbits) - 1;
		v = 0;
		if( pendbits > 32 ){
			value >>= bits - pendbits + 32;
			bits = pendbits - 32;
			v = value & (1L << bits) - 1;
		}
		while( pendbits >= ca ){
			emitau( pendv, pendrel, ca );
			pendrel = 0;
			if( ca < 32 ){
				pendv >>= ca;
				if( pendv < 0 )	pendv &= (1L << mca) - 1;
				pendv |= v << mca;
				v >>= ca;
			} else {
				pendv = v;
				v = 0;
			}
			pendbits -= ca;
		}
		if( pendbits ) listau( objf, pendv, ca );
	}
}

listau( objf, v, bits ) long v;{

	/* this routine puts some text into the listing - If it does
	   not fit, then we flush the line and start a new one
	*/

	if( llobt + objf > llobnd ){
		putline();
		lllocval = (curloc/curadu) & lllocmask;
		lllocsiz = lllocspec;
		llobt = llobj;
	}
	if( llobt != llobj && llobt[-1] == 0 ) llobt[-1] = ' ';
	if( bits < 32 ) v &= (1L << bits) - 1;
	if( msblst ){
		int	i;
		for( i=llobt-llobj; i >= 0; i-- ) llobj[i+objf] = llobj[i];
		while( --objf >= 0 ){
			llobj[objf] = hextab[ v & 0xf ];
			v >>= 4;
		}
	} else {
		llobt[objf] = 0;
		while( --objf >= 0 ){
			llobt[objf] = hextab[ v & 0xf ];
			v >>= 4;
		}
	}
}

/*
 * emitl - Emits a long to the object file.
 */

emitl( value, reloc ) long value; uns reloc;{

	reg	i;
	reg	j;

	if( pass2 ) emitv( value, reloc, lngbit );
		else curloc += lngbit;
}

/*
 * emitw - Emits a word to the object file.
 */

emitw( value, reloc ) uns value, reloc;{

	if( pass2 ) emitv( (long)value, reloc, wrdbit );
		else curloc += wrdbit;
}

/*
 * oflush - Outputs an object block to the object file.
 */

oflush(){

	reg char	*op;

	if( objtyp && OBJECT ){
		fputc( objtyp, OBJECT );
		fputc( OBJSIZ-( relbot-objtop ), OBJECT );
		for( op = objbuf; op < objtop; op++ )
			fputc(*op, OBJECT );
		/* the following output the relocation information
		   backward to compensate for the way it was put in
		   in emitau (RMM 21 Dec 87) */

		for( op = &objbuf[OBJSIZ-1]; op >= relbot; op-- )
			fputc(*op, OBJECT );
		filchk( OBJECT, "object" );
	}
	objtop = objbuf;
	relbot = &objbuf[OBJSIZ];
	objtyp = 0;
}

/*
 * oneed - guarantees n bytes available in buffer
 */

oneed( n ){

	if( relbot-objtop < n ) oflush();
}

/*
 * oputb - Puts a byte into the object buffer.
 */


oputb( c ) char c;{


	if( objtop >= relbot ) fatal( "70 Object buffer overflow" );
	*objtop++ = c;
}

/*
 * oputr - Puts a byte into the relocation area.
 */

oputrb( c ) char c; {

	if( objtop >= relbot ) fatal( "70 Object buffer overflow" );
	*--relbot = c;
}

/*
 * oputl - Puts a long word into the object buffer.
 */

oputl( l ) long l;{

	reg uns	i;

	i = l;		oputb( i );
	i >>= 8;	oputb( i );
	i = l >> 16;	oputb( i );
	i >>= 8;	oputb( i );
}

/*
 * oputs - Puts a symbol into the object buffer.
 */

oputs( s ) reg char *s;{


	reg int	i;

	i = SYMSIZ;
	do {
		if( *s == '\0' ) break;
		if( objtop >= relbot ) oputb(0);
		*objtop++ = *s++;
	} while( --i > 0 );
	oputb( '\0' );
}

/*
 * oputw - Puts a word into the object buffer.
 */

oputw( w ) uns w;{

	oputb( w );
	oputb( w >> 8 );
}
