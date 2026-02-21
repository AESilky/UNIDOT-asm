/************************************************************************
*									*
*	Copyright (C) 1979,1986 by John D. Polstra & Unidot, Inc.	*
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
* Updated 1/2026 by AESilky to compile on current GCC running on 64-bit *
* Linux. No functional changes are intended.				*
*									*
*************************************************************************/

/************************************************************************
*									*
*		    Unidot Syntax Processor				*
*			   Includes					*
*									*
*************************************************************************/

/* @(#)$Header: usp.h,v 1.2 86/10/08 22:47:03 jdp Exp $ */

#ifndef USEVM		/* (ES) Added to use UNIDOT Virtual Memory system */
#ifndef BIGMEM
#define BIGMEM		/* Indicate BIGMEM if not using Virtual Memory */
#endif // !BIGMEM
#define VMALIGN 4 	/* Alignment for `aligned_alloc` */
#endif // !USEVM


/* compile-time parameters, i.e. sizes of things, etc */

// (ES - Let compiler take care of register) #define reg		register
/* (ES) Change these to typedefs
#define uns		unsigned
#define uint		unsigned int
#define ushort		unsigned short
*/
typedef unsigned int	uns;
typedef unsigned int	uint;
typedef unsigned short	ushort;

#define	DICTSIZE	256	/* # of dict entries			*/
#define RMARG		72	/* right margin for output		*/
#define SCANSIZE	1024	/* size of scantab			*/
#define	SEMSIZE		256	/* size of semtab			*/

/* Constants for sets represented as bits in an array or shorts		*/
#define S		4	/* shift is 4				*/
#define M		15	/* mask is 15				*/

#ifdef BIGMEM		/* use big tables for system with beaucoup memory */
#define	CLOSIZE		4096	/* shorts allocated for closing a state */
#define	NUCSIZE		1024	/* shorts allocated for largest nucleus */

#ifdef DEBUG	/* Same size as onyx so listings will compare */
#define SETSIZE		(7*16)	/* # of non-terminals available		*/
#else
#define SETSIZE		256	/* # of non-terminals available		*/
#endif

#define ALTSIZE		200	/* size of alttab			*/
#define RSIZE		200	/* size of rtab				*/
#define	STRINGSIZE	8192	/* bytes of string storage for dict	*/
#define XSIZE		512	/* size of xtab				*/
#else
#define	CLOSIZE		2048	/* shorts allocated for closing a state */
#define	NUCSIZE		512	/* shorts allocated for largest nucleus */
#define SETSIZE		160	/* # of non-terminals available		*/
#define ALTSIZE		75	/* size of alttab			*/
#define RSIZE		100	/* size of rtab				*/
#define	STRINGSIZE	4096	/* bytes of string storage for dict	*/
#define XSIZE		256	/* size of xtab				*/
#endif

/* commonly used masks */

#define BM		0xff	/* byte mask for avoiding sign extension */

/* names of the temporary files */

#ifdef msdos
#define	dname "uspdicta"	/* dictionary file name */
#define	pname "uspproda"	/* productions file name */
#define sname "uspstaba"	/* state table file name */
#define	hname "usphseta"	/* headset file name */
#define	aname "uspataba"	/* action table file name */
#define	xname "uspaltsa"	/* alttab file name */
#else
#define	dname "uspxdicta"	/* dictionary file name */
#define	pname "uspxproda"	/* productions file name */
#define sname "uspxstaba"	/* state table file name */
#define	hname "uspxhseta"	/* headset file name */
#define	aname "uspxataba"	/* action table file name */
#define	xname "uspxaltsa"	/* alttab file name */
#endif

/*
 * dictent - the dictionary entry.  the dictionary is an array of these
 * entries.  there is one entry for each grammar symbol.
*/

#define DICTENT	struct dictent

