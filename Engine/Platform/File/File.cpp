#include "File.hpp"
#include "Core/EngineLogger.hpp"

#include <algorithm>
#include <sys/stat.h>

#ifdef DPLATFORM_WINDOWS
#include <Windows.h>
#endif

//File::File(const FString& filename) {
//
//}

File::File(const std::string& fn) {
	FullPath = fn;

	// 统一路径分隔符
	std::replace(FullPath.begin(), FullPath.end(), '\\', '/');

	// 规范化 ../，解析为绝对路径
	NormalizePath();

	// 解析目录、文件名、扩展名
	size_t PrePathIndex = FullPath.find_last_of('/');
	size_t SufPathIndex = FullPath.find_last_of('.');

	if (PrePathIndex == std::string::npos) {
		PrePath = "";
		FileName = (SufPathIndex != std::string::npos)
			? FullPath.substr(0, SufPathIndex)
			: FullPath;
	}
	else {
		PrePath = FullPath.substr(0, PrePathIndex + 1);
		FileName = (SufPathIndex != std::string::npos && SufPathIndex > PrePathIndex)
			? FullPath.substr(PrePathIndex + 1, SufPathIndex - PrePathIndex - 1)
			: FullPath.substr(PrePathIndex + 1);
	}

	FileType = (SufPathIndex != std::string::npos)
		? FullPath.substr(SufPathIndex)
		: "";
}

void File::NormalizePath() {
	std::filesystem::path p(FullPath);

	if (p.is_relative()) {
#ifdef DPLATFORM_WINDOWS
		wchar_t exePathBuf[MAX_PATH] = {};
		GetModuleFileNameW(nullptr, exePathBuf, MAX_PATH);
		std::filesystem::path exeDir = std::filesystem::path(exePathBuf).parent_path();
#else
		std::filesystem::path exeDir = std::filesystem::current_path();
#endif
		p = exeDir / p;
	}

	// lexically_normal 解析 ../ ./ 等，不要求路径实际存在
	FullPath = p.lexically_normal().generic_string();
}

bool File::IsExist() const {
#ifdef _MSC_VER
	struct _stat buffer;
	return _stat(FullPath.c_str(), &buffer) == 0;
#else
	struct stat buffer;
	return ::stat(FullPath.c_str(), &buffer) == 0;
#endif
}

size_t File::GetFileSize() const {
	std::ifstream f(FullPath, std::ios::binary | std::ios::ate);
	if (!f.is_open()) {
		GLOG(Log::eError, "File::GetFileSize: cannot open '%s'", FullPath.c_str());
		return 0;
	}
	return static_cast<size_t>(f.tellg());
}

std::string File::ReadText() const {
	std::ifstream f(FullPath, std::ios::binary);
	if (!f.is_open()) {
		GLOG(Log::eError, "File::ReadText: cannot open '%s'", FullPath.c_str());
		return "";
	}

	std::ostringstream buf;
	buf << f.rdbuf();
	return buf.str();
}

std::vector<unsigned char> File::ReadBytes() const {
	std::ifstream f(FullPath, std::ios::binary);
	if (!f.is_open()) {
		GLOG(Log::eError, "File::ReadBytes: cannot open '%s'", FullPath.c_str());
		return {};
	}

	f.seekg(0, std::ios::end);
	size_t size = static_cast<size_t>(f.tellg());
	f.seekg(0, std::ios::beg);

	std::vector<unsigned char> buffer(size);
	f.read(reinterpret_cast<char*>(buffer.data()), size);

	if (static_cast<size_t>(f.gcount()) != size) {
		GLOG(Log::eError, "File::ReadBytes: expected %zu bytes, got %zu from '%s'",
			size, static_cast<size_t>(f.gcount()), FullPath.c_str());
		return {};
	}

	return buffer;
}

bool File::WriteText(const std::string& text, std::ios::openmode mode) {
	std::ofstream f(FullPath, mode);
	if (!f.is_open()) {
		GLOG(Log::eError, "File::WriteText: cannot open '%s'", FullPath.c_str());
		return false;
	}

	f << text;
	return f.good();
}

bool File::WriteBytes(const char* source, size_t size, std::ios::openmode mode) {
	if (!source) {
		GLOG(Log::eError, "File::WriteBytes: null source pointer.");
		return false;
	}

	std::ofstream f(FullPath, mode | std::ios::binary);
	if (!f.is_open()) {
		GLOG(Log::eError, "File::WriteBytes: cannot open '%s'", FullPath.c_str());
		return false;
	}

	f.write(source, static_cast<std::streamsize>(size));
	return f.good();
}

std::vector<std::string> File::ReadLines() const {
	std::ifstream f(FullPath);
	if (!f.is_open()) {
		GLOG(Log::eError, "File::ReadLines: cannot open '%s'", FullPath.c_str());
		return {};
	}

	std::vector<std::string> lines;
	std::string line;
	while (std::getline(f, line)) {
		// 去掉 Windows 换行符残留的 \r
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		lines.push_back(std::move(line));
	}

	return lines;
}

bool File::ReadLineByLine(const std::function<bool(size_t line_index, const std::string& line)>& callback) const {
	if (!callback) {
		GLOG(Log::eError, "File::ReadLineByLine: null callback.");
		return false;
	}

	std::ifstream f(FullPath);
	if (!f.is_open()) {
		GLOG(Log::eError, "File::ReadLineByLine: cannot open '%s'", FullPath.c_str());
		return false;
	}

	std::string line;
	size_t line_count = 1;
	while (std::getline(f, line)) {
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		// callback 返回 false 时提前停止，但仍返回 true 表示文件读取正常
		if (!callback(line_count++, line)) {
			break;
		}
	}

	return true;
}

