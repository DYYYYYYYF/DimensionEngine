#include "YAMLReader.h"
#include <fstream>
#include <sstream>

YAMLReader::YAMLReader(const std::string& filepath) : IsParsed(false), Filename(filepath) {
	try {
		Context = YAML::LoadFile(filepath);
		IsParsed = true;
		GLOG(Log::eInfo, "YAML file loaded successfully: %s", filepath.c_str());
	}
	catch (const YAML::Exception& e) {
		GLOG(Log::eError, "Failed to load YAML file: %s. Error: %s", filepath.c_str(), e.what());
		IsParsed = false;
	}
}

YAMLReader::~YAMLReader() {
	// 析构函数
}

void YAMLReader::SetPropertyString(const std::string& key, const std::string& val) {
	SetPropertyValue(key, val);
}

std::string YAMLReader::ReadPropertyString(const std::string& key) {
	std::vector<std::string> Keys = SplitPath(key);
	return QueryYAMLValue<std::string>(Context, Keys);
}

void YAMLReader::SetPropertyInt(const std::string& key, int val) {
	SetPropertyValue(key, val);
}

int YAMLReader::ReadPropertyInt(const std::string& key) {
	std::vector<std::string> Keys = SplitPath(key);
	return QueryYAMLValue<int>(Context, Keys);
}

void YAMLReader::SetPropertyFloat(const std::string& key, float val) {
	SetPropertyValue(key, val);
}

float YAMLReader::ReadPropertyFloat(const std::string& key) {
	std::vector<std::string> Keys = SplitPath(key);
	return QueryYAMLValue<float>(Context, Keys);
}

void YAMLReader::SetPropertyDouble(const std::string& key, double val) {
	SetPropertyValue(key, val);
}

double YAMLReader::ReadPropertyDouble(const std::string& key) {
	std::vector<std::string> Keys = SplitPath(key);
	return QueryYAMLValue<double>(Context, Keys);
}

void YAMLReader::SetPropertyBool(const std::string& key, bool val) {
	SetPropertyValue(key, val);
}

bool YAMLReader::ReadPropertyBool(const std::string& key) {
	std::vector<std::string> Keys = SplitPath(key);
	return QueryYAMLValue<bool>(Context, Keys);
}

void YAMLReader::SetPropertyMatrix(const std::string& key, const Matrix4& Mat) {
	std::vector<std::string> Keys = SplitPath(key);
	YAML::Node matrixNode = MatrixToYAML(Mat);
	if (!ModifyYAMLValueByPath(Context, Keys, matrixNode)) {
		GLOG(Log::eError, "Modify matrix property failed. property: %s. file: %s.", key.c_str(), Filename.c_str());
	}
	SaveFile();
}

Matrix4 YAMLReader::ReadPropertyMatrix(const std::string& key) {
	std::vector<std::string> Keys = SplitPath(key);
	YAML::Node matrixNode = QueryYAMLValue<YAML::Node>(Context, Keys);
	return YAMLToMatrix(matrixNode);
}

void YAMLReader::SetPropertyVector(const std::string& key, const Vector& Vec) {
	std::vector<std::string> Keys = SplitPath(key);
	YAML::Node vectorNode = VectorToYAML(Vec);
	if (!ModifyYAMLValueByPath(Context, Keys, vectorNode)) {
		GLOG(Log::eError, "Modify vector property failed. property: %s. file: %s.", key.c_str(), Filename.c_str());
	}
	SaveFile();
}

Vector YAMLReader::ReadPropertyVector(const std::string& key) {
	std::vector<std::string> Keys = SplitPath(key);
	YAML::Node vectorNode = QueryYAMLValue<YAML::Node>(Context, Keys);
	return YAMLToVector(vectorNode);
}

bool YAMLReader::AddPropertyInt(const std::string& key, int val) {
	std::vector<std::string> Keys = SplitPath(key);
	bool result = AddYAMLValueByPath(Context, Keys, val);
	if (result) {
		SaveFile();
	}
	return result;
}

bool YAMLReader::AddPropertyFloat(const std::string& key, float val) {
	std::vector<std::string> Keys = SplitPath(key);
	bool result = AddYAMLValueByPath(Context, Keys, val);
	if (result) {
		SaveFile();
	}
	return result;
}

bool YAMLReader::AddPropertyDouble(const std::string& key, double val) {
	std::vector<std::string> Keys = SplitPath(key);
	bool result = AddYAMLValueByPath(Context, Keys, val);
	if (result) {
		SaveFile();
	}
	return result;
}

bool YAMLReader::AddPropertyString(const std::string& key, const std::string& val) {
	std::vector<std::string> Keys = SplitPath(key);
	bool result = AddYAMLValueByPath(Context, Keys, val);
	if (result) {
		SaveFile();
	}
	return result;
}

