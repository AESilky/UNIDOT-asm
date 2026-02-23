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
* Updated 2026 by AESilky with permission from RSS			*
*									*
************************************************************************/
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

#include "../incl/aesbf.h"

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

#ifndef USEVM
#define VMALIGN 4 	/* Alignment for `aligned_alloc` */
#endif // !USEVM


#define NULLCA 	'\000'			/* NULL char value */

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
#define DEBOUT(n,a)	if(debug>n)printf a
#else
#define DEBOUT(n,a)
#endif


/* maximum number externals/module */
#define	EXTSIZ	512		
/* log base 2 of hash table size */
#define	HSHLOG	6		
/* maximum module name length */
#define	NAMSIZ	32		
/* maximum object block length */
#define	OBJSIZ	255		
/* maximum number of sections */
#define	SECSIZ	256		
/* Maximum number of groups */
#define GRPSIZ  32		

/*
 * Symbol attributes (in sy_atr).
 */
/* referenced but still undefined in pass 2 */
#define	SAUP2	0001		
/* defined in pass 2 */
#define	SADP2	0002		
/* multiply defined */
#define	SAMUD	0004		
/*
 * Symbol types (in sy_typ).
 */
/* undefined Global */
#define	STUND	0
/* defined Global */
#define	STGLO	1
/* Section */
#define	STSEC	2
/* Group   */
#define STGRP	3
/*
 * Type definitions. - (ES) Now using actual `typedef` statements.
 */
#ifndef reg
#define	reg
#endif // reg
//#ifndef uns
//#define	uns	unsigned
//#endif // uns
//#ifndef struct
//#define struct register_ struct
//#endif
//#ifndef ushort
//#define ushort unsigned short
//#endif

/*
 * Pseudo-functions.
 */
#define olodb(p)	(*(p)&0377)
#define	ostob(b,p)	(*(p)=(b))

/*
 * Structure declarations.
 */

#ifdef xref_t
#define XREFENT	struct xref_
/* cross reference entry	*/
XREFENT {
	/* next entry in chain	*/
	XREFENT		*xr_lnk;
	/* module name with low bit used for module with definition */
	char		*xr_mdname;
};
#endif

#ifdef STATS
#define BUFUSE	0		/* space used for buffers	*/
#define SECUSE	1		/* space used for sections	*/
#define SYMUSE	2		/* space used for symbols	*/
#define GRPUSE	3		/* space used for groups	*/
#define OTHUSE	4		/* space used for other		*/
#endif

typedef struct oblock_ oblock_t;		/* OBLOCK */
/* object block */
struct oblock_ {
	/* pointer to next byte in buffer */
	char	*ob_ptr;
	/* limit of data in buffer */
	char	*ob_top;
	/* block type */
	char	ob_type;
	/* data from the block */
	char	ob_buf[OBJSIZ];
};

typedef struct sytab_ sytab_t;		/* SYTAB */
/* symbol table entry */
struct sytab_ {
	/* link to next entry in hash chain */
	sytab_t		*sy_lnk;
#ifdef xref_t
	/* head of cross reference chain */
	XREFENT		*sy_xref;
#endif
	/* value of symbol */
	long		sy_val;
	/* ordinal for output, URBEXT+	*/
	short		sy_ord;
	/* relocation of symbol */
	char		sy_rel;
	/* type of symbol */
	char		sy_typ;
	/* attributes of symbol */
	char		sy_atr;
	/* overlay number	*/
	char		sy_ovl;
	/* actually variable length */
	char		sy_str[2];
};

typedef struct section_ section_t;		/* SECTION */
/* section table entry */
struct section_ {
	/* symbol table pointer		*/
	sytab_t		*se_sym;
	/* base address of section	*/
	long		se_val;
	/* cumulative size to current module */
	long		se_cum;
	/* size in current module	*/
	long		se_mod;
	/* position in output module (pass2 a.out position)	*/
	long		se_fpos;

	/* note, fpos is used to keep the length of the section as
	   announced in the input module, just for checking purposes
	   in pass1.  It is used in pass2 to keep track of where in
	   the a.out file stuff is to be written.
	*/

