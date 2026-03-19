#include "FileWatcher.h"
#include "iostream"
#include "Platform/File/File.hpp"
#include "Core/Event.hpp"
#include "Core/DMemory.hpp"
#include <filesystem>

#ifdef DPLATFORM_WINDOWS
#include "tchar.h"
#include <sys/stat.h>
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
		GLOG(Log::eDebug, "Add file %s in watcher list.", FilePath.c_str());
	}
}

void FileWatcher::AddWatchFile(const std::string& file) {
	WatchableFile* Instance = NewObject<WatchableFile>(file);
	if (!Instance) {
		GLOG(Log::eError, "Failed to create WatchableFile instance for file: %s", file.c_str());
		return;
	}

	if (!Instance->IsExist()) {
		GLOG(Log::eError, "File does not exist: %s", file.c_str());
		return;
	}

	WatchedFiles.Push(Instance);
}

void FileWatcher::Update()
{
	for (WatchableFile* ModifiedFile : WatchedFiles) {
		if (!ModifiedFile) continue;

		bool modified = ModifiedFile->CheckFileModification();
		if (modified)
		{
			ModifiedFile->UpdateLastModInfo();
			std::string Filename = ModifiedFile->GetFilename();
			if (ModifiedFile->GetFileType().compare(".hlsl") == 0) {
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
#ifdef _MSC_VER
	struct _stat fileStatus;
	if (_stat(name.c_str(), &fileStatus) == -1)
	{
		return false;
	}
#else
	struct stat fileStatus;
	if (::stat(name.c_str(), &fileStatus) == -1)
	{
		return false;
	}
#endif

	fi->mtime = from_time_t(fileStatus.st_mtime);
	fi->size = fileStatus.st_size;

	return true;
}
