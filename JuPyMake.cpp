#include <Python.h>

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include <string>

#include <polymake/Main.h>
#include <polymake/Vector.h>

/*
 * Python different version stuff
 */

#if PY_MAJOR_VERSION >= 3
#define to_python_string(o) PyUnicode_FromString(o)
#else
#define to_python_string(o) PyString_FromString(const_cast<char*>(o))
#endif

polymake::Main* main_polymake_session;
PyObject* JuPyMakeError;

static PyObject * ToPyBool( bool input )
{
    if(input){
      return Py_True;
    }
    return Py_False;
}

/*
 * Python functions
 */
static PyObject * RunPolymakeCommand( PyObject* self, PyObject* args )
{
    const char * input_string;
    if (! PyArg_ParseTuple(args, "s", &input_string) )
        return NULL;
    std::string polymake_input(input_string);
    bool exception;
    bool success;
    std::string output;
    std::string error;
    try{
      std::tie(success,output,error) = main_polymake_session->shell_execute(polymake_input);
      exception = false;
    }catch(const std::exception& e ){
      exception = true;
      success = false;
      error = e.what();
      output = "";
    }
    return PyTuple_Pack( 4, ToPyBool( exception ), ToPyBool( success ), to_python_string( output.c_str() ), to_python_string( error.c_str() ) );
}

static PyObject * GetPolymakeCompletion( PyObject* self, PyObject* args )
{
    const char* input_string;
    if (! PyArg_ParseTuple(args, "s", &input_string) )
        return NULL;
    std::string polymake_input(input_string);
    std::vector<std::string> completions;
    try{
        completions = main_polymake_session->shell_complete(polymake_input);
    }catch(const std::exception& e ){
         PyErr_SetString( JuPyMakeError, e.what() );
         return NULL;
    }
    int completions_length = completions.size();
    PyObject* return_list = PyList_New( completions_length );
    for(int i=0;i<completions_length;i++){
        PyList_SetItem( return_list, i, to_python_string( completions[ i ].c_str() ) );
    }
    return return_list;
}

static PyObject * GetContextHelp( PyObject* self, PyObject* args, PyObject* kwargs )
{
    const char* input_string;
    int position = -1;
    bool full=false;
    bool html=false;
    static char* kwlist[] = { "input", "position", "full", "html", NULL};
    if (! PyArg_ParseTupleAndKeywords(args, kwargs, "s|ipp", kwlist, &input_string, &position, &full, &html ) )
        return NULL;
    std::string polymake_input(input_string);
    if(position == -1){
        position = static_cast<int>(std::string::npos);
    }
    std::vector<std::string> results;
    try{
        results = main_polymake_session->shell_context_help(polymake_input,position,full,html);
    }catch(const std::exception& e ){
        PyErr_SetString( JuPyMakeError, e.what() );
        return NULL;
    }
    int results_length = results.size();
    PyObject* return_list = PyList_New( results_length );
    for(int i=0;i<results_length;i++){
        PyList_SetItem( return_list, i, to_python_string( results[ i ].c_str() ) );
    }
    return return_list;
}

static PyObject * InitializePolymake( PyObject* self )
{
    try{
        main_polymake_session = new polymake::Main;
        main_polymake_session->shell_enable();
        main_polymake_session->set_application("polytope");
    }catch(const std::exception& e){
        PyErr_SetString( JuPyMakeError, e.what() );
        return NULL;
    }
    return Py_True;
}
/*
 * Python mixed init stuff
 */

struct module_state {
    PyObject *error;
};

#if PY_MAJOR_VERSION >= 3
#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))
#else
#define GETSTATE(m) (&_state)
static struct module_state _state;
#endif

static PyObject * error_out(PyObject *m) {
    struct module_state *st = GETSTATE(m);
    PyErr_SetString(st->error, "something bad happened");
    return NULL;
}

static PyMethodDef JuPyMakeMethods[] = {
    {"RunPolymakeCommand",(PyCFunction)RunPolymakeCommand, METH_VARARGS,
     "Runs a polymake command"},
    {"GetPolymakeCompletion",(PyCFunction)GetPolymakeCompletion, METH_VARARGS,
     "Get tab completions of string"},
    {"GetContextHelp",(PyCFunction)GetContextHelp,METH_VARARGS | METH_KEYWORDS,
      "Get context help of string"},
    {"InitializePolymake",(PyCFunction)InitializePolymake,METH_NOARGS,
      "Initialize the polymake session"},
      
    {NULL, NULL, 0, NULL}        /* Sentinel */
};


#if PY_MAJOR_VERSION >= 3

static int JuPyMake_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(GETSTATE(m)->error);
    return 0;
}

static int JuPyMake_clear(PyObject *m) {
    Py_CLEAR(GETSTATE(m)->error);
    return 0;
}


static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "JuPyMake",
        NULL,
        sizeof(struct module_state),
        JuPyMakeMethods,
        NULL,
        JuPyMake_traverse,
        JuPyMake_clear,
        NULL
};

#define INITERROR return NULL

PyMODINIT_FUNC PyInit_JuPyMake(void)

#else
#define INITERROR return

extern "C" void initJuPyMake(void)
#endif
{
#if PY_MAJOR_VERSION >= 3
    PyObject *module = PyModule_Create(&moduledef);
#else
    PyObject *module = Py_InitModule("JuPyMake", JuPyMakeMethods);
#endif

    if (module == NULL)
        INITERROR;
    struct module_state *st = GETSTATE(module);
    JuPyMakeError = PyErr_NewException(const_cast<char*>("JuPyMake.PolymakeError"), NULL, NULL );
    Py_INCREF( JuPyMakeError );

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}
