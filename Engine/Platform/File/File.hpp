#pragma once

#include "Defines.hpp"
#include "Containers/FString.hpp"

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

    File(const File&) = delete;
    File& operator=(const File&) = delete;

    DAPI File(const FString& filename);

    // ─── 路径信息 ──────────────────────────────────────────────────────────────
    FString GetFilename() const { return FileName; }
    FString GetFullPath() const { return FullPath; }
    FString GetPrePath()  const { return PrePath; }
    FString GetFileType() const { return FileType; }

    DAPI bool   IsExist()     const;
    DAPI size_t GetFileSize() const;

    // ─── 整体读写 ──────────────────────────────────────────────────────────────

    DAPI FString                    ReadText()  const;
    DAPI std::vector<unsigned char> ReadBytes() const;

    DAPI bool WriteText(const FString& text,
        std::ios::openmode mode = std::ios::out);

    DAPI bool WriteBytes(const char* source, size_t size,
        std::ios::openmode mode = std::ios::out | std::ios::binary);

    // ─── 按行读写 ──────────────────────────────────────────────────────────────

    DAPI std::vector<FString> ReadLines() const;

    DAPI bool ReadLineByLine(
        const std::function<bool(size_t line_index, const FString& line)>& callback) const;

    DAPI bool ReadLine(size_t line_index, FString& out_line) const;

    DAPI bool AppendLine(const FString& line);

    DAPI bool WriteLines(const std::vector<FString>& lines,
        std::ios::openmode mode = std::ios::out);

    // ─── 流式读写 ─────────────────────────────────────────────────────────────

    DAPI bool Open(eFileMode mode, bool binary = false);
    DAPI void Close();
    DAPI bool IsOpen() const;

    template<typename T>
    bool Read(T* out_value) {
        return ReadBuffer(out_value, sizeof(T));
    }

    DAPI bool ReadBuffer(void* out_data, size_t data_size,
        size_t* out_bytes_read = nullptr);

    DAPI bool ReadLine(FString& out_line);

    template<typename T>
    bool Write(const T* value) {
        return WriteBuffer(value, sizeof(T));
    }

    DAPI bool WriteBuffer(const void* data, size_t data_size,
        size_t* out_bytes_written = nullptr);

    DAPI bool WriteLine(const FString& line);

    DAPI size_t GetPosition();

    DAPI bool Seek(std::streamoff offset, std::ios::seekdir dir = std::ios::beg);

protected:
    // ─── 字符串工具（当前调用 std，未来可替换为自定义实现）──────────────────

    /**
     * @brief 查找字符最后一次出现的位置，未找到返回 -1。
     */
    static int          StrLastIndexOf(const FString& str, char c);

    /**
     * @brief 查找字符第一次出现的位置，未找到返回 -1。
     */
    static int          StrIndexOf(const FString& str, char c, size_t start_pos = 0);

    /**
     * @brief 截取子串，length = -1 表示到末尾。
     */
    static FString      StrSubStr(const FString& str, size_t start,
        size_t length = static_cast<size_t>(-1));

    /**
     * @brief 将字符串中所有 old_char 替换为 new_char（原地修改）。
     */
    static void         StrReplaceChar(FString& str, char old_char, char new_char);

    /**
     * @brief 去掉字符串末尾的 \r（处理 Windows CRLF）。
     */
    static void         StrStripCarriageReturn(FString& str);

    /**
     * @brief 将相对路径规范化为绝对路径，解析 ../ ./。
     */
    static FString      StrNormalizePath(const FString& path);

    /**
     * @brief std::string → FString（对接 std::getline 等接口用）。
     */
    static FString      StrFromStd(const std::string& str);

    /**
     * @brief FString → std::string（对接必须传 std::string 的接口用）。
     */
    static std::string  StrToStd(const FString& str);

protected:
    FString FullPath;
    FString FileName;
    FString PrePath;
    FString FileType;

    std::fstream m_stream;
    eFileMode    m_mode = eFileMode::Read;
    bool         m_binary = false;
};