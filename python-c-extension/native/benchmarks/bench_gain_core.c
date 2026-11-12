#include "hello/gain/gain_core.h"
#include "jm_bench.h"
#include "hello/clib_common.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define BENCH_N    65536
#define ITERATIONS 200

int
main(void)
{
    float _Complex *in  = malloc(BENCH_N * sizeof(float _Complex));
    if (!in) { fprintf(stderr, "OOM\n"); return 1; }
    float _Complex *out = malloc(BENCH_N * sizeof(float _Complex));
    if (!out) { fprintf(stderr, "OOM\n"); free(in); return 1; }
    for (int i = 0; i < BENCH_N; i++) in[i] = (float)(i)+ 0.0f * I;

    hello_gain_state_t *obj = hello_gain_create(2.0);

    /* volatile sink prevents DCE of the step() loop */
    volatile float _Complex _sink;

    /* warmup */
    for (int i = 0; i < 16; i++) _sink = hello_gain_step(obj, in[i]);

    uint64_t t0, t1;
    jm_bench_t _bench = {0};

    printf("=== gain benchmark ===\n");
    printf("block = %d samples,  %d iterations\n\n", BENCH_N, ITERATIONS);

    double _times_step[ITERATIONS];
    for (int r = 0; r < ITERATIONS; r++) {
        t0 = jm_bench_now_ns();
        for (int i = 0; i < BENCH_N; i++)
            _sink = hello_gain_step(obj, in[i]);
        t1 = jm_bench_now_ns();
        _times_step[r] = jm_bench_elapsed_sec(t0, t1);
    }
    jm_bench_add_unit(&_bench, "step", _times_step, ITERATIONS,
                      BENCH_N, JM_BENCH_SAMPLE);
    {
        double _s = 0.0;
        char   _rate[JM_BENCH_RATE_LEN];
        for (int r = 0; r < ITERATIONS; r++) _s += _times_step[r];
        printf("  step()   %8.1f M%s\n",
               (double)BENCH_N / (_s / ITERATIONS) / 1e6,
               jm_bench_rate_label(JM_BENCH_SAMPLE, _rate,
                                   sizeof(_rate)));
    }
    double _times_steps[ITERATIONS];
    for (int r = 0; r < ITERATIONS; r++) {
        t0 = jm_bench_now_ns();
        hello_gain_steps(obj, in, out, BENCH_N);
        t1 = jm_bench_now_ns();
        _times_steps[r] = jm_bench_elapsed_sec(t0, t1);
    }
    jm_bench_add_unit(&_bench, "steps", _times_steps, ITERATIONS,
                      BENCH_N, JM_BENCH_SAMPLE);
    {
        double _s = 0.0;
        char   _rate[JM_BENCH_RATE_LEN];
        for (int r = 0; r < ITERATIONS; r++) _s += _times_steps[r];
        printf("  steps()  %8.1f M%s\n",
               (double)BENCH_N / (_s / ITERATIONS) / 1e6,
               jm_bench_rate_label(JM_BENCH_SAMPLE, _rate,
                                   sizeof(_rate)));
    }

    (void)_sink;
    jm_bench_write_json(&_bench, "gain");
    hello_gain_destroy(obj);
    free(in);
    free(out);
    return 0;
}
