#pragma once

#include "Framework/IObject.h"

class BaseObject : public IObject {
public:
	ENGINE_API BaseObject() : IObject() {}
	ENGINE_API ~BaseObject() = default;

public:
	ENGINE_API virtual void PreInitialize() override {};
	ENGINE_API virtual bool Initialize() override { return true; };
	ENGINE_API virtual void PostInitialize() override {};

	ENGINE_API uint32_t GetUniqueID() const { return UniqueID_; }

};