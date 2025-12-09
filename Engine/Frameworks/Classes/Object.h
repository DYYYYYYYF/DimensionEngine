#pragma once

#include "BaseObject.h"

class Object : public BaseObject {
public:
	Object() { UniqueID = Identifier::AcquireNewID(this); }
	virtual ~Object() { Identifier::ReleaseID(UniqueID); }

public:
	virtual void PreInitialize() override {};
	virtual bool Initialize() override { return true; };
	virtual void PostInitialize() override {};
	uint32_t GetUniqueID() const { return UniqueID; }

};