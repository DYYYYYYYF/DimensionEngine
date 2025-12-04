#include "JsonObject.h"
#include <nlohmann/json.hpp>

struct JsonObject::JsonHandle {
	nlohmann::json value;
};

JsonObject::JsonObject() : Handle(std::make_shared<JsonHandle>()), IsDirty_(false) {}

JsonObject::JsonObject(File file) : Handle(std::make_shared<JsonHandle>()), IsDirty_(false) {
	if (file.IsExist()) {
		Handle->value = JsonObject(file.ReadBytes()).Handle->value;
	}
	else {
		Handle->value = nlohmann::json();
	}
}

JsonObject::JsonObject(JsonObject::Type type) : Handle(std::make_shared<JsonHandle>()), IsDirty_(false) {
	switch (type)
	{
	case JsonObject::Type::eObject: Handle->value = nlohmann::json::object(); break;
	case JsonObject::Type::eArray:  Handle->value = nlohmann::json::array();  break;
	}
}

JsonObject::JsonObject(const std::string& content) : Handle(std::make_shared<JsonHandle>()), IsDirty_(false) {
	try {
		Handle->value = nlohmann::json::parse(content);
	}
	catch (...) {
		Handle->value = nlohmann::json(); 
	}
}

JsonObject::~JsonObject() {
	if (IsDirty_) {
		SaveToFile(File());
	}
}

// SetArray 模板实现
template<typename T>
void JsonObject::SetArray(const std::string& key, const std::vector<T>& values) {
	nlohmann::json arr = nlohmann::json::array();
	for (const auto& val : values) {
		arr.push_back(val);
	}
	Handle->value[key] = arr;
}

// 显式实例化常用类型
template DAPI void JsonObject::SetArray<int>(const std::string&, const std::vector<int>&);
template DAPI void JsonObject::SetArray<float>(const std::string&, const std::vector<float>&);
template DAPI void JsonObject::SetArray<double>(const std::string&, const std::vector<double>&);
template DAPI void JsonObject::SetArray<bool>(const std::string&, const std::vector<bool>&);
template DAPI void JsonObject::SetArray<std::string>(const std::string&, const std::vector<std::string>&);

// SetArray for JsonObject
template<>
void JsonObject::SetArray<JsonObject>(const std::string& key, const std::vector<JsonObject>& values) {
	nlohmann::json arr = nlohmann::json::array();
	for (const auto& val : values) {
		arr.push_back(val.Handle->value);
	}
	
	JsonObject temp;
	temp.Handle->value = arr;
	Write(key, temp);
}

// SetVector2
void JsonObject::WriteVector2(const std::string& key, const Vector2& value) {
	nlohmann::json arr = nlohmann::json::array();
	arr.push_back(value.x);
	arr.push_back(value.y);

	JsonObject temp;
	temp.Handle->value = arr;
	Write(key, temp);
}

// SetVector3
void JsonObject::WriteVector3(const std::string& key, const Vector3& value) {
	nlohmann::json arr = nlohmann::json::array();
	arr.push_back(value.x);
	arr.push_back(value.y);
	arr.push_back(value.z);
	
	JsonObject temp;
	temp.Handle->value = arr;
	Write(key, temp);
}

// SetVector4
void JsonObject::WriteVector4(const std::string& key, const Vector4& value) {
	nlohmann::json arr = nlohmann::json::array();
	for (size_t i = 0; i < 4; ++i) {
		arr.push_back(value[i]);
	}
	
	JsonObject temp;
	temp.Handle->value = arr;
	Write(key, temp);
}

// SetMatrix4
void JsonObject::WriteMatrix4(const std::string& key, const Matrix4& value) {
	nlohmann::json arr = nlohmann::json::array();
	// 按行存储 4x4 矩阵
	for (size_t i = 0; i < 16; ++i) {
		arr.push_back(value[i]);
	}
	
	JsonObject temp;
    temp.Handle->value = arr;
    Write(key, temp); 
}

// GetVector2
Vector2 JsonObject::ReadVector2(const std::string& key, const Vector2& defaultValue) const {
	JsonObject obj = Read(key);
	if (!obj.IsArray() || obj.Size() != 2) return defaultValue;

	Vector2 result;
	result.x = obj.ArrayItemAt(0).GetFloat();
	result.y = obj.ArrayItemAt(1).GetFloat();
	return result;
}

