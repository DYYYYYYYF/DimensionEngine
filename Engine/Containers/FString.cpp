#include "FString.hpp"
#include <cctype>
#include <climits>
#include <cstdlib>

FString::FString() : Str(nullptr), Len(0), Capacity(0) { InitializeEmpty(); }

FString::FString(const FString& str) : Str(nullptr), Len(0), Capacity(0) {
	if (str.Len > 0 && str.Str != nullptr) {
		EnsureCapacity(str.Len);
		Len = str.Len;
		Memory::Copy(Str, str.Str, Len);
		Str[Len] = '\0';
	}
	else { InitializeEmpty(); }
}

FString::FString(FString&& str) noexcept
	: Str(str.Str), Len(str.Len), Capacity(str.Capacity) {
	str.Str = nullptr; str.Len = 0; str.Capacity = 0;
}

FString::FString(const char* str) : Str(nullptr), Len(0), Capacity(0) {
	if (str != nullptr && str[0] != '\0') {
		Len = strlen(str);
		EnsureCapacity(Len);
		Memory::Copy(Str, str, Len);
		Str[Len] = '\0';
	}
	else { InitializeEmpty(); }
}

FString::~FString() { ReleaseMemory(); }

// ============================================================
//  赋值
// ============================================================

FString& FString::operator=(const FString& str) {
	if (this == &str) return *this;
	if (str.Len > 0 && str.Str != nullptr) {
		EnsureCapacity(str.Len);
		Len = str.Len;
		Memory::Copy(Str, str.Str, Len);
		Str[Len] = '\0';
	}
	else { Clear(); }
	return *this;
}

FString& FString::operator=(FString&& str) noexcept {
	if (this != &str) {
		ReleaseMemory();
		Str = str.Str; Len = str.Len; Capacity = str.Capacity;
		str.Str = nullptr; str.Len = 0; str.Capacity = 0;
	}
	return *this;
}

FString& FString::operator=(const char* str) {
	if (str == Str) return *this;
	if (str != nullptr && str[0] != '\0') {
		size_t new_len = strlen(str);
		EnsureCapacity(new_len);
		Len = new_len;
		Memory::Copy(Str, str, Len);
		Str[Len] = '\0';
	}
	else { Clear(); }
	return *this;
}

// ============================================================
//  拼接
// ============================================================

FString& FString::operator+=(const FString& str) {
	if (str.Len > 0) {
		EnsureCapacity(Len + str.Len);
		Memory::Copy(Str + Len, str.Str, str.Len);
		Len += str.Len; Str[Len] = '\0';
	}
	return *this;
}

FString& FString::operator+=(const char* str) {
	if (str != nullptr && str[0] != '\0') {
		size_t slen = strlen(str);
		EnsureCapacity(Len + slen);
		Memory::Copy(Str + Len, str, slen);
		Len += slen; Str[Len] = '\0';
	}
	return *this;
}

FString& FString::operator+=(char c) {
	EnsureCapacity(Len + 1);
	Str[Len++] = c; Str[Len] = '\0';
	return *this;
}

// ============================================================
//  索引
// ============================================================

char& FString::operator[](size_t i) {
	if (i >= Len) throw std::out_of_range("FString index out of range");
	return Str[i];
}

const char& FString::operator[](size_t i) const {
	if (i >= Len) throw std::out_of_range("FString index out of range");
	return Str[i];
}

// ============================================================
//  流
// ============================================================

std::istream& operator>>(std::istream& is, FString& str) {
	char buf[4096];
	if (is >> buf) str = buf;
	return is;
}

// ============================================================
//  私有辅助
// ============================================================

void FString::InitializeEmpty() {
	EnsureCapacity(0);
	Str[0] = '\0'; Len = 0;
}

void FString::ReleaseMemory() {
	if (Str) {
		Memory::Free(Str, MemoryType::eMemory_Type_String);
		Str = nullptr; Len = 0; Capacity = 0;
	}
}

void FString::EnsureCapacity(size_t required_size) {
	if (required_size < Capacity) return;
	size_t new_capacity = required_size * 2 + 16;
	char* new_str = (char*)Memory::Allocate(new_capacity + 1, MemoryType::eMemory_Type_String);
	if (!new_str) {
		GLOG(Log::eFatal, "FString::EnsureCapacity: allocation failed (%zu bytes)", new_capacity + 1);
		throw std::bad_alloc();
	}
	memset(new_str, 0, new_capacity + 1);
	if (Str) {
		if (Len > 0) Memory::Copy(new_str, Str, Len);
		Memory::Free(Str, MemoryType::eMemory_Type_String);
	}
	Str = new_str; Capacity = new_capacity;
}

