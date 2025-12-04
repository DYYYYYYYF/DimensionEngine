#pragma once

#include "Defines.hpp"
#include <string>
#include <vector>

#include <fstream>
#include <sstream>

enum class eFileMode {
	Read = 0x1,
	Write = 0x2
};

class File {
public:
	DAPI File();
	DAPI File(const std::string& filename);
	DAPI virtual ~File();

public:
	std::string GetFilename() const { return FileName; }
	std::string GetFullPath() const { return FullPath; }
	std::string GetPrePath() const { return PrePath; }
	std::string GetFileType() const { return FileType; }
	DAPI std::string ReadBytes() const;
	DAPI bool WriteBytes(const char* source, size_t size, std::ios::openmode mode = std::ios::ate);
	DAPI bool IsExist() const;

protected:
	std::string FullPath;
	std::string FileName;
	std::string PrePath;
	std::string FileType;
	bool IsValid;
};
