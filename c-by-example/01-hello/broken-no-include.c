/* broken-no-include.c -- printf without its declaration.
 *
 * C99 removed "implicit declarations": a function must be declared before
 * it is called, and printf is declared in <stdio.h>, which is missing.
 * The compiler stops at stage 2 (see lesson 00).
 */
int
main (void)
{
  printf ("Hello, World!\n");
  return 0;
}
