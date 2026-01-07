/************************************************************************/
/*									*/
/* AES B.F. Hammer (to use where needed - mostly going to/from VMADR) 	*/
/*									*/
/* I'm not going to put a Copyright on this.				*/
/*									*/
/************************************************************************/

#ifndef AES_BFH_H_
#define AES_BFH_H_

typedef unsigned int uns;
typedef unsigned short ushort;
typedef unsigned long ulong;
typedef void* VMADR;

#define INT2VMA(n)	((VMADR)n)
#define LONG2VMA(n)	((VMADR)n)
#define ULONG2VMA(n)	((VMADR)n)
#define US2VMA(n)	((VMADR)((ulong)n))
#define VMA2INT(n)	((int)n)
#define VMA2LONG(n)	((long)n)
#define VMA2ULONG(n)	((ulong)n)
#define VMA2UNS(n)	((uns)n)
#define VMA2US(n)	((ushort)n)

#endif // AES_BFH_H_
