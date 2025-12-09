#include "FString.hpp"

FString::FString() : Str(nullptr), Len(0), Capacity(0) {
	InitializeEmpty();
}

FString::FString(const FString& str) : Str(nullptr), Len(0), Capacity(0) {
	if (str.Len > 0 && str.Str != nullptr) {
		EnsureCapacity(str.Len);
		Len = str.Len;
		Memory::Copy(Str, str.Str, Len);
		Str[Len] = '\0';
	}
	else {
		InitializeEmpty();
	}
}

FString::FString(FString&& str) noexcept : Str(str.Str), Len(str.Len), Capacity(str.Capacity) {
	str.Str = nullptr;
	str.Len = 0;
	str.Capacity = 0;
}

FString::FString(const char* str) : Str(nullptr), Len(0), Capacity(0) {
	if (str != nullptr && str[0] != '\0') {
		Len = strlen(str);
		EnsureCapacity(Len);
		Memory::Copy(Str, str, Len);
		Str[Len] = '\0';
	}
	else {
		InitializeEmpty();
	}
}

// 注意：模板构造函数的定义已经移到头文件中

FString::~FString() {
	ReleaseMemory();
}

// 赋值操作符实现
FString& FString::operator=(const FString& str) {
	if (this == &str) {
		return *this;
	}

	if (str.Len > 0 && str.Str != nullptr) {
		EnsureCapacity(str.Len);
		Len = str.Len;
		Memory::Copy(Str, str.Str, Len);
		Str[Len] = '\0';
	}
	else {
		Clear();
	}

	return *this;
}

FString& FString::operator=(FString&& str) noexcept {
	if (this != &str) {
		ReleaseMemory();
		Str = str.Str;
		Len = str.Len;
		Capacity = str.Capacity;

		str.Str = nullptr;
		str.Len = 0;
		str.Capacity = 0;
	}
	return *this;
}

FString& FString::operator=(const char* str) {
	if (str != nullptr && str[0] != '\0') {
		size_t new_len = strlen(str);
		EnsureCapacity(new_len);
		Len = new_len;
		Memory::Copy(Str, str, Len);
		Str[Len] = '\0';
	}
	else {
		Clear();
	}
	return *this;
}

// 复合赋值操作符实现
FString& FString::operator+=(const FString& str) {
	if (str.Len > 0) {
		size_t new_len = Len + str.Len;
		EnsureCapacity(new_len);
		Memory::Copy(Str + Len, str.Str, str.Len);
		Len = new_len;
		Str[Len] = '\0';
	}
	return *this;
}

FString& FString::operator+=(const char* str) {
	if (str != nullptr && str[0] != '\0') {
		size_t str_len = strlen(str);
		size_t new_len = Len + str_len;
		EnsureCapacity(new_len);
		Memory::Copy(Str + Len, str, str_len);
		Len = new_len;
		Str[Len] = '\0';
	}
	return *this;
}

FString& FString::operator+=(char c) {
	size_t new_len = Len + 1;
	EnsureCapacity(new_len);
	Str[Len] = c;
	Len = new_len;
	Str[Len] = '\0';
	return *this;
}

// 私有辅助方法实现
void FString::InitializeEmpty() {
	EnsureCapacity(0);  // 至少分配1个字符用于'\0'
	Str[0] = '\0';
	Len = 0;
}

void FString::ReleaseMemory() {
	if (Str != nullptr) {
		Memory::Free(Str, MemoryType::eMemory_Type_String);
		Str = nullptr;
		Len = 0;
		Capacity = 0;
	}
}

void FString::EnsureCapacity(size_t required_size) {
	if (required_size >= Capacity) {
		size_t new_capacity = required_size * 2 + 16;  // 预留增长空间
		char* new_str = (char*)Memory::Allocate(new_capacity + 1, MemoryType::eMemory_Type_String);

		if (new_str == nullptr) {
			GLOG(Log::eFatal, "Failed to allocate memory for FString");
			return;
		}

		if (Str != nullptr && Len > 0) {
			Memory::Copy(new_str, Str, Len);
			Memory::Free(Str, MemoryType::eMemory_Type_String);
		}

		Str = new_str;
		Capacity = new_capacity;
	}
}

void FString::Clear() {
	if (Str != nullptr) {
		Str[0] = '\0';
		Len = 0;
	}
}

// 字符串操作方法实现
bool FString::Equal(const FString& str) const {
	return *this == str;
}

bool FString::Equal(const char* str) const {
	if (str == nullptr) return Len == 0;
	return strcmp(Str, str) == 0;
}

