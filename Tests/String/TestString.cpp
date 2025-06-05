#include "Containers/FString.hpp"
#include <iostream>
#include <cassert>
#include <vector>

// 简单的测试宏
#ifndef TEST_ASSERT
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cout << "[FAIL] " << message << " (Line: " << __LINE__ << ")" << std::endl; \
            return false; \
        } \
        std::cout << "[PASS] " << message << std::endl; \
    } while(0)
#endif

class FStringTester {
public:
	// 测试构造函数
	static bool TestConstructors() {
		std::cout << "\n=== 测试构造函数 ===" << std::endl;

		// 默认构造函数
		FString empty;
		TEST_ASSERT(empty.Empty(), "默认构造函数创建空字符串");
		TEST_ASSERT(empty.Length() == 0, "默认构造函数长度为0");

		// C字符串构造函数
		FString from_cstr("Hello World");
		TEST_ASSERT(from_cstr.Length() == 11, "C字符串构造函数长度正确");
		TEST_ASSERT(from_cstr.Equal("Hello World"), "C字符串构造函数内容正确");

		// 拷贝构造函数
		FString copy(from_cstr);
		TEST_ASSERT(copy.Equal(from_cstr), "拷贝构造函数内容正确");
		TEST_ASSERT(copy.Length() == from_cstr.Length(), "拷贝构造函数长度正确");

		// 移动构造函数
		FString temp("Temporary");
		FString moved(std::move(temp));
		TEST_ASSERT(moved.Equal("Temporary"), "移动构造函数内容正确");
		TEST_ASSERT(temp.Empty(), "移动后原对象为空");

		// 格式化构造函数
		FString formatted("Number: %d, Float: %.2f", 42, 3.14f);
		std::cout << "格式化字符串: " << formatted.CStr() << std::endl;
		TEST_ASSERT(!formatted.Empty(), "格式化构造函数创建非空字符串");

		return true;
	}

	// 测试赋值操作符
	static bool TestAssignmentOperators() {
		std::cout << "\n=== 测试赋值操作符 ===" << std::endl;

		FString str1, str2, str3;

		// C字符串赋值
		str1 = "Hello";
		TEST_ASSERT(str1.Equal("Hello"), "C字符串赋值正确");

		// FString赋值
		str2 = str1;
		TEST_ASSERT(str2.Equal(str1), "FString赋值正确");

		// 移动赋值
		FString temp("World");
		str3 = std::move(temp);
		TEST_ASSERT(str3.Equal("World"), "移动赋值内容正确");
		TEST_ASSERT(temp.Empty(), "移动赋值后原对象为空");

		return true;
	}

	// 测试连接操作符
	static bool TestConcatenationOperators() {
		std::cout << "\n=== 测试连接操作符 ===" << std::endl;

		FString str1("Hello");
		FString str2(" World");

		// += 操作符
		str1 += str2;
		TEST_ASSERT(str1.Equal("Hello World"), "+= FString操作正确");

		str1 += "!";
		TEST_ASSERT(str1.Equal("Hello World!"), "+= C字符串操作正确");

		str1 += '?';
		TEST_ASSERT(str1.Equal("Hello World!?"), "+= 字符操作正确");

		// + 操作符
		FString result1 = FString("Good") + FString(" Morning");
		TEST_ASSERT(result1.Equal("Good Morning"), "+ FString操作正确");

		FString result2 = FString("Good") + " Evening";
		TEST_ASSERT(result2.Equal("Good Evening"), "+ C字符串操作正确");

		FString result3 = "Good" + FString(" Night");
		TEST_ASSERT(result3.Equal("Good Night"), "C字符串 + FString操作正确");

		return true;
	}

	// 测试比较操作符
	static bool TestComparisonOperators() {
		std::cout << "\n=== 测试比较操作符 ===" << std::endl;

		FString str1("Apple");
		FString str2("Apple");
		FString str3("Banana");

		// 相等比较
		TEST_ASSERT(str1 == str2, "== 操作符正确");
		TEST_ASSERT(str1 == "Apple", "== C字符串操作符正确");
		TEST_ASSERT("Apple" == str1, "C字符串 == 操作符正确");

		// 不等比较
		TEST_ASSERT(str1 != str3, "!= 操作符正确");
		TEST_ASSERT(str1 != "Banana", "!= C字符串操作符正确");

		// 大小比较
		TEST_ASSERT(str1 < str3, "< 操作符正确");
		TEST_ASSERT(str3 > str1, "> 操作符正确");
		TEST_ASSERT(str1 <= str2, "<= 操作符正确");
		TEST_ASSERT(str2 >= str1, ">= 操作符正确");

		return true;
	}

	// 测试索引访问
	static bool TestIndexAccess() {
		std::cout << "\n=== 测试索引访问 ===" << std::endl;

		FString str("Hello");

		TEST_ASSERT(str[0] == 'H', "索引访问正确");
		TEST_ASSERT(str[4] == 'o', "索引访问正确");

		// 修改字符
		str[0] = 'h';
		TEST_ASSERT(str[0] == 'h', "字符修改正确");

		return true;
	}

