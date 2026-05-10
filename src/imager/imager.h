/************************************************************************
*									*
*	Copyright 2026 AESilky						*
*									*
************************************************************************/
/************************************************************************
*									*
*			Imager - Image Builder				*
*									*
************************************************************************/
#ifndef IMAGER_H_
#define IMAGER_H_

/*
 * @(#)$Header: ulink.h,v 4.9 92/07/31 07:57:46 rmm Rel $ ulink header file
 *
 * Parameters.
 */

#include "../incl/aesbf.h"

#include <stdio.h>

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

/* maximum module name length */
#define	NAMSIZ	32		
/* maximum number of sections */
#define	SECSIZ	256		


#ifdef STATS
#define OTHUSE	4		/* space used for other		*/
#endif

typedef struct section_ {		/* section table entry				*/
	int	se_num;			/* section number				*/
	long	se_start;		/* start of section (<0 section to follow)	*/
	long	se_maxend;		/* maximum end location (0=No Max)		*/
	long	se_fpos;		/* file position when reading			*/
	long	se_loc;			/* current (end) location			*/
	char	name[NAMSIZ+1];		/* section name (file name)			*/
	int	plen;			/* length of file path + 1			*/
	char	path[];			/* section file path				*/
} section_t;


/*
 * Global variable declarations.
 */

/* file fill byte to use */
GLOBL unsigned char	fillb IX(0xff);	
/* current file name			*/
GLOBL char	*curfile IZ;	
/* old debug flag			*/
GLOBL short	debug IZ;	
/* count of errors			*/
GLOBL ushort	errct IZ;	
/* position in output file		*/
GLOBL long	fpos IZ;	
/* hiwater mark for file fills		*/
GLOBL long	hiwater IZ;
/* name of image (output) file		*/
GLOBL char* imgfile IZ;
/* input buffer pointer			*/
GLOBL char	*inbuf IZ;	
/* number of lines left on listing page */
GLOBL ushort	linect IZ;	
/* name of map file			*/
GLOBL char* mapfile IZ;
/* flag turning on load map listing	*/
GLOBL short	mflag IZ;
/* listing page number			*/
GLOBL ushort	pagect IZ;	
/* heading line for listing pages	*/
GLOBL char	*pghead IZ;	
/* program name from command line */
GLOBL char	*prname IX("ulink");	
/* processor type			*/
GLOBL char	proctype[64] IZ; 
/* number of entries in sectab		*/
GLOBL ushort	stct IX(0);
#ifdef STATS
/* number of stats		*/
GLOBL long	usestats[5];	
#endif
/* be talky				*/
GLOBL short	verbose IZ;	
/* count of warnings			*/
GLOBL ushort	warnct IZ;	

/* current source name		*/
GLOBL char	cursrc[NAMSIZ+1] IZ; 
/* section information table	*/
GLOBL section_t* sectab[SECSIZ] IZ;

/*
 * Function declarations.
 */

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
/* Read a Hex Byte from the string */
unsigned char scanbyte(char* s);


/*
 * Input/ Output variables
 */

GLOBL FILE	*IMGOUT IZ;
GLOBL FILE	*SRCIN IZ;
GLOBL FILE	*LIST IZ;

#endif // IMAGER_H_
