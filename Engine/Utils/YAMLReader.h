#pragma once

#include "Defines.hpp"
#include "yaml-cpp/yaml.h"
#include "Math/MathTypes.hpp"
#include "Core/EngineLogger.hpp"

#include <string>
#include <vector>

class YAMLReader {
public:
	ENGINE_API YAMLReader(const std::string& filepath);
	ENGINE_API virtual ~YAMLReader();

public:
	ENGINE_API bool IsValid() const { return IsParsed; }

	ENGINE_API void SetPropertyString(const std::string& key, const std::string& val);
	ENGINE_API std::string ReadPropertyString(const std::string& key);

	ENGINE_API void SetPropertyInt(const std::string& key, int val);
	ENGINE_API int ReadPropertyInt(const std::string& key);

	ENGINE_API void SetPropertyFloat(const std::string& key, float val);
	ENGINE_API float ReadPropertyFloat(const std::string& key);

	ENGINE_API void SetPropertyDouble(const std::string& key, double val);
	ENGINE_API double ReadPropertyDouble(const std::string& key);

	ENGINE_API void SetPropertyBool(const std::string& key, bool val);
	ENGINE_API bool ReadPropertyBool(const std::string& key);

	ENGINE_API void SetPropertyMatrix(const std::string& key, const Matrix4& Mat);
	ENGINE_API Matrix4 ReadPropertyMatrix(const std::string& key);

	ENGINE_API void SetPropertyVector(const std::string& key, const Vector& Vec);
	ENGINE_API Vector ReadPropertyVector(const std::string& key);

	ENGINE_API bool AddPropertyInt(const std::string& key, int val);
	ENGINE_API bool AddPropertyFloat(const std::string& key, float val);
	ENGINE_API bool AddPropertyDouble(const std::string& key, double val);
	ENGINE_API bool AddPropertyString(const std::string& key, const std::string& val);
	ENGINE_API bool AddPropertyBool(const std::string& key, bool val);
	ENGINE_API bool AddPropertyVector(const std::string& key, const Vector& val);
	ENGINE_API bool AddPropertyMatrix(const std::string& key, const Matrix4& val);

private:
	template<typename T>
	void SetPropertyValue(const std::string& key, const T& val) {
		std::vector<std::string> Keys = SplitPath(key);
		if (!ModifyYAMLValueByPath(Context, Keys, val)) {
			GLOG(Log::eError, "Modify property failed. property: %s. file: %s.", key.c_str(), Filename.c_str());
		}
		SaveFile();
	}

	template<typename T>
	T QueryYAMLValue(const YAML::Node& node, const std::vector<std::string>& keys) {
		if (keys.empty()) {
			GLOG(Log::eError, "QueryYAMLValue: Path is empty!");
			return T{};
		}

		std::string currentKey = keys[0];
		YAML::Node currentNode = node;

		// 处理数组索引
		if (currentKey.find('[') != std::string::npos && currentKey.back() == ']') {
			size_t indexStart = currentKey.find('[');
			size_t indexEnd = currentKey.find(']');
			std::string arrayKey = currentKey.substr(0, indexStart);
			int index = std::stoi(currentKey.substr(indexStart + 1, indexEnd - indexStart - 1));

			if (currentNode[arrayKey] && currentNode[arrayKey].IsSequence()) {
				const YAML::Node& array = currentNode[arrayKey];
				if (index >= 0 && index < (int)array.size()) {
					if (keys.size() == 1) {
						try {
							return array[index].as<T>();
						}
						catch (const YAML::Exception& e) {
							GLOG(Log::eError, "Failed to convert YAML value: %s", e.what());
							return T{};
						}
					}
					else {
						std::vector<std::string> remainingKeys(keys.begin() + 1, keys.end());
						return QueryYAMLValue<T>(array[index], remainingKeys);
					}
				}
				else {
					GLOG(Log::eError, "Index out of range for array: %s", arrayKey.c_str());
					return T{};
				}
			}
			else {
				GLOG(Log::eError, "Array not found or invalid: %s", arrayKey.c_str());
				return T{};
			}
		}

		if (keys.size() == 1) {
			// 到达目标键，返回其值
			if (currentNode[currentKey]) {
				try {
					return currentNode[currentKey].as<T>();
				}
				catch (const YAML::Exception& e) {
					GLOG(Log::eError, "Failed to convert YAML value for key '%s': %s", currentKey.c_str(), e.what());
					return T{};
				}
			}
			else {
				GLOG(Log::eError, "Key not found: %s", currentKey.c_str());
				return T{};
			}
		}

		// 递归查找嵌套字段
		if (currentNode[currentKey] && currentNode[currentKey].IsMap()) {
			std::vector<std::string> remainingKeys(keys.begin() + 1, keys.end());
			return QueryYAMLValue<T>(currentNode[currentKey], remainingKeys);
		}

		GLOG(Log::eError, "Invalid path or non-map node at key: %s", currentKey.c_str());
		return T{};
	}

private:
	bool CouldParsed() const { return IsParsed; }
	void SaveFile();

	std::vector<std::string> SplitPath(const std::string& key);

	template<typename T>
	bool ModifyYAMLValueByPath(YAML::Node& node, const std::vector<std::string>& keys, const T& val);

	template<typename T>
	bool AddYAMLValueByPath(YAML::Node& node, const std::vector<std::string>& keys, const T& val);

	// 辅助函数：将Matrix4转换为YAML节点
	YAML::Node MatrixToYAML(const Matrix4& matrix, bool useSequence = true);
	Matrix4 YAMLToMatrix(const YAML::Node& node);

	// 辅助函数：将Vector转换为YAML节点
	YAML::Node VectorToYAML(const Vector& vector, bool useSequence = true);
	Vector YAMLToVector(const YAML::Node& node);

private:
	bool IsParsed;
	std::string Filename;
	YAML::Node Context;
};