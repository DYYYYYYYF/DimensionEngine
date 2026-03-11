#pragma once

#include "TArray.hpp"
#include "TMap.hpp"

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <stdexcept>
#include <iostream>   // 仅用于 operator<< / operator>>，无法消除

#ifdef DPLATFORM_MACOS
#include <ctype.h>
#endif

//  UTF-8 码点解码结果
struct FCodepointResult {
	int  Codepoint;   // 解码出的 Unicode 码点
	int  Advance;     // 消耗的字节数（1–4），失败时为 0
	bool bValid;      // 解码是否成功
};

class DAPI FString {
public:
	// =========================================================
	//  构造 / 析构
	// =========================================================
	FString();
	FString(const FString& str);
	FString(FString&& str) noexcept;

	// 保留 const char* 构造：支持字面量隐式转换，是整个类的基础
	FString(const char* str);

	// printf 风格格式化，format 必须是 const char*，无法用 FString 替代
	template<typename... Args>
	FString(const char* format, Args... args);

	~FString();

	// =========================================================
	//  赋值
	//  保留 operator=(const char*)：支持 s = "literal" 语法
	// =========================================================
	FString& operator=(const FString& str);
	FString& operator=(FString&& str) noexcept;
	FString& operator=(const char* str);

	// =========================================================
	//  拼接
	//  保留 operator+=(const char*)：支持 s += "literal" 语法
	// =========================================================
	FString& operator+=(const FString& str);
	FString& operator+=(const char* str);
	FString& operator+=(char c);

	// =========================================================
	//  比较操作符
	//  保留 const char* 重载：让 s == "literal" 无需构造临时对象
	// =========================================================
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
	friend bool operator!=(const char* s1, const FString& s2) { return !(s2 == s1); }
	friend bool operator< (const FString& s1, const FString& s2) { return strcmp(s1.Str, s2.Str) < 0; }
	friend bool operator> (const FString& s1, const FString& s2) { return strcmp(s1.Str, s2.Str) > 0; }
	friend bool operator<=(const FString& s1, const FString& s2) { return strcmp(s1.Str, s2.Str) <= 0; }
	friend bool operator>=(const FString& s1, const FString& s2) { return strcmp(s1.Str, s2.Str) >= 0; }

	// =========================================================
	//  索引
	// =========================================================
	char& operator[](size_t i);
	const char& operator[](size_t i) const;

	// =========================================================
	//  流操作符
	// =========================================================
	friend std::ostream& operator<<(std::ostream& os, const FString& str) {
		return os << (str.Str ? str.Str : "");
	}
	friend std::istream& operator>>(std::istream& is, FString& str);

	// =========================================================
	//  访问器
	//  CStr() / Data() 返回 const char*：用途是对接 C API（OpenGL、文件系统等），
	//  不能改为 FString，调用方需要的就是裸指针。
	// =========================================================
	size_t      Length() const { return Len; }
	size_t      Size()   const { return Len; }
	bool        IsEmpty()  const { return Len == 0; }
	char* Data() { return Str; }
	const char* Data()   const { return Str; }
	const char* CStr()   const { return Str ? Str : ""; }

	// =========================================================
	//  容量
	// =========================================================
	void Reserve(size_t new_capacity);
	void Clear();

	// =========================================================
	//  比较（成员）
	//  Equal(const FString&) 统一入口；FString 有隐式 const char* 构造，
	//  调用方传字面量时会自动构造，因此不需要再保留 const char* 重载。
	// =========================================================
	bool Equal(const FString& other)           const;
	bool Equali(const FString& other)           const;   // 大小写不敏感
	bool Nequal(const FString& other, size_t len) const;
	bool Nequali(const FString& other, size_t len) const; // 大小写不敏感
	int  Compare(const FString& other)         const;

	// =========================================================
	//  查找
	// =========================================================
	int IndexOf(char c, size_t start_pos = 0) const;
	int LastIndexOf(char c)                        const;

	// =========================================================
	//  子串 / 修改
	// =========================================================
	FString  SubStr(size_t start, size_t length = -1) const;
	FString& Trim();
	FString  Trimmed()                              const;
	FString& ToLower();
	FString& ToUpper();
	FString  ToLowerCopy()                             const;
	FString  ToUpperCopy()                             const;

	// Insert / Replace 参数改为 const FString&；
	// 调用方传 const char* 字面量时隐式构造 FString，无感知
	FString& Insert(size_t pos, const FString& str);
	FString& Replace(const FString& old_str, const FString& new_str);
	FString& Replace(char old_char, char new_char);

	// =========================================================
	//  分割
	// =========================================================
	TArray<FString> Split(char delimiter,
		bool trim_entries = true,
		bool include_empty = true) const;

