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
*			uasz80.h - z80 specific header			*
*									*
************************************************************************/
/*
 * @(#)$Header: uasz80.z,v 1.2 87/12/01 13:30:23 rmm Rel $
 *
 * Declarations specific to uasz80
 */
#ifndef UASZ80_H_
#define UASZ80_H_

/*
 * Parameters.
 */
#define OPMAX	2		/* maximum number of instruction operands */


/*
 * Token types. (coordinated with grammar)
 */

#define	TKR8	14		/* {b,d,e,h,l} */
#define	TKZR8	15		/* {i,r} */
#define	TKSCC	16		/* {nz,z,nc} */
#define	TKA	17		/* a */
#define	TKC	18		/* c */
#define	TKHL	19		/* hl */
#define	TKSP	20		/* sp */
#define	TKBC	21		/* bc */
#define	TKDE	22		/* de */
#define	TKAF	23		/* af */
#define	TKAFP	24		/* af' */
#define	TKLCC	25		/* {po,pe,p,m} */
#define	TKZR16	26		/* {ix,iy} */

/*
 * Structure declarations.
 */

struct	format_ {		/* instruction format table entry */
	char	fm_op[OPMAX];		/* operand descriptions */
	char	fm_skel;		/* opcode skeleton word */
	char	fm_flg;			/* flags */
};
typedef struct format_ format_t;

/*
 * Global variable declarations.
 */
extern char		hlflg;		/* flag indicating hl was used */
extern char		ixiy;		/* prefix for ix/iy instruction, or 0 */
extern char		ixiyi;		/* flag indicating (ix+d) or (iy+d) */
extern uns		ixiyr;		/* (ix+d) or (iy+d) offset relocation */
extern long		ixiyv;		/* (ix+d) or (iy+d) offset value */


/*
 * Operand classes.
 */

#ifdef OCNULL
#undef OCNULL
#endif
#ifdef OCNEX
#undef OCNEX
#endif
#ifdef OCPEX
#undef OCPEX
#endif
#ifdef OCEXP
#undef OCEXP
#endif

#define OCNULL	0
#define OCNEX	1
#define OCPEX	2
#define OCEXP	3
#define OCR8	4
#define OCA	5
#define OCC	6
#define OCSCC	7
#define OCLCC	8
#define OCSSDD	9
#define OCQQ	10
#define OCIC	11
#define OCZR8	12
#define OCAF	13
#define OCAFP	14
#define OCHL	15
#define OCIHL	16
#define OCIBCDE	17
#define OCSP	18
#define OCDE	19
#define OCISP	20
#define OCR8M	21
#define OCMSK	0x1f		/* mask for operand class field in fm_op */

/*
 * Operand actions (in fm_op[]).
 */

#define OANULL	0
#define OAL3	0x20
#define OAR3	0x40
#define OA8	0x60
#define OA16	0x80
#define OA8R	0xa0
#define OARST	0xc0
#define OAINT	0xe0
#define OAMSK	0xe0		/* mask for operand action field in fm_op */

/*
 * Instruction format table flags (in fm_flg).
 */

#define FMCB	1
#define FMED	2
#define FMNIXIY	4
#define FMNDISP	8
#define FMLAST	0x10

/*
 * Global variable declarations.
 */

#endif // UASZ80_H_

