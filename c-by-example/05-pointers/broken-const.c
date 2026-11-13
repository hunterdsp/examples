/* broken-const.c -- writing through a pointer to const.
 *
 * `const float *in` is a promise: this function reads `in` and never
 * writes it. Break the promise and the build stops, which is the point:
 * a caller can hand you its data knowing it comes back unchanged.
 */
static void
gain (const float *in, float *out, int n, float g)
{
  for (int i = 0; i < n; i++)
    {
      out[i] = in[i] * g;
      in[i]  = 0.0f; /* "tidy up" the input -- not allowed */
    }
}

int
main (void)
{
  float a[2] = { 1, 2 }, b[2];
  gain (a, b, 2, 0.5f);
  return 0;
}