	/* address unit			*/
	char		se_adu;		
	/* more obj attributes		*/
	char		se_atr2;	
	/* attributes			*/
	short		se_atr;		

/* note: in the object file we only use one byte so
	the top eight bits are available for internal use -
	specifically we use one bit to indicate that data
	has actually been output to a section so that we
	can produce the block in the a.out format */

/* section initialized		*/
#define USEINIT	0x8000
/* section referenced in obj mod */
#define USEREF  0x4000
/* section extended in pass1	*/
#define USEXTD1	0x2000
/* section extended in pass2	*/
#define USEXTD2	0x1000
/* section is allocated		*/
#define USEALLO	0x0800
/* section is absolute		*/
#define USEABS	0x0400

	/* alignment			*/
	char		se_aln;		
	/* extent			*/
	char		se_ext;		
	/* group to which belongs	*/
	char		se_grp;		
	/* within clue			*/
	char		se_wth;		
	/* extension of section		*/
	char		se_xtd;		
};

typedef struct group_ group_t;		/* GROUP */

/* Group table entry		*/
struct group_ {				
	/* link to next item		*/
	group_t		*gr_lnk;	
	/* Symtab pointer for group	*/
	sytab_t		*gr_sym;	

	/* Note: the first item in the chain points to the symbol for
	   the group, the subsequent items point to the symbols for
	   the included sections */
};

/*
 * Global variable declarations.
 */

/* produce a.out format			*/
GLOBL short	afmt IZ;	
/* produce binary format (sets afmt as well) */
GLOBL short	binfmt IZ;
/* file fill byte to use */
GLOBL unsigned char	fillb IZ;	
/* absolute addressing unit		*/
GLOBL short	absadu IZ;	
/* code addressing unit			*/
GLOBL short	codadu IZ;	
/* code section ptr for split		*/
GLOBL section_t	*codsep IZ;	
/* common alignment			*/
GLOBL short	commalign IX(1); 
/* name of unnamed section		*/
GLOBL char	*commvar IZ;	
/* current addressing unit		*/
GLOBL short	curadu IZ;	
/* current file name			*/
GLOBL char	*curfile IZ;	
#ifdef xref_t
/* pointer to saved current module name	*/
GLOBL char	*curmdp IZ;	
#endif
/* offset of current text block		*/
GLOBL long	curoff IZ;	
/* section of current text block	*/
GLOBL ushort	cursec IZ;	
/* data addressing unit			*/
GLOBL short	datadu IZ;	
/* data section ptr for split		*/
GLOBL section_t	*datsep IZ;	
/* old debug flag			*/
GLOBL short	debug IZ;	
/* count of errors			*/
GLOBL ushort	errct IZ;	
/* number of entries in extvec		*/
GLOBL ushort	evct IZ;	
/* position in output file		*/
GLOBL long	fpos IZ;	
/* number of groups			*/
GLOBL short	grpx IZ;	
/* input buffer pointer			*/
GLOBL char	*inbuf IZ;	
/* current library block		*/
GLOBL oblock_t	libblk IZ;	
/* number of lines left on listing page */
GLOBL ushort	linect IZ;	
/* name of map file			*/
GLOBL char	*mapfile IZ;	
/* flag turning on load map listing	*/
GLOBL short	mflag IZ;	
/* flag turning on local sym list	*/
GLOBL short	nflag IZ;	
/* don't mix attributes	 (z8k)		*/
GLOBL short	nomix IZ;	
/* sort map in numeric order		*/
GLOBL short	numorder IZ;	
/* name of object file			*/
GLOBL char	*objfile IZ;	
/* output section of current text	*/
GLOBL short	outsec IZ;	
/* overlay control file			*/
GLOBL char	*ovlfile IZ;	
/* current overlay number		*/
GLOBL short	ovlnum IZ;	
/* count of multiply defined symbols	*/
GLOBL ushort	mulct IZ;	
/* next output ordinal to assign	*/
GLOBL short	nxtord IX(URBEXT); 
/* block from current object file	*/
GLOBL oblock_t	objblk IZ;	
/* count of section overlaps		*/
GLOBL ushort	ovct IZ;	
/* listing page number			*/
GLOBL ushort	pagect IZ;	
/* flag indicating second pass		*/
GLOBL short	pass2 IZ;	
/* heading line for listing pages	*/
GLOBL char	*pghead IZ;	
#ifdef NSC
/* program name from command line */
GLOBL char	*prname IX("nlink");	
#else
/* program name from command line */
GLOBL char	*prname IX("ulink");	
#endif
/* processor type			*/
GLOBL char	proctype[64] IZ; 
/* size of relocation section		*/
GLOBL long	relsize IZ;	
/* keep section ID's flag		*/
GLOBL short	kflag IZ;	
/* output relocation information switch	*/
GLOBL short	rflag IZ;	
/* suppress symbols flag		*/
GLOBL short	sflag IZ;	
/* split i-d flag			*/
GLOBL short	split IZ;	
/* number of entries in sectab		*/
GLOBL ushort	stct IX(1);	
/* size of symbol table section		*/
GLOBL long	symsize IZ;	
/* number of entries in secvec		*/
GLOBL ushort	svct IX(1);	
/* transfer address			*/
GLOBL long	tranad IZ;	
/* flag indicating transfer address output */
GLOBL short	traflg IZ;	
/* top of text in objbuf		*/
GLOBL char	*txttop IZ;	
/* number of still undefined globals	*/
GLOBL ushort	undct IZ;	
#ifdef STATS
/* number of stats		*/
GLOBL long	usestats[5];	
#endif
/* be talky				*/
GLOBL short	verbose IZ;	
/* count of warnings			*/
GLOBL ushort	warnct IZ;	

