#pragma once

#include "Defines.hpp"
#include "Containers/FString.hpp"

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <functional>

enum class eFileMode {
    Read = 0x1,
	Write = 0x2,
	ReadWrite = 0x3
};

class File {
public:
    File() = default;
	File(File&&) = default;
	File& operator=(File&&) = default;
	virtual ~File() = default;

	// 明确禁止拷贝
	File(const File&) = delete;
	File& operator=(const File&) = delete;

	//DAPI File(const FString& filename);
	DAPI File(const std::string& filename);

    // ─── 路径信息 ──────────────────────────────────────────────────────────────
    std::string GetFilename() const { return FileName; }
    std::string GetFullPath() const { return FullPath; }
    std::string GetPrePath()  const { return PrePath; }
    std::string GetFileType() const { return FileType; }

    /**
     * @brief 检查文件是否存在。
     */
    DAPI bool IsExist() const;

    /**
     * @brief 获取文件大小（字节）。不会读取文件内容。
     * @return 文件大小，失败返回 0。
     */
    DAPI size_t GetFileSize() const;

    // ─── 整体读写 ──────────────────────────────────────────────────────────────

    /**
     * @brief 读取文件全部内容为文本字符串（保留原始换行符）。
     * @return 文件内容，失败返回空字符串。
     */
    DAPI std::string ReadText() const;

    /**
     * @brief 读取文件全部内容为二进制字节数组。
     *        内存由 vector 自动管理，失败返回空 vector。
     */
    DAPI std::vector<unsigned char> ReadBytes() const;

    /**
     * @brief 将文本写入文件（覆盖写）。
     * @param text 要写入的字符串。
     * @param mode 写入模式，默认覆盖写。可传 std::ios::app 实现追加。
     */
    DAPI bool WriteText(const std::string& text,
        std::ios::openmode mode = std::ios::out);

    /**
     * @brief 将二进制数据写入文件（覆盖写）。
     * @param source 源数据指针。
     * @param size   写入字节数。
     * @param mode   写入模式，默认覆盖写。
     */
    DAPI bool WriteBytes(const char* source, size_t size,
        std::ios::openmode mode = std::ios::out | std::ios::binary);

    // ─── 按行读写 ──────────────────────────────────────────────────────────────

    /**
     * @brief 读取文件所有行，每行作为一个字符串存入 vector。
     *        行尾换行符不包含在字符串内。
     * @return 所有行的字符串数组，失败返回空 vector。
     */
    DAPI std::vector<std::string> ReadLines() const;

    /**
     * @brief 逐行读取文件，每读到一行就调用一次 callback。
     *        适合大文件，避免一次性把所有行加载进内存。
     * @param callback 回调函数，参数为当前行内容。返回 false 可提前停止读取。
     * @return 成功打开并读取完毕返回 true（callback 返回 false 也算 true）。
     */
    DAPI bool ReadLineByLine(const std::function<bool(size_t line_index, const std::string& line)>& callback) const;

    /**
     * @brief 读取指定行（从 0 开始计数）。
     * @param line_index 行索引。
     * @param out_line   输出的行内容。
     * @return 找到该行返回 true，行号超出范围返回 false。
     */
    DAPI bool ReadLine(size_t line_index, std::string& out_line) const;

    /**
     * @brief 向文件末尾追加一行文本（自动在末尾加换行符）。
     * @param line 要追加的文本（不需要手动加 \n）。
     * @return 成功返回 true。
     */
    DAPI bool AppendLine(const std::string& line);

    /**
     * @brief 将多行文本写入文件（覆盖写，每行自动加换行符）。
     * @param lines 要写入的字符串数组。
     * @param mode  写入模式，默认覆盖写。
     * @return 成功返回 true。
     */
    DAPI bool WriteLines(const std::vector<std::string>& lines,
        std::ios::openmode mode = std::ios::out);

    // 流式读写（需要先 Open，用完后 Close）
    /**
     * @brief 打开文件，建立流式读写状态。
     * @param mode   读写模式。
     * @param binary 是否以二进制模式打开（读二进制文件时必须为 true）。
     * @return 成功返回 true。
     */
    DAPI bool Open(eFileMode mode, bool binary = false);

    /**
     * @brief 关闭已打开的文件流。
     */
    DAPI void Close();

    /**
     * @brief 文件流是否已打开。
     */
    DAPI bool IsOpen() const;

    /**
     * @brief 流式读取：将 sizeof(T) 字节读入 out_value。
     *        模板方法，直接传变量指针，无需手动写 sizeof。
     *
     * 示例：
     *   unsigned short version;
     *   f.Read(&version);       // 自动读取 sizeof(unsigned short) 字节
     *
     *   uint32_t nameLen;
     *   f.Read(&nameLen);
     *
     * @return 成功返回 true。
     */
    template<typename T>
    bool Read(T* out_value) {
        return ReadBuffer(out_value, sizeof(T));
    }

    /**
     * @brief 流式读取：读取指定字节数到 buffer。
     *        用于读取不定长数据（如字符串、数组）。
     *
     * 示例：
     *   char name[256];
     *   f.ReadBuffer(name, sizeof(char) * nameLen);
     *
     * @param out_data  输出 buffer，由调用方提供。
     * @param data_size 要读取的字节数。
     * @param out_bytes_read 实际读取的字节数（可为 nullptr）。
     * @return 成功返回 true。
     */
    DAPI bool ReadBuffer(void* out_data, size_t data_size,
        size_t* out_bytes_read = nullptr);

    /**
     * @brief 流式读取一行文本。
     * @param out_line 输出行内容（不含换行符）。
     * @return 成功读到一行返回 true，到达文件末尾返回 false。
     */
    DAPI bool ReadLine(std::string& out_line);

    /**
     * @brief 流式写入：将 sizeof(T) 字节从 value 写入文件。
     *
     * 示例：
     *   unsigned short version = 1;
     *   f.Write(&version);
     *
     * @return 成功返回 true。
     */
    template<typename T>
    bool Write(const T* value) {
        return WriteBuffer(value, sizeof(T));
    }

    /**
     * @brief 流式写入：写入指定字节数到文件。
     * @param data      源数据指针。
     * @param data_size 写入字节数。
     * @param out_bytes_written 实际写入的字节数（可为 nullptr）。
     * @return 成功返回 true。
     */
    DAPI bool WriteBuffer(const void* data, size_t data_size,
        size_t* out_bytes_written = nullptr);

    /**
     * @brief 流式写入一行文本（自动追加换行符）。
     * @param line 要写入的文本。
     * @return 成功返回 true。
     */
    DAPI bool WriteLine(const FString& line);

    /**
     * @brief 获取当前读写位置（字节偏移）。
     */
    DAPI size_t GetPosition();

    /**
     * @brief 移动读写位置。
     * @param offset 偏移量。
     * @param dir    方向：std::ios::beg / std::ios::cur / std::ios::end。
     * @return 成功返回 true。
     */
    DAPI bool Seek(std::streamoff offset, std::ios::seekdir dir = std::ios::beg);

protected:
    /**
     * @brief 将 FullPath 中的 ../ 规范化，解析为基于 exe 目录的绝对路径。
     */
    void NormalizePath();

protected:
    std::string FullPath;   // 规范化后的绝对路径
    std::string FileName;   // 不含扩展名的文件名
    std::string PrePath;    // 目录部分（含末尾 /）
    std::string FileType;   // 扩展名（含 .）

	// 流式读写状态
	std::fstream  m_stream;
	eFileMode     m_mode = eFileMode::Read;
	bool          m_binary = false;
};