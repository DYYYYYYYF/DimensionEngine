#pragma once

#include "File.hpp"
#include "Math/MathTypes.hpp"
#include <string>
#include <memory>

class JsonObject {
public:
	enum class Type { eNull, eBool, eNumber, eString, eObject, eArray };

public:
	DAPI JsonObject();
	DAPI JsonObject(const File& file);
	DAPI JsonObject(JsonObject::Type type);
	DAPI JsonObject(const FString& content);
	DAPI virtual ~JsonObject();

public:
	template<typename T>
	DAPI void SetArray(const std::string& key, const std::vector<T>& values);

	// 设置向量
	DAPI void WriteVector2(const std::string& key, const Vector2& value);
	DAPI void WriteVector3(const std::string& key, const Vector3& value);
	DAPI void WriteVector4(const std::string& key, const Vector4& value);

	// 获取向量（带默认值）
	DAPI Vector2 ReadVector2(const std::string& key, const Vector2& defaultValue = Vector2()) const;
	DAPI Vector3 ReadVector3(const std::string& key, const Vector3& defaultValue = Vector3()) const;
	DAPI Vector4 ReadVector4(const std::string& key, const Vector4& defaultValue = Vector4()) const;

	// 设置矩阵
	DAPI void WriteMatrix4(const std::string& key, const Matrix4& value);
	DAPI Matrix4 ReadMatrix4(const std::string& key, const Matrix4& defaultValue = Matrix4()) const;

	// 获取
	DAPI JsonObject Read(const std::string& path) const;
	DAPI std::string ReadString(const std::string& path, const std::string& defaultValue = "") const;
	DAPI int ReadInt(const std::string& path, int defaultValue = 0) const;
	DAPI float ReadFloat(const std::string& path, float defaultValue = 0.0f) const;
	DAPI bool ReadBool(const std::string& path, bool defaultValue = false) const;
	DAPI double ReadDouble(const std::string& path, double defaultValue = 0.0) const;

	DAPI std::string ReadString() const;
	DAPI int         ReadInt()    const;
	DAPI float       ReadFloat()  const;
	DAPI bool        ReadBool()   const;

	// 设置
	DAPI void Write(const std::string& path, const JsonObject& value);
	DAPI void WriteString(const std::string& path, const std::string& value);
	DAPI void WriteInt(const std::string& path, int value);
	DAPI void WriteFloat(const std::string& path, float value);
	DAPI void WriteBool(const std::string& path, bool value);

	// 类型检查
	DAPI bool IsObject() const;
	DAPI bool IsArray() const;
	DAPI bool IsString() const;
	DAPI bool IsNull() const;
	DAPI bool IsBool() const;
	DAPI bool IsNumber() const;

	DAPI JsonObject Get(const std::string& key) const;
	DAPI void Set(const std::string& key, const JsonObject& value);

	// 数组
	DAPI JsonObject ArrayItemAt(size_t index) const;
	DAPI void ArrayPush(const JsonObject& value);
	DAPI bool ArrayRemoveAt(size_t index);

	// 对象操作
	DAPI bool HasKey(const std::string& key) const;
	DAPI bool Remove(const std::string& key);
	DAPI std::vector<std::string> GetKeys() const;

	// 文件操作
	DAPI bool SaveToFile(File& file) const;
	DAPI bool LoadFromFile(const File& file);

	// 实用方法
	DAPI bool IsEmpty() const;
	DAPI JsonObject Clone() const;
	DAPI void Merge(const JsonObject& other);

	DAPI size_t Size() const;
	DAPI std::string Dump(int indent = -1) const;
	DAPI void Clear();

private:
	float GetFloat() const;
	std::string GetString() const;
	bool GetBool(bool defaultValue = false) const;
	int GetInt(int defaultValue = 0) const;
	double GetDouble(double defaultValue = 0.0) const;
	void SetString(const std::string& key, const std::string& value);
	void SetInt(const std::string& key, int value);
	void SetFloat(const std::string& key, float value);
	void SetBool(const std::string& key, bool value);

	std::vector<std::string> SplitPath(const std::string& path) const;

private:
	struct JsonHandle;
	std::shared_ptr<JsonHandle> Handle;
	mutable bool IsDirty_;

};