bool File::ReadLine(size_t line_index, std::string& out_line) const {
	std::ifstream f(FullPath);
	if (!f.is_open()) {
		GLOG(Log::eError, "File::ReadLine: cannot open '%s'", FullPath.c_str());
		return false;
	}

	std::string line;
	size_t current = 0;
	while (std::getline(f, line)) {
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		if (current == line_index) {
			out_line = std::move(line);
			return true;
		}
		++current;
	}

	GLOG(Log::eWarn, "File::ReadLine: line index %zu out of range in '%s'",
		line_index, FullPath.c_str());
	return false;
}

bool File::AppendLine(const std::string& line) {
	std::ofstream f(FullPath, std::ios::app);
	if (!f.is_open()) {
		GLOG(Log::eError, "File::AppendLine: cannot open '%s'", FullPath.c_str());
		return false;
	}

	f << line << '\n';
	return f.good();
}

bool File::WriteLines(const std::vector<std::string>& lines, std::ios::openmode mode) {
	std::ofstream f(FullPath, mode);
	if (!f.is_open()) {
		GLOG(Log::eError, "File::WriteLines: cannot open '%s'", FullPath.c_str());
		return false;
	}

	for (const auto& line : lines) {
		f << line << '\n';
		if (!f.good()) {
			GLOG(Log::eError, "File::WriteLines: write failed for '%s'", FullPath.c_str());
			return false;
		}
	}

	return true;
}

bool File::Open(eFileMode mode, bool binary) {
	if (m_stream.is_open()) {
		GLOG(Log::eWarn, "File::Open: '%s' is already open.", FullPath.c_str());
		return false;
	}

	m_mode = mode;
	m_binary = binary;

	std::ios::openmode flags = {};

	if (static_cast<int>(mode) & static_cast<int>(eFileMode::Read))  flags |= std::ios::in;
	if (static_cast<int>(mode) & static_cast<int>(eFileMode::Write)) flags |= std::ios::out;
	if (binary) flags |= std::ios::binary;

	m_stream.open(FullPath, flags);

	if (!m_stream.is_open()) {
		GLOG(Log::eError, "File::Open: cannot open '%s'", FullPath.c_str());
		return false;
	}

	return true;
}

void File::Close() {
	if (m_stream.is_open()) {
		m_stream.close();
	}
}

bool File::IsOpen() const {
	return m_stream.is_open();
}

bool File::ReadBuffer(void* out_data, size_t data_size, size_t* out_bytes_read) {
	if (!m_stream.is_open()) {
		GLOG(Log::eError, "File::ReadBuffer: file not open.");
		return false;
	}
	if (!out_data || data_size == 0) {
		GLOG(Log::eError, "File::ReadBuffer: invalid arguments.");
		return false;
	}

	m_stream.read(reinterpret_cast<char*>(out_data), data_size);
	size_t read = static_cast<size_t>(m_stream.gcount());

	if (out_bytes_read) {
		*out_bytes_read = read;
	}

	if (read != data_size) {
		GLOG(Log::eError, "File::ReadBuffer: expected %zu bytes, got %zu from '%s'",
			data_size, read, FullPath.c_str());
		return false;
	}

	return true;
}

bool File::ReadLine(std::string& out_line) {
	if (!m_stream.is_open()) {
		GLOG(Log::eError, "File::ReadLine: file not open.");
		return false;
	}

	if (!std::getline(m_stream, out_line)) {
		return false;  // EOF 或错误
	}

	// 去掉 Windows \r\n 的残留 \r
	if (!out_line.empty() && out_line.back() == '\r') {
		out_line.pop_back();
	}

	return true;
}

bool File::WriteBuffer(const void* data, size_t data_size, size_t* out_bytes_written) {
	if (!m_stream.is_open()) {
		GLOG(Log::eError, "File::WriteBuffer: file not open.");
		return false;
	}
	if (!data || data_size == 0) {
		GLOG(Log::eError, "File::WriteBuffer: invalid arguments.");
		return false;
	}

	std::streampos before = m_stream.tellp();
	m_stream.write(reinterpret_cast<const char*>(data), data_size);
	std::streampos after = m_stream.tellp();

	size_t written = static_cast<size_t>(after - before);
	if (out_bytes_written) {
		*out_bytes_written = written;
	}

	if (!m_stream.good()) {
		GLOG(Log::eError, "File::WriteBuffer: write failed for '%s'", FullPath.c_str());
		return false;
	}

	return true;
}

bool File::WriteLine(const FString& line) {
	if (!m_stream.is_open()) {
		GLOG(Log::eError, "File::WriteLine: file not open.");
		return false;
	}

	m_stream << line << '\n';
	return m_stream.good();
}

size_t File::GetPosition() {
	if (!m_stream.is_open()) return 0;
	return static_cast<size_t>(m_stream.tellg());
}

bool File::Seek(std::streamoff offset, std::ios::seekdir dir) {
	if (!m_stream.is_open()) {
		GLOG(Log::eError, "File::Seek: file not open.");
		return false;
	}
	m_stream.seekg(offset, dir);
	m_stream.seekp(offset, dir);
	return m_stream.good();
}