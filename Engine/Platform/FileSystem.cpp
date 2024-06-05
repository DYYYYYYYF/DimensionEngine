#include "FileSystem.hpp"

#include "Core/EngineLogger.hpp"
#include "Core/DMemory.hpp"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>


bool FileSystemExists(const char* path) {
#ifdef _MSC_VER
	struct _stat buffer;
	return _stat(path, &buffer) == 0;
#else
	struct stat buffer;
	return stat(path, &buffer) == 0;
#endif
}

bool FileSystemOpen(const char* path, FileMode mode, bool binary, FileHandle* handle) {
	handle->is_valid = false;
	handle->handle = nullptr;
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
		LOG_ERROR("Invalid mode passed while trying to open file: %s", path);
		return false;
	}

	// Attempt to open the file
	FILE* File = fopen(path, ModeStr);
	if (!File) {
		LOG_ERROR("Error opening file: %s", path);
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

bool FileSystemSize(FileHandle* handle, size_t* size) {
	if (handle->handle) {
		fseek((FILE*)handle->handle, 0, SEEK_END);
		*size = ftell((FILE*)handle->handle);
		rewind((FILE*)handle->handle);
		return true;
	}

	return false;
}

bool FileSystemReadLine(FileHandle* handle, int max_length, char** line_buf, size_t* length) {
	if (handle->handle && line_buf && length && max_length > 0) {
		char* buffer = *line_buf;
		if (fgets(buffer, max_length, (FILE*)handle->handle) != 0) {
			*length = strlen(*line_buf);
			//buffer[*length - 1] = '\0';
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

bool FileSystemReadAllBytes(FileHandle* handle, unsigned char* out_bytes, size_t* out_bytes_read) {
	if (handle->handle) {
		// File size
		size_t size = 0;
		if (!FileSystemSize(handle, &size)) {
			return false;
		}

		*out_bytes_read = fread(out_bytes, 1, size, (FILE*)handle->handle);
		return *out_bytes_read == size;
	}

	return false;
}

bool FileSystemReadAllText(FileHandle* handle, char* out_text, size_t* out_bytes_read) {
	if (handle->handle) {
		// File size
		size_t size = 0;
		if (!FileSystemSize(handle, &size)) {
			return false;
		}

		*out_bytes_read = fread(out_text, 1, size, (FILE*)handle->handle);
		return *out_bytes_read == size;
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