bool FString::Equali(const char* str) const {
	if (str == nullptr) return Len == 0;
#if defined(__GNUC__)
	return strcasecmp(Str, str) == 0;
#elif defined(_MSC_VER)
	return _strcmpi(Str, str) == 0;
#else
	return false;
#endif
}

int FString::IndexOf(char c, size_t start_pos) const {
	if (Str == nullptr || start_pos >= Len) {
		return -1;
	}

	for (size_t i = start_pos; i < Len; ++i) {
		if (Str[i] == c) {
			return static_cast<int>(i);
		}
	}
	return -1;
}

FString FString::SubStr(size_t start, int length) const {
	if (start >= Len) {
		return FString();
	}

	size_t actual_length;
	if (length < 0) {
		actual_length = Len - start;
	}
	else {
		actual_length = std::min(static_cast<size_t>(length), Len - start);
	}

	if (actual_length == 0) {
		return FString();
	}

	FString result;
	result.EnsureCapacity(actual_length);
	result.Len = actual_length;
	Memory::Copy(result.Str, Str + start, actual_length);
	result.Str[actual_length] = '\0';

	return result;
}

// 类型转换实现
bool FString::ToBool() const {
	if (Str == nullptr) return false;
	return (strcmp(Str, "1") == 0) ||
		(strcmp(Str, "true") == 0) ||
		(strcmp(Str, "True") == 0) ||
		(strcmp(Str, "TRUE") == 0);
}

int FString::ToInt() const {
	if (Str == nullptr) return 0;
	return atoi(Str);
}

float FString::ToFloat() const {
	if (Str == nullptr) return 0.0f;
	return static_cast<float>(atof(Str));
}

double FString::ToDouble() const {
	if (Str == nullptr) return 0.0;
	return atof(Str);
}

// UTF-8支持实现（修复原有bug）
bool FString::BytesToCodepoint(const char* bytes, uint32_t offset, int* out_codepoint, unsigned char* out_advance) {
	if (bytes == nullptr || out_codepoint == nullptr || out_advance == nullptr) {
		return false;
	}

	int codepoint = static_cast<int>(static_cast<unsigned char>(bytes[offset]));

	if (codepoint >= 0 && codepoint <= 0x7F) {
		// 单字节ASCII字符
		*out_advance = 1;
		*out_codepoint = codepoint;
		return true;
	}
	else if ((codepoint & 0xE0) == 0xC0) {
		// 双字节字符
		codepoint = ((bytes[offset + 0] & 0x1F) << 6) + (bytes[offset + 1] & 0x3F);
		*out_advance = 2;
		*out_codepoint = codepoint;
		return true;
	}
	else if ((codepoint & 0xF0) == 0xE0) {
		// 三字节字符
		codepoint = ((bytes[offset + 0] & 0x0F) << 12) +
			((bytes[offset + 1] & 0x3F) << 6) +
			(bytes[offset + 2] & 0x3F);
		*out_advance = 3;
		*out_codepoint = codepoint;
		return true;
	}
	else if ((codepoint & 0xF8) == 0xF0) {
		// 四字节字符（修复原有bug）
		codepoint = ((bytes[offset + 0] & 0x07) << 18) +
			((bytes[offset + 1] & 0x3F) << 12) +
			((bytes[offset + 2] & 0x3F) << 6) +  // 修复：这里原来错误地使用了offset+1
			(bytes[offset + 3] & 0x3F);
		*out_advance = 4;
		*out_codepoint = codepoint;
		return true;
	}
	else {
		*out_codepoint = 0;
		*out_advance = 0;
		GLOG(Log::eError, "Invalid UTF-8 sequence");
		return false;
	}
}

// 静态方法实现
// 注意：静态模板方法Format的定义已经移到头文件中

void FString::Copy(char* dst, const char* src, size_t max_size) {
	if (dst == nullptr || src == nullptr || max_size == 0) {
		return;
	}
	strncpy(dst, src, max_size - 1);
	dst[max_size - 1] = '\0';
}

char* FString::AllocateCopy(const char* str) {
	if (str == nullptr) {
		return nullptr;
	}

	size_t length = strlen(str);
	char* copy = (char*)Memory::Allocate(length + 1, MemoryType::eMemory_Type_String);
	if (copy != nullptr) {
		Memory::Copy(copy, str, length);
		copy[length] = '\0';
	}
	return copy;
}

