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
*			ulink.h - linker header				*
*									*
************************************************************************/
#ifndef ULINK_H_
#define ULINK_H_

/*
 * @(#)$Header: ulink.h,v 4.9 92/07/31 07:57:46 rmm Rel $ ulink header file
 *
 * Parameters.
 */

#include <stdio.h>

#ifdef vms
#define NOTUNIX
#define FATEXIT 0x18008012
#define BADEXIT 0x18008012
#define WRNEXIT 1
#define GOODEXIT 1
#ifndef __HOST__
#define __HOST__ "vax.vms"
#endif
#endif

#ifdef msdos
#define NOTUNIX
#define FATEXIT 16
#define BADEXIT 8
#define WRNEXIT 4
#define GOODEXIT 0
#ifndef __HOST__
#define __HOST__ "msdos"
#endif
#endif

#ifndef NOTUNIX
#define FATEXIT 1
#define BADEXIT 1
#define WRNEXIT 0
#define GOODEXIT 0
#ifndef __HOST__
#define __HOST__ "xxx.unix"
#endif
#endif
/*
 *  the following is MACHINE SPECIFIC!!!
 */


#ifndef ALIGN
#define ALIGN 4		/* this must be 4 on the 3b2 and the SUN */
#endif


/* to include the variables in a module define VARS */
/* to do the Whitesmithian thing define NOBSS */

#ifdef VARS
#define GLOBL
#ifdef NOBSS
#define IZ ={0}
#else
#define IZ
#endif
#define IX(X) ={X}
#else
#define GLOBL extern
#endif
#ifndef IZ
#define IZ
#define IX(X)
#endif
#ifdef DEBUG
#define DEB(n,a)	if(debug>n)printf a
#else
#define DEB(n,a)
#endif


#define	EXTSIZ	512		/* maximum number externals/module */
#define	HSHLOG	6		/* log base 2 of hash table size */
#define	NAMSIZ	32		/* maximum module name length */
#define	OBJSIZ	255		/* maximum object block length */
#define	SECSIZ	256		/* maximum number of sections */
#define GRPSIZ  32		/* Maximum number of groups */
/*
 * Symbol attributes (in sy_atr).
 */
#define	SAUP2	0001		/* referenced but still undefined in pass 2 */
#define	SADP2	0002		/* defined in pass 2 */
#define	SAMUD	0004		/* multiply defined */
/*
 * Symbol types (in sy_typ).
 */
#define	STUND	0		/* undefined global */
#define	STGLO	1		/* defined global */
#define	STSEC	2		/* section */
#define STGRP	3		/* Group   */
/*
 * Type definitions.
 */
#ifndef reg
#define	reg	register
#endif // reg
#ifndef uns
#define	uns	unsigned
#endif // uns
#ifndef struct
#define struct register struct
#endif
#ifndef ushort
#define ushort unsigned short
#endif

/*
 * Pseudo-functions.
 */
#define olodb(p)	(*(p)&0377)
#define	ostob(b,p)	(*(p)=(b))
/*
 * Structure declarations.
 */

#ifdef XREF
#define XREFENT	struct xref
XREFENT {			/* cross reference entry	*/
	XREFENT		*xr_lnk;	/* next entry in chain	*/
	char		*xr_mdname;	/* module name with low bit used for */
					/* module with definition */
};
#endif

#ifdef STATS
#define BUFUSE	0		/* space used for buffers	*/
#define SECUSE	1		/* space used for sections	*/
#define SYMUSE	2		/* space used for symbols	*/
#define GRPUSE	3		/* space used for groups	*/
#define OTHUSE	4		/* space used for other		*/
#endif

#define OBLOCK struct oblock
OBLOCK {			/* object block */
	char	*ob_ptr;		/* pointer to next byte in buffer */
	char	*ob_top;		/* limit of data in buffer */
	char	ob_type;		/* block type */
	char	ob_buf[OBJSIZ];		/* data from the block */
};

