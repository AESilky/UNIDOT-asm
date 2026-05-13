/************************************************************************
*									*
*	Copyright (C) 2026, AESilky					*
*									*
*									*
*************************************************************************/

/************************************************************************
*									*
*			Binary Image Stacker/Creator			*
*									*
*			imager.c - main imager module			*
*									*
************************************************************************/


static char rcsid[] =
"@(#)$Header: imager.c, 2026 main imager module";

#define VARS 1			/* put the variables in this module */
#define IMGCNT	200		/* max number of files to process for image */

#include <stdarg.h> 		/* For va_arg */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef msdos
int	_iomode = 1;
#endif

#include "imager.h"

char	locfname[] = "ulcXXXXXX";	/* last 6 chars must be 'X' for `mkstemp` */
short	filex;
short	singlecol;
short	wrnex = WRNEXIT;
char	nil[2];			/* empty string	*/
char	*files[IMGCNT];
long	curtime;
char	datstr[32];
char	date[16];
char	timstr[16];

/* Declarations (local) */

void copymsg(FILE* f);
void error(char* s, ...);
void fclean(long pos, int length);	/* Fill the image file from pos for length	*/
void flushclos(FILE* f);
void imgspec(char* imgspecf);	/* process an imgage spec file 				*/
void ifinish();			/* finish up the image output file 			*/
void init(int argc, char** argv);
char* lastcomp();		/* Return pointer to last component			*/
void map();			/* Create Section Map file 				*/
void nocreat(char* s);		/* cannot create file 					*/
void noread(char* s);		/* cannot read file 					*/
void page();
void quit(int status);
long scanaddr(char* s);		/* Convert a hex string to an address value		*/
void secproc(int sn);		/* Process the specified section into the image		*/
void secsdef(int filex, char** files);	/* Define sections				*/
void secsproc();		/* Process sections into the image			*/
bool sexst(char* s);		/* Section exists? 					*/
section_t* selook(char* s);	/* Return the section for 'name'. Create if needed.	*/
void usage(int n);		/* n is Exit Code					*/


void intr(){  fprintf(stderr,"INTERRUPT!\n"); quit( FATEXIT ); }
/*		Here starts imager			*/


int main(int argc, char* argv[], char* env[]) {

#ifdef msdos
	argc = argsetup( argv, 1000, &argv, argv[0] );
	signal( SIGINT, intr ); /* interrupt            */
#endif
#ifndef NOTUNIX
	if( signal( SIGINT, SIG_IGN ) == SIG_DFL )
		signal( SIGINT, intr );		/* interrupt		*/
	if( signal( SIGQUIT, SIG_IGN ) == SIG_DFL )
		signal( SIGQUIT, intr );	/* quit			*/
	signal( SIGTERM, intr );		/* terminate		*/
	signal( SIGHUP, SIG_IGN );		/* hangup		*/
#endif

	time(&curtime );
	strcpy( datstr, ctime(&curtime ));
	strncpy( timstr, datstr+11, 8);
	strncpy( date, datstr+4, 7 );
	strncpy( date+7, datstr+20, 4 );
#ifdef STATS
	inbuf = palloc( BUFSIZ, BUFUSE );
#else
	inbuf = palloc( BUFSIZ );
#endif
	prname = lastcomp( argv[0] );
	if( argc <= 2) usage(1);		/* Need Image-File and at least one Source */
	if (*argv[1] == '-') error("F80 Image file not specified");	/* Need Image-File before options */
	imgfile = argv[1];
	init( argc-2, argv+2 );
	if( filex == 0 ) usage(2);
	if( verbose ) copymsg(stderr);
	IMGOUT = fopen( imgfile, "w" );
	if( IMGOUT == NULL ) nocreat( imgfile );
	fpos = 0;
#ifdef STATS
	setbuf( IMGOUT, palloc(BUFSIZ,BUFUSE));
#else
	setbuf( IMGOUT, palloc(BUFSIZ));
#endif
	if( mflag ){
		LIST = fopen( mapfile, "w" );
		if( LIST == NULL ) nocreat( mapfile );
#ifdef msdos
		setmode(fileno(LIST),"text");
#endif
#ifdef STATS
		setbuf( LIST, palloc(BUFSIZ,BUFUSE));
#else
		setbuf( LIST, palloc(BUFSIZ));
#endif
	}
	DEBOUT(0, ("sections definitions:\n"));
#ifdef STATS
#ifdef DEBUG
prstats("creating sections");
#endif
#endif
	secsdef( filex, files );
#ifdef STATS
#ifdef DEBUG
prstats("generate");
#endif
#endif
	DEBOUT(0, ("process sections : \n"));
	secsproc();
	ifinish();
	if( verbose ) printf("end of imging\n");
	flushclos( IMGOUT ); IMGOUT=(FILE*)0;
	if( mflag ) map();
	if( errct ){
		printf("%4d imager warnings\n", warnct);
		printf( "%4d imager errors\n", errct );
		quit( BADEXIT );
	}
	if( verbose | warnct) printf("%4d imager warnings\n  no imager errors\n", warnct);
	quit( warnct ? wrnex : GOODEXIT );
}

