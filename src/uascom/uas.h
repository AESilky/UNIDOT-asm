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
*			uas.h - common declarations			*
*									*
************************************************************************/


/* @(#)$Header: uas.h,v 6.19 88/10/09 13:34:37 rmm Rel $ uas common header */
#ifndef UAS_H_
#define UAS_H_

#include <stdlib.h>
#include "../incl/aesbfh.h"

#ifndef USEVM
#ifndef BIGMEM
#define BIGMEM		/* Indicate BIGMEM if not using Virtual Memory */
#endif // !BIGMEM
#define VMALIGN 4 	/* Alignment for `aligned_alloc` */
#endif // !USEVM

extern int xscanc();

#ifdef vms
#define NOTUNIX
#define FATEXIT 0x18008012
#define WRNEXIT 0x18008012
#define BADEXIT 1
#define GOODEXIT 1
#ifndef __HOST__
#define __HOST__ "vax.vms"
#endif
#endif // vms

#ifdef msdos
#define NOTUNIX
#define FATEXIT 16
#define BADEXIT 8
#define WRNEXIT 4
#define GOODEXIT 0
#ifndef __HOST__
#define __HOST__ "msdos"
#endif
#endif // msdos

#ifndef NOTUNIX
#define FATEXIT 1
#define BADEXIT 1
#define WRNEXIT 0
#define GOODEXIT 0
#ifndef __HOST__
#define __HOST__ "xxx.unix"
#endif // __HOST__
#endif // NOTUNIX
#include <stdio.h>
#ifndef reg
#define	reg	/* abbreviation for good tabbing	*/
#endif // reg

/* Declarations common to all of the cross assemblers */

/* to include the variables in a module define VARS
	ie.	#define VARS	1
   if you are dealing with a C compiler that requires all variables
   to be initialized somewhere (Whitesmith for example) define NOBSS:
	ie.	#define NOBSS	1
*/

#ifdef VARS
#define GLOBL
#ifdef NOBSS
#define IZ ={0}
#else
#define IZ
#endif // NOBSS
#define IX(X) ={X}
#else
#define GLOBL extern
#define IZ
#define IX(X)
#endif // VARS

#ifdef BIGDEBUG
#define BDEB(x,y) if(debug>x)printf y
#else
#define BDEB(x,y)
#endif // BIGDEBUG

	/* The following items are MACHINE SPECIFIC!!!		*/

#ifndef ALIGN
#define ALIGN 4	 	/* this must be 4 on the 3b2 */
//#define ALIGN 2		/* this should be 2 on most 16 bit machines */
#endif // ALIGN

#ifdef USEVM
#ifdef BIGMEM		/* define this to avoid virtual memory overhead */
#define VALN(n)		(virtop+(n)>blklim?valloc(n):(virtop+=(n))-(n))
#define rfetch(n)	(phyp[(n)>>blklog]+(n&(BUFSIZ-1)))
#define wfetch(n)	(phyp[(n)>>blklog]+(n&(BUFSIZ-1)))
#define rlimit(n)	(phyp[(n)>>blklog]+BUFSIZ)
#define PHNUM		128		/* number of BUFSIZ block allowed */
#else
#define VALN(n)		valloc(n)
#define VMTAB struct vmtab
#define VMNUM		10		/* number of in-core buffers	*/
#define VMCNT		256		/* number of out-core buffers	*/
VMTAB {
	ushort	vm_flg;		/* high two bits have flags, low block # */
	ushort	vm_lru;		/* lru indicator			*/
	char	*vm_buf;	/* real mem ptr if not zero		*/
};
extern char		*vmrfetch();
extern char		*vmwfetch();
#ifndef VMDEBUG
#define rfetch(n)	((vmw = &vmtab[(n)>>blklog])->vm_buf?\
	(((vmw->vm_lru= ++vmlrux),\
	(vmw->vm_buf+((n)&(BUFSIZ-1))))):\
	vmwfetch(n))
#define wfetch(n)	((vmw = &vmtab[(n)>>blklog])->vm_buf?\
	(((vmw->vm_lru= ++vmlrux),\
	(vmw->vm_flg |= VMDIR),\
	(vmw->vm_buf+((n)&(BUFSIZ-1))))):\
	vmwfetch(n))
#else
extern char	*rfetch();
extern char	*wfetch();
#endif // VMDEBUG
#define rlimit(n)	(vmtab[(n)>>blklog].vm_buf + BUFSIZ)
	/* note rlimit must be called only AFTER calling rfetch or wfetch */
