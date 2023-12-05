#include "FileUtil.hpp"

// TODO: Remove
#include <iostream>
#include "FileUtil.hpp"

std::fstream FileUtil::m_FileStream;

bool FileUtil::LoadFile(const char* file, std::ios::openmode mode) {
	m_FileStream.open(file, mode);

	if (!m_FileStream.is_open()) {
		WARN("Open %s failed.", file);
		return false;
	}
	
	return true;
}