void FString::Free(char* str) {
	if (str == nullptr) {
		return;
	}

	size_t size = 0;
	unsigned short alignment = 0;
	if (Memory::GetAlignmentSize(str, &size, &alignment)) {
		Memory::FreeAligned(str, size, MemoryType::eMemory_Type_String);
	}
	else {
		Memory::Free(str, MemoryType::eMemory_Type_String);
	}
}

char* FString::Trim(char* str) {
	if (str == nullptr) {
		return str;
	}

	// 移除开头的空白字符
	while (isspace((unsigned char)*str)) {
		str++;
	}

	if (*str == '\0') {
		return str;
	}

	// 移除结尾的空白字符
	char* end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end)) {
		end--;
	}
	end[1] = '\0';

	return str;
}

int FString::IndexOf(const char* str, char c) {
	if (str == nullptr) {
		return -1;
	}

	size_t length = strlen(str);
	for (size_t i = 0; i < length; ++i) {
		if (str[i] == c) {
			return static_cast<int>(i);
		}
	}
	return -1;
}

void FString::Mid(char* dst, const char* src, size_t start, int length) {
	if (dst == nullptr || src == nullptr) {
		return;
	}

	if (length == 0) {
		dst[0] = '\0';
		return;
	}

	size_t src_length = strlen(src);
	if (start >= src_length) {
		dst[0] = '\0';
		return;
	}

	if (length > 0) {
		size_t copy_len = std::min(static_cast<size_t>(length), src_length - start);
		Memory::Copy(dst, src + start, copy_len);
		dst[copy_len] = '\0';
	}
	else {
		// 负值表示复制到字符串末尾
		size_t copy_len = src_length - start;
		Memory::Copy(dst, src + start, copy_len);
		dst[copy_len] = '\0';
	}
}

std::vector<char*> FString::Split(const char* str, char delimiter, bool trim_entries, bool include_empty) {
	std::vector<char*> result;

	if (str == nullptr) {
		return result;
	}

	const char* start = str;
	const char* current = str;

	while (*current != '\0') {
		if (*current == delimiter) {
			size_t length = current - start;

			if (length > 0 || include_empty) {
				char* segment = (char*)Memory::Allocate(length + 1, MemoryType::eMemory_Type_String);
				if (segment != nullptr) {
					Memory::Copy(segment, start, length);
					segment[length] = '\0';

					if (trim_entries) {
						char* trimmed = Trim(segment);
						if (strlen(trimmed) > 0 || include_empty) {
							result.push_back(segment);
						}
						else {
							Free(segment);
						}
					}
					else {
						result.push_back(segment);
					}
				}
			}

			start = current + 1;
		}
		current++;
	}

	// 处理最后一个段
	size_t length = current - start;
	if (length > 0 || include_empty) {
		char* segment = (char*)Memory::Allocate(length + 1, MemoryType::eMemory_Type_String);
		if (segment != nullptr) {
			Memory::Copy(segment, start, length);
			segment[length] = '\0';

			if (trim_entries) {
				char* trimmed = Trim(segment);
				if (strlen(trimmed) > 0 || include_empty) {
					result.push_back(segment);
				}
				else {
					Free(segment);
				}
			}
			else {
				result.push_back(segment);
			}
		}
	}

	return result;
}

bool FString::Equal(const char* str1, const char* str2) {
	if (str1 == nullptr && str2 == nullptr) return true;
	if (str1 == nullptr || str2 == nullptr) return false;
	return strcmp(str1, str2) == 0;
}

bool FString::Equali(const char* str1, const char* str2) {
	if (str1 == nullptr && str2 == nullptr) return true;
	if (str1 == nullptr || str2 == nullptr) return false;
#if defined(__GNUC__)
	return strcasecmp(str1, str2) == 0;
#elif defined(_MSC_VER)
	return _strcmpi(str1, str2) == 0;
#else
	return false;
#endif
}

bool FString::Nequal(const char* str1, const char* str2, size_t len) {
	if (str1 == nullptr && str2 == nullptr) return true;
	if (str1 == nullptr || str2 == nullptr) return false;
	return strncmp(str1, str2, len) == 0;
}

bool FString::Nequali(const char* str1, const char* str2, size_t len) {
	if (str1 == nullptr && str2 == nullptr) return true;
	if (str1 == nullptr || str2 == nullptr) return false;
#if defined(__GNUC__)
	return strncasecmp(str1, str2, len) == 0;
#elif defined(_MSC_VER)
	return _strnicmp(str1, str2, len) == 0;
#else
	return false;
#endif
}

