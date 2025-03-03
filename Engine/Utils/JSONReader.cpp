#include "JSONReader.h"
#include "Platform/File.hpp"

#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"

JSONReader::JSONReader(const std::string& filepath) {
	IsParsed = false;
	Filename = filepath;

	FILE* fp = fopen(filepath.c_str(), "r");
	if (!fp) {
		LOG_WARN("Can not open json file: %s", filepath.c_str());
		return;
	}

	// 读取内容
	char Buffer[65536];
	rapidjson::FileReadStream InputStream(fp, Buffer, sizeof(Buffer));

	// 解析JSON
	Context.ParseStream(InputStream);
	fclose(fp);

	// 检查解析结果
	if (Context.HasParseError()) {
		LOG_WARN("Can not open json file: %s", filepath.c_str());
		return;
	}

	IsParsed = true;
}

JSONReader::~JSONReader() {

}

bool JSONReader::FindProperty(const rapidjson::Value& Val, const std::string& TargetKey, std::string& V) {
	// 如果当前值是对象，遍历其成员
	bool Result = false;
	if (Val.IsObject()) {
		for (auto it = Val.MemberBegin(); it != Val.MemberEnd(); ++it) {
			const char* key = it->name.GetString();
			const rapidjson::Value& val = it->value;

			// 如果找到目标键
			if (TargetKey == key) {
				if (val.IsString()) {
					V = val.GetString();
					return true;
				}
				else if (val.IsInt()) {
					V = std::to_string(val.GetInt());
					return true;
				}
			}

			// 递归遍历嵌套对象或数组
			if (FindProperty(val, TargetKey, V)) {
				return true;
			}
		}
	}

	return Result;
}

void JSONReader::SetPropertyString(const std::string& key, const std::string& val) {
	std::vector<std::string> Keys = SplitPath(key);
	rapidjson::Value NewVal(val.c_str(), (rapidjson::SizeType)val.length());
	if (!ModifyJSONValueByPath(Context, Keys, NewVal, Context.GetAllocator())) {
		LOG_ERROR("Modify property failed. property: %s. file: %s.", key.c_str(), Filename.c_str());
	}

	// 将修改后的 JSON 转换为字符串
	SaveFile();
}

std::string JSONReader::ReadPropertyString(const std::string& key) {
	if (Context.IsObject()) {
		std::vector<std::string> Keys = SplitPath(key);
		return QueryJSONValue<const char*>(Context, Keys);
	}
	return "";
}

void JSONReader::SetPropertyInt(const std::string& key, int val) {
	SetPropertyNumber(key, val);
}

int JSONReader::ReadPropertyInt(const std::string& key) {
	if (Context.IsObject()) {
		std::vector<std::string> Keys = SplitPath(key);
		return QueryJSONValue<int>(Context, Keys);
	}
	return -1;
}

void JSONReader::SetPropertyFloat(const std::string& key, float val) {
	SetPropertyNumber(key, val);
}

float JSONReader::ReadPropertyFloat(const std::string& key) {
	if (Context.IsObject()) {
		std::vector<std::string> Keys = SplitPath(key);
		return QueryJSONValue<float>(Context, Keys);
	}
	return -1.0f;
}

void JSONReader::SetPropertyDouble(const std::string& key, double val) {
	SetPropertyNumber(key, val);
}

double JSONReader::ReadPropertyDouble(const std::string& key) {
	if (Context.IsObject()) {
		std::vector<std::string> Keys = SplitPath(key);
		return QueryJSONValue<double>(Context, Keys);
	}
	return -1.0;
}

void JSONReader::SetPropertyMatrix(const std::string& key, const Matrix4& Mat) {
	for (int i = 0; i < 16; ++i) {
		std::string KeyAtIndex = key + "[" + std::to_string(i) + "]";
		std::vector<std::string> keys = SplitPath(KeyAtIndex);
		SetPropertyNumber(KeyAtIndex, Mat[i]);
	}
}

