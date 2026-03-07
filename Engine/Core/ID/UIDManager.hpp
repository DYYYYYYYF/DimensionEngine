#pragma once

#include <unordered_map>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <chrono>
#include <thread>

class UIDManager
{
public:

	using IDType = uint64_t;

	static UIDManager& Get()
	{
		static UIDManager instance;
		return instance;
	}

public:

	IDType Allocate()
	{
		uint64_t timestamp = GetTimestamp();
		uint64_t process = GetProcessID();
		uint64_t seq = counter.fetch_add(1, std::memory_order_relaxed);

		return
			((timestamp & 0xFFFFFFFF) << 32) |
			((process & 0xFFFF) << 16) |
			(seq & 0xFFFF);
	}

	void Register(IDType id, void* object)
	{
		std::lock_guard<std::mutex> lock(mutex);
		objects[id] = object;
	}

	void Unregister(IDType id)
	{
		std::lock_guard<std::mutex> lock(mutex);
		objects.erase(id);
	}

	void* Find(IDType id)
	{
		std::lock_guard<std::mutex> lock(mutex);

		auto it = objects.find(id);
		if (it == objects.end())
			return nullptr;

		return it->second;
	}

private:

	uint32_t GetTimestamp()
	{
		auto now = std::chrono::system_clock::now();
		return (uint32_t)std::chrono::duration_cast<std::chrono::seconds>(
			now.time_since_epoch()).count();
	}

	uint16_t GetProcessID()
	{
		static uint16_t pid =
			(uint16_t)(std::hash<std::thread::id>{}(std::this_thread::get_id()) & 0xFFFF);

		return pid;
	}

private:

	std::unordered_map<IDType, void*> objects;

	std::mutex mutex;

	std::atomic<uint16_t> counter{ 1 };
};