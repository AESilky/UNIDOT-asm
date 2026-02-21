/* Function to fulfill `int regcheck(sytab_t* syp)` needed for the link.      */
/*  Will need to figure out what it is passing in and what check is needed. */

#include "../uascom/funcdefs.h"

int regcheck(sytab_t* syp) {
	//sytab_t{			/* symbol table entry		*/
	//VMADR	sy_lnk;			/* link to next hash entry	*/
	//VMADR	sy_xlk;			/* link to rear of xref chain	*/
	//VMADR	sy_val;			/* value of symbol		*/
	//uns	sy_rel;			/* relocation of symbol		*/
	//char	sy_typ;			/* type of symbol		*/
	//char	sy_atr;			/* attributes of symbol		*/
	//char	sy_str[SYMSIZ];		/* symbol mnemonic string	*/
	//};
	if (syp) {
		return 1;
	}
	return -1;
}
