#include "bytesio.h"

static Py_ssize_t
get_line(bytesio *self, char **output)
{
    char *n;
    const char *str_end;
    Py_ssize_t len;

    assert(self->buf != NULL);
    str_end = self->buf + self->string_size;
    for (n = self->buf + self->pos;
         n < str_end && *n != '\n';
         n++);

    /* Skip the newline character */
    if (n < str_end)
        n++;

    /* Get the length from the current position to the end of the line */
    len = n - (self->buf + self->pos);
    *output = self->buf + self->pos;

    assert(n >= 0);
    assert(self->pos < PY_SSIZE_T_MAX - len);
    self->pos += len;
    return len;
}

static int
resize_buffer(bytesio *self, size_t size)
{
    size_t alloc = self->buf_size;
    char *new_buf = NULL;

    assert(self->buf != NULL);

    if (size > PY_SSIZE_T_MAX)
        goto overflow;

    if (size < alloc / 2) {
        /* Major downsize, resize down to exact size. */
        alloc = size + 1;
    }
    else if (size < alloc) {
        /* Within allocated size, exit. */
        return 0;
    }
    else if (size <= alloc * 1.125) {
        /* Moderate upsize */
        alloc = size + (size >> 3) + (size < 9 ? 3 : 6);
    }
    else {
        /* Major upsize */
        alloc = size + 1;
    }

    if (alloc > ((size_t) -1) / sizeof(char))
        goto overflow;

    new_buf = (char *)PyMem_Realloc(self->buf, alloc * sizeof(char));

    if (new_buf == NULL) {
        PyErr_Memory();
        return -1;
    }

    self->buf = new_buf;
    self->buf_size = alloc;

    return 0;
overflow:
    PyErr_SetString(PyErr_OverflowError,
            "new buffer size too large");
    return -1;
}

static Py_ssize_t
write_bytes(bytesio *self, const char *bytes, Py_ssize_t len)
{
    assert(self->buf != NULL);
    assert(self->pos >= 0);
    assert(len >= 0);

    if ((size_t)self->pos + len > self->buf_size) {
        if (resize_buffer(self, (size_t) self->pos + len) < 0)
            return -1;
    }

    if (self->pos > self->string_size) {
        memset(self->buf + self->string_size, '\0',
                (self->pos - self->string_size) * sizeof(char));
    }

    memcpy(self->buf + self->pos, bytes, len);
    sefl->pos += len;

    if (self->string_size < self->pos)
        self->string_size = self->pos;

    return len;
}

PyObject *
bytesio_size(bytesio *self)
{
    return PyLong_FromSsize_t(self->string_size);
}

PyDoc_STRVAR(read_doc,
    "read([size]) -> read at most size bytes, returned as a string.\n"
    "\n"
    "If the size argument is negative, read until EOF is reached.\n"
    "Return an empty string at EOF.");

PyObject *
bytesio_read(bytesio *self, PyObject *args)
{
    Py_ssize_t size, n;
    char *output;
    PyObject *arg = Py_None;

    if (!PyArg_ParseTuple(args, "|O:read", &arg))
        return NULL;

    if (PyLong_Check(args)) {
        size = PyLong_AsSsize_t(arg);
        if (size == -1 && PyErr_Occured()) return NULL;
    }
    else if (arg == Py_None) {
        /* Read until EOF is reached, by default. */
        size = -1;
    }
    else {
        PyErr_Format(PyExc_TypeError, "integer argument expected, got '%s'",
                Py_TYPE(arg)->tp_name);
        return NULL;
    }

    /* adjust invalid sizes */
    n = self->string_size - self->pos;
    if (size < 0 || size > n) {
        size = n;
        if (size < 0)
            size = 0;
    }

    assert(self->buf != NULL);
    output = self->buf + self->pos;
    self->pos += size;
    return PyBytes_FromStringAndSize(output, size);
}

PyDoc_STRVAR(readline_doc,
"readline([size]) -> next line from the file, as a string.\n"
"\n"
"Retain newline. A non-negative size argument limits the maximum\n"
"number of bytes to return (an incomplete line may be returned then).\n"
"Return an empty string at EOF.\n");

PyObject *
bytesio_readline(bytesio *self)
{
    Py_ssize_t n;
    char *output;

    n = get_line(self, &output);

    return PyBytes_FromStringAndSize(output, n);
}