#endif // BIGMEM
#define VAL1		(virtop>=blklim?valloc(1):virtop++)
#ifdef BIGMEM
GLOBL char* phyp[PHNUM] IZ;	/* in core pointers		*/
#else
GLOBL VMTAB	vmtab[VMCNT] IZ;	/* in core headers		*/
#endif // BIGMEM
#else
#define VAL1		aligned_alloc(VMALIGN, 1)
#define VALN(n)		aligned_alloc(VMALIGN, n)
#define rfetch(n)	(n)
#define wfetch(n)	(n)
#define rlimit(n)	(n+1)
#endif // USEVM



	/* Parameters */

#define	IISIZ	  30		/* size of parse stack in frames	*/
#define	INSIZ	(BUFSIZ*2)	/* size of input stack in bytes		*/
#define	LLERX	   6		/* max number of errors per line	*/
#define	LLLOC	   4		/* length of location field in listing	*/
#ifdef OLDSTYLE
#define	LLOBJ	   8		/* length of object field in listing	*/
#else
#define	LLOBJ	   11		/* length of object field in listing	*/
#endif // OLDSTYLE
#define	LLPP	  58		/* listing lines/page (must be < 99)	*/
#define	LLSEQ	   5		/* length of sequence field in listing	*/
#define NMCCNT	   8		/* chain count for numeric labels	*/
#define	OBJSIZ	 255		/* maximum object block length		*/
#define	OHSHLOG	   6		/* log base 2 of opcode hash table size */
#define	SECSIZ	 255		/* max number of sections including abs */
#define	SHSHLOG	   9		/* log base 2 of symbol hash table size */
#define	SLINSIZ	 196		/* maximum source line length		*/
#define	STRSIZ	 256		/* maximum string length		*/
#define	SYMSIZ	  32		/* maximum symbol length		*/
#define	TITSIZ	  47		/* maximum title string length		*/
#define ULXSIZ	  16		/* number of using entries		*/

	/* Assembler directive numbers */

#define	ADABS	 19		/* .abs					*/
#define	ADALIGN	 20		/* .align				*/
#define	ADBLOCK	  7		/* .block				*/
#define	ADBYTE	  4		/* .byte				*/
#define	ADCLIST	  9		/* .clist				*/
#define ADCOMM   34		/* .comm				*/
#define	ADCOMMON 26		/* .common				*/
#define ADDOUB	 40		/* .double				*/
#define ADDSECT	 35		/* .dsect				*/
#define ADDROP	 36		/* .drop				*/
#define	ADEJECT	 25		/* .eject				*/
#define	ADELSE	 15		/* .else				*/
#define	ADEND	  1		/* .end					*/
#define	ADENDIF	 16		/* .endif				*/
#define	ADENDM	 23		/* .endm				*/
#define	ADENDR	 28		/* .endr				*/
#define	ADEQU	  2		/* .equ					*/
#define	ADERROR	 30		/* .error				*/
#define	ADEXIT	 31		/* .exit				*/
#define ADFLOAT	 39		/* .float				*/
#define	ADGLOB	 18		/* .global				*/
#define ADGROUP	 38		/* .group				*/
#define	ADIF	 17		/* .if					*/
#define	ADINPUT	  3		/* .input				*/
#define	ADLIST	 14		/* .list				*/
#define ADLOCLAB 42		/* .loclab				*/
#define	ADLONG	 24		/* .long				*/
#define	ADMAC	 22		/* .macro				*/
#define	ADMLIST	 41		/* .mlist				*/
#define	ADORG	  8		/* .org					*/
#define	ADREPT	 29		/* .repeat				*/
#define	ADSECT	 10		/* .sect				*/
#define	ADSET	  6		/* .set					*/
#define	ADSPACE	 11		/* .space				*/
#define	ADSTITL	 12		/* .stitle				*/
#define	ADTITLE	 13		/* .title				*/
#define ADUSING  37		/* .using				*/
#define	ADWARN	 33		/* .warn				*/
#define	ADWITHN	 21		/* .within				*/
#define	ADWORD	  5		/* .word				*/

	/* note 42 last used so far - machine specific 51+	*/

	/* used in various directives	*/