bool YAMLReader::AddPropertyBool(const std::string& key, bool val) {
	std::vector<std::string> Keys = SplitPath(key);
	bool result = AddYAMLValueByPath(Context, Keys, val);
	if (result) {
		SaveFile();
	}
	return result;
}

bool YAMLReader::AddPropertyVector(const std::string& key, const Vector& val) {
	std::vector<std::string> Keys = SplitPath(key);
	YAML::Node vectorNode = VectorToYAML(val);
	bool result = AddYAMLValueByPath(Context, Keys, vectorNode);
	if (result) {
		SaveFile();
	}
	return result;
}

bool YAMLReader::AddPropertyMatrix(const std::string& key, const Matrix4& val) {
	std::vector<std::string> Keys = SplitPath(key);
	YAML::Node matrixNode = MatrixToYAML(val);
	bool result = AddYAMLValueByPath(Context, Keys, matrixNode);
	if (result) {
		SaveFile();
	}
	return result;
}

void YAMLReader::SaveFile() {
	try {
		std::ofstream file(Filename);
		file << Context;
		file.close();
	}
	catch (const std::exception& e) {
		GLOG(Log::eError, "Failed to save YAML file: %s. Error: %s", Filename.c_str(), e.what());
	}
}

std::vector<std::string> YAMLReader::SplitPath(const std::string& key) {
	std::vector<std::string> keys;
	std::string current;

	for (char c : key) {
		if (c == '.') {
			if (!current.empty()) {
				keys.push_back(current);
				current.clear();
			}
		}
		else {
			current += c;
		}
	}

	if (!current.empty()) {
		keys.push_back(current);
	}

	return keys;
}

template<typename T>
bool YAMLReader::ModifyYAMLValueByPath(YAML::Node& node, const std::vector<std::string>& keys, const T& val) {
	if (keys.empty()) {
		return false;
	}

	if (keys.size() == 1) {
		std::string currentKey = keys[0];

		// 处理数组索引
		if (currentKey.find('[') != std::string::npos && currentKey.back() == ']') {
			size_t indexStart = currentKey.find('[');
			size_t indexEnd = currentKey.find(']');
			std::string arrayKey = currentKey.substr(0, indexStart);
			int index = std::stoi(currentKey.substr(indexStart + 1, indexEnd - indexStart - 1));

			if (!node[arrayKey]) {
				node[arrayKey] = YAML::Node(YAML::NodeType::Sequence);
			}

			YAML::Node& array = node[arrayKey];
			if (array.IsSequence()) {
				// 扩展数组大小以适应索引
				while ((int)array.size() <= index) {
					array.push_back(YAML::Node());
				}
				array[index] = val;
				return true;
			}
		}
		else {
			node[currentKey] = val;
			return true;
		}
	}

	std::string currentKey = keys[0];

	// 处理数组索引的嵌套情况
	if (currentKey.find('[') != std::string::npos && currentKey.back() == ']') {
		size_t indexStart = currentKey.find('[');
		size_t indexEnd = currentKey.find(']');
		std::string arrayKey = currentKey.substr(0, indexStart);
		int index = std::stoi(currentKey.substr(indexStart + 1, indexEnd - indexStart - 1));

		if (!node[arrayKey]) {
			node[arrayKey] = YAML::Node(YAML::NodeType::Sequence);
		}

		YAML::Node& array = node[arrayKey];
		if (array.IsSequence()) {
			while ((int)array.size() <= index) {
				array.push_back(YAML::Node(YAML::NodeType::Map));
			}

			std::vector<std::string> remainingKeys(keys.begin() + 1, keys.end());
			return ModifyYAMLValueByPath(array[index], remainingKeys, val);
		}
	}
	else {
		if (!node[currentKey]) {
			node[currentKey] = YAML::Node(YAML::NodeType::Map);
		}

		std::vector<std::string> remainingKeys(keys.begin() + 1, keys.end());
		return ModifyYAMLValueByPath(node[currentKey], remainingKeys, val);
	}

	return false;
}

template<typename T>
bool YAMLReader::AddYAMLValueByPath(YAML::Node& node, const std::vector<std::string>& keys, const T& val) {
	// 检查键是否已存在
	std::vector<std::string> Keys = SplitPath(keys[0]);
	YAML::Node testNode = node;
	bool exists = true;

	try {
		for (const auto& key : keys) {
			if (testNode[key]) {
				testNode = testNode[key];
			}
			else {
				exists = false;
				break;
			}
		}
	}
	catch (...) {
		exists = false;
	}

	if (exists) {
		GLOG(Log::eWarn, "Property already exists, use SetProperty instead: %s", keys[0].c_str());
		return false;
	}

	return ModifyYAMLValueByPath(node, keys, val);
}

