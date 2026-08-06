#pragma once

#include "Core/Identifier.hpp"

// ------------------------------- IObject ------------------------------------
class ENGINE_API IObject {
public:
	virtual void PreInitialize() = 0;
	virtual bool Initialize() = 0;
	virtual void PostInitialize() = 0;

protected:
	uint32_t UniqueID_;
};

// ------------------------------- UObject ------------------------------------
template<typename T>
class TRequireClassType
{
private:
	template<typename U>
	static auto TestHasGetTypeID(int) -> decltype(std::declval<const U>().GetTypeID(), std::true_type{});

	template<typename U>
	static std::false_type TestHasGetTypeID(...);

public:
	static constexpr bool HasGetTypeID = decltype(TestHasGetTypeID<T>(0))::value;
};

// 辅助宏：同时继承 + 输出带类名的static_assert
#define REQUIRE_CLASS_TYPE(ClassName)  \
	public TRequireClassType<ClassName> { \
	static_assert(TRequireClassType<ClassName>::HasGetTypeID, \
	"\n**************** ERROR ****************\n" \
	"Class " #ClassName " forgot add DECLARE_CLASS_TYPE(" #ClassName ") macro!\n" \
	"Please add DECLARE_CLASS_TYPE(" #ClassName ") inside class body.\n" \
	"****************************************\n"); \
	}

#define DECLARE_CLASS_TYPE(ClassName)  public:                   \
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

class ENGINE_API UObject : public IObject {
public:
	UObject() { UniqueID_  = Identifier::AcquireNewID(this); }
	virtual ~UObject() { Identifier::ReleaseID(UniqueID_); }

public:
	virtual void PreInitialize() override {};
	virtual bool Initialize() override { return true; };
	virtual void PostInitialize() override {};

	uint32_t GetUniqueID() const { return UniqueID_; }

public:
	virtual uint32_t GetTypeID() const = 0;

};