#define STROK	1		/* a string is permissible here		*/
#define NOSTR	0		/* a string is not permissible here	*/
#define NOREL	1		/* relocation not permissible here	*/
#define RELOK	0		/* relocation is permissible here	*/

	/* Parsing constants and flags */

#define	IIAFLG	020000		/* alternate left parts flag		*/
#define	IIDSYM	0377		/* default symbol in scntab		*/
#define	IIESYM	0376		/* error symbol in scntab		*/
#define	IILMSK	0007		/* production length mask		*/
#define	IINFLG	0400		/* nonterminal symbol flag		*/
#define	IIRFLG	040000		/* read and reduce flag			*/
#define	IIXFLG	0100000		/* transition flag			*/

	/* Input stack frame types (in in_typ) */

#define	INFILE	1		/* file					*/
#define	INMAC	2		/* macro expansion			*/
#define	INRPT	3		/* repeat				*/
#define	INARG	4		/* macro argument			*/

	/* Operand classes (these are very hard to change) */

#define	OCNULL	0x00		/* no operand */
#define	OCNEX	0x01		/* nn					*/
#define	OCPEX	0x02		/* (nn)					*/
#define	OCEXP	0x03		/* {nn,(nn)}				*/
#define OCSTR	0x04		/* "string"				*/

	/* Operand flags (in op_flg and ps_flg) */

#define	OFFOR	0x8000		/* operand contains a forward reference	*/

	/* Opcode types (in oc_typ) */

#define	OTUND	0		/* undefined				*/
#define	OTINS	1		/* instruction				*/
#define	OTDIR	2		/* directive				*/
#define	OTMAC	3		/* macro				*/

	/* Symbol attribute flags (in sy_atr) */

#define	SADP2	0001		/* defined in pass 2			*/
#define	SAMUD	0002		/* multiply defined			*/
#define	SAGLO	0004		/* global				*/
#define	SAREF	0010		/* was referenced			*/

	/* Symbol types (in sy_typ) */

#define	STUND	0		/* undefined				*/
#define	STKEY	1		/* keyword				*/
#define	STSEC	2		/* section				*/
#define	STLAB	3		/* label or equs			*/
#define	STVAR	4		/* variable (redefinable)		*/
#define STGRP	5		/* group symbol				*/
#define STKEQ	6		/* equivalenced to key			*/
#define STNLAB	7		/* numeric label			*/

	/* Token types and values returned from the lexical scanner */

#define	TKEOF	0		/* end of file -- must be 0		*/
#define	TKDOL	1		/* dollar sign				*/
#define	TKSYM	2		/* symbol				*/
#define	TKCON	3		/* constant				*/
#define	TKSTR	4		/* quoted string			*/
#define	TKUNOP	5		/* unary operator			*/
#define	TKMULOP	6		/* multiplicative operator		*/
#define		TVMUL	1		/* multiplication		*/
#define		TVDIV	2		/* division			*/
#define		TVMOD	3		/* modulo			*/
#define		TVSHL	4		/* shift left			*/
#define		TVSHR	5		/* shift right			*/
#define	TKADDOP	7		/* additive operator			*/
#define		TVADD	1		/* addition			*/
#define		TVSUB	2		/* subtraction			*/
#define	TKRELOP	8		/* relational operator			*/
#define		TVEQ	1		/* equal 			*/
#define		TVNE	2		/* not equal			*/
#define		TVLT	3		/* less than			*/
#define		TVGT	4		/* greater than			*/
#define		TVLE	5		/* less than or equal		*/
#define		TVGE	6		/* greater than or equal	*/
#define	TKANDOP	9		/* logical and operator			*/
#define	TKXOROP	10		/* exclusive or operator		*/
#define	TKOROP	11		/* inclusive or operator		*/
#define	TKLPAR	12		/* left parenthesis			*/
#define	TKRPAR	13		/* right parenthesis			*/
#define TKNLAB	14		/* nnn$					*/
#define	TKCOM	100		/* comma				*/
#define	TKCOLON	101		/* colon				*/
#define	TKEOL	102		/* end of line				*/
#define	TKSPC	103		/* white space				*/
#define	TKERR	127		/* erroneous token			*/

	/* Virtual memory buffer flags (in vm_flg) */

#define	VMDIR	0x8000		/* dirty bit				*/
#define VMMEM	0x4000		/* memory is allocated			*/
#define VMDSK	0x2000		/* disk block is allocated		*/
#define VMBLK	0x1fff		/* disk or mem block number		*/

	/* Cross reference masks (in xr_pl) */

