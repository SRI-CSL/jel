
#include <jel/jel.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

static PyObject *JelError;

static PyObject *
encode(PyObject *self, PyObject *args)
{
    const char *in_jpeg, *message;
    Py_ssize_t in_jpeg_length, out_jpeg_length, message_length;
    int err;
    jel_config *jel = NULL;
    char *out_jpeg = NULL;
    PyObject *value = NULL;

    in_jpeg = NULL;
    if (!PyArg_ParseTuple(args, "y#y#", &in_jpeg, &in_jpeg_length,
                          &message, &message_length)) {
        return NULL;
    }
    out_jpeg_length = in_jpeg_length + in_jpeg_length / 2;
    out_jpeg = malloc(out_jpeg_length);
    if (out_jpeg == NULL) {
        PyErr_Format(JelError, "output jpeg malloc failed");
        goto done;
    }
    jel = jel_init(JEL_NLEVELS);
    if (jel == NULL) {
        PyErr_Format(JelError, "jel_init failed");
        goto done;
    }
    err = jel_set_mem_source(jel, (unsigned char *)in_jpeg, in_jpeg_length);
    if (err < 0) {
        PyErr_Format(JelError, "jel_set_mem_source: %d", err);
        goto done;
    }
    err = jel_set_mem_dest(jel, (unsigned char *)out_jpeg, out_jpeg_length);
    if (err < 0) {
        PyErr_Format(JelError, "jel_set_mem_dest: %d", err);
        goto done;
    }
    err = jel_setprop(jel, JEL_PROP_EMBED_LENGTH, 1);
    if (err < 0) {
        PyErr_Format(JelError, "jel_setprop: %d", err);
        goto done;
    }
    err = jel_embed(jel, (unsigned char *)message, message_length);
    if (err < 0) {
        PyErr_Format(JelError, "jel_embed: %d", err);
        goto done;
    }
    out_jpeg_length = jel->jpeglen;

    value = Py_BuildValue("y#", out_jpeg, out_jpeg_length);

  done:
    if (jel) {
        jel_free(jel);
    }
    if (out_jpeg) {
        free(out_jpeg);
    }

    return value;
}

static PyObject *
decode(PyObject *self, PyObject *args)
{
    const char *in_jpeg;
    Py_ssize_t in_jpeg_length, message_length;
    int err;
    char *message = NULL;
    jel_config *jel = NULL;
    PyObject *value = NULL;

    if (!PyArg_ParseTuple(args, "y#", &in_jpeg, &in_jpeg_length)) {
        return NULL;
    }
    jel = jel_init(JEL_NLEVELS);
    if (jel == NULL) {
        PyErr_Format(JelError, "jel_init failed");
        goto done;
    }

    /*
    jel->logger = stderr;
    jel_verbose = 1;
    */

    err = jel_set_mem_source(jel, (unsigned char *)in_jpeg, in_jpeg_length);
    if (err < 0) {
        PyErr_Format(JelError, "jel_set_mem_source: %d", err);
        goto done;
    }
    message_length = jel_raw_capacity(jel) * 2;
    message = malloc(message_length);
    if (message == NULL) {
        PyErr_Format(JelError, "message malloc failed");
        goto done;
    }
    err = jel_setprop(jel, JEL_PROP_EMBED_LENGTH, 1);
    if (err < 0) {
        PyErr_Format(JelError, "jel_setprop: %d", err);
        goto done;
    }
    message_length = jel_extract(jel, (unsigned char *)message, message_length);
    if (message_length < 0) {
        PyErr_Format(JelError, "jel_extract: %d", message_length);
        goto done;
    }

    value = Py_BuildValue("y#", message, message_length);

  done:
    if (message) {
        free(message);
    }
    if (jel) {
        jel_free(jel);
    }

    return value;
}

static PyMethodDef JelMethods[] = {
    {"encode",  encode, METH_VARARGS,
     "JEL encode."},
    {"decode",  decode, METH_VARARGS,
     "JEL decode."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef jel_module = {
    PyModuleDef_HEAD_INIT,
    "pyjel",   /* name of module */
    NULL,
    -1,
    JelMethods
};

PyMODINIT_FUNC
PyInit_pyjel(void)
{
    PyObject *m;

    m = PyModule_Create(&jel_module);
    if (m == NULL)
        return NULL;
    JelError = PyErr_NewException("pyjel.error", NULL, NULL);
    Py_XINCREF(JelError);
    if (PyModule_AddObject(m, "error", JelError) < 0) {
        Py_XDECREF(JelError);
        Py_CLEAR(JelError);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