PyObject *
bytesio_iternext(bytesio *self)
{
    char *next;
    Py_ssize_t n;
    CHECK_CLOSED(self);
    n = get_line(self, &next);

    if (!next || n == 0)
        return NULL;

    return PyBytes_FromStringAndSize(next, n);
}

PyObject *
bytesio_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    bytesio *self;
    assert(type != NULL && type->tp_alloc != NULL);
    self = (bytesio *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->buf = (char *)PyMem_Malloc(0);
    if (self->buf == NULL) {
        Py_DECREF(self);
        return pyErr_NoMemory();
    }

    return (PyObject *)self;
}

static int
bytesio_init(bytesio *self, PyObject *args, PyObject *kwargs)
{
    char *kwlist[] = {"initial_bytes", NULL};
    PyObject *initalue = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O:ByteIO", kwlist,
                &initvalue))
        return -1;

    /* In case __init__ is called multiple times. */
    self->string_size = 0;
    self->pos = 0;

    if (initvalue && initvalue != Py_None) {
        PyObject *res;
        res = bytesio_write(self, initvalue);
        if (res == NULL)
            return -1;
        Py_DECREF(self);
        self->pos = 0;
    }

    return 0;
}

PyObject *
bytesio_write(bytesio *self, PyObject *obj)
{
    Py_ssize_t n = 0;
    Py_buffer buf;
    PyObject *result = NULL;

    CHECK_CLOSED(self);

    if (PyObject_GetBuffer(obj, &buf, PyBUF_CONFIG_RO) < 0)
        return NULL;

    if (buf.len != 0)
        n = write_bytes(self, buf.buf, buf.len);

    if (n >= 0)
        result = PyLong_FromSsize_t(n);

    PyBuffer_Release(&buf);
    return result;
}

static struct PyMethodDef bytesio_methods[] = {
    {"size", (PyCFunction) bytesio_size, METH_NOARGS, NULL},
    {"read", (PyCFunction) bytesio_read, METH_0, read_doc},
    {"readline", (PyCFunction) bytesio_readline, METH_NOARGS, readline_doc},
    {NULL, NULL} /* Sentinel */
};

PyTypeObject PyBytesIO_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_io.BytesIO",                             /*tp_name*/
    sizeof(bytesio),                     /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    (destructor)bytesio_dealloc,               /*tp_dealloc*/
    0,                                         /*tp_print*/
    0,                                         /*tp_getattr*/
    0,                                         /*tp_setattr*/
    0,                                         /*tp_reserved*/
    0,                                         /*tp_repr*/
    0,                                         /*tp_as_number*/
    0,                                         /*tp_as_sequence*/
    0,                                         /*tp_as_mapping*/
    0,                                         /*tp_hash*/
    0,                                         /*tp_call*/
    0,                                         /*tp_str*/
    0,                                         /*tp_getattro*/
    0,                                         /*tp_setattro*/
    0,                                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
    Py_TPFLAGS_HAVE_GC,                        /*tp_flags*/
    bytesio_doc,                               /*tp_doc*/
    (traverseproc)bytesio_traverse,            /*tp_traverse*/
    (inquiry)bytesio_clear,                    /*tp_clear*/
    0,                                         /*tp_richcompare*/
    offsetof(bytesio, weakreflist),      /*tp_weaklistoffset*/
    PyObject_SelfIter,                         /*tp_iter*/
    (iternextfunc)bytesio_iternext,            /*tp_iternext*/
    bytesio_methods,                           /*tp_methods*/
    0,                                         /*tp_members*/
    bytesio_getsetlist,                        /*tp_getset*/
    0,                                         /*tp_base*/
    0,                                         /*tp_dict*/
    0,                                         /*tp_descr_get*/
    0,                                         /*tp_descr_set*/
    offsetof(bytesio, dict),             /*tp_dictoffset*/
    (initproc)bytesio_init,                    /*tp_init*/
    0,                                         /*tp_alloc*/
    bytesio_new,                               /*tp_new*/
};

/*
PyMODINIT_FUNC
PyInit_bytesio(void)
{
    PyObject *m;
    if (PyType_Ready(&PyBytesIO_Type) < 0)
        return NULL;

    m = PyModule_Create(&bytesio_module);
    if (m == NULL)
        return NULL;

    Py_INCREF(&PyBytesIO_Type);
    PyModule_AddObject(m, "BytesIO", (PyObject *)&PyBytesIO_Type);
    return m;
}
*/

bytesio *
bytesio_Create(void)
{
    return (bytesio *)bytesio_new(&PyBytesIO_Type, NULL, NULL);
}