/*
 * init - Initialization processing.
 */

void init(int argc, char** argv) {

	char	*ap;
	int	i;

	for( i=0; i<argc; i++ ){
		ap = argv[i];
		if( *ap != '-' ){
			if( filex >= IMGCNT ) error("F07 too many source files");
			files[filex++] = ap;
			continue;
		}

		/* read command line switches */

		ap++;
		while( *ap ) switch( *ap++ ){

		case 'B':
		case 'b': 
			  if (*ap) {
				fillb = scanbyte(ap);	/* fill byte specified */
				ap = nil;
			  }
			  else {
				error("F06 Fill byte not specified");
			  }
			  continue;

		case '0': if( *ap != 'd' ) usage(3);
			  while( *ap++ == 'd' ) debug++;
			  verbose++;		/* also, be verbose */
			  ap--;
			  continue;

		case 'F':
		case 'f': imgspec( *ap ? ap : argv[++i] ); /* img file */
			  ap = nil;
			  break;

		case 'G':
		case 'g': wrnex = GOODEXIT;
			  break;

		case 'M':
		case 'm': mflag = 1;
			  if( *ap == 0 ) continue;
			  if( *ap == '=' ) ap++;
			  if( mapfile ) error("F08 Too many mapfile names");
			  mapfile = ap;
			  ap = nil;
			  break;

		case 'V':
		case 'v': verbose++;		/* be talky	*/
			  continue;

		default:  usage(4);		

		}
	}
	if( i > argc ) usage(5);
}

void imgspec( char *imgspecf ) {	/* process an imgage spec file */

	char		*ap;
	int		i;
	int		ax;
	FILE		*IMGSPECF;
	char		tmp[128];
#define IMGSIZ	128
	char		*aav[IMGSIZ];

	if( imgspecf == 0 ) usage(6);
	if( *imgspecf == '=' ) imgspecf++;
	IMGSPECF = fopen(imgspecf,"r");
#ifdef msdos
	setmode(fileno(IMGSPECF),"text");
#endif
	if( IMGSPECF == NULL ) noread(imgspecf);
	setbuf(IMGSPECF,0);
	ax = 0;
	for(;;){
		i = getc(IMGSPECF);
		if( i == ';' ) while( i != EOF && i != 032 && i != '\n' )
					i = getc(IMGSPECF);
		if( i == EOF || i == 032 ) break;
		if( i == ' ' || i == '\t' || i == '\n' ) continue;
		if( ax >= IMGSIZ ) error("F04 too many imager specs");
		ap = tmp;
		while( i != ' ' && i != '\n' && i != '\t' && i != EOF ){
			*ap++ = i;
			i = getc(IMGSPECF);
		}
		*ap++ = 0;
#ifdef STATS
		aav[ax++] = ap = palloc( strlen(tmp)+1, OTHUSE );
#else
		aav[ax++] = ap = palloc( strlen(tmp)+1 );
#endif
		strcpy( ap, tmp );
	}
	aav[ax] = NULLCA;
	flushclos( IMGSPECF ); IMGSPECF=(FILE*)0;
	init( ax, aav );		/* recurse */
}
/*

 * scanaddr - convert an address
 */
long scanaddr(char* s) {

	long	v;
	int	c;

	v = 0L;
	while( (c = *s++ ) != '\0' ){
		if( '0' <= c && c <= '9' ) c += 0- '0'; else
		if( 'a' <= c && c <= 'f' ) c += 10- 'a'; else
		if( 'A' <= c && c <= 'F' ) c += 10- 'A'; else
			error( "F05 Bad location address" );
		v = (v << 4) | c;
	}
	return v;
}

/*
 * scanbyte - convert a byte
 */