void FString::DirectoryFromPath(char* dst, const char* path) {
	if (dst == nullptr || path == nullptr) {
		return;
	}

	size_t length = strlen(path);
	for (int i = static_cast<int>(length) - 1; i >= 0; --i) {
		char c = path[i];
		if (c == '/' || c == '\\') {
			Memory::Copy(dst, path, i + 1);
			dst[i + 1] = '\0';
			return;
		}
	}

	// 没有找到路径分隔符，返回空字符串
	dst[0] = '\0';
}

void FString::FilenameFromPath(char* dst, const char* path) {
	if (dst == nullptr || path == nullptr) {
		return;
	}

	size_t length = strlen(path);
	for (int i = static_cast<int>(length) - 1; i >= 0; --i) {
		char c = path[i];
		if (c == '/' || c == '\\') {
			strcpy(dst, path + i + 1);
			return;
		}
	}

	// 没有找到路径分隔符，整个字符串就是文件名
	strcpy(dst, path);
}

void FString::FilenameNoExtensionFromPath(char* dst, const char* path) {
	if (dst == nullptr || path == nullptr) {
		return;
	}

	size_t length = strlen(path);
	size_t start = 0;
	size_t end = length;

	// 找到最后一个路径分隔符
	for (int i = static_cast<int>(length) - 1; i >= 0; --i) {
		char c = path[i];
		if (c == '/' || c == '\\') {
			start = i + 1;
			break;
		}
	}

	// 找到最后一个点号（扩展名）
	for (int i = static_cast<int>(length) - 1; i >= static_cast<int>(start); --i) {
		if (path[i] == '.') {
			end = i;
			break;
		}
	}

	size_t filename_length = end - start;
	if (filename_length > 0) {
		Memory::Copy(dst, path + start, filename_length);
		dst[filename_length] = '\0';
	}
	else {
		dst[0] = '\0';
	}
}

uint32_t FString::UTF8Length(const char* str) {
	if (str == nullptr) {
		return 0;
	}

	uint32_t length = 0;
	for (uint32_t i = 0; str[i] != '\0'; ++length) {
		int c = static_cast<int>(static_cast<unsigned char>(str[i]));

		if (c >= 0 && c <= 0x7F) {
			// 单字节ASCII字符
			i += 1;
		}
		else if ((c & 0xE0) == 0xC0) {
			// 双字节字符
			i += 2;
		}
		else if ((c & 0xF0) == 0xE0) {
			// 三字节字符
			i += 3;
		}
		else if ((c & 0xF8) == 0xF0) {
			// 四字节字符
			i += 4;
		}
		else {
			// 无效的UTF-8序列
			GLOG(Log::eError, "Invalid UTF-8 sequence in string");
			return 0;
		}
	}

	return length;
}

bool FString::ToBool(const char* str) {
	if (str == nullptr) return false;
	return (strcmp(str, "1") == 0) ||
		(strcmp(str, "true") == 0) ||
		(strcmp(str, "True") == 0) ||
		(strcmp(str, "TRUE") == 0);
}

bool FString::ToFloat(const char* str, float* out_value) {
	if (str == nullptr || out_value == nullptr) {
		return false;
	}

	char* endptr;
	float value = strtof(str, &endptr);

	// 检查是否有有效的转换
	if (endptr == str || (*endptr != '\0' && !isspace(*endptr))) {
		return false;
	}

	*out_value = value;
	return true;
}

bool FString::ToInt(const char* str, int* out_value) {
	if (str == nullptr || out_value == nullptr) {
		return false;
	}

	char* endptr;
	long value = strtol(str, &endptr, 10);

	// 检查是否有有效的转换
	if (endptr == str || (*endptr != '\0' && !isspace(*endptr))) {
		return false;
	}

	// 检查范围
	if (value < INT_MIN || value > INT_MAX) {
		return false;
	}

	*out_value = static_cast<int>(value);
	return true;
}

bool FString::ToDouble(const char* str, double* out_value) {
	if (str == nullptr || out_value == nullptr) {
		return false;
	}

	char* endptr;
	double value = strtod(str, &endptr);

	// 检查是否有有效的转换
	if (endptr == str || (*endptr != '\0' && !isspace(*endptr))) {
		return false;
	}

	*out_value = value;
	return true;
}

// 双目操作符实现
FString operator+(const FString& s1, const FString& s2) {
	FString result(s1);
	result += s2;
	return result;
}

FString operator+(const FString& s1, const char* s2) {
	FString result(s1);
	result += s2;
	return result;
}

FString operator+(const char* s1, const FString& s2) {
	FString result(s1);
	result += s2;
	return result;
}