#pragma once

#include "Defines.hpp"
#include "rapidjson/document.h"
#include "Core/EngineLogger.hpp"

#include <string>

class JSONReader {
public:
	ENGINE_API JSONReader(const std::string& filepath);
	ENGINE_API virtual ~JSONReader();

public:
	ENGINE_API void SetPropertyString(const std::string& key, const std::string& val);
	ENGINE_API std::string ReadPropertyString(const std::string& key);

	ENGINE_API void SetPropertyInt(const std::string& key, int val);
	ENGINE_API int ReadPropertyInt(const std::string& key);

	ENGINE_API void SetPropertyFloat(const std::string& key, float val);
	ENGINE_API float ReadPropertyFloat(const std::string& key);

	ENGINE_API void SetPropertyDouble(const std::string& key, double val);
	ENGINE_API double ReadPropertyDouble(const std::string& key);

private:
	template<typename T>
	void SetPropertyNumber(const std::string& key, T val) {
		std::vector<std::string> Keys = SplitPath(key);
		rapidjson::Value NewVal(val);
		if (!ModifyJSONValueByPath(Context, Keys, NewVal, Context.GetAllocator())) {
			LOG_ERROR("Modify property failed. property: %s. file: %s.", key.c_str(), Filename.c_str());
		}

		// 将修改后的 JSON 转换为字符串
		SaveFile();
	}

private:
	bool CouldParsed() const { return IsParsed; }
	bool FindProperty(const rapidjson::Value& Val, const std::string& TargetKey, std::string& V);
	std::vector<std::string> SplitPath(const std::string& key);
	bool ModifyJSONValueByPath(rapidjson::Value& node, const std::vector<std::string>& keys,
		const rapidjson::Value& val, rapidjson::Document::AllocatorType& allocator);
	void SaveFile();

private:
	bool IsParsed;
	std::string Filename;
	rapidjson::Document Context;
};