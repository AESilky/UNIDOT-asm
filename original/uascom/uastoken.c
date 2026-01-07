/************************************************************************
*									*
*	Copyright (C) 1982,1985, by Unidot, Inc.			*
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
*			uas.token.c - token scanner			*
*									*
************************************************************************/


static char rcsid[] =
"@(#)$Header: uastoken.c,v 6.15 90/08/16 10:22:23 rmm Rel $ uas token scanner";

#include "uas.h"
int notokerr = 0;		/* used in uas.c to avoid spurious error msg*/

/*
 * stresc - Processes a string escape.  This routine is called after the
 * escape character (normally '\') has been seen.  It scans off the remainder
 * of the escape and returns its value.
 */

stresc(){

	reg int	ct,
		val;

	scanc();
	if( '0' <= ch && ch <= '7' ){ /* an octal escape */
		val = 0;
		ct = 3;
		do {
			val = ( val << 3 )+ch- '0';
			scanc();
		} while( --ct > 0 && '0' <= ch && ch <= '7' );
		unscanc();
		return val;
	}
	switch( ch ){ /* a single-character escape */

	case 'b': case 'B': return '\b';

	case 'f': case 'F': return '\f';

	case 'n': case 'N': return '\n';

	case 'r': case 'R': return '\r';

	case 't': case 'T': return '\t';

	case 'x': case 'X':	/* hex of form \xnn */
		val = hexch();
		if( val < 0 ){
			error( "32 Character following x not a hex digit");
			return 0;
		}
		ct = hexch();
		if( ct >= 0 ) val = (val << 4) | ct;
		return val;
	}

	return ch;

}

hexch(){

	scanc();
	if( ch >= '0' && ch <= '9' ) return ch - '0';
	if( ch >= 'a' && ch <= 'f' ) return ch - 'a' + 10;
	if( ch >= 'A' && ch <= 'F' ) return ch - 'A' + 10;
	unscanc();
	return -1;
}
/*
 * token - Scans the next token and returns its type.  Also sets up the
 * token's type and value information in the global cells toktyp, tokval,
 * and tokstr.
 */

