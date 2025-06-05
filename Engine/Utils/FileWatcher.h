#include <ctime>
#include <chrono>
#include <string>
#include "Containers/TArray.hpp"
#include "Platform/File.hpp"

typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<long long, std::micro>> Time;

template <typename FromDuration>
inline Time time_cast(std::chrono::time_point<std::chrono::system_clock, FromDuration> const& tp)
{
	return std::chrono::time_point_cast<std::chrono::duration<long long, std::micro>, std::chrono::system_clock> (tp);
}

inline Time now()
{
	return time_cast(std::chrono::system_clock::now());
}

inline Time from_time_t(time_t t_time)
{
	return time_cast(std::chrono::system_clock::from_time_t(t_time));
}

struct WatchableFileInfo {
	Time mtime;
	off_t size;
};

class WatchableFile : public File
{
public:
	WatchableFile() : File(){
		LastFileInfo.mtime = time_cast(std::chrono::system_clock::now());
		LastFileInfo.size = 0;
		UpdateLastModInfo();
	}

	WatchableFile(const std::string& file) :File(file) {
		LastFileInfo.mtime = time_cast(std::chrono::system_clock::now());
		LastFileInfo.size = 0;
		UpdateLastModInfo();
	}

public:
	bool CheckFileModification() const;
	void UpdateLastModInfo();
	bool GetFileInfo(WatchableFileInfo* fi, const std::string& name) const;

public:
	WatchableFileInfo LastFileInfo;
};

class FileWatcher {

public:
	FileWatcher();
	~FileWatcher();

public:
	void AddWatchFolder(const std::string& file, bool recursion = false);
	void AddWatchFile(const std::string& file);
	void Update();

private:
	TArray<WatchableFile> WatchedFiles;
};

