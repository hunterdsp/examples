/*
 * gain_ext.c — Python C extension for gain
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>
#include "hello/clib_common.h"

#include "hello/gain/gain_core.h"

#ifndef JM_ARRAY_ARG_DEFINED
#define JM_ARRAY_ARG_DEFINED
/* Convert a Python argument for an array parameter to an ndarray of
 * `typenum` meeting `requirements` -- PyArray_FROM_OTF, except that for a
 * one-byte element type a byte buffer (bytes, bytearray, memoryview) is its
 * bytes, one element per byte (gh-1700). `name` is the parameter, for the
 * message. `hint` is its declared str_hint, or NULL. Declaring one is the
 * opt-in to refusing text (gh-1824): a str, or a bytes numpy would parse as
 * a number, and a str's refusal ends with the hint (gh-1756). With NULL,
 * numpy converts a str as it converts anything else. Returns a new
 * reference, or NULL with an exception. */
static inline PyArrayObject *
jm_array_arg_hint(PyObject *obj, int typenum, int requirements,
                  const char *name, const char *hint)
{
    int one_byte = typenum == NPY_UINT8 || typenum == NPY_INT8;
    int text = PyUnicode_Check(obj) || (!one_byte && PyBytes_Check(obj));
    if (hint && text) {
        /* The hint says where text goes instead: a str's refusal only. */
        int say = PyUnicode_Check(obj);
        PyErr_Format(PyExc_TypeError,
                     "%s must be an array of numbers, not %.200s%s%s", name,
                     Py_TYPE(obj)->tp_name, say ? ": " : "",
                     say ? hint : "");
        return NULL;
    }
    if (one_byte && !PyArray_Check(obj) && PyObject_CheckBuffer(obj)) {
        PyObject *view = PyMemoryView_FromObject(obj);
        if (!view)
            return NULL;
        if (PyMemoryView_GET_BUFFER(view)->itemsize == 1) {
            PyObject *raw = PyArray_FromBuffer(
                view, PyArray_DescrFromType(typenum), -1, 0);
            Py_DECREF(view);
            if (!raw)
                return NULL;
            PyObject *arr = PyArray_FROM_OTF(raw, typenum, requirements);
            Py_DECREF(raw);
            return (PyArrayObject *)arr;
        }
        Py_DECREF(view);
    }
    return (PyArrayObject *)PyArray_FROM_OTF(obj, typenum, requirements);
}
/* `unused` (gh-1747): emitted into every extension translation unit,
 * including one that takes no array, or calls only jm_array_arg_hint above
 * -- which needs no mark, since this wrapper always calls it. */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((unused))
#endif
static inline PyArrayObject *
jm_array_arg(PyObject *obj, int typenum, int requirements, const char *name)
{
    return jm_array_arg_hint(obj, typenum, requirements, name, NULL);
}
#endif /* JM_ARRAY_ARG_DEFINED */

#ifndef JM_INFLIGHT_DEFINED
#define JM_INFLIGHT_DEFINED
/* gh-2272: the GIL-free calls running on one object, so its explicit
 * teardown can wait for them instead of freeing under them. `n` is read
 * and written only with the GIL held. `idle` exists only once a teardown
 * has had to wait: that teardown holds it, and the last call to leave
 * gives it back. */
typedef struct {
    Py_ssize_t n;
    int waiting;
    PyThread_type_lock idle;
} jm_inflight_t;