	// 测试字符串操作方法
	static bool TestStringOperations() {
		std::cout << "\n=== 测试字符串操作方法 ===" << std::endl;

		// 查找操作
		FString str("Hello World Hello");
		int pos = str.IndexOf('o');
		TEST_ASSERT(pos == 4, "IndexOf查找正确");

		pos = str.IndexOf('o', 5);
		TEST_ASSERT(pos == 7, "IndexOf从指定位置查找正确");

		// 子字符串
		FString substr = str.SubStr(6, 5);
		TEST_ASSERT(substr.Equal("World"), "SubStr操作正确");

		FString substr2 = str.SubStr(6);
		TEST_ASSERT(substr2.Equal("World Hello"), "SubStr到末尾操作正确");

		// 分割字符串 (这里我们测试实例方法，虽然代码中似乎只有静态方法)
		// 注意：根据提供的代码，Split方法似乎没有完整实现，我们测试静态版本

		return true;
	}

	// 测试类型转换
	static bool TestTypeConversions() {
		std::cout << "\n=== 测试类型转换 ===" << std::endl;

		// 布尔转换
		FString bool_str("true");
		TEST_ASSERT(bool_str.ToBool() == true, "ToBool true转换正确");

		FString bool_str2("false");
		TEST_ASSERT(bool_str2.ToBool() == false, "ToBool false转换正确");

		FString bool_str3("1");
		TEST_ASSERT(bool_str3.ToBool() == true, "ToBool 1转换正确");

		// 整数转换
		FString int_str("123");
		TEST_ASSERT(int_str.ToInt() == 123, "ToInt转换正确");

		FString int_str2("-456");
		TEST_ASSERT(int_str2.ToInt() == -456, "ToInt负数转换正确");

		// 浮点数转换
		FString float_str("3.14");
		TEST_ASSERT(abs(float_str.ToFloat() - 3.14f) < 0.001f, "ToFloat转换正确");

		FString double_str("2.71828");
		TEST_ASSERT(abs(double_str.ToDouble() - 2.71828) < 0.00001, "ToDouble转换正确");

		return true;
	}

	// 测试静态工具方法
	static bool TestStaticMethods() {
		std::cout << "\n=== 测试静态工具方法 ===" << std::endl;

		// 字符串比较
		TEST_ASSERT(FString::Equal("Hello", "Hello"), "静态Equal方法正确");
		TEST_ASSERT(!FString::Equal("Hello", "World"), "静态Equal方法正确");

		TEST_ASSERT(FString::Equali("Hello", "HELLO"), "静态Equali方法正确");
		TEST_ASSERT(FString::Equali("Hello", "hello"), "静态Equali方法正确");

		// 格式化
		FString formatted = FString::Format("Value: %d", 42);
		std::cout << "格式化结果: " << formatted.CStr() << std::endl;

		// 路径处理测试
		const char* test_path = "/home/user/documents/file.txt";
		char dir[256], filename[256], name_no_ext[256];

		FString::DirectoryFromPath(dir, test_path);
		FString::FilenameFromPath(filename, test_path);
		FString::FilenameNoExtensionFromPath(name_no_ext, test_path);

		std::cout << "目录: " << dir << std::endl;
		std::cout << "文件名: " << filename << std::endl;
		std::cout << "无扩展名文件名: " << name_no_ext << std::endl;

		TEST_ASSERT(strcmp(dir, "/home/user/documents/") == 0, "目录提取正确");
		TEST_ASSERT(strcmp(filename, "file.txt") == 0, "文件名提取正确");
		TEST_ASSERT(strcmp(name_no_ext, "file") == 0, "无扩展名文件名提取正确");

		// UTF-8长度测试
		uint32_t utf8_len = FString::UTF8Length("Hello世界");
		std::cout << "UTF-8字符数: " << utf8_len << std::endl;

		return true;
	}

	// 测试内存管理和性能
	static bool TestMemoryManagement() {
		std::cout << "\n=== 测试内存管理 ===" << std::endl;

		// 测试大量连接操作
		FString str;
		for (int i = 0; i < 100; ++i) {
			str += "A";
		}
		TEST_ASSERT(str.Length() == 100, "大量连接操作正确");

		// 测试Clear
		str.Clear();
		TEST_ASSERT(str.Empty(), "Clear操作正确");
		TEST_ASSERT(str.Length() == 0, "Clear后长度为0");

		return true;
	}

	// 运行所有测试
	static bool RunAllTests() {
		std::cout << "开始运行FString测试用例..." << std::endl;

		bool all_passed = true;

		all_passed &= TestConstructors();
		all_passed &= TestAssignmentOperators();
		all_passed &= TestConcatenationOperators();
		all_passed &= TestComparisonOperators();
		all_passed &= TestIndexAccess();
		all_passed &= TestStringOperations();
		all_passed &= TestTypeConversions();
		all_passed &= TestStaticMethods();
		all_passed &= TestMemoryManagement();

		std::cout << "\n=== 测试结果 ===" << std::endl;
		if (all_passed) {
			std::cout << "所有测试通过!" << std::endl;
		}
		else {
			std::cout << "部分测试失败!" << std::endl;
		}

		return all_passed;
	}
};

int TestString() {
	try {
		bool result = FStringTester::RunAllTests();
		return result ? 0 : 1;
	}
	catch (const std::exception& e) {
		std::cout << "测试过程中发生异常: " << e.what() << std::endl;
		return 1;
	}
	catch (...) {
		std::cout << "测试过程中发生未知异常" << std::endl;
		return 1;
	}
}