void FString::Clear() {
	if (Str) { Str[0] = '\0'; Len = 0; }
}

void FString::Reserve(size_t new_capacity) {
	if (new_capacity > Capacity) EnsureCapacity(new_capacity);
}

// ============================================================
//  流
// ============================================================

// ============================================================
//  比较（成员）—— 参数统一为 const FString&
// ============================================================

bool FString::Equal(const FString& other) const {
	return *this == other;
}

bool FString::Equali(const FString& other) const {
	return FString::Equali(*this, other);
}

bool FString::Nequal(const FString& other, size_t len) const {
	return strncmp(CStr(), other.CStr(), len) == 0;
}

bool FString::Nequali(const FString& other, size_t len) const {
	return FString::Nequali(*this, other, len);
}

// ============================================================
//  查找
// ============================================================

int FString::IndexOf(char c, size_t start_pos) const {
	if (!Str || start_pos >= Len) return -1;
	for (size_t i = start_pos; i < Len; ++i)
		if (Str[i] == c) return static_cast<int>(i);
	return -1;
}

int FString::LastIndexOf(char c) const {
	if (!Str || Len == 0) return -1;
	for (int i = static_cast<int>(Len) - 1; i >= 0; --i)
		if (Str[i] == c) return i;
	return -1;
}

// ============================================================
//  子串 / 修改
// ============================================================

FString FString::SubStr(size_t start, size_t length) const {
	if (start >= Len) return FString();
	size_t actual = (length < 0)
		? Len - start
		: (static_cast<size_t>(length) < Len - start ? static_cast<size_t>(length) : Len - start);
	if (actual == 0) return FString();
	FString result;
	result.EnsureCapacity(actual);
	result.Len = actual;
	Memory::Copy(result.Str, Str + start, actual);
	result.Str[actual] = '\0';
	return result;
}

FString& FString::Trim() {
	if (!Str || Len == 0) return *this;
	size_t start = 0;
	while (start < Len && isspace((unsigned char)Str[start])) ++start;
	size_t end = Len;
	while (end > start && isspace((unsigned char)Str[end - 1])) --end;
	size_t new_len = end - start;
	if (start > 0 && new_len > 0) memmove(Str, Str + start, new_len);
	Str[new_len] = '\0'; Len = new_len;
	return *this;
}

FString  FString::Trimmed()     const { FString c(*this); c.Trim();    return c; }
FString  FString::ToLowerCopy() const { FString c(*this); c.ToLower(); return c; }
FString  FString::ToUpperCopy() const { FString c(*this); c.ToUpper(); return c; }

FString& FString::ToLower() {
	for (size_t i = 0; i < Len; ++i)
		Str[i] = static_cast<char>(tolower((unsigned char)Str[i]));
	return *this;
}

FString& FString::ToUpper() {
	for (size_t i = 0; i < Len; ++i)
		Str[i] = static_cast<char>(toupper((unsigned char)Str[i]));
	return *this;
}

// Insert(size_t, const FString&) —— 单一入口
FString& FString::Insert(size_t pos, const FString& str) {
	if (str.IsEmpty()) return *this;
	if (pos > Len) pos = Len;
	EnsureCapacity(Len + str.Len);
	memmove(Str + pos + str.Len, Str + pos, Len - pos + 1);
	Memory::Copy(Str + pos, str.Str, str.Len);
	Len += str.Len;
	return *this;
}

// Replace(const FString&, const FString&)
FString& FString::Replace(const FString& old_str, const FString& new_str) {
	if (old_str.IsEmpty() || !Str) return *this;

	const char* o = old_str.CStr();
	const char* n = new_str.CStr();
	size_t old_len = old_str.Len;
	size_t new_len = new_str.Len;

	size_t count = 0;
	const char* p = Str;
	while ((p = strstr(p, o)) != nullptr) { ++count; p += old_len; }
	if (count == 0) return *this;

	size_t result_len = Len + count * (new_len - old_len);
	char* buf = (char*)Memory::Allocate(result_len + 1, MemoryType::eMemory_Type_String);
	if (!buf) throw std::bad_alloc();

	char* dst = buf;
	const char* src = Str;
	while ((p = strstr(src, o)) != nullptr) {
		size_t prefix = static_cast<size_t>(p - src);
		Memory::Copy(dst, src, prefix); dst += prefix;
		Memory::Copy(dst, n, new_len);  dst += new_len;
		src = p + old_len;
	}
	size_t tail = Len - static_cast<size_t>(src - Str);
	Memory::Copy(dst, src, tail);
	dst[tail] = '\0';

	Memory::Free(Str, MemoryType::eMemory_Type_String);
	Str = buf; Len = result_len; Capacity = result_len;
	return *this;
}

