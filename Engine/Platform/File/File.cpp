#include "File.hpp"
#include "Core/EngineLogger.hpp"

#include <string>
#include <filesystem>
#include <sys/stat.h>

#ifdef DPLATFORM_WINDOWS
#include <Windows.h>
#endif

File::File(const FString& fn) {
	FullPath = fn;

	StrReplaceChar(FullPath, '\\', '/');
	FullPath = StrNormalizePath(FullPath);

	int PrePathIndex = StrLastIndexOf(FullPath, '/');
	int SufPathIndex = StrLastIndexOf(FullPath, '.');

	if (PrePathIndex == -1) {
		PrePath = "";
		FileName = (SufPathIndex != -1)
			? StrSubStr(FullPath, 0, static_cast<size_t>(SufPathIndex))
			: FullPath;
	}
	else {
		PrePath = StrSubStr(FullPath, 0, static_cast<size_t>(PrePathIndex + 1));
		FileName = (SufPathIndex != -1 && SufPathIndex > PrePathIndex)
			? StrSubStr(FullPath,
				static_cast<size_t>(PrePathIndex + 1),
				static_cast<size_t>(SufPathIndex - PrePathIndex - 1))
			: StrSubStr(FullPath, static_cast<size_t>(PrePathIndex + 1));
	}

	FileType = (SufPathIndex != -1)
		? StrSubStr(FullPath, static_cast<size_t>(SufPathIndex))
		: FString("");
}

bool File::IsExist() const {
#ifdef _MSC_VER
	struct _stat buffer;
	return _stat(FullPath.CStr(), &buffer) == 0;
#else
	struct stat buffer;
	return ::stat(FullPath.CStr(), &buffer) == 0;
#endif
}

size_t File::GetFileSize() const {
	std::ifstream f(FullPath.CStr(), std::ios::binary | std::ios::ate);
	if (!f.is_open()) {
		GLOG(Log::eError, "File::GetFileSize: cannot open '%s'", FullPath.CStr());
		return 0;
	}
	return static_cast<size_t>(f.tellg());
}

FString File::ReadText() const {
	std::ifstream f(FullPath.CStr(), std::ios::binary);
	if (!f.is_open()) {
		GLOG(Log::eError, "File::ReadText: cannot open '%s'", FullPath.CStr());
		return "";
	}

	std::ostringstream buf;
	buf << f.rdbuf();
	return StrFromStd(buf.str());
}

std::vector<unsigned char> File::ReadBytes() const {
	std::ifstream f(FullPath.CStr(), std::ios::binary);
	if (!f.is_open()) {
		GLOG(Log::eError, "File::ReadBytes: cannot open '%s'", FullPath.CStr());
		return {};
	}

	f.seekg(0, std::ios::end);
	size_t size = static_cast<size_t>(f.tellg());
	f.seekg(0, std::ios::beg);

	std::vector<unsigned char> buffer(size);
	f.read(reinterpret_cast<char*>(buffer.data()), size);

	if (static_cast<size_t>(f.gcount()) != size) {
		GLOG(Log::eError, "File::ReadBytes: expected %zu bytes, got %zu from '%s'",
			size, static_cast<size_t>(f.gcount()), FullPath.CStr());
		return {};
	}

	return buffer;
}

bool File::WriteText(const FString& text, std::ios::openmode mode) {
	std::ofstream f(FullPath.CStr(), mode);
	if (!f.is_open()) {
		GLOG(Log::eError, "File::WriteText: cannot open '%s'", FullPath.CStr());
		return false;
	}
	f << text.CStr();
	return f.good();
}

bool File::WriteBytes(const char* source, size_t size, std::ios::openmode mode) {
	if (!source) {
		GLOG(Log::eError, "File::WriteBytes: null source pointer.");
		return false;
	}
	std::ofstream f(FullPath.CStr(), mode | std::ios::binary);
	if (!f.is_open()) {
		GLOG(Log::eError, "File::WriteBytes: cannot open '%s'", FullPath.CStr());
		return false;
	}
	f.write(source, static_cast<std::streamsize>(size));
	return f.good();
}

std::vector<FString> File::ReadLines() const {
	std::ifstream f(FullPath.CStr());
	if (!f.is_open()) {
		GLOG(Log::eError, "File::ReadLines: cannot open '%s'", FullPath.CStr());
		return {};
	}

	std::vector<FString> lines;
	std::string raw;
	while (std::getline(f, raw)) {
		FString line = StrFromStd(raw);
		StrStripCarriageReturn(line);
		lines.push_back(std::move(line));
	}
	return lines;
}

bool File::ReadLineByLine(
	const std::function<bool(size_t, const FString&)>& callback) const {

	if (!callback) {
		GLOG(Log::eError, "File::ReadLineByLine: null callback.");
		return false;
	}

	std::ifstream f(FullPath.CStr());
	if (!f.is_open()) {
		GLOG(Log::eError, "File::ReadLineByLine: cannot open '%s'", FullPath.CStr());
		return false;
	}

	std::string raw;
	size_t line_count = 1;
	while (std::getline(f, raw)) {
		FString line = StrFromStd(raw);
		StrStripCarriageReturn(line);
		if (!callback(line_count++, line)) break;
	}
	return true;
}

