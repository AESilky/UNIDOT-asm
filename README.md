# UNIDOT Macro Assembler - from the 1980's - 1990's

This is the UNIDOT macro assembler and linker used in the 1980's to assemble and link the ZED source.

As is, this is a Z80 assembler ('uasz80'), but versions for other processors existed in the 1980's.
Everything is here to create assemblers for others (maybe the Z180 and/or Z280).

## Directory Structure

``` raw
root
  incl/
    ulktab.h
    uobj.h
    urel.h
  uascom/
  uasz80/
  ulink/
```

The ZIPs contain files with uppercase names (probably zipped from DOS). To convert to lowercase (needed) use the command:
`rename 'y/A-Z/a-z/' *`

The original source is in '/original' and the updated source (updated to compile with GCC on Linux) is in '/src'.
To get it to compile a few changes have been made. They are mostly around the following:
1) Forward declarations were added for functions. Most were put in a new, 'funcdefs.h' file in each part of the tree.
2) Some casting macros were created to move values into and out of 'void*' variables (they were used as a kind of non-typed storage location).
3) The system provided memory management was used in place of the home-grown 'virtual memory' system in the code.
4) Creating more complete function signatures
5) Adding '#include' statements for some system headers (they must have been automatic in the older compilers).

Build the assembler from the 'uasz80' directory (it builds the common and the z80 specific part).
Build the linker from the 'ulink' directory.

2026: Added Binary (ROM) output option to ulink to directly generate a ROM image file so an additional loader
isn't required.