FString& FString::Replace(char old_char, char new_char) {
	for (size_t i = 0; i < Len; ++i)
		if (Str[i] == old_char) Str[i] = new_char;
	return *this;
}

// ============================================================
//  Split
// ============================================================

TArray<FString> FString::Split(char delimiter, bool trim_entries, bool include_empty) const {
	TArray<FString> result;
	if (!Str || Len == 0) return result;
	size_t start = 0;
	for (size_t i = 0; i <= Len; ++i) {
		if (i == Len || Str[i] == delimiter) {
			size_t seg_len = i - start;
			if (seg_len > 0 || include_empty) {
				FString seg = SubStr(start, static_cast<int>(seg_len));
				if (trim_entries) seg.Trim();
				if (!seg.IsEmpty() || include_empty)
					result.Push(static_cast<FString&&>(seg));
			}
			start = i + 1;
		}
	}
	return result;
}

// ============================================================
//  类型转换（成员）—— 委托静态版本
// ============================================================

bool   FString::ToBool()   const { return FString::ToBool(*this); }
int    FString::ToInt()    const { int    v = 0;    FString::ToInt(*this, &v); return v; }
float  FString::ToFloat()  const { float  v = 0.0f; FString::ToFloat(*this, &v); return v; }
double FString::ToDouble() const { double v = 0.0;  FString::ToDouble(*this, &v); return v; }

// ============================================================
//  UTF-8
// ============================================================

uint32_t FString::UTF8Length() const { return FString::UTF8Length(*this); }

FCodepointResult FString::BytesToCodepoint(const FString& string, size_t bytes_len, uint32_t offset) {
	return FString::BytesToCodepoint(string.CStr(), bytes_len, offset);
}

FCodepointResult FString::BytesToCodepoint(const char* bytes, size_t bytes_len, uint32_t offset) {
	FCodepointResult r{ 0, 0, false };
	if (!bytes || offset >= bytes_len) return r;
	unsigned char c = static_cast<unsigned char>(bytes[offset]);
	if (c <= 0x7F) {
		r.Codepoint = c; r.Advance = 1; r.bValid = true;
	}
	else if ((c & 0xE0) == 0xC0) {
		if (offset + 1 >= bytes_len) return r;
		r.Codepoint = ((bytes[offset + 0] & 0x1F) << 6) | (bytes[offset + 1] & 0x3F);
		r.Advance = 2; r.bValid = true;
	}
	else if ((c & 0xF0) == 0xE0) {
		if (offset + 2 >= bytes_len) return r;
		r.Codepoint = ((bytes[offset + 0] & 0x0F) << 12)
			| ((bytes[offset + 1] & 0x3F) << 6)
			| (bytes[offset + 2] & 0x3F);
		r.Advance = 3; r.bValid = true;
	}
	else if ((c & 0xF8) == 0xF0) {
		if (offset + 3 >= bytes_len) return r;
		r.Codepoint = ((bytes[offset + 0] & 0x07) << 18)
			| ((bytes[offset + 1] & 0x3F) << 12)
			| ((bytes[offset + 2] & 0x3F) << 6)
			| (bytes[offset + 3] & 0x3F);
		r.Advance = 4; r.bValid = true;
	}
	else {
		GLOG(Log::eError, "Invalid UTF-8 sequence at offset %u", offset);
	}
	return r;
}

// ============================================================
//  静态工厂
// ============================================================

FString FString::FromBool(bool   v) { return FString(v ? "true" : "false"); }
FString FString::FromInt(int    v) { char b[32];  snprintf(b, sizeof(b), "%d", v); return FString(b); }
FString FString::FromFloat(float  v) { char b[64];  snprintf(b, sizeof(b), "%g", v); return FString(b); }
FString FString::FromDouble(double v) { char b[128]; snprintf(b, sizeof(b), "%lg", v); return FString(b); }

// ============================================================
//  静态路径处理（参数为 const FString&）
// ============================================================

FString FString::DirectoryFromPath(const FString& path) {
	int len = static_cast<int>(path.Len);
	for (int i = len - 1; i >= 0; --i)
		if (path.Str[i] == '/' || path.Str[i] == '\\')
			return path.SubStr(0, i + 1);
	return FString();
}

