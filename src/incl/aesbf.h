/************************************************************************/
/*									*/
/* AES B.F. Hammer (to use where needed - mostly going to/from VMADR) 	*/
/*									*/
/* This file is,							*/
/* Copyright 2026, AESilky						*/
/*									*/
/************************************************************************/

#ifndef AES_BF_H_
#define AES_BF_H_

typedef unsigned char byte;
typedef unsigned int uns;
typedef unsigned short ushort;
typedef unsigned long ulong;
typedef void* VMADR;

/* Specific structure value converters */
// OPCODE Value
#define OCVAL(n)	((VMADR)((ulong)(n)))
#define OCVAL_I(n)	((int)(n))
#define OCVAL_UL(n)	((ulong)(n))
#define OCVAL_UI(n)	((uns)(n))

// PARSE STACK FRAME Value
#define PSVAL1(n)	((VMADR)((ulong)(n)))
#define PSVAL1_I(n)	((int)(n))
#define PSVAL1_UL(n)	((ulong)(n))
#define PSVAL1_UI(n)	((uns)(n))

// SYMBOL TABLE Value
#define SYVAL(n)	((VMADR)((ulong)(n)))
#define SYVAL1_I(n)	((int)(n))
#define SYVAL1_UL(n)	((ulong)(n))
#define SYVAL1_UI(n)	((uns)(n))

#endif // AES_BF_H_
