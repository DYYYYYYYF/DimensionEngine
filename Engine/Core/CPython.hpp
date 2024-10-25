#pragma once

#include "Defines.hpp"

class DAPI CPythonModule {
public:
	CPythonModule() {}
	CPythonModule(const char* pyFileName);

	void ExecuteFunc(const char* funcName, const char* shaderLanguage);

public:
	void SetPythonFile(const char* pyFileName) { FileName = pyFileName; }

private:
	const char* FileName = nullptr;
};
