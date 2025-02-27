#pragma once

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filereadstream.h"

#include <string>

class JSONReader {
public:
	JSONReader(const std::string& filepath);
	virtual ~JSONReader();

public:
	bool CouldParsed() const { return IsParsed; }

	bool FindProperty(const rapidjson::Value& Val, const std::string& TargetKey, std::string& V) {
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

	std::string ReadPropertyString(const std::string& key) {
		if (Context.IsObject()) {
			std::string ResString;
			FindProperty(Context, key, ResString);
			return ResString;
		}
		return "";
	}

	int ReadPropertyInt(const std::string& key) {
		if (Context.IsObject()) {
			std::string ResInt;
			FindProperty(Context, key, ResInt);
			return (int)atof(ResInt.c_str());
		}
		return -1;
	}

private:
	bool IsParsed;
	rapidjson::Document Context;
};