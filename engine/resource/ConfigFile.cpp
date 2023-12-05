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
		
		// Separate string
		std::string datas = data;
		eqPos = datas.find("=");
		std::string fir = datas.substr(0, eqPos);
		std::string sec = datas.substr(eqPos + 1, datas.length() - 1);

		SetData(fir, sec);
	}

	fs->close();
}

void ConfigFile::SaveToFile() {
	if (m_pFileName == nullptr || m_pFileName[0] == '\0') {
		return;
	}

	std::fstream* fs = FileUtil::GetFileStream();
	fs->open(m_pFileName, std::ios::out | std::ios::ate);
	fs->write("", 1);
	fs->open(m_pFileName, std::ios::out | std::ios::app);


	for (auto& a : m_Data) {
		std::string res = a.first + a.second;
		fs->write(res.data(), res.length());
	}
}