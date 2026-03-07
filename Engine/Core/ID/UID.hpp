#pragma once

#include "UIDManager.hpp"

class ENGINE_API UUID
{
public:

	using IDType = uint64_t;

	UUID()
	{
		id = UIDManager::Get().Allocate();
	}

	~UUID()
	{
		Release();
	}

	UUID(const UUID&) = delete;
	UUID& operator=(const UUID&) = delete;

	UUID(UUID&& other) noexcept
	{
		id = other.id;
		other.id = 0;
	}

	UUID& operator=(UUID&& other) noexcept
	{
		if (this != &other)
		{
			Release();
			id = other.id;
			other.id = 0;
		}

		return *this;
	}

public:

	void Register(void* object)
	{
		UIDManager::Get().Register(id, object);
	}

	void Release()
	{
		if (id != 0)
		{
			UIDManager::Get().Unregister(id);
			id = 0;
		}
	}

	IDType Get() const
	{
		return id;
	}

	operator IDType() const
	{
		return id;
	}

private:

	IDType id = 0;
};