/*

  Copyright (c) 2009, Nokia Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  
    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.  
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution.  
    * Neither the name of Nokia nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */
/*
 *  pygletmodule.c
 *
 *  Author: Ora Lassila mailto:ora.lassila@nokia.com
 *  Copyright (c) 2001-2008 Nokia. All Rights Reserved.
 */

#include <Python.h>
#include "cpiglet.h"

typedef struct {
  PyObject_HEAD
  DB db;
} PyPiglet_DBObject;

#define asDB(x) (((PyPiglet_DBObject *)x)->db)

static PyTypeObject PyPiglet_DBType;

static PyObject *PyPiglet_Exception;

#define PyPiglet_DBObject_Check(v) (Py_TYPE(v) == &PyPiglet_DBType)

static PyPiglet_DBObject *new_PyPiglet_DBObject(PyObject *arg)
{
  PyPiglet_DBObject *self;
  self = PyObject_New(PyPiglet_DBObject, &PyPiglet_DBType);
  if (self != NULL)
    self->db = NULL;
  return self;
}

static void PyPiglet_DBObject_dealloc(PyPiglet_DBObject *self)
{
  piglet_close(self->db);
  self->db = NULL;
  PyObject_Del(self);
}

static PyObject *PyPiglet_status(PigletStatus status)
{
  switch (status) {
    case PigletTrue:
      Py_INCREF(Py_True);
      return Py_True;
    case PigletFalse:
      Py_INCREF(Py_False);
      return Py_False;
    case PigletError:
      if (piglet_error_message == NULL)
        piglet_error_message = "Unknown error in Piglet";
      PyErr_SetString(PyPiglet_Exception, piglet_error_message);
      return NULL;
  }
}

PyObject *PyPiglet_open(PyObject *self, PyObject *args)
{
  char *name;
  DB db;
  PyPiglet_DBObject *o;
  if (PyArg_ParseTuple(args, "s", &name)) {
    db = piglet_open(name);
    if (db) {
      o = new_PyPiglet_DBObject(NULL);
      if (o) {
        Py_INCREF(o);
        o->db = db;
        return (PyObject *)o;
      }
    }
    return PyPiglet_status(PigletError);
  }
  return NULL;
}

PyObject *PyPiglet_close(PyObject *self, PyObject *args)
{
  if (PyArg_ParseTuple(args, ""))
    return PyPiglet_status(piglet_close(asDB(self)));
  else
    return NULL;
}

PyObject *PyPiglet_add(PyObject *self, PyObject *args)
{
  int s, p, o, src, temp = 0;
  if (PyArg_ParseTuple(args, "iiii|i", &s, &p, &o, &src, &temp))
    return PyPiglet_status(piglet_add(asDB(self), s, p, o, src, temp != 0));
  else
    return NULL;
}

PyObject *PyPiglet_add_post_process(PyObject *self, PyObject *args)
{
  int s, p, o;
  if (PyArg_ParseTuple(args, "iii", &s, &p, &o))
    return PyPiglet_status(piglet_add_post_process(asDB(self), s, p, o));
  else
    return NULL;
}

PyObject *PyPiglet_del(PyObject *self, PyObject *args)
{
  int s, p, o, src, temp = 0;
  if (PyArg_ParseTuple(args, "iiii|i", &s, &p, &o, &src, &temp))
    return PyPiglet_status(piglet_del(asDB(self), s, p, o, src, temp != 0));
  else
    return NULL;
}

PyObject *PyPiglet_count(PyObject *self, PyObject *args)
{
  int s, p, o, src, temp = 0;
  if (PyArg_ParseTuple(args, "iiii|i", &s, &p, &o, &src, &temp)) {
    int n = piglet_count(asDB(self), s, p, o, src, temp != 0);
    if (n == -1)
      return PyPiglet_status(PigletError);
    else
      return Py_BuildValue("i", n);
  }
  return NULL;
}

static bool PyPiglet_triple_callback(DB db, void *userdata, Node s, Node p, Node o)
{
  return PyList_Append((PyObject *)userdata, Py_BuildValue("(iii)", s, p, o)) ? false : true;
}

