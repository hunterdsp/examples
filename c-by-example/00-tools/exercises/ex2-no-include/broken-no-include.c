/* broken-no-include.c (exercise 2) -- stages.c without its #include.
 *
 * The preprocessor has nothing to paste, and is happy. The compiler then
 * meets puts with no declaration, and stops: stage 2.
 */

#define GREETING "compiled, linked, ran"

int
main (void)
{
  puts (GREETING);
  return 0;
}
