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
*			urel.h - relocation action header		*
*									*
************************************************************************/

/*
 * @(#)$Header: urel.z,v 1.7 88/04/07 13:32:45 rmm Rel $
 *
 * Relocation action tables
 */


#define A_HALT	0
#define A_SWAP	1
#define A_DUP	2
#define A_POP	3
#define A_INCAP	4
#define A_ADD	5
#define A_SUB	6
#define A_AND	7
#define A_OR	8
#define A_XOR	9
#define A_SHR	10
#define A_SHL	11
#define A_NEG	12
#define A_CMP	13
#define A_MUL	14
#define A_DIV	15
#define A_MOD	16
#define A_RNG	17
#define A_SEXT	18
#define A_MERGE	19
#define A_LI1	20
#define A_LI2	21
#define A_LI4	22
#define A_LLC	23
#define A_LTU	24
#define A_LTG	25
#define A_RNG8	26
#define A_RNG8U	27
#define A_RNG16	28
#define A_RNG16U	29
#define A_LRI	30
#define A_LT0	31
#define A_LT1	32
#define A_ST0	33
#define A_ST1	34
#define A_LNK	35
#define A_LRIL	36
#define A_LDK	64
#define A_LDL	96
#define A_LDM	128
#define A_STL	160
#define A_STM	192


#define X_EXEC		0
#define X_LDK		2
#define X_LDL		3
#define X_LDM		4
#define X_STL		5
#define X_STM		6
#define X_SHF		5
#define X_MSK		7

