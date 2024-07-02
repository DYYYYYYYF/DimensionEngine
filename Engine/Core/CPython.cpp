#include "CPython.hpp"

#include "Core/EngineLogger.hpp"
#include <Python.h>

CPythonModule::CPythonModule(const char* pyFileName) {
	FileName = pyFileName;
}

void CPythonModule::ExecuteFunc(const char* funcName) {
	Py_Initialize();

	PyRun_SimpleString("import sys");
	PyRun_SimpleString("sys.path.append('./')");

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

	PyObject_CallObject(PyFunc, nullptr);
	Py_Finalize();
}
