/*
 *  romemwrite.c
 *
 *  This program attempts to write into its own text segment,
 *  which ought to be read-only.
 *
 *  This code is borrowed from the crash.c program, which is
 *   more general than this one in that it will generate lots
 *   of other kinds of improper memory accesses.
 *
 *  The only advantage of this program is that it does not
 *  require argument passing, so it can be used even if argument
 *  passing is not implemented.
 */ 

#include <stdio.h>
#include <stdlib.h>

#define INVAL_INSN  0x0000003f

int
main()
{
  unsigned int *x = (unsigned int *) main;

  printf("Trying to write to the text segment\n");
  printf("This program should fail if the text segment is read-only.\n");
  printf("However, the kernel should not crash...\n");

  *x = INVAL_INSN;

  printf("IF THIS PRINTS, THE TEST FAILED\n");
  exit(1);
}
