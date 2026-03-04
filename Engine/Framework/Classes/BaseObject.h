#pragma once

#include "Framework/IObject.h"

class ENGINE_API ABaseObject : public IObject {
public:
	ABaseObject() : IObject() {}
	~ABaseObject() = default;

public:
	virtual void PreInitialize() override {};
	virtual bool Initialize() override { return true; };
	virtual void PostInitialize() override {};

	uint32_t GetUniqueID() const { return UniqueID_; }

};