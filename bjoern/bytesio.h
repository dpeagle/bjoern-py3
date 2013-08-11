#ifndef __bjoern_bytesio_h_
#define __bjoern_bytesio_h_

#include <Python.h>

#define CHECK_CLOSED(self)                          \
    if ((self)->buf == NULL) {                      \
        PyErr_SetString(PyExc_ValueError,           \
                "I/O Operation on closed file.");   \
        return NULL;                                \
    }                                               \

typedef struct {
    PyObject_HEAD
    char *buf;
    Py_ssize_t pos;
    Py_ssize_t string_size;
    size_t buf_size;
} bytesio;

// These is the minimal api required by PEP 3333
PyObject *bytesio_size(bytesio *self);

PyObject *bytesio_read(bytesio *self, PyObject *args);

PyObject *bytesio_readline(bytesio *self);

PyObject *bytesio_iternext(bytesio *self);

// Constructor destructor etc

//void bytesio_dealloc(bytesio *self);

PyObject *bytesio_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);

PyObject *bytesio_write(bytesio *self, PyObject *obj);

Py_ssize_t bytesio_write_bytes(bytesio *self, const char *bytes, Py_ssize_t len);

PyTypeObject BytesIO_Type;
#endif