token(){

	reg CHENT	*chp;
	reg char	*strp;
	reg int		c;
	reg int		radix;
	char		lch;
	char		*lscanpt;

	strp = tokstr;
	tokpt2 = tokpt;
	lscanpt = tokpt = scanpt;	/* save for error print		*/
	scanc();
	if( ch < 0 ) return toktyp = TKEOF;
	if( chclass[ch] & (I|D|L) ){			/* symbol */
moreid:		*strp++ = ch;
		while( chclass[*strp++ = c = *scanpt++] & (I|D|N) );
		ch = c;
		if( ch == 0 || ch == escchr ){
			lscanpt = scanpt-1;
			xscanc();
			if( chclass[ch] & (I|D|N) ) goto moreid;
		}
		*--strp = 0;
		unscanc();
		tokpt = scanpt-1;
		if( tokpt < lscanpt ) tokpt = lscanpt;
		if( chclass[tokstr[0]] & D ){	/* constant */
			c = *--strp;
			radix = 10;
			toktyp = TKCON;
			if( tokstr[0] == '0' &&
			     (tokstr[1] == 'x' || tokstr[1] == 'X')){
				strp = tokstr+2;
				radix = 16;
			} else {
				if( tokstr[0] == '0' ) radix = 8;
				if( c < '0' || c > '9' ){
					switch( c ){

				case '$': radix = 10; toktyp = TKNLAB; break;

				case 'B':
				case 'b': radix = 2; break;

				case '.':
				case 'D':
				case 'd': radix = 10; break;

				case 'H':
				case 'h': radix = 16; break;

				case 'O': case 'Q':
				case 'o': case 'q': radix = 8; break;

				default: goto badcon;
					}
					*strp = '\0';
				}
				strp = tokstr;
			}
#ifdef MEC
numlabscn:
#endif
			tokval = 0;
			while( c = *strp++ ){
				if( '0' <= c && c <= '9' )	c -= '0';
				else if( 'A' <= c && c <= 'F' )	c += 10 - 'A';
				else if( 'a' <= c && c <= 'f' )	c += 10 - 'a';
				if( c < 0 || c >= radix ) goto badcon;
				tokval = tokval * radix + c;
			}
			BDEB(5,("token found con <%lx>\n",tokval));
			return toktyp;
badcon:			if( !notokerr ) error("83 illegal constant");
			goto reterr;
		}
		tokstr[SYMSIZ] = 0;
#ifdef MEC
		if( tokstr[0] == '$' && strp-tokstr > 1 &&
		    chclass[tokstr[1]] & D ){	/* constant */
			strp = tokstr+1;
			toktyp = TKNLAB;
			radix = 10;
			goto numlabscn;
		}
#endif
		if( tokstr[0] == '.' && strp - tokstr == 2 ){
			ch = (tokstr[1] | 0x80) & 0xff;
			lch = ch;
			for( chp = chtab; chp->ch_chr; chp++ )
				if( lch == chp->ch_chr ) break;
			tokval = chp->ch_val;
			return toktyp = chp->ch_typ;
		}
#ifdef ONECASE
		if( upperonly ){
			strp = tokstr;
			while( ch = *strp++ )
				if( ch >= 'a' && ch <= 'z' )
					strp[-1] = ch + 'A' - 'a';
		}
#endif
		BDEB(5,("token found SYM <%s>\n",tokstr));
		return toktyp = TKSYM;
	}
	switch( ch ){

case ' ':
case '\t':			/* white space */
		tokpt = scanpt;
		do scanc(); while( white( ch ));
		if( ch == ',' ) goto cmscn;
		if( ch == ';' || ch == '\n' ) goto eol;		/* comment */
		unscanc();
		return toktyp = TKSPC;

case ',':
cmscn:		do scanc(); while( white(ch) );	/* skip trailing white */
		unscanc();
		return toktyp = TKCOM;

case ';':
case '\n':					/* comment or end of line */
eol:		while( ch != '\n' ) scanc();
		scanpt[-1] = '\n';
		*scanpt = 0;
		return toktyp = TKEOL;

case '\'':					/* character constant	*/
		scanc();
		if( ch == '\n' ){
			unscanc();
			goto reterr;
		}
		tokval = (ch == escchr) ? stresc() : ch;
		scanc();
		if( ch != '\'' ){
			unscanc();
			goto reterr;
		}
		tokpt = scanpt-1;
		return toktyp = TKCON;

case '"':					/* quoted string	*/
		scanc();
		while( ch != '"' ){
			if( ch == '\n' ){
				unscanc();
				goto reterr;
			}
			if( ch == escchr ) ch = stresc();
			if( strp < tokstr+STRSIZ ) *strp++ = ch;
			scanc();
		}
		*strp = '\0';
		tokval = strp-tokstr;
		tokpt = scanpt-1;
	BDEB(0,("string: <%s>",tokstr));
		return toktyp = TKSTR;

case '<':			/* <, <<, or <= */
		scanc();
		if( ch == '<' ){
			tokval = TVSHL;
			return toktyp = TKMULOP;
		}
		if( ch == '=' ){
			tokval = TVLE;
			return toktyp = TKRELOP;
		}
		unscanc();
		tokpt = scanpt-1;
		tokval = TVLT;
		return toktyp = TKRELOP;

case '>':			/* >, >>, or >= */
		scanc();
		if( ch == '>' ){
			tokval = TVSHR;
			return toktyp = TKMULOP;
		}
		if( ch == '=' ){
			tokval = TVGE;
			return toktyp = TKRELOP;
		}
		unscanc();
		tokpt = scanpt-1;
		tokval = TVGT;
		return toktyp = TKRELOP;

case '=':			/* = or == */
		scanc();
		if( ch != '=' ) unscanc();
		tokval = TVEQ;
		tokpt = scanpt-1;
		return toktyp = TKRELOP;

case '!':				/* ! or != */
		scanc();
		if( ch == '=' ){
			tokval = TVNE;
			tokpt = scanpt-1;
			return toktyp = TKRELOP;
		}
		unscanc();
		goto reterr;
	}

	/* single character token */

	lch = ch;		/* avoid sign extension problems	*/
	for( chp = chtab; chp->ch_chr; chp++ ) if( lch == chp->ch_chr ) break;
	tokval = chp->ch_val;
	toktyp = chp->ch_typ;
	return toktyp;

	/* error return	*/

reterr:	tokpt = scanpt;
	if( notokerr ) return toktyp = TKSPC;
	return toktyp = TKERR;
}
/*
 * xscanc - Takes care of the cases of character scanning which would be
 * difficult for the macro scanc() to handle.  Namely, these are end of line
 * processing and continuation processing.
 */


xscanc(){

top:
	if( ch == '\0' ){
		getline();
		ch = *scanpt++;
		goto top;
	}
	if( ch == escchr && *scanpt == '\n' ){ /* continuation */
		scanpt++;
		putline();
		ch = *scanpt++;
		goto top;
	}
	return ch;
}
