/* average.c (exercise 3) -- the moving average with a running sum.
 *
 * The lesson's version re-adds the whole window for every output: WINDOW
 * additions per sample. This one keeps the sum from the last sample, adds
 * the sample entering the window and subtracts the one leaving it: one
 * addition and one subtraction per sample, however wide the window.
 */
#include <stdio.h>
#include <string.h> /* memcmp */

enum
{
  WINDOW = 4
};

/* The lesson's version, kept to compare against. */
static void
moving_average (const float in[], float out[], int n)
{
  for (int i = 0; i < n; i++)
    {
      float sum   = 0.0f;
      int   count = 0;
      for (int k = 0; k < WINDOW && i - k >= 0; k++)
        {
          sum += in[i - k];
          count++;
        }
      out[i] = sum / (float)count;
    }
}

static void
running_average (const float in[], float out[], int n)
{
  float sum = 0.0f;
  for (int i = 0; i < n; i++)
    {
      sum += in[i]; /* the new sample enters the window... */
      if (i >= WINDOW)
        sum -= in[i - WINDOW]; /* ...and the oldest one leaves it */
      int count = i + 1 < WINDOW ? i + 1 : WINDOW;
      out[i]    = sum / (float)count;
    }
}

int
main (void)
{
  float in[] = { 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
  enum
  {
    N = sizeof in / sizeof in[0]
  };
  float slow[N], fast[N];

  moving_average (in, slow, N);
  running_average (in, fast, N);
  printf ("running:");
  for (int i = 0; i < N; i++)
    printf (" %4.2f", fast[i]);
  printf ("\nsame as re-adding the window: %s\n",
          memcmp (slow, fast, sizeof slow) == 0 ? "yes" : "no");
  printf ("additions per sample: %d re-adding, 2 running\n", WINDOW);
  return 0;
}
