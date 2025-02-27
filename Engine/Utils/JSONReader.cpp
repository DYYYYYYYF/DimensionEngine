#include "JSONReader.h"
#include "Platform/File.hpp"
#include "Core/EngineLogger.hpp"

JSONReader::JSONReader(const std::string& filepath) {
	IsParsed = false;

	FILE* fp = fopen((std::string(ROOT_PATH) + "/Config/EngineConfig.json").c_str(), "r");
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