#if defined(__GNUC__) || defined(__clang__)
__attribute__((unused))
#endif
static inline void
jm_inflight_enter(jm_inflight_t *g)
{
    g->n++;
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((unused))
#endif
static inline void
jm_inflight_leave(jm_inflight_t *g)
{
    if (--g->n == 0 && g->waiting) {
        g->waiting = 0;
        PyThread_release_lock(g->idle);
    }
}

/* Called with the GIL held, after the object refuses new calls. Returns
 * with none running: 0, or -1 with MemoryError when no lock could be had,
 * in which case nothing has been waited for. */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((unused))
#endif
static inline int
jm_inflight_wait(jm_inflight_t *g)
{
    while (g->n > 0) {
        if (!g->idle) {
            g->idle = PyThread_allocate_lock();
            if (!g->idle) {
                PyErr_NoMemory();
                return -1;
            }
        }
        /* Take the lock, then block on it again with the GIL released:
         * the last jm_inflight_leave releases it. The GIL is dropped by
         * function here, not by the macro pair, so the macro still marks
         * exactly the calls a binding runs without it. */
        (void)PyThread_acquire_lock(g->idle, NOWAIT_LOCK);
        g->waiting = 1;
        PyThreadState *ts = PyEval_SaveThread();
        (void)PyThread_acquire_lock(g->idle, WAIT_LOCK);
        PyEval_RestoreThread(ts);
        PyThread_release_lock(g->idle);
    }
    return 0;
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((unused))
#endif
static inline void
jm_inflight_free(jm_inflight_t *g)
{
    if (g->idle) {
        PyThread_free_lock(g->idle);
        g->idle = NULL;
    }
}
#endif /* JM_INFLIGHT_DEFINED */

#ifndef JM_CAPSULE_LEND_DEFINED
#define JM_CAPSULE_LEND_DEFINED
/* gh-2328: a lent capsule's context is the object that owns its pointer.
 * The destructor releases that reference and nothing else -- the pointer is
 * the owner's to free -- so whoever keeps the capsule keeps the owner. */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((unused))
#endif
static void
jm_capsule_lend_release(PyObject *cap)
{
    Py_XDECREF((PyObject *)PyCapsule_GetContext(cap));
}
#if defined(__GNUC__) || defined(__clang__)
__attribute__((unused))
#endif
static PyObject *
jm_capsule_lend(void *ptr, const char *name, PyObject *owner)
{
    PyObject *cap = PyCapsule_New(ptr, name, jm_capsule_lend_release);
    if (!cap)
        return NULL;
    Py_INCREF(owner);
    if (PyCapsule_SetContext(cap, owner) < 0) {
        Py_DECREF(owner);
        Py_DECREF(cap);
        return NULL;
    }
    return cap;
}
#endif /* JM_CAPSULE_LEND_DEFINED */


/* ======================================================== */
/* GainObject — wraps hello_gain_state_t *       */
/* ======================================================== */

typedef struct {
    PyObject_HEAD
    hello_gain_state_t *handle;
} GainObject;

static void
Gain_dealloc(GainObject *self)
{
    if (self->handle)
        hello_gain_destroy(self->handle);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
Gain_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    /* tp_new allocates only; __init__ reads the arguments. */
    (void)args;
    (void)kwds;
    GainObject *self = (GainObject *)type->tp_alloc(type, 0);
    if (self)
        self->handle = NULL;
    return (PyObject *)self;
}

static int
Gain_init(GainObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"k", NULL};
    double k = 2.0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|d", kwlist,
                                     &k))
        return -1;
    self->handle = hello_gain_create(k);
    if (!self->handle) {
        PyErr_SetString(PyExc_MemoryError,
                        "hello_gain_create returned NULL");
        return -1;
    }
    return 0;
}

static PyObject *
Gain_reset(GainObject *self, PyObject *Py_UNUSED(ignored))
{
    if (!self->handle) {
        PyErr_SetString(PyExc_RuntimeError, "destroyed");
        return NULL;
    }
    hello_gain_reset(self->handle);
    Py_RETURN_NONE;
}

static PyObject *
Gain_step(GainObject *self, PyObject *args)
{
    if (!self->handle) {
        PyErr_SetString(PyExc_RuntimeError, "destroyed");
        return NULL;
    }
    Py_complex x_raw = {0.0, 0.0};
    if (!PyArg_ParseTuple(args, "D", &x_raw))
        return NULL;
    float _Complex x = (float)x_raw.real + (float)x_raw.imag * I;
    if (!self->handle) {
        PyErr_SetString(PyExc_RuntimeError, "destroyed");
        return NULL;
    }
    float _Complex y = hello_gain_step(self->handle, x);
    return PyComplex_FromDoubles((double)crealf(y), (double)cimagf(y));
}

