/* Added to fix 'implicit declaration' warnings with GCC */
#ifdef __GNUC__
#ifndef FUNCDEFS_H_
#define FUNCDEFS_H_

extern void dopass(int filex, char** files);

extern void init(int argc, char* argv[]);

extern void intr();

extern void main(int argc, char* argv[], char* env[]);

extern void quit(int status);


#endif // FUNCDEFS_H_
#endif // __GNUC__
