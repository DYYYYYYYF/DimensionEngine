#pragma once

#include "Defines.hpp"
#include "yaml-cpp/yaml.h"
#include "Math/MathTypes.hpp"
#include "Core/EngineLogger.hpp"

#include <string>
#include <vector>

class YAMLReader {
public:
	ENGINE_API YAMLReader(const std::string& filepath, bool enableAutoBatch = true);
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

	ENGINE_API void Flush();                   
	ENGINE_API void EnableAutoBatch(bool enable = true);
	ENGINE_API bool IsAutoBatchEnabled() const { return m_autoBatch; }

private:
	template<typename T>
	void SetPropertyValue(const std::string& key, const T& val);

	template<typename T>
	T QueryYAMLValue(const YAML::Node& node, const std::vector<std::string>& keys);

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

	void SaveIfNeeded();

private:
	bool IsParsed;
	std::string Filename;
	YAML::Node Context;

	bool m_autoBatch;     
	bool m_isDirty;      
};