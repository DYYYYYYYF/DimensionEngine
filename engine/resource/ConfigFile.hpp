#pragma once

#include <unordered_map>
#include <string>
#include "Resource.hpp"
#include "../global/FileUtil.hpp"
#include "../EngineStructures.hpp"

enum LOADSTATE{
	eConfig = 0,
	eModelFile,
	ePipeline,
	eTexture,
	eMax
};

struct ModelFile {
public:
	void SetModelFile(std::string filename) { modelFile = filename; }
	std::string GetModelFile() const { return modelFile; }

	void SetVertFile(std::string filename) { vertFile = filename; }
	std::string GetVertFile() const { return vertFile; }

	void SetFragFile(std::string filename) { fragFile = filename; }
	std::string GetFragFile() const { return fragFile; }

	void SetTextureFile(std::string filename) { textureFile = filename; }
	std::string GetTextureFile() const { return textureFile; }

private:
	std::string modelFile;
	std::string vertFile;
	std::string fragFile;

	std::string textureFile;
};

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

	const std::vector<ModelFile>& GetModelFiles() const { return m_Models; }

	void LoadFile(const char* file);

	virtual void SaveToFile() override;

private:
	std::unordered_map<std::string, std::string> m_Data;
	std::vector<ModelFile> m_Models;
	
	LOADSTATE m_curState = LOADSTATE::eMax;
};
