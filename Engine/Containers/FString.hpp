#pragma once

#include "Core/DMemory.hpp"
#include "Core/EngineLogger.hpp"

#include <cstdio>
#include <cstring>
#include <vector>
#include <iostream>
#include <stdexcept>

#ifdef DPLATFORM_MACOS
#include <ctype.h>
#endif

class DAPI FString {
public:
	// === 构造函数和析构函数 ===
	FString();
	FString(const FString& str);
	FString(FString&& str) noexcept;  // 添加移动构造函数
	FString(const char* str);

	template<typename... Args>
	FString(const char* format, Args... args);

	~FString();

	// === 赋值操作符 ===
	FString& operator=(const FString& str);
	FString& operator=(FString&& str) noexcept;  // 添加移动赋值
	FString& operator=(const char* str);

	// === 复合赋值操作符 ===
	FString& operator+=(const FString& str);
	FString& operator+=(const char* str);
	FString& operator+=(char c);

	// === 比较操作符 ===
	friend bool operator==(const FString& s1, const FString& s2) {
		return s1.Len == s2.Len && strcmp(s1.Str, s2.Str) == 0;
	}
	friend bool operator==(const FString& s1, const char* s2) {
		return s2 != nullptr && strcmp(s1.Str, s2) == 0;
	}
	friend bool operator==(const char* s1, const FString& s2) {
		return s1 != nullptr && strcmp(s1, s2.Str) == 0;
	}
	friend bool operator!=(const FString& s1, const FString& s2) { return !(s1 == s2); }
	friend bool operator!=(const FString& s1, const char* s2) { return !(s1 == s2); }
	friend bool operator!=(const char* s1, const FString& s2) { return !(s1 == s2); }

	friend bool operator>(const FString& s1, const FString& s2) { return strcmp(s1.Str, s2.Str) > 0; }
	friend bool operator<(const FString& s1, const FString& s2) { return strcmp(s1.Str, s2.Str) < 0; }
	friend bool operator>=(const FString& s1, const FString& s2) { return strcmp(s1.Str, s2.Str) >= 0; }
	friend bool operator<=(const FString& s1, const FString& s2) { return strcmp(s1.Str, s2.Str) <= 0; }

	// === 索引操作符 ===
	char& operator[](size_t i) {
		if (i >= Len) throw std::out_of_range("FString index out of range");
		return Str[i];
	}
	const char& operator[](size_t i) const {
		if (i >= Len) throw std::out_of_range("FString index out of range");
		return Str[i];
	}

	// === 流操作符 ===
	friend std::ostream& operator<<(std::ostream& os, const FString& str) {
		os << (str.Str ? str.Str : "");
		return os;
	}
	friend std::istream& operator>>(std::istream& is, FString& str);

public:
	// === 字符串比较方法 ===
	bool Equal(const FString& str) const;
	bool Equal(const char* str) const;
	bool Equali(const char* str) const;  // 大小写不敏感
	bool Nequal(const char* str, size_t len) const;
	bool Nequali(const char* str, size_t len) const;  // 大小写不敏感

	// === 字符串操作方法 ===
	std::vector<FString> Split(char delimiter, bool trim_entries = true, bool include_empty = true) const;
	int IndexOf(char c, size_t start_pos = 0) const;
	int LastIndexOf(char c) const;
	FString SubStr(size_t start, int length = -1) const;  // 修复：返回新对象而不是修改引用
	FString& Trim();
	FString Trimmed() const;
	FString& ToLower();
	FString& ToUpper();
	FString ToLowerCopy() const;
	FString ToUpperCopy() const;

	// === 格式化和插入 ===
	FString& Insert(size_t pos, const char* str);
	FString& Insert(size_t pos, const FString& str);
	FString& Replace(const char* old_str, const char* new_str);
	FString& Replace(char old_char, char new_char);

	// === 类型转换 ===
	bool ToBool() const;
	int ToInt() const;
	float ToFloat() const;
	double ToDouble() const;

	// === UTF-8 支持 ===
	uint32_t UTF8Length() const;
	static bool BytesToCodepoint(const char* bytes, uint32_t offset, int* out_codepoint, unsigned char* out_advance);

	// === 访问器 ===
	size_t Length() const { return Len; }
	size_t Size() const { return Len; }  // STL兼容
	bool Empty() const { return Len == 0; }
	char* Data() { return Str; }
	const char* Data() const { return Str; }
	const char* CStr() const { return Str ? Str : ""; }  // 安全的C字符串访问

	// 容量管理
	void Reserve(size_t new_capacity);
	void Clear();

public:
	// === 静态工厂方法 ===
	template<typename... Args>
	static FString Format(const char* format, Args... args);

	static FString FromBool(bool value);
	static FString FromInt(int value);
	static FString FromFloat(float value);
	static FString FromDouble(double value);

	// === 静态实用函数（原先的类型函数） ===
	static void Copy(char* dst, const char* src, size_t max_size);
	static char* AllocateCopy(const char* str);
	static void Free(char* str);
	static char* Trim(char* str);
	static int IndexOf(const char* str, char c);
	static void Mid(char* dst, const char* src, size_t start, int length = -1);
	static std::vector<char*> Split(const char* str, char delimiter, bool trim_entries = true, bool include_empty = true);

	// 字符串比较（静态版本）
	static bool Equal(const char* str1, const char* str2);
	static bool Equali(const char* str1, const char* str2);
	static bool Nequal(const char* str1, const char* str2, size_t len);
	static bool Nequali(const char* str1, const char* str2, size_t len);

	// 路径处理
	static void DirectoryFromPath(char* dst, const char* path);
	static void FilenameFromPath(char* dst, const char* path);
	static void FilenameNoExtensionFromPath(char* dst, const char* path);

	// UTF-8处理（静态版本）
	static uint32_t UTF8Length(const char* str);

	// 类型转换（静态版本）
	static bool ToBool(const char* str);
	static bool ToFloat(const char* str, float* out_value);
	static bool ToInt(const char* str, int* out_value);
	static bool ToDouble(const char* str, double* out_value);

private:
	void ReleaseMemory();
	void EnsureCapacity(size_t required_size);
	void InitializeEmpty();

private:
	char* Str;
	size_t Len;
	size_t Capacity;  // 添加容量管理
};

// === 双目操作符 ===
DAPI FString operator+(const FString& s1, const FString& s2);
DAPI FString operator+(const FString& s1, const char* s2);
DAPI FString operator+(const char* s1, const FString& s2);

// === 模板函数实现（必须在头文件中） ===
template<typename... Args>
inline FString::FString(const char* format, Args... args) : Str(nullptr), Len(0), Capacity(0) {
	if (format != nullptr) {
		// 计算所需的长度
		int required_len = snprintf(nullptr, 0, format, args...);
		if (required_len > 0) {
			Len = static_cast<size_t>(required_len);
			EnsureCapacity(Len);
			snprintf(Str, Len + 1, format, args...);
		}
		else {
			InitializeEmpty();
		}
	}
	else {
		InitializeEmpty();
	}
}

template<typename... Args>
inline FString FString::Format(const char* format, Args... args) {
	return FString(format, args...);
}