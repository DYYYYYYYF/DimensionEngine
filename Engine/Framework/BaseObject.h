#pragma once

#include "Framework/IObject.h"

// 类声明
#define DECLARE_CLASS_TYPE(ClassName)                        \
    static uint32_t StaticTypeID() {                             \
        static uint32_t ID = ComponentTypeCounter::Next();       \
        return ID;                                               \
    }                                                            \
    virtual uint32_t GetTypeID() const override { return StaticTypeID(); }

// 计数器
struct ComponentTypeCounter {
	static uint32_t Next() {
		static uint32_t Counter = 0;
		return Counter++;
	}
};

class ENGINE_API ABaseObject : public IObject {
public:
	ABaseObject() : IObject() {}
	~ABaseObject() = default;

public:
	virtual void PreInitialize() override {};
	virtual bool Initialize() override { return true; };
	virtual void PostInitialize() override {};

public:
	virtual uint32_t GetTypeID() const = 0;

};