/* current module name		*/
GLOBL char	curmod[NAMSIZ+1] IZ; 
/* ext symbol vector for relocation */
GLOBL sytab_t	*extvec[EXTSIZ] IZ; 
/* pointers to group headers	*/
GLOBL group_t	*grptab[GRPSIZ] IZ; 
/* section information table	*/
GLOBL section_t	*sectab[SECSIZ] IZ; 
/* section vector for relocation	*/
GLOBL char	secvec[SECSIZ] IZ;  
/* symbol hash table		*/
GLOBL sytab_t	*syhtab[1<<HSHLOG] IZ; 

/*
 * Function declarations.
 */

/* Symbol hashing function */
ushort hash();		
/* group differences		*/
long gdiff();	
/* Return next long word from objbuf */
long ogetl();	
/* Return ptr to next string in objbuf */
char *ogets();	
/* Return specified 8051 11-bit field */
ushort olodj11();	
/* Return specified long word from objbuf */
long olodl();	
/* Return specified long word, MSB first */
long olodlm();	
/* Return specified word from objbuf */
ushort olodw();	
/* Return specified word, MSB first */
ushort olodwm();	
#ifdef STATS
/* Memory allocation routine */
char *palloc(uns size, int which);
/* Memory allocation initialized to zero */
char *zpalloc(uns size, int which);
/* Print stats with a header */
void prstats(char* s);
#else
/* Memory allocation routine */
char *palloc(uns size);
/* Memory allocation initialized to zero */
char *zpalloc(uns size);
#endif
/* Return relocation base address */
long rbase();	
/* Return pointer to last component	*/
char *lastcomp();	
/* Return section base address (for 8086) */
long sbase();
/* Read a Hex Byte from the string */
unsigned char scanbyte(char* s);
/* Section table lookup and entry routine */
section_t *selook();	
/* Symbol table lookup and entry routine */
sytab_t *sylook();	
/* Symbol table hash chain merger */
sytab_t *symerge();	
/* Symbol table lookup routine */
sytab_t *sypeek();	


/*
 * Input/ Output variables
 */

GLOBL FILE	*OBJOUT IZ;
GLOBL FILE	*OBJIN IZ;
GLOBL FILE	*LIST IZ;
GLOBL FILE	*LOCFILE IZ;
GLOBL FILE	*SYMFILE IZ;
GLOBL FILE	*RELFILE IZ;
/* overlay control stream	*/
GLOBL FILE	*OVLYFILE IZ;		

#endif // ULINK_H_
