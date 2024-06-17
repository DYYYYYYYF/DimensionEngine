#include "TString.hpp"

String::String() {
	this->Str = (char*)Memory::Allocate(sizeof(char), MemoryType::eMemory_Type_String);
	this->Str[0] = '\0';
	this->Len = 0;
}

String::String(const String& str) {
	this->Len = str.Length();
	if (Len == 0) {
		String();
	}

	this->Str = (char*)Memory::Allocate(this->Len + 1, MemoryType::eMemory_Type_String);
	Memory::Copy(this->Str, str.Str, Len);
	this->Str[Len] = '\0';
}

String::String(const char* str) {
	if (str == nullptr) {
		String();
	}

	this->Len = strlen(str);
	this->Str = (char*)Memory::Allocate(Len + 1, MemoryType::eMemory_Type_String);
	Memory::Copy(this->Str, str, Len);
	this->Str[Len] = '\0';
}

template<typename... Args>
String::String(const char* format, Args... args) {
	// TODO: Implement
}

String::~String() {
	ReleaseMemory();
}

String& String::operator=(const String& str) {
	if (this == &str) {
		return *this;
	}

	ReleaseMemory();
	Len = str.Length();
	Str = (char*)Memory::Allocate(Len + 1, MemoryType::eMemory_Type_String);
	Memory::Copy(Str, str.Str, Len);
	Str[Len] = '\0';
	return *this;
}

String& String::operator=(const char* str) {
	ReleaseMemory();
	Len = strlen(str);
	Str = (char*)Memory::Allocate(Len + 1, MemoryType::eMemory_Type_String);
	Memory::Copy(Str, str, Len);
	Str[Len] = '\0';
	return *this;
}

String& String::operator+=(const String& str) {
	String Temp = *this;
	if (Str != nullptr) {
		Memory::Free(Str, Len + 1, MemoryType::eMemory_Type_String);
		Str = nullptr;
	}

	this->Len += str.Length();
	this->Str = (char*)Memory::Allocate(Len + 1, MemoryType::eMemory_Type_String);
	strncpy(this->Str, Temp.ToString(), Temp.Length());
	strncpy(this->Str + Temp.Length(), str.ToString(), str.Length());
	Str[Len] = '\0';
	
	Temp.ReleaseMemory();
	return *this;
}

String operator+(const String& s1, const String& s2){
	String Result(s1);
	Result += s2;
	return Result;
}

bool String::Equal(const String& str) {
	if (str.Length() == 0) {
		return false;
	}

	return *this == str;
}

bool String::Equal(const char* str) {
	if (str == nullptr || str[0] == '\0') {
		return false;
	}

	return *this == str;
}

// Case-insensitive string comparison.
bool  String::Equali(const char* str) {
#if defined(__GUNC__)
	return strcasecmp(this->Str, str) == 0;
#elif (defined _MSC_VER)
	return _strcmpi(this->Str, str) == 0;
#endif
	return false;
}

bool String::Nequal(const char* str, size_t len) {
	return strncmp(this->Str, str, len);
}

bool String::Nequali(const char* str, size_t len) {
#if defined(__GUNC__)
	return strncasecmp(this->Str, str, len) == 0;
#elif (defined _MSC_VER)
	return _strnicmp(this->Str, str, len) == 0;
#endif
	return false;
}

std::vector<char*> String::Split(char delimiter, bool trim_entries /*= true*/, bool include_empty /*= true*/) {
	std::vector<char*> Vector;

	if (this->Str == nullptr) {
		return Vector;
	}

	char* Result = nullptr;
	uint32_t TrimmedLength = 0;
	char Buffer[16384];
	uint32_t CurrentLength = 0;

	// Iterate each character until a delimiter is reached.
	for (uint32_t i = 0; i < Len; ++i) {
		char c = this->Str[i];

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

int String::IndexOf(char c) {
	if (this->Str == nullptr) {
		return -1;
	}

	if (this->Len > 0) {
		for (int i = 0; i < Len; ++i) {
			if (this->Str[i] == c) {
				return i;
			}
		}
	}

	return -1;
}


String& String::SubStr(size_t start, int length /*= -1*/) {
	if (length == 0) {
		return *this;
	}

	String src(*this);
	if (start >= Len) {
		this->Str[0] = 0;
		return *this;
	}

	if (length > 0) {
		for (size_t i = start, j = 0; j < length && src[i]; ++i, ++j) {
			this->Str[j] = src[i];
		}
		this->Str[start + length] = '\0';
	}
	else {
		// If a negative value is passed, proceed to the end of string.
		size_t j = 0;
		for (size_t i = start; src[i]; ++i, ++j) {
			this->Str[j] = src[i];
		}

		this->Str[start + j] = '\0';
	}

	src.ReleaseMemory();

	return *this;
}

uint32_t String::UTF8Length() {
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
			// 4-byte character, incre,ent thrice more.
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

bool String::BytesToCodepoint(const char* bytes, uint32_t offset, int* out_codepoint, unsigned char* out_advance) {
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
