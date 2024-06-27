#pragma once

#include "string.h"
#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

#include <cstdio>
#include <string.h>
#include <vector>
#include <iostream>

#ifdef DPLATFORM_MACOS
#include <ctype.h>
#endif

template<typename... Args>
inline void StringFormat(char* dst, size_t size, const char* format, Args... args) {
	size_t len = snprintf(NULL, 0, format, args...);
	if (len > 0) {
		snprintf(dst, len + 1, format, args...);
	}
}

class DAPI String {
public:
	String();
	String(const String& str);
	String(const char* str);

	template<typename... Args>
	String(const char* format, Args... args);

	~String();

	String& operator=(const String& str);
	String& operator=(const char* str);
	String& operator+=(const String& str);
	friend bool operator==(const String& s1, const String& s2) { return strcmp(s1.Str, s2.Str) == 0; }
	friend bool operator>(const String& s1, const String& s2) { return strcmp(s1.Str, s2.Str) > 0; }
	friend bool operator<(const String& s1, const String& s2) { return strcmp(s1.Str, s2.Str) < 0; }

	char& operator[](int i) { return Str[i]; }
	const char& operator[](int i) const { return Str[i]; }
	char& operator[](size_t i) { return Str[i]; }
	const char& operator[](size_t i) const { return Str[i]; }

	friend std::ostream& operator<<(std::ostream& os, String& str) { os << str.Str; return os; }
	friend std::istream& operator>>(std::istream& is, String& str) {
		char Temp[256];
		std::cin.getline(Temp, 256);
		str = Temp;
		return is;
	}

public:
	// Case-sensitive string comparison.
	bool Equal(const String& str);
	// Case-sensitive string comparison.
	bool Equal(const char* str);
	// Case-insensitive string comparison.
	bool Equali(const char* str);
	// Case-sensitive string comparison.
	bool Nequal( const char* str, size_t len);
	// Case-insensitive string comparison.
	bool Nequali(const char* str, size_t len);

	std::vector<char*> Split(char delimiter, bool trim_entries = true, bool include_empty = true);
	int IndexOf(char c);
	String& SubStr(size_t start, int length = -1);

	inline bool ToBool() {
		if (this->Str == nullptr) { return false; }
		return (strcmp(this->Str, "1") == 0) || (strcmp(this->Str, "true") == 0);
	}

	inline float ToFloat() {
		return(float)atof(this->Str);
	}

	uint32_t UTF8Length();
	bool BytesToCodepoint(const char* bytes, uint32_t offset, int* out_codepoint, unsigned char* out_advance);

public:
	size_t Length() const { return Len; }
	char* ToString() { return Str; }
	const char* ToString() const { return Str; }

private:
	void ReleaseMemory() {
		if (Str != nullptr) {
			size_t Size = 0;
			unsigned short  Alignment = 0;
			if (Memory::GetAlignmentSize(Str, &Size, &Alignment)) {
				Memory::FreeAligned(Str, Size, Alignment, MemoryType::eMemory_Type_String);
			}

			Str = nullptr;
			Len = 0;
		}
	}

// TODO: Remove above functions. Use class func.
public:
	static void Append(char* dst, size_t size, const char* src, const char* append) {
		StringFormat(dst, 512, "%s%s", src, append);
	}

	static void Append(char* dst, size_t size, const char* src, int append) {
		StringFormat(dst, 512, "%s%i", src, append);
	}

	static void Append(char* dst, size_t size, const char* src, bool append) {
		StringFormat(dst, 512, "%s%s", src, append ? "true" : "false");
	}

	static void Append(char* dst, size_t size, const char* src, float append) {
		StringFormat(dst, 512, "%s%f", src, append);
	}

	static void Append(char* dst, size_t size, const char* src, char append) {
		StringFormat(dst, 512, "%s%c", src, append);
	}

private:
	char* Str;
	size_t Len;

};

DAPI String operator+(const String& s1, const String& s2);

