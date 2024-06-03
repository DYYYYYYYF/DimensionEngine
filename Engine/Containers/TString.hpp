#include "string.h"

inline char* Strtrim(char* str) {

	while (isspace((unsigned char)*str)) {
		str++;
	}

	if (*str) {
		char* p = str;
		while (*p) {
			p++;
		}

		while (isspace((unsigned char)*(--p)))
			;
		p[1] = '\0';
	}

	return str;
}

inline int StringIndexOf(char* str, char c) {
	if (!str) {
		return -1;
	}

	size_t Length = strlen(str);
	if (Length > 0) {
		for (int i = 0; i < Length; ++i) {
			if (str[i] == c) {
				return i;
			}
		}
	}

	return -1;
}

inline void StringMid(char* dst, const char* src, size_t start, int length = -1) {
	if (length == 0) {
		return;
	}

	size_t SrcLength = strlen(src);
	if (start >= SrcLength) {
		dst[0] = 0;
		return;
	}

	if (length > 0) {
		for (size_t i = start, j = 0; j < length && src[i]; ++i, ++j) {
			dst[j] = src[i];
		}
		dst[start + length] = 0;
	}
	else {
		// If a negative value is passed, proceed to the end of string.
		size_t j = 0;
		for (size_t i = start; src[i]; ++i, ++j) {
			dst[j] = src[i];
		}

		dst[start + j] = 0;
	}
}

inline std::vector<char*> StringSplit(const char* str, char delimiter, bool trim_entries, bool include_empty) {
	std::vector<char*> Vector;

	if (str == nullptr) {
		return Vector;
	}

	char* Result = nullptr;
	uint32_t TrimmedLength = 0;
	uint32_t Length = (uint32_t)strlen(str);
	char Buffer[16384];
	uint32_t CurrentLength = 0;

	// Iterate each character until a delimiter is reached.
	for (uint32_t i = 0; i < Length; ++i) {
		char c = str[i];

		// Found delimiter, finalize string.
		if (c == delimiter) {
			Buffer[CurrentLength] = '\0';
			Result = Buffer;
			TrimmedLength = CurrentLength;

			//Trim if applicable.
			if (trim_entries && CurrentLength > 0) {
				Result = Strtrim(Result);
				TrimmedLength = (uint32_t)strlen(Result);
			}

			// Add new entry.
			if (TrimmedLength > 0 || include_empty) {
				char* Entry = (char*)Memory::Allocate(sizeof(char) * (TrimmedLength + 1), MemoryType::eMemory_Type_String);
				if (TrimmedLength == 0) {
					Entry[0] = '\0';
				}
				else {
					strncpy(Entry, Result, TrimmedLength);
					Entry[TrimmedLength] = '\0';
				}

				Vector.push_back(Entry);
			}

			// Clear the buffer.
			Memory::Zero(Buffer, sizeof(char) * 16384);
			CurrentLength = 0;
			continue;
		}

		Buffer[CurrentLength] = c;
		CurrentLength++;
	}

	// At the end of the string. If any chars are queued up, read them.
	Result = Buffer;
	TrimmedLength = CurrentLength;
	// Trim if applicable
	if (trim_entries && CurrentLength > 0) {
		Result = Strtrim(Result);
		TrimmedLength = (uint32_t)strlen(Result);
	}

	// Add new entry.
	if (TrimmedLength > 0 || include_empty) {
		char* Entry = (char*)Memory::Allocate(sizeof(char) * (TrimmedLength + 1), MemoryType::eMemory_Type_String);
		if (TrimmedLength == 0) {
			Entry[0] = '\0';
		}
		else {
			strncpy(Entry, Result, TrimmedLength);
			Entry[TrimmedLength] = '\0';
		}

		Vector.push_back(Entry);
	}

	return Vector;
}

inline bool StringToBool(const char* str) {
	if (str == nullptr) {return false;}
	return (strcmp(str, "1") == 0) || (strcmp(str, "true") == 0);
}

inline char* StringCopy(const char* str) {
	size_t Length = strlen(str);
	char* Copy = (char*)Memory::Allocate(Length + 1, MemoryType::eMemory_Type_String);
	Memory::Copy(Copy, str, Length);
	Copy[Length] = '\0';
	return Copy;
}

inline bool StringToFloat(const char* str, float* f) {
	*f = (float)atof(str);
	return true;
}

// Case-sensitive string comparison.
inline bool StringEqual(const char* str0, const char* str1) {
	return strcmp(str0, str1) == 0;
}

// Case-insensitive string comparison.
inline bool StringEquali(const char* str0, const char* str1) {
#if defined(__GUNC__)
	return strcasecmp(str0, str1) == 0;
#elif (defined _MSC_VER)
	return _strcmpi(str0, str1) == 0;
#endif
	return false;
}

inline bool StringNequal(const char* str0, const char* str1, size_t len) {
	return strncmp(str0, str1, len);
}

inline bool StringNequali(const char* str0, const char* str1, size_t len) {
#if defined(__GUNC__)
	return strncasecmp(str0, str1, len) == 0;
#elif (defined _MSC_VER)
	return _strnicmp(str0, str1, len) == 0;
#endif
	return false;
}

class String {
public:
	static void Append(char* dst, const char* src, const char* append) {
		sprintf(dst, "%s%s", src, append);
	}

	static void Append(char* dst, const char* src, int append) {
		sprintf(dst, "%s%i", src, append);
	}

	static void Append(char* dst, const char* src, bool append) {
		sprintf(dst, "%s%s", src, append ? "true" : "false");
	}

	static void Append(char* dst, const char* src, float append) {
		sprintf(dst, "%s%f", src, append);
	}

	static void Append(char* dst, const char* src, char append) {
		sprintf(dst, "%s%c", src, append);
	}

};

inline void StringDirectoryFromPath(char* dst, const char* path) {
	size_t Length = strlen(path);
	for (int i = (int)Length; i >= 0; --i) {
		char c = path[i];
		if (c == '/' || c == '\\') {
			strncpy(dst, path, i + 1);
			dst[i + 1] = '\0';
			return;
		}
	}
}

inline void StringFilenameFromPath(char* dst, const char* path) {
	size_t Length = strlen(path);
	for (int i = (int)Length; i >= 0; --i) {
		char c = path[i];
		if (c == '/' || c == '\\') {
			strcpy(dst, path + i + 1);
			return;
		}
	}
}

inline void StringFilenameNoExtensionFromPath(char* dst, const char* path) {
	size_t Length = strlen(path);
	size_t Start = 0;
	size_t End = 0;
	for (int i = (int)Length; i >= 0; --i) {
		char c = path[i];
		if (End == 0 && c == '.') {
			End = i;
		}
		if (Start == 0 && (c == '/' || c == '\\')) {
			Start = i + 1;
			break;
		}
	}

	StringMid(dst, path, Start, static_cast<int>(End - Start));
}