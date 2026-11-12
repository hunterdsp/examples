#include "hello/gain/gain_core.h"

hello_gain_state_t *
hello_gain_create(double k)
{
    hello_gain_state_t *obj = calloc(1, sizeof(*obj));
    if (!obj)
        return NULL;
    obj->k = k;
    return obj;
}

void
hello_gain_destroy(hello_gain_state_t *state)
{
    free(state);
}

void
hello_gain_reset(hello_gain_state_t *state)
{
    state->k = 2.0;
}

void hello_gain_steps(
    hello_gain_state_t *state,
    const float _Complex    *input,
    float _Complex          *output,
    size_t               n)
{
    for (size_t i = 0; i < n; i++)
        output[i] = hello_gain_step(state, input[i]);
}

double
hello_gain_get_k(const hello_gain_state_t *state)
{
    return state->k;
}

void
hello_gain_set_k(hello_gain_state_t *state, double val)
{
    state->k = val;
}
