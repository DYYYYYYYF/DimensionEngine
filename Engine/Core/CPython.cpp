#include "CPython.hpp"

#include "Core/EngineLogger.hpp"
#include <Python.h>

CPythonModule::CPythonModule(const char* pyFileName) {
	FileName = pyFileName;
}

void CPythonModule::ExecuteFunc(const char* funcName, const char* shaderLanguage) {
	Py_Initialize();

	PyRun_SimpleString("import sys");
#ifdef DPLATFORM_WINDOWS
    PyRun_SimpleString("sys.path.append('../Scripts')");
#elif DPLATFORM_MACOS
    PyRun_SimpleString("sys.path.append('../../Scripts')");
#endif

	PyObject* PyMoudle = PyImport_ImportModule(FileName);
	if (PyMoudle == nullptr) {
		LOG_WARN("No such python script named '%s'.", FileName);
		return;
	}

	PyObject* PyFunc = PyObject_GetAttrString(PyMoudle, funcName);
	if (!PyFunc || !PyCallable_Check(PyFunc)) {
		LOG_WARN("No such function '%s' in python script '%s'.", funcName, FileName);
		return;
	}
    
    // Attributes
    PyObject* PyParams = PyTuple_New(1);
    
    /**
	 * s: string
	 * i: int
	 */
    PyTuple_SetItem(PyParams, 0, Py_BuildValue("s", shaderLanguage));

	PyObject_CallObject(PyFunc, PyParams);
	Py_Finalize();
}
