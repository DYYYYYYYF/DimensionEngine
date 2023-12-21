#include "ConfigFile.hpp"

void ConfigFile::LoadFile(const char* file) {
	if (file == nullptr || file[0] == '\0') {
		return;
	}

	SetFileName(file);
	std::fstream* fs = FileUtil::GetFileStream();
	fs->open(file, std::ios::in);

	char data[256];
	size_t eqPos = 0;
	while (!fs->eof()) {
		fs->getline(data, 256);
		if (data == nullptr || data[0] == '\0') continue;
		
		std::string datas = data;
		if (datas.find("Config") != datas.npos && m_curState != LOADSTATE::eConfig) {
			m_curState = LOADSTATE::eConfig;
		}
		else if (datas.find("Model") != datas.npos && m_curState != LOADSTATE::eModelFile) {
			m_curState = LOADSTATE::eModelFile;
		} 
		else if (datas.find("Pipeline") != datas.npos && m_curState != LOADSTATE::ePipeline) {
			m_curState = LOADSTATE::ePipeline;
		} 
		else if (datas.find("Texture") != datas.npos && m_curState != LOADSTATE::eTexture) {
			m_curState = LOADSTATE::eTexture;
		}

		// Separate string
		if (m_curState == LOADSTATE::eConfig) {
			fs->getline(data, 256);

			if (data == nullptr || data[0] == '\0') continue;

			datas = data;
			eqPos = datas.find("=");

			std::string fir = datas.substr(0, eqPos);
			std::string sec = datas.substr(eqPos + 1, datas.length() - 1);

			SetData(fir, sec);
		}

		ModelFile modelFile;
		if (m_curState == LOADSTATE::eModelFile) {
			fs->getline(data, 256);
			if (data == nullptr || data[0] == '\0') continue;
			
			int nModelNum = std::stoi(data);
			m_Models.resize(nModelNum);

			for (int i = 0; i < nModelNum; ++i) {

				fs->getline(data, 256);
				m_Models[i].SetModelFile(data);
			}
		}

		if (m_curState == LOADSTATE::ePipeline) {
			for (int i = 0; i < (int)m_Models.size(); ++i) {

				fs->getline(data, 256);
				m_Models[i].SetVertFile(data);

				fs->getline(data, 256);
				m_Models[i].SetFragFile(data);
			}
		}

		if (m_curState == LOADSTATE::eTexture) {
			for (int i = 0; i < (int)m_Models.size(); ++i) {
				fs->getline(data, 256);
				m_Models[i].SetTextureFile(data);
			}
		}
	}

	fs->close();
}

void ConfigFile::SaveToFile() {
	if (m_pFileName == nullptr || m_pFileName[0] == '\0') {
		return;
	}

	std::fstream* fs = FileUtil::GetFileStream();
	fs->open(m_pFileName, std::ios::out | std::ios::ate);

	std::string res;
	res += "Config\n";
	for (auto& a : m_Data) {
		res += a.first + "=" + a.second + '\n';
	}

	res += "\nModel\n";
	res += std::to_string(m_Models.size()) + '\n';
	for (int i = 0; i < m_Models.size(); ++i) {
		res += m_Models[i].GetModelFile() + '\n';
	}

	res += "\nPipeline\n";
	for (int i = 0; i < m_Models.size(); ++i) {
		res += m_Models[i].GetVertFile() + '\n';
		res += m_Models[i].GetFragFile() + '\n';
	}

	res += "\nTexture\n";
	for (int i = 0; i < m_Models.size(); ++i) {
		res += m_Models[i].GetTextureFile() + '\n';
	}

	fs->write(res.data(), res.length());
}