#define SYTAB struct sytab
SYTAB {				/* symbol table entry */
	SYTAB		*sy_lnk;	/* link to next entry in hash chain */
#ifdef XREF
	XREFENT		*sy_xref;	/* head of cross reference chain */
#endif
	long		sy_val;		/* value of symbol */
	short		sy_ord;		/* ordinal for output, URBEXT+	*/
	char		sy_rel;		/* relocation of symbol */
	char		sy_typ;		/* type of symbol */
	char		sy_atr;		/* attributes of symbol */
	char		sy_ovl;		/* overlay number	*/
	char		sy_str[2];	/* actually variable length */
};

#define SECTION struct section
SECTION {			/* section table entry */
	SYTAB		*se_sym;	/* symbol table pointer		*/
	long		se_val;		/* base address of section	*/
	long		se_cum;		/* cumulative size to current module */
	long		se_mod;		/* size in current module	*/
	long		se_fpos;	/* position in output module	*/

	/* note, fpos is used to keep the length of the section as
	   announced in the input module, just for checking purposes
	   in pass1.  It is used in pass2 to keep track of where in
	   the a.out file stuff is to be written.
	*/

	char		se_adu;		/* address unit			*/
	char		se_atr2;	/* more obj attributes		*/
	short		se_atr;		/* attributes			*/

		/* note: in the object file we only use one byte so
		   the top eight bits are available for internal use -
		   specifically we use one bit to indicate that data
		   has actually been output to a section so that we
		   can produce the block in the a.out format */

#define USEINIT	0x8000			/* section initialized		*/
#define USEREF  0x4000			/* section referenced in obj mod */
#define USEXTD1	0x2000			/* section extended in pass1	*/
#define USEXTD2	0x1000			/* section extended in pass2	*/
#define USEALLO	0x0800			/* section is allocated		*/
#define USEABS	0x0400			/* section is absolute		*/

	char		se_aln;		/* alignment			*/
	char		se_ext;		/* extent			*/
	char		se_grp;		/* group to which belongs	*/
	char		se_wth;		/* within clue			*/
	char		se_xtd;		/* extension of section		*/
};

#define GROUP struct group

GROUP {				/* Group table entry			*/
	GROUP		*gr_lnk;	/* link to next item		*/
	SYTAB		*gr_sym;	/* Symtab pointer for group	*/

	/* Note: the first item in the chain points to the symbol for
	   the group, the subsequent items point to the symbols for
	   the included sections */
};

/*
 * Global variable declarations.
 */

