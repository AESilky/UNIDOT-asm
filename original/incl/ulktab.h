
/*
 * now the traditional actions
 */

static char urnop[] = {0};
static char urseg[] = {96|1, 25, 64|4, 10, 5, 160|1, 0};
static char urtla[] = {96|31, 24, 5, 160|31, 0};
static char ura16m[] = {96|1, 24, 5, 160|1, 0};
static char urtsb[] = {96|15, 24, 23, 6, 64|4, 10, 5,
	64|1, 6, 160|15, 0};
static char urzof[] = {128|1, 24, 5, 192|1, 0};
static char urtsa[] = {96|15, 24, 5, 21, 0, 0x80, 12,
	21, 0, 0x7f, 17, 160|15, 0};
static char ura8[] = {96|0, 24, 5, 160|0, 0};
static char urtlac[] = {96|31, 13, 24, 5, 13, 160|31, 0};
static char urj11[] = {
	96|1,		/* get two bytes containing the item	*/
	2,			/* make a copy				*/
	2,			/* and another copy			*/
	20, 0xff,		/* low byte mask			*/
	7,			/* get low 8 bits			*/
	24,			/* load target address			*/
	5,			/* and add in low bits			*/
	1,			/* get full word on top again		*/
	64|5, 10,		/* shift top 3 bits to right place	*/
	21, 0, 0x7,		/* get mask for these bits		*/
	7,			/* and clean them out			*/
	5,			/* get final target address		*/
	2,			/* make a copy				*/
	23,			/* get source address			*/
	64|2, 5,		/* offset it by two			*/
	9,			/* need to check high bits		*/
	21, 0, 0xfe,		/* by cleaning out the bottom 11 bits	*/
	7,
	64|0, 64|0, 17, /* this is a range check		*/
	3,			/* discard the checking value		*/
	2,			/* dup the relocated address		*/
	64|5, 11,		/* put the high bits in right place	*/
	21, 0, 0xe0,		/* merge mask for high bits		*/
	19,
	21, 0xff, 0xe0,	/* merge mask for all 11 bits		*/
	19,
	192|1,		/* put it away				*/
	0};
static char urtsac[] = {96|15, 24, 64|4, 10, 5,
	21, 0, 0x80, 12, 21, 0, 0x7f, 17, 13, 160|15, 0};
static char urzss[] = {24, 64|16, 10, 192|0,
	64|1, 4, 128|0, 24, 5, 21, 0xff, 0xff,
	7, 64|0, 20, 0xff, 17, 192|0, 0};
static char urrel[] = {96|1, 24, 23, 6, 64|2, 6,
	5, 160|1, 0};
static char ura16[] = {96|1, 24, 5, 160|1, 0};
static char uroff[] = {96|1, 24, 25, 6, 5, 160|1, 0};
static char ura32m[] = {128|3, 24, 5, 192|3, 0};
static char ursrel[] = {96|0, 24, 23, 6, 64|1, 6, 5,
	160|0, 0};
static char ura32[] = {96|3, 24, 5, 160|3, 0};
static char ursoff[] = {96|0, 24, 25, 6, 5, 160|0, 0};
static char urzls[] = {24, 64|16, 10, 20, 0x80, 8, 192|0,
	64|2, 4, 128|1, 24, 5, 192|1, 0};
static char urzsg[] = {24, 64|8, 10, 21, 0, 0x7f, 7, 192|1,
	0};

char *uractions[] = {		/* standard table			*/
/* UR_NOP */	0,		/* no relocation operation		*/
/* -- */	0,
/* UR_SEG */	urseg,		/* 8086 segment relocation		*/
/* UR_TLA */	urtla,		/* relocate TMS34010 32 bit bit address	*/
/* UR_A16M */	ura16m,		/* add base to word field (MSB first)	*/
/* UR_TSB */	urtsb,		/* relocate TMS34010 16 bit branch tgt	*/
/* UR_ZOF */	urzof,		/* relocate Z8001 16-bit offset		*/
/* UR_TSA */	urtsa,		/* relocate TMS34010 16 bit bit address */
/* UR_A8 */	ura8,		/* add base to byte field		*/
/* UR_TLAC */	urtlac,		/* rel TMS30401 32 bit complemented	*/
/* UR_J11 */	urj11,		/* 8051 11-bit jump target relocation	*/
/* UR_TSAC */	urtsac,		/* rel TMS30401 16 bit complemented	*/
/* UR_ZSS */	urzss,		/* relocate Z8001 short seg address	*/
/* -- */	0,
/* UR_REL */	urrel,		/* 8086 relative offset   		*/
/* -- */	0,
/* UR_A16 */	ura16,		/* add base to word field		*/
/* -- */	0,
/* UR_OFF */	uroff,		/* 8086 offset relocation		*/
/* -- */	0,
/* UR_A32M */	ura32m,		/* add base to long field (MSB first)	*/
/* -- */	0,
/* UR_SREL */	ursrel,		/* 8086 relative offset short 		*/
/* -- */	0,
/* UR_A32 */	ura32,		/* add base to long field		*/
/* -- */	0,
/* UR_SOFF */	ursoff,		/* 8086 short offset relocation		*/
/* -- */	0,
/* UR_ZLS */	urzls,		/* relocate Z8001 long seg address	*/
/* -- */	0,
/* UR_ZSG */	urzsg		/* relocate Z8001 16-bit segment number */
};