unsigned char scanbyte(char* s) {

	short	v;
	int	c;

	v = 0;
	while ((c = *s++) != '\0') {
		if ('0' <= c && c <= '9') c += 0 - '0'; else
			if ('a' <= c && c <= 'f') c += 10 - 'a'; else
				if ('A' <= c && c <= 'F') c += 10 - 'A'; else
					error("F11 Bad byte value");
		v = (v << 4) | c;
	}
	if (v > 0xff)
		error("F11 Bad byte value");
	return (unsigned char)v;
}


/*
 * secdef - define a single section from the name.
 * 	Name from the command line or spec file is in the form: name[:[start][:end]]
 * 	If `start` isn't specified the section follows the previous section.
 * 	If `end` isn't specified the end is defined by the beginning of the following section.
 */
void secdef(char* s) {

	char*		filepath;
	char* 		parts[3];
	int		partno = 0;
	int		i;
	size_t		tlen = strlen(s);
	size_t		plen;
	char		secspec[tlen+2];
	section_t*	sec;

	strcpy(secspec, s);
	for (i=0; i<3; i++) parts[i] = ""; // Start with 3 empty strings

	parts[partno++] = secspec;
	for (i=0; i<tlen; i++) {
		if (secspec[i] == ':') { /* next part */
			if (partno > 2)
				error("F10 source (section) specifier has too many parts");
			secspec[i] = '\000';
			parts[partno++] = &secspec[i+1];
		}
	}
	// A secspec that looked like: "name:1:2" now looks like "name""1""2"
	// and parts is: parts[0]="name" parts[1]="1" parts[2]="2"
	filepath = parts[0];
	plen = strlen(filepath)+1;
	parts[0] = lastcomp(filepath);
	// Make sure we have a name
	if (strlen(parts[0]) < 1)
		error("F12 source (section) name is empty");
	// We have what we need, create a section
	sec = (section_t*)zpalloc(sizeof(section_t)+plen+1);
	strcpy(sec->name, parts[0]);
	strcpy(sec->path, filepath);
	sec->plen = plen;
	// Set start
	if (*parts[1]=='\0') {
		// Start not specified - use end of previous section
		sec->se_start = (0-stct); // If this is the 1st one the value will be 0
	}
	else {
		sec->se_start = scanaddr(parts[1]);
	}
	// Set end
	if (*parts[2] != '\0') {
		sec->se_maxend = scanaddr(parts[2]);
	}
	sectab[stct] = sec;
	sec->se_num = ++stct;

	return;
}

/*
 * secsdef - define the sections from the names.
 * 
 * Called once to define all the sections (no sections should be defined before this is called).
 */
void secsdef(int filex, char** files) {
	for (int i = 0; i < filex; i++) secdef(files[i]);
}

/*
 * secproc - process the contents of a section into the image.
 */
void secproc(int sn) {
	section_t* secp;
	section_t* snxt;
	int		i;
	int		j;
	long		l;
	long		maxend;
	bool		reof = false;


	secp = sectab[sn];		/* get the section		*/
	strcpy(cursrc, secp->name);	/* source name */
	secp->se_fpos = 0;		/* 0 file pos			*/

	// Fill as needed to the specified start point
	if (secp->se_start >= 0) {
		l = secp->se_start - fpos;
		if (l < 0) {
			error("W10 Specified start before current image position. Specified:%06X Current:%06X", secp->se_start, fpos);
			goto _finally;
		}
		if (l > 0) fclean(fpos, l);
	}
	// Get the max end point
	if (secp->se_maxend > 0) {
		maxend = secp->se_maxend;
	}
	else {
		// The max end is determined by the start of the next section
		maxend = 0; 	// start with no specified max
		j = 1;		// amount to back off from following sections
		for (i=sn+1; i<stct; i++) {
			snxt = sectab[i];
			if (snxt->se_start >= 0) {
				maxend = snxt->se_start - j;
				break;
			}
			else if (snxt->se_maxend > 0) {
				maxend = snxt->se_maxend - j;
				break;
			}
			j++;
		}
	}
	if (verbose) {printf("Processing %s - Start:%06X", cursrc, fpos); if (maxend > 0) printf(" Max:%06X", maxend); printf("\n");}
	l = fpos;
	// Copy the section source into the image, stopping by section max end
	secp->se_loc = fpos;
	if ((SRCIN = fopen(secp->path, "r")) == NULL) {
		error("45 Cannot open %s", secp->path);
		return;
	}
#ifdef STATS
	if (inbuf == NULL) inbuf = palloc(BUFSIZ, BUFUSE);
#else
	if (inbuf == NULL) inbuf = palloc(BUFSIZ);
#endif
	setbuf(SRCIN, inbuf);

	while (maxend == 0 || fpos <= maxend) {
		if ((i = getc(SRCIN)) == EOF) {
			reof = true;
			break;
		}
		putc(i, IMGOUT);
		fpos++;
		secp->se_fpos++;
	}
	if (!reof) {
		// Didn't reach the end of the source file before the specified end
		error("W11 Truncated at length %d. Start:%06X Spec-End:%06X Current:%06X", secp->se_fpos, l, maxend, fpos);
	}
	// The image contents are written up to fpos, move the 'hiwater' for fills
	hiwater = fpos;
_finally:
	fflush(IMGOUT);
	cursrc[0] = NULLCA;
	if (SRCIN) fclose(SRCIN);
	SRCIN = (FILE*)0;
}