// GetVector3
Vector3 JsonObject::ReadVector3(const std::string& key, const Vector3& defaultValue) const {
	JsonObject obj = Read(key);
	if (!obj.IsArray() || obj.Size() != 3) return defaultValue;

	Vector3 result;
	result.x = obj.ArrayItemAt(0).GetFloat();
	result.y = obj.ArrayItemAt(1).GetFloat();
	result.z = obj.ArrayItemAt(2).GetFloat();
	return result;
}

// GetVector4
Vector4 JsonObject::ReadVector4(const std::string& key, const Vector4& defaultValue) const {
	JsonObject obj = Read(key);
	if (!obj.IsArray() || obj.Size() != 4) return defaultValue;

	Vector4 result;
	result.x = obj.ArrayItemAt(0).GetFloat();
	result.y = obj.ArrayItemAt(1).GetFloat();
	result.z = obj.ArrayItemAt(2).GetFloat();
	result.w = obj.ArrayItemAt(3).GetFloat();
	return result;
}

// GetMatrix4
Matrix4 JsonObject::ReadMatrix4(const std::string& key, const Matrix4& defaultValue) const {
	JsonObject obj = Read(key);
	if (!obj.IsArray() || obj.Size() != 16) return defaultValue;

	Matrix4 result;
	for (size_t i = 0; i < 16; ++i) {
		result[i] = obj.ArrayItemAt(i).GetFloat();
	}
	return result;
}

// Read - 支持路径访问
JsonObject JsonObject::Read(const std::string& path) const {
	JsonObject result;

	if (path.empty()) {
		result.Handle->value = Handle->value;
		return result;
	}

	std::vector<std::string> tokens = SplitPath(path);
	nlohmann::json current = Handle->value;

	try {
		for (const auto& token : tokens) {
			if (current.is_object() && current.contains(token)) {
				current = current[token];
			}
			else {
				// 路径不存在，返回 null
				result.Handle->value = nlohmann::json();
				return result;
			}
		}

		result.Handle->value = current;
	}
	catch (...) {
		result.Handle->value = nlohmann::json();
	}

	return result;
}

std::string JsonObject::ReadString(const std::string& path, const std::string& defaultValue) const {
	JsonObject obj = Read(path);
	return obj.IsString() ? obj.GetString() : defaultValue;
}

int JsonObject::ReadInt(const std::string& path, int defaultValue) const {
	JsonObject obj = Read(path);
	return obj.IsNumber() ? obj.GetInt() : defaultValue;
}

float JsonObject::ReadFloat(const std::string& path, float defaultValue) const {
	JsonObject obj = Read(path);
	return obj.IsNumber() ? obj.GetFloat() : defaultValue;
}

bool JsonObject::ReadBool(const std::string& path, bool defaultValue) const {
	JsonObject obj = Read(path);
	return obj.IsBool() ? obj.GetBool() : defaultValue;
}

double JsonObject::ReadDouble(const std::string& path, double defaultValue) const {
	JsonObject obj = Read(path);
	return obj.IsNumber() ? obj.GetDouble() : defaultValue;
}

// Write - 支持路径写入，自动创建中间对象
void JsonObject::Write(const std::string& path, const JsonObject& value) {
	if (path.empty()) return;

	std::vector<std::string> tokens = SplitPath(path);

	if (tokens.empty()) return;

	// 确保根是对象
	if (!Handle->value.is_object()) {
		Handle->value = nlohmann::json::object();
	}

	nlohmann::json* current = &Handle->value;

	// 遍历路径，创建中间对象
	for (size_t i = 0; i < tokens.size() - 1; ++i) {
		const std::string& token = tokens[i];

		if (!current->contains(token) || !(*current)[token].is_object()) {
			(*current)[token] = nlohmann::json::object();
		}

		current = &(*current)[token];
	}

	// 设置最终值
	(*current)[tokens.back()] = value.Handle->value;
}

void JsonObject::WriteString(const std::string& path, const std::string& value) {
	JsonObject temp;
	temp.Handle->value = value;
	Write(path, temp);
}

void JsonObject::WriteInt(const std::string& path, int value) {
	JsonObject temp;
	temp.Handle->value = value;
	Write(path, temp);
}

void JsonObject::WriteFloat(const std::string& path, float value) {
	JsonObject temp;
	temp.Handle->value = value;
	Write(path, temp);
}

void JsonObject::WriteBool(const std::string& path, bool value) {
	JsonObject temp;
	temp.Handle->value = value;
	Write(path, temp);
}

bool JsonObject::IsObject() const {
	return Handle->value.is_object();
}

bool JsonObject::IsArray() const {
	return Handle->value.is_array();
}