bool File::ReadLine(size_t line_index, FString& out_line) const {
	std::ifstream f(FullPath.CStr());
	if (!f.is_open()) {
		GLOG(Log::eError, "File::ReadLine: cannot open '%s'", FullPath.CStr());
		return false;
	}

	std::string raw;
	size_t current = 0;
	while (std::getline(f, raw)) {
		if (current == line_index) {
			out_line = StrFromStd(raw);
			StrStripCarriageReturn(out_line);
			return true;
		}
		++current;
	}

	GLOG(Log::eWarn, "File::ReadLine: line index %zu out of range in '%s'",
		line_index, FullPath.CStr());
	return false;
}

bool File::AppendLine(const FString& line) {
	std::ofstream f(FullPath.CStr(), std::ios::app);
	if (!f.is_open()) {
		GLOG(Log::eError, "File::AppendLine: cannot open '%s'", FullPath.CStr());
		return false;
	}
	f << line.CStr() << '\n';
	return f.good();
}

bool File::WriteLines(const std::vector<FString>& lines, std::ios::openmode mode) {
	std::ofstream f(FullPath.CStr(), mode);
	if (!f.is_open()) {
		GLOG(Log::eError, "File::WriteLines: cannot open '%s'", FullPath.CStr());
		return false;
	}
	for (const auto& line : lines) {
		f << line.CStr() << '\n';
		if (!f.good()) {
			GLOG(Log::eError, "File::WriteLines: write failed for '%s'", FullPath.CStr());
			return false;
		}
	}
	return true;
}

bool File::Open(eFileMode mode, bool binary) {
	if (m_stream.is_open()) {
		GLOG(Log::eWarn, "File::Open: '%s' is already open.", FullPath.CStr());
		return false;
	}

	m_mode = mode;
	m_binary = binary;

	std::ios::openmode flags = {};
	if (static_cast<int>(mode) & static_cast<int>(eFileMode::Read))  flags |= std::ios::in;
	if (static_cast<int>(mode) & static_cast<int>(eFileMode::Write)) flags |= std::ios::out;
	if (binary) flags |= std::ios::binary;

	m_stream.open(FullPath.CStr(), flags);

	if (!m_stream.is_open()) {
		GLOG(Log::eError, "File::Open: cannot open '%s'", FullPath.CStr());
		return false;
	}
	return true;
}

void File::Close() {
	if (m_stream.is_open()) m_stream.close();
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

	if (out_bytes_read) *out_bytes_read = read;

	if (read != data_size) {
		GLOG(Log::eError, "File::ReadBuffer: expected %zu bytes, got %zu from '%s'",
			data_size, read, FullPath.CStr());
		return false;
	}
	return true;
}

bool File::ReadLine(FString& out_line) {
	if (!m_stream.is_open()) {
		GLOG(Log::eError, "File::ReadLine: file not open.");
		return false;
	}

	std::string raw;
	if (!std::getline(m_stream, raw)) return false;

	out_line = StrFromStd(raw);
	StrStripCarriageReturn(out_line);
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

	if (out_bytes_written) *out_bytes_written = static_cast<size_t>(after - before);

	if (!m_stream.good()) {
		GLOG(Log::eError, "File::WriteBuffer: write failed for '%s'", FullPath.CStr());
		return false;
	}
	return true;
}

bool File::WriteLine(const FString& line) {
	if (!m_stream.is_open()) {
		GLOG(Log::eError, "File::WriteLine: file not open.");
		return false;
	}
	m_stream << line.CStr() << '\n';
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


// ─── 字符串工具实现（当前调用 std，未来可替换）──────────────────────────────

int File::StrLastIndexOf(const FString& str, char c) {
	const char* s = str.CStr();
	for (int i = static_cast<int>(str.Length()) - 1; i >= 0; --i) {
		if (s[i] == c) return i;
	}
	return -1;
}

int File::StrIndexOf(const FString& str, char c, size_t start_pos) {
	const char* s = str.CStr();
	for (size_t i = start_pos; i < str.Length(); ++i) {
		if (s[i] == c) return static_cast<int>(i);
	}
	return -1;
}

FString File::StrSubStr(const FString& str, size_t start, size_t length) {
	std::string s(str.CStr());
	if (start >= s.size()) return FString("");
	if (length == static_cast<size_t>(-1)) return FString(s.substr(start).c_str());
	return FString(s.substr(start, length).c_str());
}

void File::StrReplaceChar(FString& str, char old_char, char new_char) {
	for (size_t i = 0; i < str.Length(); ++i) {
		if (str[i] == old_char) str[i] = new_char;
	}
}

void File::StrStripCarriageReturn(FString& str) {
	if (str.Length() > 0 && str[str.Length() - 1] == '\r') {
		str = StrSubStr(str, 0, str.Length() - 1);
	}
}

FString File::StrNormalizePath(const FString& path) {
	std::filesystem::path p(path.CStr());

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

	return FString(p.lexically_normal().generic_string().c_str());
}

FString File::StrFromStd(const std::string& str) {
	return FString(str.c_str());
}

std::string File::StrToStd(const FString& str) {
	return std::string(str.CStr());
}