Matrix4 JSONReader::ReadPropertyMatrix(const std::string& key) {
	Matrix4 ResultMat;
	for (int i = 0; i < 16; ++i) {
		std::string KeyAtIndex = key + "[" + std::to_string(i) + "]";
		std::vector<std::string> keys = SplitPath(KeyAtIndex);
		ResultMat[i] = QueryJSONValue<float>(Context, keys);
	}

	return ResultMat;
}

void JSONReader::SetPropertyVector(const std::string& key, const Vector& Vec) {
	for (int i = 0; i < 3; ++i) {
		std::string KeyAtIndex1 = key + "[" + std::to_string(0) + "]";
		SetPropertyNumber(KeyAtIndex1, Vec.x);
		std::string KeyAtIndex2 = key + "[" + std::to_string(1) + "]";
		SetPropertyNumber(KeyAtIndex2, Vec.y);
		std::string KeyAtIndex3 = key + "[" + std::to_string(2) + "]";
		SetPropertyNumber(KeyAtIndex3, Vec.z);
	}
}

Vector JSONReader::ReadPropertyVector(const std::string& key) {
	Vector ResultVec;
	std::string KeyAtIndex1 = key + "[" + std::to_string(0) + "]";
	std::vector<std::string> keys1 = SplitPath(KeyAtIndex1);
	ResultVec.x = QueryJSONValue<float>(Context, keys1);
	std::string KeyAtIndex2 = key + "[" + std::to_string(1) + "]";
	std::vector<std::string> keys2 = SplitPath(KeyAtIndex2);
	ResultVec.y = QueryJSONValue<float>(Context, keys2);
	std::string KeyAtIndex3 = key + "[" + std::to_string(2) + "]";
	std::vector<std::string> keys3 = SplitPath(KeyAtIndex3);
	ResultVec.z = QueryJSONValue<float>(Context, keys3);
	return ResultVec;
}

bool JSONReader::AddPropertyInt(const std::string& key, int val) {
	std::vector<std::string> keys = SplitPath(key);
	rapidjson::Value NewVal(val);
	return AddJSONValueByPath(Context, keys, NewVal, Context.GetAllocator());
}

bool JSONReader::AddPropertyFloat(const std::string& key, float val) {
	std::vector<std::string> keys = SplitPath(key);
	rapidjson::Value NewVal(val);
	return AddJSONValueByPath(Context, keys, NewVal, Context.GetAllocator());
}

bool JSONReader::AddPropertyDouble(const std::string& key, double val) {
	std::vector<std::string> keys = SplitPath(key);
	rapidjson::Value NewVal(val);
	return AddJSONValueByPath(Context, keys, NewVal, Context.GetAllocator());
}

bool JSONReader::AddPropertyString(const std::string& key, const std::string& val) {
	std::vector<std::string> keys = SplitPath(key);
	rapidjson::Value NewVal(val.c_str(), (rapidjson::SizeType)val.length());
	return AddJSONValueByPath(Context, keys, NewVal, Context.GetAllocator());
}

bool JSONReader::AddPropertyVector(const std::string& key, const Vector& val) {
	std::string KeyAtIndex1 = key + "[" + std::to_string(0) + "]";
	std::vector<std::string> keys = SplitPath(KeyAtIndex1);
	rapidjson::Value NewValX(val.x);
	rapidjson::Value NewValY(val.y);
	rapidjson::Value NewValZ(val.z);
	return AddJSONValueByPath(Context, keys, NewValX, Context.GetAllocator()) &&
		AddJSONValueByPath(Context, keys, NewValY, Context.GetAllocator())&&
		AddJSONValueByPath(Context, keys, NewValZ, Context.GetAllocator());
}

bool JSONReader::AddPropertyMatrix(const std::string& key, const Matrix4& val) {
	for (int i = 0; i < 16; ++i) {
		std::string KeyAtIndex = key + "[" + std::to_string(i) + "]";
		std::vector<std::string> keys = SplitPath(KeyAtIndex);
		rapidjson::Value NewVal(val[i]);
		if (!AddJSONValueByPath(Context, keys, NewVal, Context.GetAllocator())) {
			return false;
		}
	}
	return true;
}