static PyObject *
Gain_steps(GainObject *self, PyObject *args, PyObject *kwds)
{
    if (!self->handle) {
        PyErr_SetString(PyExc_RuntimeError, "destroyed");
        return NULL;
    }
    static char *kwlist[] = {"x", "out", NULL};
    PyObject *in_obj  = NULL;
    PyObject *out_obj = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwds,
            "O|O", kwlist,
            &in_obj, &out_obj))
        return NULL;

    PyArrayObject *in_arr = (PyArrayObject *)
        jm_array_arg(in_obj, NPY_COMPLEX64, NPY_ARRAY_C_CONTIGUOUS, "x");
    if (!in_arr)
        return NULL;

    Py_ssize_t n = PyArray_SIZE(in_arr);

    if (out_obj && out_obj != Py_None) {
        /* Require the exact dtype AND C-contiguity — either mismatch makes
         * the marshal write into a temp copy, not the caller's buffer. */
        if (!PyArray_Check(out_obj) ||
            PyArray_TYPE((PyArrayObject *)out_obj) != NPY_COMPLEX64 ||
            !PyArray_IS_C_CONTIGUOUS((PyArrayObject *)out_obj) ||
            !PyArray_ISWRITEABLE((PyArrayObject *)out_obj)) {
            PyErr_SetString(PyExc_TypeError,
                "out must be a writable, C-contiguous"
                " ndarray of the output dtype");
            Py_DECREF(in_arr);
            return NULL;
        }
        PyArrayObject *out_arr = (PyArrayObject *)
            jm_array_arg(out_obj, NPY_COMPLEX64, NPY_ARRAY_C_CONTIGUOUS | NPY_ARRAY_WRITEABLE, "out");
        if (!out_arr) { Py_DECREF(in_arr); return NULL; }
        if (PyArray_SIZE(out_arr) != n) {
            PyErr_Format(PyExc_ValueError,
                "out length %zd != input length %zd",
                (Py_ssize_t)PyArray_SIZE(out_arr), (Py_ssize_t)n);
            Py_DECREF(out_arr);
            Py_DECREF(in_arr);
            return NULL;
        }
        if (!self->handle) {
            Py_DECREF(in_arr); Py_DECREF(out_arr);
            PyErr_SetString(PyExc_RuntimeError, "destroyed");
            return NULL;
        }
        hello_gain_steps(
            self->handle,
            (const float _Complex *)PyArray_DATA(in_arr),
            (float _Complex *)PyArray_DATA(out_arr),
            (size_t)n);
        Py_DECREF(in_arr);
        return (PyObject *)out_arr;
    }

    npy_intp dims[] = {n};
    PyObject *out_arr = PyArray_SimpleNew(1, dims, NPY_COMPLEX64);
    if (!out_arr) {
        Py_DECREF(in_arr);
        return NULL;
    }

    if (!self->handle) {
        Py_DECREF(in_arr); Py_DECREF(out_arr);
        PyErr_SetString(PyExc_RuntimeError, "destroyed");
        return NULL;
    }
    hello_gain_steps(
        self->handle,
        (const float _Complex *)PyArray_DATA(in_arr),
        (float _Complex *)PyArray_DATA((PyArrayObject *)out_arr),
        (size_t)n);

    Py_DECREF(in_arr);
    return out_arr;
}


static PyObject *
Gain_get_k(
    GainObject *self, PyObject *Py_UNUSED(ignored))
{
    if (!self->handle) {
        PyErr_SetString(PyExc_RuntimeError, "destroyed");
        return NULL;
    }
    return PyFloat_FromDouble(hello_gain_get_k(self->handle));
}

static PyObject *
Gain_set_k(
    GainObject *self, PyObject *args)
{
    if (!self->handle) {
        PyErr_SetString(PyExc_RuntimeError, "destroyed");
        return NULL;
    }
    double v = 0.0;
    if (!PyArg_ParseTuple(args, "d", &v))
        return NULL;
    if (!self->handle) {
        PyErr_SetString(PyExc_RuntimeError, "destroyed");
        return NULL;
    }
    hello_gain_set_k(self->handle, v);
    Py_RETURN_NONE;
}


