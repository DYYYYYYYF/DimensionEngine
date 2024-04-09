#pragma once
#pragma once
#include <fstream>
#include "../../engine/core/EngineLogger.hpp"

class FileUtil {
public:
	FileUtil() {}
	virtual ~FileUtil() { m_FileStream.close(); }

	static bool LoadFile(const char* file, std::ios::openmode mode = std::ios::in | std::ios::out);
	static std::fstream* GetFileStream() { return &m_FileStream; }

private:
	static std::fstream m_FileStream;
};