// ----------------------------- Utils ------------------------------------ //
std::vector<std::string> JSONReader::SplitPath(const std::string& key) {
	std::vector<std::string> keys;
	std::stringstream ss(key);
	std::string k;
	while (std::getline(ss, k, '.')) {
		keys.push_back(k);
	}
	return keys;
}

void JSONReader::SaveFile() {
	FILE* fp = fopen(Filename.c_str(), "w");
	if (!fp) {
		LOG_WARN("Can not open json file: %s", Filename.c_str());
		return;
	}

	char writeBuffer[65536];
	rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
	rapidjson::Writer<rapidjson::FileWriteStream> writer(os);
	Context.Accept(writer);
	fclose(fp);
}

bool JSONReader::ModifyJSONValueByPath(rapidjson::Value& node, const std::vector<std::string>& keys, 
	const rapidjson::Value& val, rapidjson::Document::AllocatorType& allocator) {
	if (keys.empty()) return false;

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
				node[arrayKey.c_str()][index].CopyFrom(val, allocator);
				return true;
			}
			else {
				LOG_ERROR("Index out of range for array: %s", arrayKey.c_str());
				return false;
			}
		}
		else {
			LOG_ERROR("Array not found or invalid: %s", arrayKey.c_str());
			return false;
		}
	}

	if (keys.size() == 1) {
		// 到达目标键，修改其值
		if (node.HasMember(currentKey.c_str())) {
			node[currentKey.c_str()].CopyFrom(val, allocator);
			return true;
		}
		return false;
	}

	// 递归查找嵌套字段
	if (node.HasMember(currentKey.c_str()) && node[currentKey.c_str()].IsObject()) {
		std::vector<std::string> remainingKeys(keys.begin() + 1, keys.end());
		return ModifyJSONValueByPath(node[currentKey.c_str()], remainingKeys, val, allocator);
	}

	return false;
}

bool JSONReader::AddJSONValueByPath(rapidjson::Value& node, const std::vector<std::string>& keys,
	const rapidjson::Value& val, rapidjson::Document::AllocatorType& allocator) {
	if (keys.empty()) {
		LOG_ERROR("QueryJSONValue: Path is empty!");
		return false;
	}

	if (keys.size() < 2) {
		std::string currentKey = keys[0];
		if (keys.size() == 1) {
			// 处理数组索引
			if (currentKey.find('[') != std::string::npos && currentKey.back() == ']') {
				size_t indexStart = currentKey.find('[');
				size_t indexEnd = currentKey.find(']');
				std::string arrayKey = currentKey.substr(0, indexStart);
				int index = std::stoi(currentKey.substr(indexStart + 1, indexEnd - indexStart - 1));

				// 确保 node[arrayKey] 是一个数组
				if (!node.HasMember(arrayKey.c_str())) {
					node[arrayKey.c_str()].SetArray(); // 初始化为数组
				}
				if (node[arrayKey.c_str()].IsArray()) {
					rapidjson::Value NewLayer(rapidjson::kObjectType);
					rapidjson::Value key(currentKey.c_str(), allocator);
					rapidjson::Value valValue(val, allocator); 

					NewLayer.AddMember(key, valValue, allocator);
					node[arrayKey.c_str()].PushBack(NewLayer, allocator);
					return true;
				}
			} else {
				// 确保 node 是一个对象
				if (node.IsObject()) {
					rapidjson::Value key(currentKey.c_str(), allocator);
					rapidjson::Value valValue(val, allocator);
					node.AddMember(key, valValue, allocator);
					return true;
				}
			}
		}
	}


	// 递归查找嵌套字段
	if (node.HasMember(currentKey.c_str()) && node[currentKey.c_str()].IsObject()) {
		std::vector<std::string> remainingKeys(keys.begin() + 1, keys.end());
		return AddJSONValueByPath(node[currentKey.c_str()], remainingKeys, val, allocator);
	}

	LOG_ERROR("Add property failed: %s.", currentKey);
	return false;
}