#define _GNU_SOURCE
#include <dlfcn.h>
#include <execinfo.h>
#include <Python.h>

static PyObject* (*real_function)(const char*, Py_ssize_t) = NULL;

PyObject* PyBytes_FromStringAndSize(const char* v, Py_ssize_t len)
{
    void* buffer[2];
    PyObject* p = NULL;

    /* get real function */
    if (NULL == real_function) {
        real_function = (PyObject* (*)(const char*, Py_ssize_t))dlsym(RTLD_NEXT, "PyBytes_FromStringAndSize");
        if (NULL == real_function) {
            fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
            exit(1);
        }
    }

    /* fast path */
    if (v != NULL) {
        return real_function(v, len);
    }

    char* env = getenv("WEBSOCKETS_BYTES_NULL_HOOK");
    if ((env != NULL) && (env[0] == '1')) {
        int nb = backtrace(buffer, 2);
        if (nb == 2) {
            char ** syms = backtrace_symbols(buffer, nb);
            if (syms != NULL) {
                if (strstr(syms[1], "/speedups.") != NULL) {
                    fputs("'PyBytes_FromStringAndSize hooked to return NULL' ", stderr);
                    free(syms);
                    return PyErr_NoMemory();
                }
                free(syms);
            }
        }
    }
    return real_function(v, len);
}
