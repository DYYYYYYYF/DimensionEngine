#include "File.hpp"
#include "FileSystem.hpp"
#include "Core/EngineLogger.hpp"

#include <string>
#include <algorithm>
#include <sys/stat.h>
#ifdef DPLATFORM_MACOS
#include <sys/stat.h>
#endif

File::File() : IsValid(false) {}
File::~File() {}

File::File(const std::string& fn) {
	FullPath = fn;
	std::replace(FullPath.begin(), FullPath.end(), '\\', '/');
	size_t PrePathIndex = FullPath.find_last_of('/');
	size_t SufPathIndex = FullPath.find_last_of(".");

	// 没有前缀路径
	if (PrePathIndex == std::string::npos) {
		PrePath = "";
		FileName = FullPath.substr(0, SufPathIndex);
	}
	else {
		PrePath = FullPath.substr(0, PrePathIndex + 1);
		FileName = FullPath.substr(PrePathIndex + 1, SufPathIndex - PrePathIndex - 1);
	}

	FileType = FullPath.substr(SufPathIndex);
}

std::string File::ReadBytes() const {
	std::stringstream buffer;
	std::ifstream inFile(FullPath); // 打开文件

	if (!inFile) { // 检查文件是否成功打开
		std::cerr << "无法打开文件!" << std::endl;
		return "";
	}

	std::string line;
	while (std::getline(inFile, line)) { 
		buffer << line << std::endl;
	}

	inFile.close();

	return std::string(buffer.str());
}

bool File::WriteBytes(const char* source, size_t size, std::ios::openmode mode) {
	std::ofstream outFile(FullPath, mode);

	if (!outFile) { 
		std::cerr << "无法打开文件!" << std::endl;
		return false; 
	}

	// 向文件写入内容
	outFile.write(source, size);
	outFile.close(); // 关闭文件

	return true;
}

bool File::IsExist() const {
#ifdef _MSC_VER
	struct _stat buffer;
	return _stat(FullPath.c_str(), &buffer) == 0;
#else
	struct stat buffer;
	return ::stat(FullPath.c_str(), &buffer) == 0;
#endif
}
