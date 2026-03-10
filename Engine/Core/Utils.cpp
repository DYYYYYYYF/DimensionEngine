#include "Utils.hpp"
#include "Platform/File.hpp"
#include "Rendering/Resources/Shader.hpp"
#include "Systems/ResourceSystem.h"

std::string Utils::Strtrim(const std::string& str) {
	// 找到左边第一个非空白字符
	size_t start = str.find_first_not_of(" \t\n\r\f\v");
	if (start == std::string::npos) {
		return ""; // 全是空白字符
	}

	// 找到右边最后一个非空白字符
	size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(start, end - start + 1);
}

std::vector<std::string> Utils::StringSplit(const std::string& str, char delimiter, bool trim_entries, bool include_empty) {
	std::vector<std::string> Result;
	size_t Head = 0;
	size_t Rear = str.find(delimiter);

	while (Rear != std::string::npos) {
		std::string SubString = str.substr(Head, Rear - Head);

		if (trim_entries) {
			SubString = Strtrim(SubString);
		}

		// 如果字符串不为空或者需要包含空字符都存储子字符串
		if (!SubString.empty() || include_empty) {
			Result.push_back(SubString); 
		}

		Head = Rear + 1;                                 // 更新起始位置
		Rear = str.find(delimiter, Head);                // 查找下一个分隔符
	}

	// 处理最后一个子字符串
	if (Head < str.length()) {
		std::string LastSubString = str.substr(Head);
		if (trim_entries) {
			LastSubString = Strtrim(LastSubString);

		}
		if (!LastSubString.empty() || include_empty) {
			Result.push_back(LastSubString);

		}
	}

	return Result;
}
