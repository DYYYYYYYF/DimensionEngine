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

	void SetTranslate(std::string vec_str) { 
		Vector3 val = ConvertStrToVec3(vec_str);
		transflate = val; 
	}
	Vector3 GetTranslate() const { return transflate; }

	void SetRotate(std::string vec_str) {
		Vector3 val = ConvertStrToVec3(vec_str);
		rotate = val; 
	}
	Vector3 GetRotate() const { return rotate; }

	void SetScale(std::string vec_str) {
		Vector3 val = ConvertStrToVec3(vec_str);
		scale = val; 
	}
	Vector3 GetScale() const { return scale; }

private:
	Vector3 ConvertStrToVec3(const std::string& val) {
		Vector3 vec = { 1, 1, 1 };

		size_t sPos = val.find_first_of(' ');
		size_t ePos = val.find_last_of(' ');

		float x = std::stof(val.substr(0, sPos).c_str());
		vec.x = x;
		float y = std::stof(val.substr(sPos + 1, ePos).c_str());
		vec.y = y;
		float z = std::stof(val.substr(ePos + 1, val.npos).c_str());
		vec.z = z;

		return vec;
	}

private:
	std::string modelFile;
	std::string vertFile;
	std::string fragFile;
	std::string textureFile;

	Vector3 transflate;
	Vector3 rotate;
	Vector3 scale;
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
		m_Data[key] = val;
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