static bool PyPiglet_node_callback(DB db, void *userdata, Node n)
{
  return PyList_Append((PyObject *)userdata, Py_BuildValue("i", n)) ? false : true;
}

PyObject *PyPiglet_query(PyObject *self, PyObject *args)
{
  int s, p, o, source=0;
  if (PyArg_ParseTuple(args, "iii|i", &s, &p, &o, &source)) {
    PyObject *triples = PyList_New(0);
    if (piglet_query(asDB(self), s, p, o, source, triples, PyPiglet_triple_callback))
      return triples;
    else {
      Py_DECREF(triples);
      return PyPiglet_status(PigletError);
    }
  }
  return NULL;
}

PyObject *PyPiglet_sources(PyObject *self, PyObject *args)
{
  int s, p, o;
  if (PyArg_ParseTuple(args, "iii", &s, &p, &o)) {
    PyObject *sources = PyList_New(0);
    if (piglet_sources(asDB(self), s, p, o, sources, PyPiglet_node_callback))
      return sources;
    else {
      Py_DECREF(sources);
      return PyPiglet_status(PigletError);
    }
  }
  return NULL;
}

PyObject *PyPiglet_info(PyObject *self, PyObject *args)
{
  PyObject *info;
  int n;
  char *uri;
  if (PyArg_ParseTuple(args, "i", &n)) {
    if (n < 0) {
      int dt = 0;
      char lang[256];
      lang[0] = '\0';
      uri = piglet_info(asDB(self), n, &dt, lang);
      info = Py_BuildValue("(sis)", uri, dt, ((lang[0] == '\0') ? NULL : lang));
      free(uri);
      return info;
    }
    else {
      uri = piglet_info(asDB(self), n, NULL, NULL);
      if (uri == NULL) {
        Py_INCREF(Py_None);
        return Py_None;
      }
      else {
        info = Py_BuildValue("s", uri);
        free(uri);
        return info;
      }
    }
  }
  return NULL;
}

PyObject *PyPiglet_node(PyObject *self, PyObject *args)
{
  char *uri;
  if (PyArg_ParseTuple(args, "s", &uri)) {
    int n = piglet_node(asDB(self), uri);
    // free uri??
    // free(uri);
    if (n == 0)
      return PyPiglet_status(PigletError);
    else
      return Py_BuildValue("i", n);
  }
  return NULL;
}

PyObject *PyPiglet_literal(PyObject *self, PyObject *args)
{
  char *contents, *lang = NULL;
  int dt = 0;
  if (PyArg_ParseTuple(args, "s|is", &contents, &dt, &lang)) {
    int n = piglet_literal(asDB(self), contents, dt, lang);
    // free contents, language??
    // free(contents);
    // free(lang);
    if (n == 0)
      return PyPiglet_status(PigletError);
    else
      return Py_BuildValue("i", n);
  }
  return NULL;
}

PyObject *PyPiglet_augmentLiteral(PyObject *self, PyObject *args)
{
  int literal = 0, dt = 0;
  if (PyArg_ParseTuple(args, "ii", &literal, &dt))
    return PyPiglet_status(piglet_augment_literal(asDB(self), literal, dt));
  else
    return NULL;
}

PyObject *PyPiglet_load(PyObject *self, PyObject *args)
{
  int source, append = 0, verbose = 0;
  char *script = NULL;
  if (PyArg_ParseTuple(args, "i|iiz", &source, &append, &verbose, &script, NULL))
    // must extend for script execution
    return PyPiglet_status(piglet_load(asDB(self), source, append != 0, verbose != 0, script, NULL));
  else
    return NULL;
}

PyObject *PyPiglet_node_tostring(PyObject *self, PyObject *args)
{
  int node;
  if (PyArg_ParseTuple(args, "i", &node)) {
    char *s = piglet_node_tostring(asDB(self), node);
    if (s == NULL)
      return PyPiglet_status(PigletError);
    else
      return Py_BuildValue("s", s);
  }
  return NULL;
}

PyObject *PyPiglet_triple_tostring(PyObject *self, PyObject *args)
{
  int s, p, o;
  if (PyArg_ParseTuple(args, "iii", &s, &p, &o)) {
    char *str = piglet_triple_tostring(asDB(self), s, p, o);
    if (str == NULL)
      return PyPiglet_status(PigletError);
    else
      return Py_BuildValue("s", str);
  }
  return NULL;
}

