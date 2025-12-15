#pragma once

#include "Framework/IObject.h"

class BaseObject : public IObject {
public:
	BaseObject() { UniqueID = Identifier::AcquireNewID(this); }
	virtual ~BaseObject() { Identifier::ReleaseID(UniqueID); }

public:
	virtual void PreInitialize() override {};
	virtual bool Initialize() override { return true; };
	virtual void PostInitialize() override {};
	uint32_t GetUniqueID() const { return UniqueID; }

};