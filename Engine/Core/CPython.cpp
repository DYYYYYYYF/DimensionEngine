#include "CPython.hpp"
#include "Core/EngineLogger.hpp"

#include <Python.h>

CPythonModule::CPythonModule(const char* pyFileName) {
	FileName = pyFileName;
}

void CPythonModule::ExecuteFunc(const char* funcName, const char* shaderLanguage) {
	Py_Initialize();

	PyRun_SimpleString("import sys");
	std::string Header = std::string("sys.path.append('") + ROOT_PATH + "/Scripts')";
    PyRun_SimpleString(Header.c_str());

	PyObject* PyMoudle = PyImport_ImportModule(FileName);
	if (PyMoudle == nullptr) {
		GLOG(Log::eWarn, "No such python script named '%s'.", FileName);
		Py_Finalize();
		return;
	}

	PyObject* PyFunc = PyObject_GetAttrString(PyMoudle, funcName);
	if (!PyFunc || !PyCallable_Check(PyFunc)) {
		GLOG(Log::eWarn, "No such function '%s' in python script '%s'.", funcName, FileName);
		Py_Finalize();
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

	// TODO: Reload shader module.
}