#define	XRDEF	0100000		/* Symbol defined at this reference	*/

	/* Character classification flag bits */

#define	D	01		/* Digit				*/
#define	I	02		/* First character of identifier	*/
#define L	04		/* Leading character only		*/
#define N	010		/* Any but leading character		*/

	/* Shorthand definitions */


	/* Pseudo-variables */

GLOBL FILE	*LIST IZ;	/* pointer to listing output structures	*/
GLOBL FILE	*OBJECT IZ;	/* pointer to object output structure	*/
GLOBL FILE	*ERRFIL	IZ;	/* error file				*/

	/* Pseudo-functions */

#define scanc()		((ch = *scanpt++)!='\0'&&ch!=escchr ? ch : xscanc())
#define	unscanc()	(ch = *--scanpt)
#define white(c)	(c==' ' || c=='\t')
#define eof(c)		((c & 0xff) == 0xff)

	/* Structure declarations */


#define CHENT	struct chent
#define SECTION struct section
#define PSFRAME struct psframe
#define OPERAND struct operand
#define OCTAB	struct octab
#define INPUT	struct input
#define SYTAB	struct sytab
#define NUMLAB	struct numlab
#define NUMCHN	struct numchn
#define XREF	struct xref
#define VLSIZ	struct varinstr
#define ERF	struct errframe
#define USING	struct using
#define SZY	struct szytab
#define SZE	struct szyentry
#define MCH	struct mchain
#define GRP	struct grchain

GRP {
	VMADR	gr_lnk;			/* next item in group list	*/
	VMADR	gr_sym;			/* ptr to symbol		*/
};

MCH {
	VMADR	mc_lnk;			/* link to next macro def	*/
	VMADR	mc_def;			/* pass 1 definition		*/
	short	mc_arg;			/* number of arguments		*/
};

SZE {			/* one szymanski entry				*/
	long	sze_lc;			/* location of vli		*/
	uns	sze_flg;		/* flag bits			*/
	VMADR	sze_tgt;		/* symbol table for tgt		*/
};

#define SZENO	((BUFSIZ-sizeof(SZY *)-sizeof(short))/sizeof(SZE))

SZY {			/* table for szymanski work			*/
	SZY	*szy_lnk;		/* link to next table		*/
	short	szy_cnt;		/* number of entries		*/
	SZE	szy_sze[SZENO];		/* entries			*/
};

USING {
	uns	us_reg;			/* symbol entry for reg		*/
	uns	us_sect;		/* section number		*/
	long	us_off;			/* possible offset in sect	*/
};

ERF {				/* save for structures			*/
	char	*er_msg;		/* message string		*/
	int	er_par[2];		/* message parameters		*/
	char	er_flg;			/* warning or hard error flag	*/
	char	er_col;			/* column marker for error	*/
};

#define	ER_WRN	1		/* used in er_flg to make it a warning	*/

VLSIZ {			/* table for variable length instructions	*/
	short	vl_inc;			/* increment for this choice	*/
	short	vl_neg;			/* negative reach (in adu's)	*/
	short	vl_pos;			/* positive reach (in adu's)	*/
};

CHENT {			/* single-character token table entry		*/
	char	ch_chr;			/* character			*/
	char	ch_typ;			/* token type			*/
	char	ch_val;			/* token value			*/
};

INPUT {			/* input stack frame				*/
	INPUT	*in_ofp;		/* pointer to previous frame	*/
	char	in_typ;			/* frame type			*/
	char	in_rpt;			/* repeat count			*/
	char	in_lst;			/* listing level		*/
	union {
		struct {
			char	*ix_pt;	/* pointer to next character	*/
			int	ix_ct;	/* number of characters left	*/
		} in_xx;
		VMADR	ix_vmp;
	}	in_yy;
	int	in_fd;			/* file descriptor		*/
	uns	in_seq;			/* line number			*/
	char	*in_fname;		/* input file name		*/
	char	in_buf[BUFSIZ];		/* buffer area (variable size & use */
};
	/* define three more convenient names */

#define in_ptr in_yy.in_xx.ix_pt
#define in_cnt in_yy.in_xx.ix_ct
#define in_vmp in_yy.ix_vmp

OCTAB {			/* opcode table entry				*/
	OCTAB	*oc_lnk;		/* link to next entry in hash chain */
	VMADR	oc_val;			/* value of opcode		*/
	char	oc_typ;			/* type of opcode		*/
	char	oc_arg;			/* highest formal # for macro	*/
	char	oc_str[SYMSIZ];		/* opcode mnemonic string	*/
};