FString FString::FilenameFromPath(const FString& path) {
	int len = static_cast<int>(path.Len);
	for (int i = len - 1; i >= 0; --i)
		if (path.Str[i] == '/' || path.Str[i] == '\\')
			return path.SubStr(static_cast<size_t>(i + 1));
	return path;
}

FString FString::FilenameNoExtensionFromPath(const FString& path) {
	size_t len = path.Len;
	size_t start = 0;
	size_t end = len;
	for (int i = static_cast<int>(len) - 1; i >= 0; --i)
		if (path.Str[i] == '/' || path.Str[i] == '\\') { start = i + 1; break; }
	for (int i = static_cast<int>(len) - 1; i >= static_cast<int>(start); --i)
		if (path.Str[i] == '.') { end = i; break; }
	return path.SubStr(start, static_cast<int>(end - start));
}

// ============================================================
//  静态比较（参数为 const FString&）
// ============================================================

bool FString::Equal(const FString& s1, const FString& s2) {
	return s1 == s2;
}

bool FString::Equali(const FString& s1, const FString& s2) {
	if (s1.Len != s2.Len) return false;
#if defined(__GNUC__)
	return strncasecmp(s1.CStr(), s2.CStr(), s1.Len) == 0;
#elif defined(_MSC_VER)
	return _strnicmp(s1.CStr(), s2.CStr(), s1.Len) == 0;
#else
	for (size_t i = 0; i < s1.Len; ++i)
		if (tolower((unsigned char)s1.Str[i]) != tolower((unsigned char)s2.Str[i])) return false;
	return true;
#endif
}

bool FString::Nequal(const FString& s1, const FString& s2, size_t len) {
	return strncmp(s1.CStr(), s2.CStr(), len) == 0;
}

bool FString::Nequali(const FString& s1, const FString& s2, size_t len) {
#if defined(__GNUC__)
	return strncasecmp(s1.CStr(), s2.CStr(), len) == 0;
#elif defined(_MSC_VER)
	return _strnicmp(s1.CStr(), s2.CStr(), len) == 0;
#else
	for (size_t i = 0; i < len; ++i) {
		if (i >= s1.Len || i >= s2.Len) return s1.Len == s2.Len;
		if (tolower((unsigned char)s1.Str[i]) != tolower((unsigned char)s2.Str[i])) return false;
	}
	return true;
#endif
}

// ============================================================
//  静态 UTF-8（参数为 const FString&）
// ============================================================

uint32_t FString::UTF8Length(const FString& str) {
	const char* s = str.CStr();
	uint32_t length = 0;
	for (size_t i = 0; s[i] != '\0'; ++length) {
		unsigned char c = static_cast<unsigned char>(s[i]);
		if (c <= 0x7F)           i += 1;
		else if ((c & 0xE0) == 0xC0)  i += 2;
		else if ((c & 0xF0) == 0xE0)  i += 3;
		else if ((c & 0xF8) == 0xF0)  i += 4;
		else { GLOG(Log::eError, "Invalid UTF-8 sequence"); return 0; }
	}
	return length;
}

// ============================================================
//  静态类型转换（参数为 const FString&）
// ============================================================

bool FString::ToBool(const FString& str) {
	return str == "1" || str == "true" || str == "True" || str == "TRUE";
}

bool FString::ToFloat(const FString& str, float* out_value) {
	if (!out_value || str.IsEmpty()) return false;
	char* end; float v = strtof(str.CStr(), &end);
	if (end == str.CStr() || (*end != '\0' && !isspace((unsigned char)*end))) return false;
	*out_value = v; return true;
}

bool FString::ToInt(const FString& str, int* out_value) {
	if (!out_value || str.IsEmpty()) return false;
	char* end; long v = strtol(str.CStr(), &end, 10);
	if (end == str.CStr() || (*end != '\0' && !isspace((unsigned char)*end))) return false;
	if (v < INT_MIN || v > INT_MAX) return false;
	*out_value = static_cast<int>(v); return true;
}

bool FString::ToDouble(const FString& str, double* out_value) {
	if (!out_value || str.IsEmpty()) return false;
	char* end; double v = strtod(str.CStr(), &end);
	if (end == str.CStr() || (*end != '\0' && !isspace((unsigned char)*end))) return false;
	*out_value = v; return true;
}

// ============================================================
//  双目 operator+
// ============================================================

FString operator+(const FString& s1, const FString& s2) { FString r(s1); r += s2; return r; }
FString operator+(const FString& s1, const char* s2) { FString r(s1); r += s2; return r; }
FString operator+(const char* s1, const FString& s2) { FString r(s1); r += s2; return r; }