YAML::Node YAMLReader::MatrixToYAML(const Matrix4& matrix, bool useSequence/* = true*/) {
	YAML::Node node;

	if (useSequence) {
		// 扁平数组格式
		for (int i = 0; i < 16; ++i) {
			node.push_back(matrix.data[i]);
		}
	}
	else {
		// 带 type 和 data 字段的结构化格式
		node["type"] = "Matrix4";
		YAML::Node dataNode;
		for (int i = 0; i < 4; ++i) {
			YAML::Node row;
			for (int j = 0; j < 4; ++j) {
				row.push_back(matrix.GetRow(i)[j]);
			}
			dataNode.push_back(row);
		}
		node["data"] = dataNode;
	}

	return node;
}

Matrix4 YAMLReader::YAMLToMatrix(const YAML::Node& node) {
	Matrix4 matrix;

	try {
		if (!node) return matrix;

		// 扁平数组格式
		if (node.IsSequence() && node.size() == 16) {
			for (int i = 0; i < 16; ++i) {
				matrix.data[i] = node[i].as<float>();
			}
		}
		// 结构化格式
		else if (node["type"] && node["type"].as<std::string>() == "Matrix4" && node["data"]) {
			const YAML::Node& dataNode = node["data"];
			for (int i = 0; i < 4 && i < (int)dataNode.size(); ++i) {
				const YAML::Node& row = dataNode[i];
				for (int j = 0; j < 4 && j < (int)row.size(); ++j) {
					matrix.data[j * 4 + i] = row[j].as<float>();  // 注意列主序！
				}
			}
		}
		else {
			GLOG(Log::eError, "Invalid matrix format in YAML");
		}
	}
	catch (const YAML::Exception& e) {
		GLOG(Log::eError, "Failed to parse matrix from YAML: %s", e.what());
	}

	return matrix;
}

YAML::Node YAMLReader::VectorToYAML(const Vector& vector, bool useSequence/* = true*/) {
	YAML::Node vectorNode;

	if (useSequence) {
		vectorNode.push_back(vector.x);
		vectorNode.push_back(vector.y);
		vectorNode.push_back(vector.z);
	}
	else {
		vectorNode["type"] = "Vector";
		vectorNode["x"] = vector.x;
		vectorNode["y"] = vector.y;
		vectorNode["z"] = vector.z;
	}

	return vectorNode;
}

Vector YAMLReader::YAMLToVector(const YAML::Node& node) {
	Vector vector;

	try {
		if (!node)
			return vector;

		if (node.IsMap()) {
			// 旧格式：{ type: Vector, x: ..., y: ..., z: ... }
			if (node["type"] && node["type"].as<std::string>() != "Vector") {
				GLOG(Log::eError, "Expected type 'Vector', got '%s'", node["type"].as<std::string>().c_str());
				return vector;
			}
			if (node["x"] && node["x"].IsScalar()) vector.x = node["x"].as<float>();
			if (node["y"] && node["y"].IsScalar()) vector.y = node["y"].as<float>();
			if (node["z"] && node["z"].IsScalar()) vector.z = node["z"].as<float>();
		}
		else if (node.IsSequence() && node.size() == 3) {
			// 新格式：[x, y, z]
			vector.x = node[0].as<float>();
			vector.y = node[1].as<float>();
			vector.z = node[2].as<float>();
		}
		else {
			GLOG(Log::eError, "Invalid vector format in YAML node");
		}
	}
	catch (const YAML::Exception& e) {
		GLOG(Log::eError, "Failed to parse vector from YAML: %s", e.what());
	}

	return vector;
}

// 显式实例化模板
template bool YAMLReader::ModifyYAMLValueByPath<int>(YAML::Node&, const std::vector<std::string>&, const int&);
template bool YAMLReader::ModifyYAMLValueByPath<float>(YAML::Node&, const std::vector<std::string>&, const float&);
template bool YAMLReader::ModifyYAMLValueByPath<double>(YAML::Node&, const std::vector<std::string>&, const double&);
template bool YAMLReader::ModifyYAMLValueByPath<bool>(YAML::Node&, const std::vector<std::string>&, const bool&);
template bool YAMLReader::ModifyYAMLValueByPath<std::string>(YAML::Node&, const std::vector<std::string>&, const std::string&);
template bool YAMLReader::ModifyYAMLValueByPath<YAML::Node>(YAML::Node&, const std::vector<std::string>&, const YAML::Node&);

template bool YAMLReader::AddYAMLValueByPath<int>(YAML::Node&, const std::vector<std::string>&, const int&);
template bool YAMLReader::AddYAMLValueByPath<float>(YAML::Node&, const std::vector<std::string>&, const float&);
template bool YAMLReader::AddYAMLValueByPath<double>(YAML::Node&, const std::vector<std::string>&, const double&);
template bool YAMLReader::AddYAMLValueByPath<bool>(YAML::Node&, const std::vector<std::string>&, const bool&);
template bool YAMLReader::AddYAMLValueByPath<std::string>(YAML::Node&, const std::vector<std::string>&, const std::string&);
template bool YAMLReader::AddYAMLValueByPath<YAML::Node>(YAML::Node&, const std::vector<std::string>&, const YAML::Node&);