DICTENT {
	short	dlink;		/* index into PRODENT table		*/
	short	dstring;	/* index into string table		*/
	char	dbprec;		/* precedence of binary op or 0		*/
	char	duprec;		/* precedence of unary op or 0		*/
	char	dfreq;		/* relative frequency of occurrence	*/
	char	dflags;		/* various flags (defined below)	*/
};

/*
 * dictionary flag bits (in dflags)
*/

#define	LP	0200	/* symbol appears in left part of a production	*/
#define	RP	0100	/* symbol appears in right part of a production	*/
#define	MT	0040	/* symbol can derive the empty string		*/
#define	LA	0020	/* symbol declared left-associative		*/
#define	RA	0010	/* symbol declared right-associative		*/
#define	RL	0004	/* unary op groups right-to-left		*/
#define CM	0001	/* temporary check-mark				*/

/*
   prodent - productions data structure entry.  one of these is built
   for each use of a grammar symbol or semantic number in a production.
*/

#define PRODENT struct prodent
PRODENT {
	short	plink;	/* index link into PRODENT table		*/
	char	pel;	/* dictionary index or semantic number		*/
	char	pflags;	/* flags and position in production		*/
};

/* productions flag bits and fields (in pflags) */

#define	SEM	0200	/* productions entry is a semantic number	*/
#define	NT	0010	/* symbol referred to by pel is nonterminal	*/
#define	POS	0007	/* mask for position field			*/

/* set - context set, stored as a bit vector */

#define SET struct set

SET {
	short	sword[SETSIZE/16];
};

/* cgrp - configuration group.  consists of a pointer into a production
   and a context set */

#define CGRP struct cgrp
CGRP {
	short	cpp;		/* pointer into production */
	SET	cset;		/* context set */
};

/* state - a header followed by one or more configuration groups */

#define STATE struct state
STATE {
	short	slink;		/* index into state table		*/
	short	xlink;		/* pointer to transition list		*/
	short	ssize;		/* number of config-groups in nucleus	*/
	CGRP	scg[1];		/* configuration groups			*/
};

#define LIST union list
#define XLIST struct xlist
#define RLIST struct rlist

LIST {
	XLIST {				/* xlist - transition list entry.   */
		char	xxsym;		/* transition symbol element number */
		char	xxflags;	/* flags			    */
		short	xxsuc;		/* successor state pointer	    */
	} xxx;
	RLIST {				/* rlist - reduction list entry.    */
		char	rrsym;		/* symbol to match		    */
		char	rrflags;	/* flags (same as xflags)	    */
		char	rrsem;		/* semantic routine number	    */
		char	rrlp;		/* new left part		    */
	} rrr;
/* aliases to avoid non-esthetic code */
#define	xsym	xxx.xxsym
#define	xflags	xxx.xxflags
#define	xsuc	xxx.xxsuc
#define	rsym	rrr.rrsym
#define	rflags	rrr.rrflags
#define	rsem	rrr.rrsem
#define	rlp	rrr.rrlp
};

/* transition list flag bits (in xflags).  in addition to the
   flags listed below, the NT flag (defined for pflags, above) is used */

#define NEWST	0200	/* transition to new state */
#define RD	0100	/* read token and reduce */
#define ALT	0040	/* rlp is index in alttab */
#define	XEND	0020	/* end of state table xlist */

/* aent - action table entry.  a header followed by a variable number of
   xlist and rlist entries  */

#define AENT struct aent
AENT {
	short	alink;		/* linear list link */
	char	nr;		/* number of rlist entries */
	char	nall;		/* total number of xlist + rlist entries */
	LIST	act[1];		/* rlist / xlist entries */
};

/* altent - alternate semantic routine use entry.  specifies a semantic
   number, its minimum left part, and a bit string of legal left parts.  */

#define ALTENT struct altent
ALTENT {
	char	asem;		/* semantic routine number */
	char	alp;		/* lowest legal left part */
	ushort	abits;		/* bit string of legal left parts */
};