OPERAND {		/* operand descriptor				*/
	long	op_cls;			/* set of classes (bit vector)	*/
	long	op_val;			/* value of operand		*/
	uns	op_rel;			/* relocation of operand	*/
	int	op_flg;			/* flags			*/
	char	*op_ptr;		/* for error messages		*/
};

PSFRAME {		/* parse stack frame				*/
	short	*ps_state;		/* parse state			*/
	int	ps_sym;			/* lookahead symbol		*/
	long	ps_val0;		/* usually subexpression value	*/
	VMADR	ps_val1;		/* usually subexpr relocation	*/
	int	ps_flg;			/* flags			*/
};

SECTION {		/* section table entry				*/
	VMADR	se_sym;			/* symbol table pointer		*/
	long	se_loc;			/* location counter		*/
	char	se_aln;			/* alignment			*/
	char	se_ext;			/* extent			*/
	char	se_adu;			/* address unit, this sect	*/
	char	se_within;		/* within byte, this sect	*/
	ushort	se_atr;			/* attributes			*/

	/* note, only one byte of section attibute is sent to ulink, so
	   the top byte can be used for assembler private stuff - in
	   particular for the Z8000 it tells whether we are currently
	   doing segmented code or not.
	   We also use this to indicate that a section is a dummy sect */
};

#define SEATRSEG 0x8000			/* segment code is being generated */
#define SEATDUMY 0x4000			/* section is dummy		*/
#define SEATADDU (USMADU<<8)		/* addru byte			*/
#define SEATWITH (USMWTH<<8)		/* within byte			*/
#define SEATSLEN (USMLEN<<8)		/* section length present	*/

SYTAB {				/* symbol table entry			*/
	VMADR	sy_lnk;			/* link to next hash entry	*/
	VMADR	sy_xlk;			/* link to rear of xref chain	*/
	VMADR	sy_val;			/* value of symbol		*/
	uns	sy_rel;			/* relocation of symbol		*/
	char	sy_typ;			/* type of symbol		*/
	char	sy_atr;			/* attributes of symbol		*/
	char	sy_str[SYMSIZ];		/* symbol mnemonic string	*/
};

NUMLAB {			/* numeric label entry			*/
	VMADR	nm_lnk;			/* link to next entry		*/
	uns	nm_lab;			/* this numeric entry		*/
	long	nm_val;			/* value of symbol		*/
	uns	nm_rel;			/* relocation of symbol		*/
	char	nm_typ;			/* type of symbol		*/
	char	nm_atr;			/* attributes of symbol		*/
	/* NOTE this structure MUST be laid out like SYTAB */
};

NUMCHN {			/* chain of entries this local level	*/
	NUMCHN	*nc_lnk;		/* link to next chain		*/
	VMADR	nc_nm[NMCCNT];		/* chains at this level		*/
};

XREF {			/* cross reference entry			*/
	VMADR	xr_lnk;			/* circular link to next entry	*/
	int	xr_pl;			/* page and line number		*/
};

	/* Global variable declarations.  First arrays then scalars	*/


extern char	chclass[];		/* character classification table (in assy src) */
extern CHENT	chtab[];		/* single-character token table (in assy src) */
GLOBL char	date[12] IZ;		/* string containing date	*/
GLOBL char	datstr[26] IZ;		/* string containing date and time */
GLOBL GRP	grptab[8] IZ;		/* heads of group chains	*/
GLOBL PSFRAME	iips[IISIZ] IZ;		/* parsing stack		*/
GLOBL long	instk[INSIZ/sizeof(long)] IZ;	/* input stack		*/
GLOBL char	labstr[SYMSIZ+1] IZ;	/* label string			*/
GLOBL ERF	llerf[LLERX] IZ;	/* error message frames		*/
GLOBL char	llloc[LLLOC+4] IZ;	/* location field in listing line */
GLOBL char	llobj[LLOBJ+3] IZ;	/* object field in listing line */
GLOBL char	llseq[LLSEQ+2] IZ;	/* sequence field in listing line */
GLOBL char	llsrc[SLINSIZ+2] IZ;	/* source field in listing line */
GLOBL char	objbuf[OBJSIZ] IZ;	/* object block construction area */
GLOBL OCTAB	*ochtab[1<<OHSHLOG] IZ;	/* opcode hash table		*/
GLOBL char	opcstr[SYMSIZ+1] IZ;	/* opcode string		*/
GLOBL uns	pendrel IZ;		/* pending reloc		*/
GLOBL long	pendv IZ;		/* pending value		*/
GLOBL char	proctype[64] IZ;	/* processor type		*/
GLOBL char	savstr[STRSIZ+1] IZ;	/* string save area		*/
GLOBL SECTION	sectab[SECSIZ] IZ;	/* section table		*/
GLOBL char	sline[SLINSIZ+2] IZ;	/* buffer with current source line */
GLOBL VMADR	syhtab[1<<SHSHLOG] IZ;	/* symbol hash table		*/
GLOBL char	titl1[TITSIZ+1] IZ;	/* first title line		*/
GLOBL char	titl2[TITSIZ+1] IZ;	/* second title line		*/
GLOBL char	tokstr[STRSIZ+1] IZ;	/* string from token scanner	*/
GLOBL USING	ulist[ULXSIZ] IZ;	/* using table			*/