PyObject *PyPiglet_expand(PyObject *self, PyObject *args)
{
  char *qname;
  PyObject *rval;
  if (PyArg_ParseTuple(args, "s", &qname)) {
    char *s = piglet_expand(asDB(self), qname);
    if (s == NULL) {
      Py_INCREF(Py_None);
      return Py_None;
    }
    else {
      rval = Py_BuildValue("s", s);
      free(s);
      return rval;
    }
  }
  return NULL;
}

PyObject *PyPiglet_expand_m3(PyObject *self, PyObject *args)
{
  char *qname;
  PyObject *rval;
  if (PyArg_ParseTuple(args, "s", &qname)) {
    char *s = piglet_expand_m3(asDB(self), qname);
    if (s == NULL) {
      Py_INCREF(Py_None);
      return Py_None;
    }
    else {
      rval = Py_BuildValue("s", s);
      free(s);
      return rval;
    }
  }
  return NULL;
}

PyObject *PyPiglet_abbreviate(PyObject *self, PyObject *args)
{
  char *uri;
  if (PyArg_ParseTuple(args, "s", &uri)) {
    char *s = piglet_abbreviate(asDB(self), uri);
    if (s == NULL) {
      Py_INCREF(Py_None);
      return Py_None;
    }
    else return Py_BuildValue("s", s);
  }
  return NULL;
}

PyObject *PyPiglet_add_namespace(PyObject *self, PyObject *args)
{
  char *prefix, *uri;
  if (PyArg_ParseTuple(args, "ss", &prefix, &uri))
    return PyPiglet_status(piglet_add_namespace(asDB(self), prefix, uri));
  else
    return NULL;
}

PyObject *PyPiglet_del_namespace(PyObject *self, PyObject *args)
{
  char *prefix;
  if (PyArg_ParseTuple(args, "s", &prefix))
    return PyPiglet_status(piglet_del_namespace(asDB(self), prefix));
  else
    return NULL;
}

PyObject *PyPiglet_match(PyObject *self, PyObject *args)
{
  char *pattern;
  if (PyArg_ParseTuple(args, "s", &pattern)) {
    PyObject *matches = PyList_New(0);
    if (piglet_match(asDB(self), pattern, matches, PyPiglet_node_callback))
      return matches;
    else {
      Py_DECREF(matches);
      return PyPiglet_status(PigletError);
    }
  }
  return NULL;
}

PyObject *PyPiglet_transaction(PyObject *self, PyObject *args)
{
  if (PyArg_ParseTuple(args, ""))
    return PyPiglet_status(piglet_transaction(asDB(self)));
  else
    return NULL;
}

PyObject *PyPiglet_commit(PyObject *self, PyObject *args)
{
  if (PyArg_ParseTuple(args, ""))
    return PyPiglet_status(piglet_commit(asDB(self)));
  else
    return NULL;
}

PyObject *PyPiglet_rollback(PyObject *self, PyObject *args)
{
  if (PyArg_ParseTuple(args, ""))
    return PyPiglet_status(piglet_rollback(asDB(self)));
  else
    return NULL;
}

#define method(name, func, doc) {name, func, METH_VARARGS, PyDoc_STR(doc)}

