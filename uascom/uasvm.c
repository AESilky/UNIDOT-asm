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
*			uas.vm.c - virtual memory handler		*
*									*
************************************************************************/

static char rcsid[] =
"@(#)$Header: uasvm.c,v 6.13 89/03/17 08:16:02 rmm Rel $ uas virt mem handler";



#include "uas.h"
#include "funcdefs.h"		/* Forward defines for GCC */

#ifdef USEVM
#include <fcntl.h> /* For 'open' */
#include <unistd.h> /* For 'close' */

#ifndef BIGMEM
#define VMNUMX	64			/* better be bigger than VMNUM	*/
static	short	vmbaln = 0;		/* buffers allocated		*/
static	short	vmnum = VMNUM;		/* value for pass1		*/
static	short	vmnumx = VMNUMX;	/* value for pass2		*/
static  ushort	vmalo[16] = {0};	/* blocks to free		*/
static	short	vmax = 0;		/* number of blocks to free	*/
static	short	vmtx = 0;		/* next vmt to allocate		*/
#ifdef msdos
#define DOSINT 0x21
#define F_CF            0x01            /* carry flag                   */
#define REG86	struct reg86
REG86 {
	unsigned r_ax;
	unsigned r_bx;
	unsigned r_cx;
	unsigned r_dx;
	unsigned r_si;
	unsigned r_di;
	unsigned r_ds;
	unsigned r_es;
	unsigned r_flags;
};
static	char	vmfn[] = "vmXXXXXX.$$$";
#else
static	char	vmfn[] = "/tmp/vmaXXXXX";
#endif

/*		virtual memory handler			*/


	/* note: the following routines are included when
	   BIGMEM is not defined.  This uses the file system for
	   a virtual memory so that very large files can be assembled
	   on a small machine.  If a machine with very large (or virtual)
	   memory is available, then BIGMEM should be defined.  Or if
	   it is known that only modest size files will be assembled.
	*/

/*
 * vmgbuf - Returns a pointer to a vmtab structure
 */

VMTAB *
vmgbuf( adr ) VMADR adr;{

	reg char	*bufptr;
	reg VMTAB	*vmp;
	reg int		i;
	reg int		blk;
	reg ushort	j;

#ifdef	STATS
	vmgct++;
#endif

	/* we wouldn't be here if the vm_buf field associated with
	   adr were not zero, so the first thing to do is to get a
	   buffer.  The oldest buffer is the vtmab index vmlru[0];
	*/

	blk = adr >> blklog;

if( debug > 3 || adr > virtop || blk >= VMCNT ||vmtab[blk].vm_buf){
printf("vmgbuf( %x ), virtop = %x, blk = %d\n",adr,virtop,blk);
if(!debug)fatal("blk is too big, virtop = %x, blklim = %x\n",virtop,blklim);
}
	while( vmtx <= blk ){			/* snowplow	*/
		vmtab[vmtx].vm_flg = 0;
		vmtab[vmtx].vm_lru = 0;
		vmtab[vmtx].vm_buf = 0;
		vmtx++;
	}
	if( vmbaln < vmnumx && (vmbaln < vmnum || pass2) ){
		vmrq++;
		bufptr = palloc( BUFSIZ );
		vmrq--;
		if( bufptr != (char *)-1 ){
			vmbaln++;
			goto readin;
		}
		vmnumx = vmbaln;		/* shut off further tries */
	}

	/* now locate the candidate for replacement */

	vmp = vmtab;
	j = 0xffff;
	vmtab[blk].vm_lru = 0;			/* don't select this one */
	for( i=0; i<vmtx; i++ )
		if( vmtab[i].vm_buf && vmtab[i].vm_lru < j )
			j = (vmp = &vmtab[i])->vm_lru;
	if( vmlrux >= 32767 ){			/* prevent lrux overflow */
		for( i=0; i<vmtx; i++ )
			vmtab[i].vm_lru >>= 1;
		vmlrux >>= 1;
	}

	/* now see if the block is dirty */

	bufptr = vmp->vm_buf;
	vmp->vm_buf = 0;
	vmp->vm_lru = 0;
	if( vmp->vm_flg & VMDIR ){		/* yes, write it out */
		if( !(vmp->vm_flg & (VMDSK|VMMEM)) ) vmp->vm_flg = vmballoc();
		i = vmp->vm_flg & VMBLK;
#ifdef	STATS
		vmwct++;
#endif
		if( vmp->vm_flg & VMDSK ){
			lseek( vmfd, (long)i << blklog, 0 );
			if( write( vmfd, bufptr, BUFSIZ ) != BUFSIZ )
				error("F55 vmwrite fail");
#ifdef msdos
		} else {
			blkmv( bufptr, _ds_reg(), 0, i << 3, BUFSIZ );
#else
		} else {
			fatal("failed to write block %d\n",blk);
#endif
		}
		vmp->vm_flg &= ~VMDIR;
	}
readin:
	vmp = &vmtab[blk];
	vmp->vm_buf = bufptr;
	vmp->vm_lru = ++vmlrux;
	if( vmp->vm_flg & (VMDSK|VMMEM) ){
		/* now page it in from disk or ram	*/
		i = vmp->vm_flg & VMBLK;
		if( vmp->vm_flg & VMDSK ){
			lseek( vmfd, (long)i << blklog, 0 );
			if( read( vmfd, bufptr, BUFSIZ ) != BUFSIZ )
				error("F91 vmread fail");
#ifdef msdos
		} else {
			blkmv( 0, i << 3, bufptr, _ds_reg(), BUFSIZ );
#else
		} else {
			fatal("%d vmp->vm_flg is %x\n",blk,vmp->vm_flg);
#endif
		}
	} else {
		for( i=0; i<BUFSIZ; i++ ) bufptr[i] = NULLCA;
	}
	return vmp;
}
/*
 * vmrfetch - Fetches a specified virtual address for reading, and returns
 * a pointer to the in-core copy of the address.
 */