GLOBL short	afmt IZ;	/* produce a.out format			*/
GLOBL short	absadu IZ;	/* absolute addressing unit		*/
GLOBL short	codadu IZ;	/* code addressing unit			*/
GLOBL SECTION	*codsep IZ;	/* code section ptr for split		*/
GLOBL short	commalign IX(1); /* common alignment			*/
GLOBL char	*commvar IZ;	/* name of unnamed section		*/
GLOBL short	curadu IZ;	/* current addressing unit		*/
GLOBL char	*curfile IZ;	/* current file name			*/
#ifdef XREF
GLOBL char	*curmdp IZ;	/* pointer to saved current module name	*/
#endif
GLOBL long	curoff IZ;	/* offset of current text block		*/
GLOBL ushort	cursec IZ;	/* section of current text block	*/
GLOBL short	datadu IZ;	/* data addressing unit			*/
GLOBL SECTION	*datsep IZ;	/* data section ptr for split		*/
GLOBL short	debug IZ;	/* old debug flag			*/
GLOBL ushort	errct IZ;	/* count of errors			*/
GLOBL ushort	evct IZ;	/* number of entries in extvec		*/
GLOBL long	fpos IZ;	/* position in output file		*/
GLOBL short	grpx IZ;	/* number of groups			*/
GLOBL char	*inbuf IZ;	/* input buffer pointer			*/
GLOBL OBLOCK	libblk IZ;	/* current library block		*/
GLOBL ushort	linect IZ;	/* number of lines left on listing page */
GLOBL char	*mapfile IZ;	/* name of map file			*/
GLOBL short	mflag IZ;	/* flag turning on load map listing	*/
GLOBL short	nflag IZ;	/* flag turning on local sym list	*/
GLOBL short	nomix IZ;	/* don't mix attributes	 (z8k)		*/
GLOBL short	numorder IZ;	/* sort map in numeric order		*/
GLOBL char	*objfile IZ;	/* name of object file			*/
GLOBL short	outsec IZ;	/* output section of current text	*/
GLOBL char	*ovlfile IZ;	/* overlay control file			*/
GLOBL short	ovlnum IZ;	/* current overlay number		*/
GLOBL ushort	mulct IZ;	/* count of multiply defined symbols	*/
GLOBL short	nxtord IX(URBEXT); /* next output ordinal to assign	*/
GLOBL OBLOCK	objblk IZ;	/* block from current object file	*/
GLOBL ushort	ovct IZ;	/* count of section overlaps		*/
GLOBL ushort	pagect IZ;	/* listing page number			*/
GLOBL short	pass2 IZ;	/* flag indicating second pass		*/
GLOBL char	*pghead IZ;	/* heading line for listing pages	*/
#ifdef NSC
GLOBL char	*prname IX("nlink");	/* program name from command line */
#else
GLOBL char	*prname IX("ulink");	/* program name from command line */
#endif
GLOBL char	proctype[64] IZ; /* processor type			*/
GLOBL long	relsize IZ;	/* size of relocation section		*/
GLOBL short	kflag IZ;	/* keep section ID's flag		*/
GLOBL short	rflag IZ;	/* output relocation informaion switch	*/
GLOBL short	sflag IZ;	/* suppress symbols flag		*/
GLOBL short	split IZ;	/* split i-d flag			*/
GLOBL ushort	stct IX(1);	/* number of entries in sectab		*/
GLOBL long	symsize IZ;	/* size of symbol table section		*/
GLOBL ushort	svct IX(1);	/* number of entries in secvec		*/
GLOBL long	tranad IZ;	/* transfer address			*/
GLOBL short	traflg IZ;	/* flag indicating transfer address output */
GLOBL char	*txttop IZ;	/* top of text in objbuf		*/
GLOBL ushort	undct IZ;	/* number of still undefined globals	*/
#ifdef STATS
GLOBL long	usestats[5];	/* number of stats		*/
#endif
GLOBL short	verbose IZ;	/* be talky				*/
GLOBL ushort	warnct IZ;	/* count of warnings			*/

GLOBL char	curmod[NAMSIZ+1] IZ; /* current module name		*/
GLOBL SYTAB	*extvec[EXTSIZ] IZ; /* ext symbol vector for relocation */
GLOBL GROUP	*grptab[GRPSIZ] IZ; /* pointers to group headers	*/
GLOBL SECTION	*sectab[SECSIZ] IZ; /* section information table	*/
GLOBL char	secvec[SECSIZ] IZ;  /* section vector for relocation	*/
GLOBL SYTAB	*syhtab[1<<HSHLOG] IZ; /* symbol hash table		*/

/*
 * Function declarations.
 */

ushort		hash();		/* Symbol hashing function */
long		gdiff();	/* group differences		*/
long		ogetl();	/* Return next long word from objbuf */
char		*ogets();	/* Return ptr to next string in objbuf */
ushort		olodj11();	/* Return specified 8051 11-bit field */
long		olodl();	/* Return specified long word from objbuf */
long		olodlm();	/* Return specified long word, MSB first */
ushort		olodw();	/* Return specified word from objbuf */
ushort		olodwm();	/* Return specified word, MSB first */
char		*palloc();	/* Memory allocation routine */
char		*zpalloc();	/* Memory allocation guarantees zero	*/
long		rbase();	/* Return relocation base address */
char		*lastcomp();	/* Return pointer to last component	*/
long		sbase();	/* Return section base address (for 8086) */
SECTION		*selook();	/* Section table lookup and entry routine */
SYTAB		*sylook();	/* Symbol table lookup and entry routine */
SYTAB		*symerge();	/* Symbol table hash chain merger */
SYTAB		*sypeek();	/* Symbol table lookup routine */


/*
 * Input/ Output variables
 */

GLOBL FILE	*OBJOUT IZ;
GLOBL FILE	*OBJIN IZ;
GLOBL FILE	*LIST IZ;
GLOBL FILE	*LOCFILE IZ;
GLOBL FILE	*SYMFILE IZ;
GLOBL FILE	*RELFILE IZ;
GLOBL FILE	*OVLYFILE IZ;		/* overlay control stream	*/

#endif // ULINK_H_