static PyMethodDef PyPiglet_DBObject_methods[] = {
  method("close",          PyPiglet_close,           "close() -> bool"),
  method("add",            PyPiglet_add,             "add(s, p, o, src, temp) -> bool"),
  method("addPostProcess", PyPiglet_add_post_process, "addPostProcess(s, p, o) -> bool"),
  method("delete",         PyPiglet_del,             "delete(s, p, o, src, temp) -> bool"),
  method("count",          PyPiglet_count,           "count(s, p, o, src, temp) -> int"),
  method("query",          PyPiglet_query,           "query(s, p, o) -> list"),
  method("sources",        PyPiglet_sources,         "sources(s, p, o) -> list"),
  method("info",           PyPiglet_info,            "info(node) -> (uri, dt, lang)"),
  method("node",           PyPiglet_node,            "node(uri) -> node"),
  method("literal",        PyPiglet_literal,         "literal(string[, datatype, language]) -> node"),
  method("augmentLiteral", PyPiglet_augmentLiteral,  "augmentLiteral(literal, datatype) -> bool"),
  method("load",           PyPiglet_load,            "load(node, append) -> bool"),
  method("nodeToString",   PyPiglet_node_tostring,   "nodeToString(node) -> string"),
  method("tripleToString", PyPiglet_triple_tostring, "tripleToString(s, p, o) -> string"),
  method("expand",         PyPiglet_expand,          "expand(qname) -> uri"),
  method("expand_m3",      PyPiglet_expand_m3,       "expand_m3(qname) -> uri"),
  method("abbreviate",     PyPiglet_abbreviate,      "abbreviate(uri) -> qname"),
  method("addNamespace",   PyPiglet_add_namespace,   "addNamespace(prefix, uri) -> bool"),
  method("delNamespace",   PyPiglet_del_namespace,   "delNamespace(prefix) -> bool"),
  method("match",          PyPiglet_match,           "match(pattern) -> list"),
  method("transaction",    PyPiglet_transaction,     "transaction() -> bool"),
  method("commit",         PyPiglet_commit,          "commit() -> bool"),
  method("rollback",       PyPiglet_rollback,        "rollback() -> bool"),
  {NULL, NULL}
};

static PyTypeObject PyPiglet_DBType = {
  PyObject_HEAD_INIT(NULL)
  0,
  "piglet.DB",                           /*tp_name*/
  sizeof(PyPiglet_DBObject),             /*tp_basicsize*/
  0,                                     /*tp_itemsize*/
  (destructor)PyPiglet_DBObject_dealloc, /*tp_dealloc*/
  0,                                     /*tp_print*/
  0,                                     /*tp_getattr*/
  0,                                     /*tp_setattr*/
  0,                                     /*tp_compare*/
  0,                                     /*tp_repr*/
  0,                                     /*tp_as_number*/
  0,                                     /*tp_as_sequence*/
  0,                                     /*tp_as_mapping*/
  0,                                     /*tp_hash*/
  0,                                     /*tp_call*/
  0,                                     /*tp_str*/
  0,                                     /*tp_getattro*/
  0,                                     /*tp_setattro*/
  0,                                     /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,                    /*tp_flags*/
  0,                                     /*tp_doc*/
  0,                                     /*tp_traverse*/
  0,                                     /*tp_clear*/
  0,                                     /*tp_richcompare*/
  0,                                     /*tp_weaklistoffset*/
  0,                                     /*tp_iter*/
  0,                                     /*tp_iternext*/
  PyPiglet_DBObject_methods,             /*tp_methods*/
  0,                                     /*tp_members*/
  0,                                     /*tp_getset*/
  0,                                     /*tp_base*/
  0,                                     /*tp_dict*/
  0,                                     /*tp_descr_get*/
  0,                                     /*tp_descr_set*/
  0,                                     /*tp_dictoffset*/
  0,                                     /*tp_init*/
  0,                                     /*tp_alloc*/
  0,                                     /*tp_new*/
  0,                                     /*tp_free*/
  0,                                     /*tp_is_gc*/
};

static PyMethodDef PyPiglet_methods[] = {
  method("open", PyPiglet_open, "open(file) -> DB"),
  {NULL, NULL} /* sentinel */
};

PyMODINIT_FUNC initpiglet(void)
{
  if (PyType_Ready(&PyPiglet_DBType) >= 0) {
    PyObject *m = Py_InitModule3("piglet", PyPiglet_methods, PyDoc_STR("Piglet RDF triplestore"));
    if (m != NULL) {
      Py_INCREF(&PyPiglet_DBType);
      PyModule_AddObject(m, "DB", (PyObject *)&PyPiglet_DBType);
      PyPiglet_Exception = PyErr_NewException("piglet.error", NULL, NULL);
      Py_INCREF(PyPiglet_Exception);
      PyModule_AddObject(m, "error", PyPiglet_Exception);
      piglet_error_message = NULL;
    }
  }
}
