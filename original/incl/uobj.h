/************************************************************************
*									*
*	Copyright (C) 1984,1985, by Unidot, Inc.			*
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
*		     Unidot Object Module Format			*
*									*
*			uobj.h - obj header				*
*									*
************************************************************************/


/* @(#)$Header: uobj.h,v 1.8 87/12/01 05:49:06 rmm Exp $ Unidot Object Format */

/* Object file block types  */

#define	UOBOST	1		/* object start block */
#define	UOBLST	2		/* library start block */
#define	UOBSEC	3		/* sections block */
#define	UOBGLO	4		/* global symbols block */
#define	UOBLOC	5		/* local symbols block */
#define	UOBTXT	6		/* text block */
#define	UOBBSZ	7		/* bssz block */
#define	UOBTRA	8		/* transfer address block */
#define	UOBLIX	9		/* library index block */
#define	UOBLND	10		/* library end block */
#define	UOBOND	11		/* object end block */
#define	UOBMOD	12		/* module name block */
#define	UOBOVL	13		/* Overlay block */
#define UOBRLT	14		/* Relocation Type Declaration */
#define UOBPRO	15		/* Processor Name */
#define UOBCMT	16		/* Comment */
#define UOBGRP	17		/* new group control	*/
/*
 * Object file relocation actions.
 *
 * Due to needing more relocation actions the external base field was
 * cut from 12 bits to 11 bits 03/87, and the action field increased to
 * five bits.  This should inconvenience few.
 */

#define URAMSK	0xf800		/* mask for action item	*/

#define	URANOP	0		/* no relocation operation */
#define	URAA16	0x8000		/* add base to word field */
#define	URAA8	0x4000		/* add base to byte field */
#define	URAA32	0xc000		/* add base to long field */
#define	URAA16M	0x2000		/* add base to word field (MSB first) */
#define	URAA32M	0xa000		/* add base to long field (MSB first) */
#define	URAZSS	0x6000		/* relocate Z8001 short seg address */
#define	URAZLS	0xe000		/* relocate Z8001 long seg address */
#define	URASEG	0x1000		/* 8086 segment relocation */
#define URAOFF	0x9000		/* 8086 offset relocation */
#define URAREL	0x7000		/* 8086 relative offset   */
#define URASREL	0xb000		/* 8086 relative offset short  */
#define URAJ11	0x5000		/* 8051 11-bit jump target relocation */
#define URASOFF	0xd000		/* 8086 short offset relocation */
#define	URAZOF	0x3000		/* relocate Z8001 16-bit offset */
#define URAZSG	0xf000		/* relocate Z8001 16-bit segment number */

/* following actions use the 0x0800 bit */

#define URATLA	0x1800		/* relocate TMS34010 32 bit bit address */
#define URATSB	0x2800		/* relocate TMS34010 16 bit branch tgt */
#define URATSA	0x3800		/* relocate TMS34010 16 bit bit address */
#define URATLAC	0x4800		/* rel TMS30401 32 bit complemented	*/
#define URATSAC	0x5800		/* rel TMS30401 16 bit complemented	*/
#define URARWD	0x6800		/* relocate RGP 16 bit word offset	*/
#define URARAD	0x7800		/* relocate RGP 24 bit word address	*/

/* the above are the originally defined items - due to the apparent need
   for infinite expansion, we have redefined them as the following
   actions, which may be used in UOBRLT records to provide for any
   number of codes private to a particular linkage
*/

#define	UR_NOP	0		/* no relocation operation		*/
#define UR_WDW	1		/* word relocation of a 16 bit item	*/
#define	UR_SEG	2		/* 8086 segment relocation		*/
#define UR_TLA	3		/* relocate TMS34010 32 bit bit address	*/
#define	UR_A16M	4		/* add base to word field (MSB first)	*/
#define UR_TSB	5		/* relocate TMS34010 16 bit branch tgt	*/
#define	UR_ZOF	6		/* relocate Z8001 16-bit offset		*/
#define UR_TSA	7		/* relocate TMS34010 16 bit bit address */
#define	UR_A8	8		/* add base to byte field		*/
#define UR_TLAC	9		/* rel TMS30401 32 bit complemented	*/
#define UR_J11	10		/* 8051 11-bit jump target relocation	*/
#define UR_TSAC	11		/* rel TMS30401 16 bit complemented	*/
#define	UR_ZSS	12		/* relocate Z8001 short seg address	*/
#define UR_RWD	13		/* relocate RGP 16 bit word offset	*/
#define UR_REL	14		/* 8086 relative offset   		*/
#define UR_RAD	15		/* relocate RGP 24 bit word address	*/
#define	UR_A16	16		/* add base to word field		*/
#define UR_H8B	17		/* byte rel, high byte bits 11-4 BCP	*/
#define UR_OFF	18		/* 8086 offset relocation		*/
#define UR_L8B	19		/* byte rel, low byte bits 11-4 BCP	*/
#define	UR_A32M	20		/* add base to long field (MSB first)	*/
#define UR_H8W	21		/* word rel, high byte bits 11-4 BCP	*/
#define UR_SREL	22		/* 8086 relative offset short 		*/
#define UR_L8W	23		/* word rel, low byte bits 11-4 BCP	*/
#define	UR_A32	24		/* add base to long field		*/
#define UR_H8	25		/* add base >> 8 to byte field		*/
#define UR_SOFF	26		/* 8086 short offset relocation		*/
#define UR_HW8	27		/* add base >> 9 to byte field		*/
#define	UR_ZLS	28		/* relocate Z8001 long seg address	*/
#define UR_BYW	29		/* add base >> 1 to byte field		*/
#define UR_ZSG	30		/* relocate Z8001 16-bit segment number */

#define	UR_SHF	11		/* shift value for the relocation item	*/
/*
 * Object file relocation bases.
 */
#define	URBABS	0		/* absolute */
#define	URBSEC	1		/* section */
#define	URBUND	255		/* undefined */
#define	URBEXT	256		/* external */
#define	URBMSK	0x07ff		/* mask for relocation base field */
/*
 * Section attributes.
 */
#define	USENOX	0x01		/* not executable */
#define	USENOW	0x02		/* not writeable */
#define	USENOR	0x04		/* not readable */
#define	USECOM	0x08		/* common */
#define	USEFIX	0x10		/* base address fixed */
#define USEMOR	0x80		/* another attribute byte coming	*/

/*
 * More attribute bits (if USEMOR set in first attribute byte
 */

#define	USMADU	0x01		/* addru byte present			*/
#define USMWTH	0x02		/* within byt present			*/
#define USMLEN	0x04		/* section length long word present	*/
