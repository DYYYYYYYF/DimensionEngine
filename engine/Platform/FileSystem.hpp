#pragma once

#include "Defines.hpp"

struct FileHandle {
	void* handle;
	bool is_valid;
};

enum FileMode {
	eFile_Mode_Read = 0x1,
	eFile_Mode_Write = 0x2
};

DAPI bool FileSystemExists(const char* path);

DAPI bool FileSystemOpen(const char* path, FileMode mode, bool binary, FileHandle* handle);

DAPI void FileSystemClose(FileHandle* handle);

DAPI bool FileSystemReadLine(FileHandle* handle, char** line_buf);

DAPI bool FileSystemWriteLine(FileHandle* handle, const char* text);

DAPI bool FileSystemRead(FileHandle* handle, size_t data_size, void* out_data, size_t* out_bytes_read);

DAPI bool FileSystemReadAllBytes(FileHandle* handle, char** out_bytes, size_t* out_bytes_read);

DAPI bool FileSystemWrite(FileHandle* handle, size_t data_size, void* data, size_t* out_bytes_written);