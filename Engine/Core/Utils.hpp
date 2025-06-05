#pragma once

#include <string>
#include <vector>

// Shader compiler
#include <shaderc/shaderc.hpp>
#include "Resources/Shader.hpp"

class DAPI Utils {
// String
public:
	/**
	 * @brief 去除字符串左右两端无效字符
	 * @param 需要被Trim的字符串
	 * @return Trim后的字符串
	 */
	static std::string Strtrim(const std::string& str);
	/**
	 * @brief 拆分字符串
	 * @param str 源字符串
	 * @param delimiter 拆分的字符
	 * @param trim_entries 拆分后的字符串是否需要Trim
	 * @param include_empty 是否保留空字符串
	 * @return 拆分后的字符串数组
	 */
	static std::vector<std::string> StringSplit(const std::string& str, char delimiter, bool trim_entries, bool include_empty);
};

