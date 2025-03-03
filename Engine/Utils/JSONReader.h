#pragma once

#include "Defines.hpp"
#include "rapidjson/document.h"
#include "Math/MathTypes.hpp"
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

	ENGINE_API void SetPropertyMatrix(const std::string& key, const Matrix4& Mat);
	ENGINE_API Matrix4 ReadPropertyMatrix(const std::string& key);

	ENGINE_API void SetPropertyVector(const std::string& key, const Vector& Vec);
	ENGINE_API Vector ReadPropertyVector(const std::string& key);

	ENGINE_API bool AddPropertyInt(const std::string& key, int val);
	ENGINE_API bool AddPropertyFloat(const std::string& key, float val);
	ENGINE_API bool AddPropertyDouble(const std::string& key, double val);
	ENGINE_API bool AddPropertyString(const std::string& key, const std::string& val);
	ENGINE_API bool AddPropertyVector(const std::string& key, const Vector& val);
	ENGINE_API bool AddPropertyMatrix(const std::string& key, const Matrix4& val);

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

	template <typename T>
	T QueryJSONValue(const rapidjson::Value& node, const std::vector<std::string>& keys) {
		if (keys.empty()) {
			LOG_ERROR("QueryJSONValue: Path is empty!");
			return T();
		}

		std::string currentKey = keys[0];

		// 处理数组索引
		if (currentKey.find('[') != std::string::npos && currentKey.back() == ']') {
			size_t indexStart = currentKey.find('[');
			size_t indexEnd = currentKey.find(']');
			std::string arrayKey = currentKey.substr(0, indexStart);
			int index = std::stoi(currentKey.substr(indexStart + 1, indexEnd - indexStart - 1));

			if (node.HasMember(arrayKey.c_str()) && node[arrayKey.c_str()].IsArray()) {
				const rapidjson::Value& array = node[arrayKey.c_str()];
				if (index >= 0 && index < (int)array.Size()) {
					return array[index].Get<T>();
				}
				else {
					LOG_ERROR("Index out of range for array: %s", arrayKey.c_str());
					return T();
				}
			}
			else {
				LOG_ERROR("Array not found or invalid: %s", arrayKey.c_str());
				return T();
			}
		}

		if (keys.size() == 1) {
			// 到达目标键，返回其值
			if (node.HasMember(currentKey.c_str())) {
				const rapidjson::Value& val = node[currentKey.c_str()];
				if (val.Is<T>()) {
					return val.Get<T>();
				}
				else {
					return T();
				}
			}
			else {
				LOG_ERROR("Key not found: %s", currentKey);
				return T();
			}
		}

		// 递归查找嵌套字段
		if (node.HasMember(currentKey.c_str()) && node[currentKey.c_str()].IsObject()) {
			std::vector<std::string> remainingKeys(keys.begin() + 1, keys.end());
			return QueryJSONValue<T>(node[currentKey.c_str()], remainingKeys);
		}

		LOG_ERROR("Invalid path or non-object node at key: %s.", currentKey);
		return T();
	}

private:
	bool CouldParsed() const { return IsParsed; }
	bool FindProperty(const rapidjson::Value& Val, const std::string& TargetKey, std::string& V);
	void SaveFile();

	std::vector<std::string> SplitPath(const std::string& key);
	bool ModifyJSONValueByPath(rapidjson::Value& node, const std::vector<std::string>& keys,
		const rapidjson::Value& val, rapidjson::Document::AllocatorType& allocator);

	bool AddJSONValueByPath(rapidjson::Value& node, const std::vector<std::string>& keys,
		const rapidjson::Value& val, rapidjson::Document::AllocatorType& allocator);

private:
	bool IsParsed;
	std::string Filename;
	rapidjson::Document Context;
};