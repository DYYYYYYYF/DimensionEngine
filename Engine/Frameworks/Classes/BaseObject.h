#pragma once

#include "Core/Identifier.hpp"

/**
 * 仅定义Engine中最基础的数据
 */
class BaseObject {
public:
	CORE_API BaseObject() {
		UniqueID = Identifier::AcquireNewID(this);
	}

	CORE_API virtual ~BaseObject() {
		Identifier::ReleaseID(UniqueID);
	}

	CORE_API virtual void PreInitialize() = 0;
	CORE_API virtual bool Initialize() = 0;
	CORE_API virtual void PostInitialize() = 0;

public:
	CORE_API uint32_t GetUniqueID() const { return UniqueID; }

protected:
	uint32_t UniqueID;
};