inline void StringFree(char* str) {
	if (str == nullptr) {
		return;
	}

	size_t Size = 0;
	unsigned short  Alignment = 0;
	if (Memory::GetAlignmentSize(str, &Size, &Alignment)) {
		Memory::FreeAligned(str, Size, Alignment, MemoryType::eMemory_Type_String);
	}
	else {

	}
}

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

inline char* StringCopy(const char* str) {
	size_t Length = strlen(str);
	char* Copy = (char*)Memory::Allocate(Length + 1, MemoryType::eMemory_Type_String);
	Memory::Copy(Copy, str, Length);
	Copy[Length] = '\0';
	return Copy;
}

inline bool StringToBool(const char* str) {
	if (str == nullptr) { return false; }
	return (strcmp(str, "1") == 0) || (strcmp(str, "true") == 0);
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
#if defined(__GNUC__)
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
#if defined(__GNUC__)
	return strncasecmp(str0, str1, len) == 0;
#elif (defined _MSC_VER)
	return _strnicmp(str0, str1, len) == 0;
#endif
	return false;
}

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

inline uint32_t StringUTF8Length(const char* Str) {
	uint32_t Leng = 0;
	for (uint32_t i = 0; i < UINT32_MAX; ++i, ++Leng) {
		int C = (int)Str[i];
		if (C == 0) {
			break;
		}

		if (C >= 0 && C < 127) {
			// Normal ascii character, don't increment again.
			i += 0;
		}
		else if ((C & 0xE0) == 0xC0) {
			// Double-byte character, increment once more.
			i += 1;
		}
		else if ((C & 0xF0) == 0xE0) {
			// Triple-byte, increment twice more.
			i += 2;
		}
		else if ((C & 0xF8) == 0xF0) {
			// 4-byte character, increment thrice more.
			i += 3;
		}
		else {
			// NOTE: Not supporting 5 and 6-type characters; return as invalid UTF-8.
			LOG_ERROR(" String::UTF8Length() Not supporting character more than 4 bytes. Invalid UTF-8.");
			return 0;
		}
	}

	return Leng;
}

inline bool StringBytesToCodepoint(const char* bytes, uint32_t offset, int* out_codepoint, unsigned char* out_advance) {
	int CodePoint = (int)bytes[offset];
	if (CodePoint >= 0 && CodePoint < 0x7F) {
		// Normal single-byte ascii character.
		*out_advance = 1;
		*out_codepoint = CodePoint;
		return true;
	}
	else if ((CodePoint & 0xE0) == 0xC0) {
		// Double-byte character, increment once more.
		CodePoint = ((bytes[offset + 0] & 0b00011111) << 6) + (bytes[offset + 1] & 0b00111111);
		*out_advance = 2;
		*out_codepoint = CodePoint;
		return true;
	}
	else if ((CodePoint & 0xF0) == 0xE0) {
		// Triple-byte, increment twice more.
		CodePoint = ((bytes[offset + 0] & 0b00011111) << 12) + ((bytes[offset + 1] & 0b00111111) << 6) + (bytes[offset + 2] & 0b00111111);
		*out_advance = 3;
		*out_codepoint = CodePoint;
		return true;
	}
	else if ((CodePoint & 0xF8) == 0xF0) {
		// 4-byte character, increment thrice more.
		CodePoint = ((bytes[offset + 0] & 0b00011111) << 18) + ((bytes[offset + 1] & 0b00111111) << 12) + ((bytes[offset + 1] & 0b00111111) << 6) + (bytes[offset + 2] & 0b00111111);
		*out_advance = 4;
		*out_codepoint = CodePoint;
		return true;
	}
	else {
		// NOTE: Not supporting 5 and 6-type characters; return as invalid UTF-8.
		*out_codepoint = 0;
		*out_advance = 0;
		LOG_ERROR(" String::BytesToCodepoint() Not supporting character more than 4 bytes. Invalid UTF-8.");
		return false;
	}
}