GLOBL char	aduspace IZ;		/* space in listing after adu	*/
GLOBL char	aduspc2 IZ;		/* auxiliary for aduspace	*/
GLOBL char	aopt IZ;		/* allow 'a' option		*/
GLOBL char	argchr IX('?');		/* macro argument designator char */
GLOBL VMADR	blklim IZ;		/* limit of this block		*/
GLOBL VMADR	blkend IZ;		/* max of current allocation	*/
GLOBL short	blklog IZ;		/* shift for blockssize		*/
GLOBL uns	bytaln IX(8);		/* alignment of bytes		*/
GLOBL uns	bytbit IX(8);		/* default bits per byte	*/
GLOBL int	ch IZ;			/* current character from scanc */
GLOBL uns	codaln IX(8);		/* code alignment		*/
GLOBL char	colreqd IZ;		/* colon required after label	*/
GLOBL uns	condlev IZ;		/* nesting level of conditionals */
GLOBL char	condlst IZ;		/* flag enabling list of skipped code */
GLOBL uns	curadu IZ;		/* current address unit		*/
GLOBL uns	curaln IZ;		/* alignment for current section */
GLOBL uns	curatr IZ;		/* attributes for current section */
GLOBL OCTAB	*curdef IZ;		/* current macro being defined	*/
GLOBL uns	curext IX(32);		/* extent for current section	*/
GLOBL uns	curline IZ;		/* current line number		*/
GLOBL long	curloc IZ;		/* current location counter value */
GLOBL uns	curlst IZ;		/* current listing level	*/
GLOBL OPERAND	curop IZ;		/* current operand information	*/
GLOBL uns	cursec IZ;		/* current section number	*/
GLOBL int	curxpl IZ;		/* current xref page and line	*/
GLOBL char	debug IZ;		/* debugging flag		*/
GLOBL uns	defadu IX(8);		/* default address unit		*/
GLOBL uns	deflev IZ;		/* macro definition nesting level */
GLOBL char	*dfltsec IZ;		/* default section name		*/
GLOBL short	dmysec IZ;		/* last dummy section number	*/
GLOBL char	eflg IZ;		/* expression error flag	*/
GLOBL short	equfld IZ;		/* set non-zero to right align	*/
GLOBL uns	errct IZ;		/* error count			*/
GLOBL uns	errlim IZ;		/* error limit if non-zero	*/
GLOBL uns	errnum IZ;		/* output error numbers if nz	*/
GLOBL char	escchr IX('\\');	/* escape character		*/
GLOBL short	globflg IZ;		/* label was followed by ::	*/
GLOBL short	grpx IZ;		/* number of groups		*/
GLOBL char	*hextab IX("0123456789abcdef");	/* hex translate	*/
GLOBL PSFRAME	iilexeme IZ;		/* info returned from lexical scanner */
GLOBL int	iilset IZ;		/* alternate left parts set	*/
GLOBL int	iilsym IZ;		/* new left part symbol		*/
GLOBL PSFRAME	*iipsp IX(&iips[0]);	/* parsing stack pointer	*/
GLOBL PSFRAME	*iipspl IX(&iips[0]);	/* parsing left end pointer	*/
GLOBL char	*inclpath[16] IZ;	/* include paths		*/
GLOBL short	inclev IZ;		/* included levels		*/
GLOBL short	inclx IZ;		/* number of include paths	*/
GLOBL INPUT	*infp IZ;		/* input frame pointer		*/
GLOBL char	*insp IX((char*)(&instk[0])); /* input stack pointer		*/
GLOBL VMADR	label IZ;		/* sytab pointer for statement label */
GLOBL char	*labptr IZ;		/* pointer to label		*/
GLOBL int	labtyp IZ;		/* label type STLAB or STNLAB	*/
GLOBL int	labval IZ;		/* numeric value of STNLAB	*/
GLOBL VMADR	lastsym IZ;		/* last symbol in expression	*/
GLOBL char	lbrchr IX('{');		/* left brace char for macro args */
GLOBL char	lflag IZ;		/* flag set if listing is generated */
GLOBL uns	linect IZ;		/* # of lines left on listing page */
GLOBL short	llerx IZ;		/* number of messages so far	*/
GLOBL char	llfull IZ;		/* flag indicating something to list */
GLOBL char	*llobt IX(&llobj[0]);	/* top of object field		*/
GLOBL char	*llobnd IX(&llobj[LLOBJ]); /* end of object field	*/
GLOBL short	lllocsiz IZ;		/* location field size		*/
GLOBL short	lllocspec IX(0x45);	/* default lllocsiz		*/
GLOBL long	lllocmask IX(0x0ffffL); /* default lllocmask		*/
	/* note: if the width of the field is greater than 5 characters
	   then lstfmt and errfmt (in uasmisc.c) will need to
	   be changed as well		*/
