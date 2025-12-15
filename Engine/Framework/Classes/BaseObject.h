#pragma once

#include "Framework/IObject.h"

class BaseObject : public IObject {
public:
	BaseObject() : IObject() {}
	~BaseObject() = default;

public:
	virtual void PreInitialize() override {};
	virtual bool Initialize() override { return true; };
	virtual void PostInitialize() override {};

	uint32_t GetUniqueID() const { return UniqueID_; }

};