char *
vmrfetch( adr ) VMADR adr;{

	reg char	*p;
	reg VMTAB	*vmp;
	p = (vmp = vmgbuf(adr))->vm_buf + (adr & (BUFSIZ-1));
	return p;
}

/*
 * vmwfetch - Fetches a specified virtual address for writing, and returns
 * a pointer to the in-core copy of the address.
 */

char *
vmwfetch( adr ) VMADR adr;{

	reg VMTAB	*vmp;
	reg char	*p;

	vmp = vmgbuf( adr );
	vmp->vm_flg |= VMDIR;
	p = vmp->vm_buf + (adr & (BUFSIZ-1));
	return vmp->vm_buf + (adr & (BUFSIZ-1));
}

#ifdef VMDEBUG
char* rfetch(n)VMADR n;{
	char *vmr;
/*DEB*/if(n==0)debug=9;
	if(debug)printf("rfetch( %x ) ",(unsigned int)n);
	if( (vmr = vmtab[n>>blklog].vm_buf) == 0){
		if(debug) printf("NOT in mem\n");
		vmr=vmrfetch(n);
	} else {
		if(debug) printf("in mem ");
		vmr+= n&(BUFSIZ-1);
	}
	if(debug)printf("	returns %x\n",vmr);
	return vmr;
}
char* wfetch(n)VMADR n;{
	char *vmr;
/*DEB*/if(n==0)debug=9;
	if(debug)printf("wfetch( %x ) ",(unsigned int)n);
	if( (vmr = vmtab[n>>blklog].vm_buf) == 0){
		if(debug) printf("NOT in mem\n");
		vmr=vmwfetch(n);
	} else {
		if(debug) printf("in mem ");
		vmtab[n>>blklog].vm_flg |= VMDIR;
		vmr+= n&(BUFSIZ-1);
	}
	if(debug)printf("	returns %x\n",vmr);
	return vmr;
}
#endif

/*
 * vminit - Opens the virtual memory file.
 */

void vminit(){

	mktemp( vmfn );
	vmfd = creat( vmfn, 0600 );
	if( vmfd == -1 ) fatal( "53 Cannot create vm file" );
	close( vmfd );
	vmfd = open( vmfn, 2 );
	if( vmfd == -1 ) fatal( "58 Cannot reopen vm file" );
}

/* remove the vmfile if it has been opened	*/

void vclose(){

#ifdef msdos
	REG86	r;
#endif
	if( vmfd ){
		close( vmfd );
		unlink( vmfn );
	}
#ifdef msdos
	while( --vmax >= 0 ){
		r.r_ax = 0x4900;
		r.r_es = vmalo[vmax];
		intcall( &r, &r, DOSINT );
	}
#endif
}

int vmballoc(){		/* allocate a block - on ramdisk if possible
			   on disk if not	*/
	static	int	vmdblk;
	reg		i;

#ifdef msdos
	REG86		r;
	static int	noramleft;
	static ushort	mpos;		/* memory address in "pages"	*/
	static ushort	mend;		/* page == 16 bytes		*/

	if( !noramleft ){
		if( mpos + (BUFSIZ>>4) > mend ){
			r.r_ax = 0x4800;
			r.r_bx = 0x800;			/* 32767 >> 4 */
			intcall( &r, &r, DOSINT );
			if( !(r.r_flags & F_CF) ){
				vmalo[vmax++] = (uns)r.r_ax;
				if( mend == (uns)r.r_ax ){
					mend += 0x800;
				} else {
					mpos = (uns)r.r_ax;
					mend = mpos + 0x800;
					mpos = (mpos + 7) & ~7;
				}
			}
		}
		if( mpos + (BUFSIZ>>4) <= mend ){
			i = mpos >> 3;	/* block in 128 byte units	*/
			mpos += (BUFSIZ>>4);
			return i | VMMEM;
		}
		noramleft = 1;
	}
#endif
	if( vmfd == 0 ) vminit();
	i = vmdblk++;
	return i | VMDSK;
}
#endif			/* of virtual memory stuff */

/*
 * valloc - Allocates a region of virtual memory of the specified size,
 * and returns the vm address of it.  The region is guaranteed to reside
 * entirely within a single block.
 */
VMADR valloc( size ) uns size;{
	reg VMADR	base;

#ifdef BIGMEM
	reg int		n;
	if( virtop + size > blklim ){
		virtop = blklim;
		blklim += BUFSIZ;
		if( blklim < virtop ) fatal("56 Vm overflow");
		if( blklim > blkend ){
			n = blkend >> blklog;
			if( n >= PHNUM ) fatal("81 Too many virtual blocks");
			phyp[n] = palloc(BUFSIZ);
			blkend += BUFSIZ;
		}
	}
#else
	if( (base = virtop + size) > blklim || base < virtop ){
		virtop = blklim;
		blklim += BUFSIZ;
		if( blklim < virtop ) fatal("56 Vm overflow");
	}
#endif
	base = virtop;
	virtop = base + size;
	return base;
}

void valign(){		/* align the virtual pointer */
	virtop = virtop;
}

void setvirtop(v) VMADR v; {		/* reset the virtop and blklim pointers */
	virtop = v;
	blklim = (virtop + (BUFSIZ-1))+1;
}
#else
void valign() {
	return;
}
void setvirtop(VMADR v) {
	return;
}
#endif // USEVM
