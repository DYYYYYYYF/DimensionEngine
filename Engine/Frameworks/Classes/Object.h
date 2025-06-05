#pragma once

#include "Frameworks/Classes/BaseObject.h"

class Object : public BaseObject {
public:
	CORE_API Object() : BaseObject() {}
	CORE_API virtual ~Object() {}

public:
	CORE_API virtual void PreInitialize() override {};
	CORE_API virtual bool Initialize() override { return true; };
	CORE_API virtual void PostInitialize() override {};
};