static PyObject *
Gain_destroy(GainObject *self, PyObject *Py_UNUSED(ignored))
{
    if (self->handle) {
        hello_gain_destroy(self->handle);
        self->handle = NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
Gain_enter(GainObject *self, PyObject *Py_UNUSED(ignored))
{
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
Gain_exit(GainObject *self, PyObject *args)
{
    (void)args;
    if (self->handle) {
        hello_gain_destroy(self->handle);
        self->handle = NULL;
    }
    Py_RETURN_NONE;
}

static PyMethodDef Gain_methods[] = {
    {"reset",    (PyCFunction)Gain_reset,    METH_NOARGS,
     "Reset state to post-create defaults.\n"},
    {"step",     (PyCFunction)Gain_step,     METH_VARARGS,
     "step(x) -> float _Complex\n"
     "\n"
     "Process one input sample.\n"
     "\n"
     "    >>> from hello import Gain\n"
     "    >>> obj = Gain(k=2.0)\n"
     "    >>> obj.step(1.0 + 0.0j)\n"
     "    (1+0j)\n"},
    {"steps",    (PyCFunction)(void *)Gain_steps,    METH_VARARGS | METH_KEYWORDS,
     "steps(x[, out]) -> ndarray\n"
     "\n"
     "Process a block of samples in batch.\n"
     "\n"
     "    >>> import numpy as np\n"
     "    >>> from hello import Gain\n"
     "    >>> obj = Gain(k=2.0)\n"
     "    >>> y = obj.steps(np.zeros(4, dtype=np.complex64))\n"
     "    >>> y.shape\n"
     "    (4,)\n"
     "    >>> y.dtype\n"
     "    dtype('complex64')\n"},

    {"get_k",
     (PyCFunction)Gain_get_k, METH_NOARGS,
     "Get k.\n"},
    {"set_k",
     (PyCFunction)Gain_set_k, METH_VARARGS,
     "Set k.\n"},
    {"destroy",  (PyCFunction)Gain_destroy,  METH_NOARGS,
     "Release the underlying C resources immediately.\n"
     "\n"
     "Ordinarily unnecessary: the resources are freed when the object is\n"
     "garbage-collected. Call this to release them at a definite point\n"
     "instead, or use the object as a context manager, which calls it on\n"
     "exit.\n"
     "\n"
     "Idempotent: calling it again on an already-released object does\n"
     "nothing. Every other method raises ``RuntimeError`` once it has run.\n"},
    {"__enter__", (PyCFunction)Gain_enter,   METH_NOARGS,
     "Enter a context manager, returning this object.\n"
     "\n"
     "Lets a Gain be used in a `with` statement so its C resources are\n"
     "released deterministically on exit rather than at collection time.\n"
     "\n"
     "Returns\n"
     "-------\n"
     "Gain\n"
     "    This same object, not a copy.\n"},
    {"__exit__",  (PyCFunction)Gain_exit,    METH_VARARGS,
     "Exit a context manager, releasing the Gain.\n"
     "\n"
     "Equivalent to calling `destroy()`. Returns ``None``, so an exception\n"
     "raised inside the `with` body propagates normally; this never\n"
     "suppresses one.\n"
     "\n"
     "Parameters\n"
     "----------\n"
     "exc_type : object | None\n"
     "    Exception class, or None. Ignored.\n"
     "exc : object | None\n"
     "    Exception instance, or None. Ignored.\n"
     "tb : object | None\n"
     "    Traceback object, or None. Ignored.\n"},
    {NULL, NULL, 0, NULL}
};

static PyTypeObject GainType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name      = "hello.gain.Gain",
    .tp_basicsize = sizeof(GainObject),
    .tp_dealloc   = (destructor)Gain_dealloc,
    .tp_flags     = Py_TPFLAGS_DEFAULT,
    .tp_doc       = "Gain component. Wraps hello_gain_state_t.",
    .tp_methods   = Gain_methods,
    .tp_new       = Gain_new,
    .tp_init      = (initproc)Gain_init,
};


/* ======================================================== */
/* Module definition                                         */
/* ======================================================== */

static PyModuleDef hello_gain_module = {
    PyModuleDef_HEAD_INIT,
    .m_name    = "gain",
    .m_doc     = "Python binding for gain_core.h.",
    .m_size    = -1,
    .m_methods = NULL,
};

PyMODINIT_FUNC
PyInit_gain(void)
{
    import_array();
    if (PyType_Ready(&GainType) < 0)
        return NULL;

    PyObject *m = PyModule_Create(&hello_gain_module);
    if (!m)
        return NULL;

    Py_INCREF(&GainType);
    if (PyModule_AddObject(m, "Gain",
                           (PyObject *)&GainType) < 0) {
        Py_DECREF(&GainType);
        Py_DECREF(m);
        return NULL;
    }
    return m;
}