/*
 * secsproc - process the sections into the image.
 *
 * Called once to read the contents of each section and write it to the appropriate location in the output image.
 */
void secsproc() {
	for (int i = 0; i < stct; i++) secproc(i);
}

/*
 * sexst - Return true if the section exists.
 */

bool sexst(char* s) {
	for (int i = 0; i < stct; i++) {
		if (strcmp(s, sectab[i]->name) == 0) {
			return true;
		}
	}
	return false;
}

/*
 * selook - Returns a pointer to the section table entry for the
 * specified section, or NULL.
 */

section_t* selook(char* s) {

	size_t alloclen;
	size_t namelen;
	section_t* secp;

	// See if it already exists
	for (int i=0; i<stct; i++) {
		if (strcmp(s, sectab[i]->name) == 0) {
			return sectab[i];
		}
	}
	return NULL;
}

void psect(section_t* secp) {	/* print a section entry	*/

	section_t	*sep2;
	int		i = 0;

#define SE_PRINTED 1
top:	if( linect < 1 ) page();
	fprintf( LIST," %10s %8lx %8lx\n",
		secp->name,
		secp->se_start,
		secp->se_maxend);
	linect--;
}

void bigspace(int n) {		/* output big space between sections */

	if( linect < n || linect < 3 ){
		page();
	} else {
		linect -= 3;
		fprintf(LIST,"\n\n\n");
	}
}

/*
 * map - Outputs the load map.
 */

void map(){

	int		i;
	section_t	*secp;
	long		curtime;
	char		datstr[64];
	char		timstr[16];
	char		date[16];


	time(&curtime );
	strcpy( datstr, ctime(&curtime ));
	strncpy( timstr, datstr+11, 8);
	strncpy( date, datstr+4, 7 );
	strncpy( date+7, datstr+20, 4 );

	/* List information about the sections.  */

	pghead =
"Section                             Start      End";
	for( i = 0; i < stct; i++ ) psect( sectab[i] );

	fflush( LIST );
	if( ferror(LIST) ) error("F93 listing file write error");
}

void page(){

	fprintf( LIST,
		"\f\n\n%-16s    %14s  %8s%-*sPage%4d\n\n%s\n\n",
			prname,date,timstr,28, " ", ++pagect, pghead );
	linect = 56;
}

void ifinish() {		/* finish up the image output file */

	int		i;

	DEBOUT(0, ("ifin fpos = %ld\n", fpos));
	fseek(IMGOUT, fpos, SEEK_SET);	/* move to end of file */
	fclean(fpos, 0);
}

void fclean(long pos, int length) {

	if(pos + length > hiwater){
		fseek(IMGOUT, hiwater, SEEK_SET);
		DEBOUT(0, ("fclean fill (%02X): %ld\n", fillb, pos - hiwater));
		while( hiwater < pos + length ){
			putc( fillb, IMGOUT );
			hiwater++;
		}
		fpos = hiwater;
		DEBOUT(0, ("fclean: pos:%ld + len:%d = hiwater:%ld\n", pos, length, hiwater));
	}
}


/*
 * palloc - Allocates a block of physical memory of the specified size,
 * and returns a pointer to the block.  The block is guaranteed to be
 * aligned to the coarsest boundary which might be required.
 */

