#pragma once

#include "Defines.hpp"

struct FileHandle {
	void* handle = nullptr;
	bool is_valid;
};

enum FileMode {
	eFile_Mode_Read = 0x1,
	eFile_Mode_Write = 0x2
};

#define CLOSE_IF_FAILED(func, handle)			\
	if (!func)	{								\
		LOG_ERROR("File operation failed.");	\
		FileSystemClose(handle);				\
		return false;							\
	}

/*
* Checks if a file with the given path exists.
* @param path The path of the file to be checked.
* @returns True if exists.
*/
DAPI bool FileSystemExists(const char* path);

/*
* Attempt to open file located at path.
* @param path The path of the file to be opened.
* @param mode Mode flags for the file when opened (read/write). See FileMode enum in FileSystem.hpp.
* @param binary Indicates if the file should be opened in binary mode.
* @param handle A pointer to a FileHandle structure which holds the handle information.
* @returns True if opened successfully.
*/
DAPI bool FileSystemOpen(const char* path, FileMode mode, bool binary, FileHandle* handle);

/*
* Close the provided handle to a file.
*/
DAPI void FileSystemClose(FileHandle* handle);

/*
* @brief Attempts to read the size of the file to which handle is attached.
* 
* @param handle The file handle.
* @param size A point to hold the file size.
*/
DAPI bool FileSystemSize(FileHandle* handle, size_t* size);

/*
* Reads up to a newline or EOF. Allocates *line_buf, which must be freed by the caller.
* @param handle A pointer to a FileHandle structure.
* @param max_length The maximum length to be read from the line.
* @param line_buf A pointer to a character array which will be allocated and populated by this method.
* @param length A pointer to hold the line length read from the file.
* @returns True if successful.
*/
DAPI bool FileSystemReadLine(FileHandle* handle, int max_length, char** line_buf, size_t* length);

/*
* Writes text to the provided file, appending a '\n' afterward.
* @param handle A pointer to a FileHandle structure.
* @param text The text to be written.
* @returns True if successful.
*/
DAPI bool FileSystemWriteLine(FileHandle* handle, const char* text);

/*
* Reads up to data_size bytes of data into out_bytes_read.
* Allocates *out_data, which must be freed by the caller.
* @param handle A pointer to a FileHandle structure.
* @param data_size The number of bytes to read.
* @param out_data A pointer to a block of memory to be populated by this method.
* @param out_bytes_read A pointer to a number which will be populated with the number of bytes actually read from the file.
*/
DAPI bool FileSystemRead(FileHandle* handle, size_t data_size, void* out_data, size_t* out_bytes_read);

/*
* Reads all bytes of data into ou_bytes.
* @param handle A pointer to a FileHandle structure.
* @param out_data A pointer to a block of memory to be populated by this method.
* @param out_bytes_read A pointer to a number which will be populated with the number of bytes actually read from the file.
*/
DAPI bool FileSystemReadAllBytes(FileHandle* handle, unsigned char* out_bytes, size_t* out_bytes_read);

/*
* Reads all characters of data into out_text.
* @param handle A pointer to a FileHandle structure.
* @param out_text A character array which will be populated by this method.
* @param out_bytes_read A pointer to a number which will be populated with the number of bytes actually read from the file.
*/
DAPI bool FileSystemReadAllText(FileHandle* handle,char* out_text, size_t* out_bytes_read);

DAPI bool FileSystemWrite(FileHandle* handle, size_t data_size, void* data, size_t* out_bytes_written);