bool JsonObject::IsString() const {
	return Handle->value.is_string();
}

bool JsonObject::IsNull() const {
	return Handle->value.is_null();
}

bool JsonObject::IsBool() const {
	return Handle->value.is_boolean();
}

bool JsonObject::IsNumber() const {
	return Handle->value.is_number();
}

std::string JsonObject::GetString() const {
	if (Handle->value.is_string())
		return Handle->value.get<std::string>();
	return "";
}

JsonObject JsonObject::ArrayItemAt(size_t index) const {
	JsonObject Obj;
	if (IsArray()) {
		Obj.Handle->value = Handle->value.at(index);
	}
	else
	{
		Obj.Handle->value = nlohmann::json();
	}
	return Obj;
}

void JsonObject::ArrayPush(const JsonObject& value) {
	if (IsArray()) {
		Handle->value.push_back(value.Handle->value);
	}
}

bool JsonObject::ArrayRemoveAt(size_t index) {
	if (IsArray() && index < Handle->value.size()) {
		Handle->value.erase(Handle->value.begin() + index);
		return true;
	}
	return false;
}

bool JsonObject::HasKey(const std::string& key) const {
	if (!IsObject()) return false;
	return Handle->value.find(key) != Handle->value.end();
}

bool JsonObject::Remove(const std::string& key) {
	if (!IsObject()) return false;
	return Handle->value.erase(key) > 0;
}

std::vector<std::string> JsonObject::GetKeys() const {
	std::vector<std::string> keys;
	if (IsObject()) {
		for (auto it = Handle->value.begin(); it != Handle->value.end(); ++it) {
			keys.push_back(it.key());
		}
	}
	return keys;
}

bool JsonObject::SaveToFile(File file) const {
	try {
		std::string content = Dump(2);
		file.WriteBytes(content.c_str(), content.size());
		IsDirty_ = true;
		return true;
	}
	catch (...) {
		return false;
	}
}

bool JsonObject::LoadFromFile(const File& file) {
	try {
		if (!file.IsExist()) return false;
		std::string content = file.ReadBytes();
		Handle->value = nlohmann::json::parse(content);
		return true;
	}
	catch (...) {
		return false;
	}
}

bool JsonObject::IsEmpty() const {
	return Handle->value.empty();
}

JsonObject JsonObject::Clone() const {
	JsonObject copy;
	copy.Handle->value = Handle->value;
	return copy;
}

void JsonObject::Merge(const JsonObject& other) {
	if (IsObject() && other.IsObject()) {
		Handle->value.update(other.Handle->value);
	}
}

size_t JsonObject::Size() const {
	return Handle->value.size();
}

std::string JsonObject::Dump(int indent) const {
	return Handle->value.dump(indent);
}

float JsonObject::GetFloat() const {
	if (Handle->value.is_number())
		return Handle->value.get<float>();
	return 0.0f;
}

bool JsonObject::GetBool(bool defaultValue) const {
	if (Handle->value.is_boolean())
		return Handle->value.get<bool>();
	return defaultValue;
}

int JsonObject::GetInt(int defaultValue) const {
	if (Handle->value.is_number())
		return Handle->value.get<int>();
	return defaultValue;
}

double JsonObject::GetDouble(double defaultValue) const {
	if (Handle->value.is_number())
		return Handle->value.get<double>();
	return defaultValue;
}

JsonObject JsonObject::Get(const std::string& key) const {
	JsonObject Obj;
	auto it = Handle->value.find(key);
	if (it != Handle->value.end()) {
		Obj.Handle->value = *it;
	}
	else {
		Obj.Handle->value = nlohmann::json();
	}
	return Obj;
}

void JsonObject::SetString(const std::string& key, const std::string& value) {
	Handle->value[key] = value;
}

void JsonObject::SetInt(const std::string& key, int value) {
	Handle->value[key] = value;
}

void JsonObject::SetFloat(const std::string& key, float value) {
	Handle->value[key] = value;
}

void JsonObject::SetBool(const std::string& key, bool value) {
	Handle->value[key] = value;
}

void JsonObject::Set(const std::string& key, const JsonObject& value) {
	Handle->value[key] = value.Handle->value;
}

void JsonObject::Clear() {
	Handle->value = nlohmann::json();
}

std::vector<std::string> JsonObject::SplitPath(const std::string& path) const {
	std::vector<std::string> tokens;
	std::stringstream ss(path);
	std::string token;

	while (std::getline(ss, token, '.')) {
		if (!token.empty()) {
			tokens.push_back(token);
		}
	}

	return tokens;
}