#include "FileWatcher.h"
#include "iostream"
#include "Platform/File.hpp"
#include "Core/Event.hpp"
#include "Core/DMemory.hpp"
#include <filesystem>

#ifdef DPLATFORM_WINDOWS
#include "tchar.h"
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

FileWatcher::FileWatcher(){
}

FileWatcher::~FileWatcher()
{
}

void FileWatcher::AddWatchFolder(const std::string& path, bool recursion) {
	std::filesystem::path FolderPath = path;
	if (!std::filesystem::exists(FolderPath) || !std::filesystem::is_directory(FolderPath)) {
		return;
	}

	for (const auto& entry : std::filesystem::directory_iterator(FolderPath)) {
		std::string FilePath = path + entry.path().filename().string();
		AddWatchFile(FilePath);
		LOG_DEBUG("Add file %s in watcher list.", FilePath.c_str());
	}
}

void FileWatcher::AddWatchFile(const std::string& file) {
	WatchedFiles.Push(WatchableFile(file));
}

void FileWatcher::Update()
{
	for (auto& ModifiedFile : WatchedFiles) {
		bool modified = ModifiedFile.CheckFileModification();
		if (modified)
		{
			ModifiedFile.UpdateLastModInfo();
			std::string Filename = ModifiedFile.GetFilename();
			if (ModifiedFile.GetFileType().compare(".hlsl") == 0) {
				Filename = File(Filename).GetFilename();
			}

			SEventContext Context = {};
			const char* ShaderName = Filename.c_str();
			Memory::Copy(Context.data.c, ShaderName, strlen(ShaderName) + 1);
			EngineEvent::Fire(eEventCode::Reload_Shader_Module, nullptr, Context);
		}
	}
}

bool WatchableFile::CheckFileModification() const
{
	WatchableFileInfo fi;

	if (!GetFileInfo(&fi, FullPath))
	{
		return false;
	}

	bool modified = fi.mtime > LastFileInfo.mtime
		|| fi.size != LastFileInfo.size;


	return modified;
}

void WatchableFile::UpdateLastModInfo()
{
	WatchableFileInfo fi;

	if (GetFileInfo(&fi, FullPath))
	{
		LastFileInfo = fi;
	}
}

bool WatchableFile::GetFileInfo(WatchableFileInfo* fi, const std::string& name) const
{
#ifdef DPLATFORM_WINDOWS
	struct _stat fileStatus;
    if (_stat(name.c_str(), &fileStatus) == -1)
    {
        return false;
    }
#else
    struct stat fileStatus;
	if (stat(name.c_str(), &fileStatus) == -1)
	{
		return false;
	}
#endif

	fi->mtime = from_time_t(fileStatus.st_mtime);
	fi->size = fileStatus.st_size;

	return true;
}
