/* Function to fulfill `int regcheck(SYTAB* syp)` needed for the link. */

#include "../uascom/funcdefs.h"

int regcheck(SYTAB* syp) {
	//SYTAB{			/* symbol table entry		*/
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
