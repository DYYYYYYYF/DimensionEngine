#pragma once

#include "Core/Identifier.hpp"

/**
 * 仅定义Engine中最基础的数据
 */
class IObject {
public:
	IObject() = default;
	virtual ~IObject() = default;

public:
	virtual void PreInitialize() = 0;
	virtual bool Initialize() = 0;
	virtual void PostInitialize() = 0;

protected:
	uint32_t UniqueID;
};