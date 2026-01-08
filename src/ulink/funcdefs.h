/* Added to fix 'implicit declaration' warnings with GCC */
#ifdef __GNUC__
#ifndef FUNCDEFS_H_
#define FUNCDEFS_H_

extern void along(long l, FILE* file);

extern void aword(ushort w, FILE* file);

extern void dopass(int filex, char** files);

extern void error(char* s, ...);

extern void fclean(long pos, int length);

extern void init(int argc, char* argv[]);

extern void interlude();

extern void intr();

extern void locspec(char* s);

extern void main(int argc, char* argv[], char* env[]);

extern void nocreat(char* s);

extern void noread(char* s);

extern void object();

extern void obrlt();

extern char ofill(OBLOCK* obp, FILE* fp);

extern void oflush();

extern byte ogetb(OBLOCK* obp);

extern long olodl(char* p);

extern ushort olodw(char* p);

extern void oputb(uns b);

extern void oputl(long l);

extern void oputs(char* s);

extern void ostol(long l, char* p);

extern void ostow(uns w, char* p);

extern void outsym(int sect, long addr, char* str);

extern void quit(int status);

extern void relinit();

extern void sysort();

extern void treloc();


#endif // FUNCDEFS_H_
#endif // __GNUC__