GLOBL long	lllocval IZ;		/* location field value		*/
GLOBL short	llpp IX(LLPP);		/* listing lines per page	*/
GLOBL char	*llseqfmt IZ;		/* sequence field format	*/
GLOBL int	llseqval IZ;		/* sequence field value		*/
GLOBL uns	lngaln IX(8);		/* alignment of longs		*/
GLOBL uns	lngbit IX(32);		/* default bits per byte	*/
GLOBL VMADR	mchead IZ;		/* head of macro definitions	*/
GLOBL VMADR	mctail IZ;		/* tail of macro definitions	*/
GLOBL char	mctchr IX('#');		/* macro expansion count character */
GLOBL uns	mexct IZ;		/* count of macro expansions	*/
GLOBL uns	mexlev IZ;		/* macro expansion depth	*/
GLOBL uns	minaln IZ;		/* minimum section alignment value */
GLOBL uns	mlist IZ;		/* if 1 macro stuff is listed	*/
GLOBL char	msbord IZ;		/* most sig byte first order	*/
GLOBL char	msblst IZ;		/* list most sig byte first	*/
GLOBL NUMCHN	*nchd IZ;		/* head of numeric chain	*/
GLOBL NUMCHN	*nctl IZ;		/* tail of numeric chain	*/
GLOBL char	noentry IZ;		/* don't make symbol entry	*/
GLOBL char	noobj IZ;		/* no object file written if set */
GLOBL long	nxtloc IZ;		/* next location for text output */
GLOBL uns	nxtsec IZ;		/* next section for text output	*/
GLOBL char	*objtop IX(&objbuf[0]);	/* top of text info in objbuf	*/
GLOBL char	objtyp IZ;		/* object block type being built */
GLOBL OCTAB	*opcode IZ;		/* octab pointer for statement opcode */
GLOBL char	*opcptr IZ;		/* pointer to opcode		*/
GLOBL uns	pagect IZ;		/* listing page number		*/
GLOBL char	parsing IZ;		/* flag indicating we are parsing */
GLOBL char	pass2 IZ;		/* flag indicating we are in pass 2 */
GLOBL short	pendbits IZ;		/* pending bits count		*/
GLOBL short	pgwd IX(999);		/* page width			*/
GLOBL char	*phylim IZ;		/* first unallocated memory location */
GLOBL char	*phytop IZ;		/* first unused memory location	*/
GLOBL int	prevsem IZ;		/* most recent semantic routine # */
GLOBL char	*prname IZ;		/* name of this assembler	*/
GLOBL char	rbrchr IX('}');		/* right brace char for macro args */
GLOBL char	reading IX(1);		/* flag set while reading input */
GLOBL char	*relbot IX(&objbuf[OBJSIZ]);/* ptr to reloc info in objbuf */
GLOBL char	**relmap IZ;		/* relocation map		*/
GLOBL short	resbits IZ;		/* bit count in residual field	*/
GLOBL short	resid IZ;		/* residual field not emitted	*/
GLOBL uns	rmarg IX(80);		/* listing right margin column	*/
GLOBL char	rptct IZ;		/* repeat cnt for current repeat def */
GLOBL uns	rptlev IZ;		/* repeat definition nesting level */
GLOBL uns	rptline IZ;		/* line were repeat started	*/
GLOBL VMADR	rptstr IZ;		/* start of repeat definition in vm */
GLOBL short	savlen IZ;		/* len of first string		*/
GLOBL short	sav2len IZ;		/* len of second string		*/
GLOBL char	*scanpt IX(&sline[0]);	/* pointer to next character in sline */
GLOBL uns	secct IX(1);		/* number of sections defined	*/
GLOBL short	secexpr IZ;		/* non-zero if sect names ok in exprs */
GLOBL char	*slbr IX(&sline[80]);	/* place to break lines in macros */
GLOBL char	srcfile[64] IZ;		/* source file name		*/
GLOBL short	sufreqd IZ;		/* proper suffix is required	*/
GLOBL char	*sysparm IZ;		/* parameter for system		*/
GLOBL SZY	*szyhead IZ;		/* head of szymanski chain	*/
GLOBL SZY	*szycur IZ;		/* current block in szymanski	*/
GLOBL char	timstr[10] IZ;		/* string containing time	*/
GLOBL char	*tokpt IZ;		/* last token pointer		*/
GLOBL char	*tokpt2 IZ;		/* next to last token pointer	*/
GLOBL int	toktyp IZ;		/* token type from token scanner */
GLOBL long	tokval IZ;		/* value from token scanner	*/
GLOBL uns	truelev IZ;		/* true conditional nesting level */
GLOBL char	uext IZ;		/* turn undef. syms into externals */
GLOBL char	uflg IZ;		/* undefined symbol in expr flag */
GLOBL uns	urabyte IZ;		/* byte relocation action	*/
GLOBL uns	uralong IZ;		/* long relocation action	*/
GLOBL uns	uraword	IZ;		/* word relocation action	*/
GLOBL char	upperonly IZ;		/* convert all to upper case	*/
GLOBL char	verbose IZ;		/* some want it talky		*/
GLOBL VMADR	virtop IX((VMADR)4);		/* first unused vm location	*/
#ifndef BIGMEM
GLOBL int	vmfd IZ;		/* virtual memory file descriptor */
GLOBL char	*vmr IZ;		/* vm read pointer		*/
GLOBL VMTAB	*vmw IZ;		/* vm write pointer		*/
GLOBL ushort	vmlrux IZ;		/* vm lru marker		*/
#endif // BIGMEM
GLOBL char	vmrq IZ;		/* vm memory request		*/
GLOBL uns	ulx IZ;			/* number of using entries	*/
GLOBL uns	warnct IZ;		/* warning count		*/
GLOBL uns	wrdaln IX(8);		/* alignment of words		*/
GLOBL uns	wrdbit IX(16);		/* default bits per byte	*/
GLOBL char	xflag IZ;		/* flag enabling cross referencing */
GLOBL char	xsline IZ;		/* xref by source line		*/

#ifdef	STATS
GLOBL uns	ashct IZ;		/* number of symbol lookaside hits */
GLOBL uns	chnct IZ;		/* total sytab links followed	*/
GLOBL char	stats IZ;		/* flag enabling statistics	*/
GLOBL uns	sylct IZ;		/* number of calls to sylook	*/
GLOBL uns	symct IZ;		/* number of symbols		*/
GLOBL uns	vmgct IZ;		/* number of vm accesses	*/
GLOBL uns	vmrct IZ;		/* number of vm disk reads	*/
GLOBL uns	vmwct IZ;		/* number of vm disk writes	*/
#endif // STATS

	/* Function declarations */

GLOBL uns	hash();
GLOBL OCTAB	*oclook();
GLOBL char	*palloc();
GLOBL INPUT	*pushin();
GLOBL char	*rindex();
GLOBL VMADR	sylook();
GLOBL VMADR	symerge();
#ifndef __GNUC__
GLOBL char	*sprintf(); // Conflicts with GCC <stdio.h> definition
GLOBL VMADR	valloc(uns size);
#endif // __GNUC__
#define NULLCA 	'\000'			/* NULL char value */

#endif // UAS_H_
