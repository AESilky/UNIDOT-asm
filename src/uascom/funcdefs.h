/* Added to fix 'implicit declaration' warnings with GCC */
#ifdef __GNUC__
#ifndef FUNCDEFS_H_
#define FUNCDEFS_H_

#include "uas.h"

extern void assign(uns typ, long val, uns rel);

extern void def1();

extern void delim();

extern void dircom(int dirnum);

extern void direc(int dirnum);

extern void dopass();

extern void emitau(long value, uns reloc, int bits);

extern void emitb(uns value, uns reloc);

extern void emitl(long value, uns reloc);

extern void emitstr(char* s, int n);

extern void emitv(long value, uns reloc, int bits);

extern void emitw(uns value, uns reloc);

extern void equ(int symtype);

extern void error(char* s, ...);

extern void fatal(char* s, ...);

extern void filchk(FILE* f, char* s);

extern void fillin();

extern void hexit(char* p, int n, long v);

extern int iilex();

extern int iiparse();

extern void immmsg(char* s1, char* s2);	/* immediate message */

extern int include(char* file);

extern void init(int argc, char* argv[]);

extern void instr(VMADR fmp);

extern void interlude();

extern void intr();

extern void iovck();

extern void laboc();

extern void lcalign(int n);

extern void lcassign();

void listau(int objf, long v, int bits);

extern void macro(VMADR vp);

extern void main(int argc, char* argv[], char* env[]);

//void memcpy(char* d, char* s, int n);

extern void mexprint();	/* routine is called for directives that should not be printed */

extern void newsec(int f); /* f is 0 for normal sections, 1 for dummy sections */

extern void nolabel();

extern void nopend();

extern VMADR numlab(unsigned n); /* find the relevant numeric label */

extern void oflush();

extern void oneed(int n);

extern void opcinsert(octab_t* o);

extern void oputb(char c);

extern void oputl(long l);

extern void oputrb(char c);

extern void oputs(char* s);

extern void oputw(uns w);

extern int opval(char* s);

extern void pgcheck();

extern void popin();

extern void predef();

extern void pushc(char c);

extern void putline();

extern void putxref();

extern void quit(int status);

extern int regcheck(sytab_t* syp);

extern void rpt1();

extern void setorg();

extern void setsec(uns sec);

extern void setvirtop(VMADR v);

extern void skip1();

extern void skipeol();

extern void stdequend(int symtype);

extern int symcmp(char* a, char *b);

extern void symcpy(char* d, char* s);

extern void szyprocess();

extern int token();

extern VMADR uavalloc(uns size);

extern void ugetline();

extern void valign();		/* align the virtual pointe	*/

extern void warn(char* s, ...);

extern void xref(VMADR sym, int type);

/* Symantic Handler Declarations */
void sem51(int sem);

#endif // FUNCDEFS_H_
#endif // __GNUC__