#ifdef USEVM
#ifdef STATS
char* palloc(uns size, int which) {
#else
char* palloc(uns size) {
#endif


	static char* phytop;
	static char* phylim;
	char* oldtop;
	char* tmp;
	int		i;
//	extern char	*sbrk();

	size = (size + (ALIGN - 1)) & -ALIGN;
#ifdef STATS
	usestats[which] += size;
#endif
	if (phytop == 0) phytop = phylim = sbrk(0);
	oldtop = phytop;
	phytop += size;
	i = 8192;
	while (phytop > phylim) {
	again:		tmp = sbrk(i);
		if (tmp == (char*)0 || tmp == (char*)-1) {
			i >>= 1;
			if (i < 512) error("F01 out of memory");
			goto again;
		}
		if (tmp != phylim) {	/* not contiguous */
			oldtop = phylim = tmp;
			phytop = oldtop + size;
		}
		phylim += i;
	}
	return oldtop;
}
#else
#ifdef STATS
char* palloc(uns size, int which) {
	usestats[which] += size;
#else
char* palloc(uns size) {
#endif // STATS
	return ((char*)aligned_alloc(VMALIGN, size));
}
#endif // USEVM

/*
 * zpalloc() allocates space and guarantees that it is zero
 */
#ifdef STATS
char* zpalloc(uns size, int which) {
#else
char* zpalloc(uns size) {
#endif

	char* p, * q, * r;

#ifdef STATS
	p = q = palloc(size, which);
#else
	p = q = palloc(size);
#endif
	for (r = q + size; q < r; q++) *q = 0;
	return p;
}

/*
 * lastcomp - Returns a pointer to the last component of a path name
 */

char* lastcomp(char* s) {

	int	c;
	char* r;

	r = s;
	while (c = *s++)
		if (c == '/' || c == '\\' || c == ':' || c == ']')
			r = s;
	return r;
}


/*
 * usage - Show the options. Issues a fatal error message for a command line error.
 */
void usage(int n) {

	copymsg(stdout);
	printf("Usage: %s image_file_name [-option].. source1[:start1[:end1]] [[sourceN[:startN[:endN]]]]\n", prname);
	printf(" options are:\n");
	printf("	-bnn		Use nn (hex) as fill byte (default is FF)\n");
	printf("	-f<file>	<file> is an imager control file\n");
	printf("	-v		verbose option\n");
	printf("        -0d[d...]       set debug level to 'd' count\n");
	quit(n);
}

void copymsg(FILE* f) {
	fprintf(f, "imager Version 0.1, Copyright 2026 AESilky\n");
}

/*
 * quit - Exits with the specified status, after closing files.
 */

void quit(int n){

	if( mflag && LIST ) {fclose( LIST ); LIST=(FILE*)0;}
	if (IMGOUT) { fclose(IMGOUT); IMGOUT = (FILE*)0; }
	fclose( stdout );
	if( (n == BADEXIT || n == FATEXIT) && imgfile ){
		if( verbose ) printf("removing %s\n",imgfile);
		remove(imgfile);
		imgfile = (char*)0;
	}
#ifdef STATS
	if( verbose || debug || n == BADEXIT || n == FATEXIT ) prstats("at quit");
#endif
	exit(n);
}

#ifdef STATS
void prstats(char* s) {

	printf("memory stats %s  (sp = %x)\n",s,&s);
	printf("%6ld bytes used for buffers\n",usestats[BUFUSE]);
	printf("%6ld bytes used for other items\n",usestats[OTHUSE]);
}
#endif

void flushclos(FILE* f) {
	fflush( f );
	if( ferror(f) ) error("F93 file write error");
	fclose( f );
}

/*
 * error - prints an error message and quits if fatal
 */

void error(char* s, ...) {

	int		flag;
	char* p;
	char		errno[6];

	flag = *s;
	if (flag == 'F') {
		s++;
		printf("FATAL: ");
	}
	else
		if (flag == 'W') {
			s++;
			printf("WARNING: ");
		}
		else
			printf("ERROR: ");
	p = errno;
	while (*s >= '0' && *s <= '9') *p++ = *s++;
	*p = 0;
	while (*s == ' ') s++;
	if (errno[0]) printf("(%s) ", errno);
	if (cursrc[0]) printf("source %s: ", cursrc);
	va_list argptr;
	va_start(argptr, s);
	vprintf(s, argptr);
	va_end(argptr);
	printf("\n");
	if (flag == 'F') quit(FATEXIT);
	if (flag != 'W') errct++; else warnct++;
}

void nocreat(char* s) {		/* cannot create file */

	error("F02 cannot create %s", s);
}

void noread(char* s) {		/* cannot read file */

	error("F03 file %s does not exist or is unreadable", s);
}
