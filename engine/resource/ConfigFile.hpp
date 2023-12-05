#pragma once

#include <unordered_map>
#include <string>
#include "Resource.hpp"
#include "../global/FileUtil.hpp"

class ConfigFile : public Resource {
public:
	ConfigFile() {
		m_ResourceType = ResourceType::eCONFIG_FILE;
	}
	virtual ~ConfigFile() {
		m_Data.clear();
	}

	void SetData(std::string key, std::string val) {
		m_Data.insert(std::pair<std::string, std::string>(key, val));
	}
	std::string GetVal(std::string key) {
		return m_Data[key];
	}

	void LoadFile(const char* file);

	virtual void SaveToFile() override;

private:
	std::unordered_map<std::string, std::string> m_Data;

};
