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
		std::string ResString;
		FindProperty(Context, key, ResString);
		return ResString;
	}
	return "";
}

void JSONReader::SetPropertyInt(const std::string& key, int val) {
	SetPropertyNumber(key, val);
}

int JSONReader::ReadPropertyInt(const std::string& key) {
	if (Context.IsObject()) {
		std::string ResInt;
		FindProperty(Context, key, ResInt);
		if (!ResInt.empty()) {
			return (int)atof(ResInt.c_str());
		}
	}
	return -1;
}

void JSONReader::SetPropertyFloat(const std::string& key, float val) {
	SetPropertyNumber(key, val);
}

float JSONReader::ReadPropertyFloat(const std::string& key) {
	if (Context.IsObject()) {
		std::string ResInt;
		FindProperty(Context, key, ResInt);
		if (!ResInt.empty()) {
			return (float)atof(ResInt.c_str());
		}
	}
	return -1;
}

void JSONReader::SetPropertyDouble(const std::string& key, double val) {
	SetPropertyNumber(key, val);
}

double JSONReader::ReadPropertyDouble(const std::string& key) {
	if (Context.IsObject()) {
		std::string ResInt;
		FindProperty(Context, key, ResInt);
		if (!ResInt.empty()) {
			return atof(ResInt.c_str());
		}
	}
	return -1;
}

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