	// =========================================================
	//  类型转换（成员）
	// =========================================================
	bool   ToBool()   const;
	int    ToInt()    const;
	float  ToFloat()  const;
	double ToDouble() const;

	// =========================================================
	//  UTF-8
	// =========================================================
	uint32_t            UTF8Length() const;

	// bytes / bytes_len 是原始字节缓冲区，语义上不是"字符串内容"，
	// 保留 const char* 更准确（它可能含 \0 中间字节）
	static FCodepointResult BytesToCodepoint(const FString& string,
		size_t      bytes_len,
		uint32_t    offset);

	static FCodepointResult BytesToCodepoint(const char* bytes,
		size_t      bytes_len,
		uint32_t    offset);

	// =========================================================
	//  静态工厂
	//  Format 的 format 参数必须是 const char*（传给 snprintf）
	// =========================================================
	template<typename... Args>
	static FString Format(const char* format, Args... args);

	static FString FromBool(bool   value);
	static FString FromInt(int    value);
	static FString FromInt32(uint32_t    v);
	static FString FromInt64(uint64_t    v);
	static FString FromFloat(float  value);
	static FString FromDouble(double value);

	// =========================================================
	//  静态路径处理（参数改为 const FString&）
	// =========================================================
	static FString DirectoryFromPath(const FString& path);
	static FString FilenameFromPath(const FString& path);
	static FString FilenameNoExtensionFromPath(const FString& path);

	// =========================================================
	//  静态比较（参数改为 const FString&）
	// =========================================================
	static bool Equal(const FString& s1, const FString& s2);
	static bool Equali(const FString& s1, const FString& s2);
	static bool Nequal(const FString& s1, const FString& s2, size_t len);
	static bool Nequali(const FString& s1, const FString& s2, size_t len);

	// =========================================================
	//  静态 UTF-8（参数改为 const FString&）
	// =========================================================
	static uint32_t UTF8Length(const FString& str);

	// =========================================================
	//  静态类型转换（参数改为 const FString&）
	//  成功返回 true 并写入 out_value，失败返回 false
	// =========================================================
	static bool ToBool(const FString& str);
	static bool ToFloat(const FString& str, float* out_value);
	static bool ToInt(const FString& str, int* out_value);
	static bool ToDouble(const FString& str, double* out_value);

private:
	void ReleaseMemory();
	void EnsureCapacity(size_t required_size);
	void InitializeEmpty();

private:
	char* Str;
	size_t Len;
	size_t Capacity;

	// =========================================================
	//  ↓↓↓ 遗留 C 式静态接口 — 可整段删除 ↓↓↓
	// =========================================================
	// [LEGACY] static void  Copy(char* dst, const char* src, size_t max_size);
	// [LEGACY] static char* AllocateCopy(const char* str);
	// [LEGACY] static void  Free(char* str);
	// [LEGACY] static char* Trim(char* str);                        → Trim() / Trimmed()
	// [LEGACY] static int   IndexOf(const char* str, char c);       → IndexOf(char c)
	// [LEGACY] static void  Mid(char* dst, const char* src, ...);   → SubStr()
	// [LEGACY] static ... Split(const char*, char, ...);            → Split() 返回 FStringArray
	// ↑↑↑ 遗留 C 式静态接口结束 ↑↑↑
};

// ============================================================
//  双目 operator+
//  保留 const char* 重载：支持 s + "literal" 无临时对象
// ============================================================
DAPI FString operator+(const FString& s1, const FString& s2);
DAPI FString operator+(const FString& s1, const char* s2);
DAPI FString operator+(const char* s1, const FString& s2);

// ============================================================
//  模板实现（必须在头文件中）
// ============================================================
template<typename... Args>
inline FString::FString(const char* format, Args... args)
	: Str(nullptr), Len(0), Capacity(0)
{
	if (format != nullptr) {
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

// 自定义类型的Hash
template<>
struct TDefaultHasher<FString> {
	size_t operator()(const FString& str) const noexcept {
		size_t hash = 5381;
		const char* s = str.CStr();
		while (*s)
			hash = ((hash << 5) + hash) ^ static_cast<unsigned char>(*s++);
		return hash;
	}
};

// std::string类型的Hash
namespace std {
	template<>
	struct hash<FString> {
		size_t operator()(const FString& str) const noexcept {
			// DJB2 哈希算法
			size_t hash = 5381;
			const char* s = str.CStr();
			while (*s) {
				hash = ((hash << 5) + hash) ^ static_cast<unsigned char>(*s);
				++s;
			}
			return hash;
		}
	};
}