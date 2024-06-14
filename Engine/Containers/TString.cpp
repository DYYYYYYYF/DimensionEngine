#include "TString.hpp"

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

String::String() {
	this->Str = (char*)Memory::Allocate(sizeof(char), MemoryType::eMemory_Type_String);
	this->Str[0] = '\0';
	this->Length = 0;
}

String::String(const String& str) {
	this->Length = str.GetLength();
	if (Length == 0) {
		String();
	}

	this->Str = (char*)Memory::Allocate(this->Length + 1, MemoryType::eMemory_Type_String);
	Memory::Copy(this->Str, str.Str, Length);
	this->Str[Length] = '\0';
}

String::String(const char* str) {
	if (str == nullptr) {
		String();
	}

	this->Length = strlen(str);
	this->Str = (char*)Memory::Allocate(Length + 1, MemoryType::eMemory_Type_String);
	Memory::Copy(this->Str, str, Length);
	this->Str[Length] = '\0';
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
	Length = str.GetLength();
	Str = (char*)Memory::Allocate(Length + 1, MemoryType::eMemory_Type_String);
	Memory::Copy(Str, str.Str, Length);
	Str[Length] = '\0';
	return *this;
}

String& String::operator=(const char* str) {
	ReleaseMemory();
	Length = strlen(str);
	Str = (char*)Memory::Allocate(Length + 1, MemoryType::eMemory_Type_String);
	Memory::Copy(Str, str, Length);
	Str[Length] = '\0';
	return *this;
}

String& String::operator+=(const String& str) {
	String Temp = *this;
	if (Str != nullptr) {
		Memory::Free(Str, Length + 1, MemoryType::eMemory_Type_String);
		Str = nullptr;
	}

	this->Length += str.GetLength();
	this->Str = (char*)Memory::Allocate(Length + 1, MemoryType::eMemory_Type_String);
	strncpy(this->Str, Temp.ToString(), Temp.GetLength());
	strncpy(this->Str + Temp.GetLength(), str.ToString(), str.GetLength());
	Str[Length] = '\0';
	
	Temp.ReleaseMemory();
	return *this;
}

String operator+(const String& s1, const String& s2){
	String Result(s1);
	Result += s2;
	return Result;
}

bool String::Equal(const String& str) {
	if (str.GetLength() == 0) {
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
	for (uint32_t i = 0; i < Length; ++i) {
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

	if (this->Length > 0) {
		for (int i = 0; i < Length; ++i) {
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
	if (start >= Length) {
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