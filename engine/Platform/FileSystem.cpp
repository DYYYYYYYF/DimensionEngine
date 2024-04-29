#include "FileSystem.hpp"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>


bool FileSystemExists(const char* path) {
	struct stat buffer;
	return stat(path, &buffer) == 0;
}

bool FileSystemOpen(const char* path, FileMode mode, bool binary, FileHandle* handle) {
	handle->is_valid = false;
	handle->handle = 0;
	const char* ModeStr;

	if ((mode & eFile_Mode_Read) != 0 && (mode & eFile_Mode_Write) != 0) {
		ModeStr = binary ? "rwb" : "rw";
	}
	else if ((mode & eFile_Mode_Read) != 0 && (mode & eFile_Mode_Write) == 0) {
		ModeStr = binary ? "rb" : "r";
	}
	else if ((mode & eFile_Mode_Read) == 0 && (mode & eFile_Mode_Write) != 0) {
		ModeStr = binary ? "wb" : "w";
	}
	else {
		UL_ERROR("Invalid mode passed while trying to open file: %s", path);
		return false;
	}

	// Attempt to open the file
	FILE* File = fopen(path, ModeStr);
	if (!File) {
		UL_ERROR("Error opening file: %s", path);
		return false;
	}

	handle->handle = File;
	handle->is_valid = true;

	return true;
}

void FileSystemClose(FileHandle* handle) {
	if (handle->handle) {
		fclose((FILE*)handle->handle);
		handle->handle = nullptr;
		handle->is_valid = false;
	}
}

bool FileSystemReadLine(FileHandle* handle, char** line_buf) {
	if (handle->handle) {
		char buffer[32000];
		if (fgets(buffer, 32000, (FILE*)handle->handle) != 0) {
			size_t length = strlen(buffer);
			*line_buf = (char*)Memory::Allocate((sizeof(char) * length) + 1, MemoryType::eMemory_Type_String);
			strcpy(*line_buf, buffer);
			return true;
		}
	}

	return false;
}

bool FileSystemWriteLine(FileHandle* handle, const char* text) {
	if (handle->handle) {
		int result = fputs(text, (FILE*)handle->handle);
		if (result != EOF) {
			result = fputc('\n', (FILE*)handle->handle);
		}

		fflush((FILE*)handle->handle);
		return result != EOF;
	}

	return false;
}

bool FileSystemRead(FileHandle* handle, size_t data_size, void* out_data, size_t* out_bytes_read) {
	if (handle->handle && out_data) {
		*out_bytes_read = fread(out_data, 1, data_size, (FILE*)handle->handle);
		if (*out_bytes_read != data_size) {
			return false;
		}

		return true;
	}

	return false;
}

bool FileSystemReadAllBytes(FileHandle* handle, char** out_bytes, size_t* out_bytes_read) {
	if (handle->handle) {
		// File size
		fseek((FILE*)handle->handle, 0, SEEK_END);
		size_t size = ftell((FILE*)handle->handle);
		rewind((FILE*)handle->handle);

		*out_bytes = (char*)Memory::Allocate(sizeof(char) * size, MemoryType::eMemory_Type_String);
		*out_bytes_read = fread(*out_bytes, 1, size, (FILE*)handle->handle);
		if (*out_bytes_read != size) {
			return false;
		}

		return true;
	}

	return false;
}

bool FileSystemWrite(FileHandle* handle, size_t data_size, void* data, size_t* out_bytes_written) {
	if (handle->handle) {
		*out_bytes_written = fwrite(data, 1, data_size, (FILE*)handle->handle);
		if (*out_bytes_written != data_size) {
			return false;
		}

		fflush((FILE*)handle->handle);
		return true;
	}

	return false;
}