#pragma once

#include "Framework/BaseObject.h"
#include "Containers/TMap.hpp"
#include "Containers/FString.hpp"
#include "Framework/Components/TransformComponent.h"
#include <typeinfo>
#include <typeindex>

class ENGINE_API AActor : public ABaseObject {
public:
	DECLARE_CLASS_TYPE(AActor)

public:
	AActor();
	AActor(const FString& Name);
	virtual ~AActor() { Destroy(); }

public:
	virtual void BeginPlay();
	virtual void Tick(float DeltaTime);
	virtual void Destroy();

public:
	void SetLocation(const Vector3& Loc) { LocalTransform->SetLocation(Loc); }
	Vector3 GetLocation() const { return LocalTransform->GetLocation(); }

	void SetQuaternion(const Quaternion& Quat) { LocalTransform->SetQuaternion(Quat); }
	Quaternion GetQuaternion() const { return LocalTransform->GetQuaternion(); }

	void SetScale(const Vector3& Sca) { LocalTransform->SetScale(Sca); }
	Vector3 GetScale() const { return LocalTransform->GetScale(); }

	void Rotate(const Quaternion& Quat) { LocalTransform->Rotate(Quat); }

	Matrix4 GetLocalTransform() const;
	Matrix4 GetWorldTransform() const;

	bool AttachTo(AActor* Own);
	bool AddChild(AActor* Child);

	UTransformComponent* GetTransformComponent() const { return LocalTransform; }

	void SetName(const FString& Name) { Name_ = Name; }
	FString GetName() const { return Name_; }

public:
	template<typename T, typename... Args>
	T* CreateComponent(Args&&... args) {
		static_assert(std::is_base_of<UComponent, T>::value,
			"T must derive from Component");

		T* Comp = NewObject<T>(std::forward<Args>(args)...);
		AddComponent(Comp);

		return Comp;
	}

	template<typename T>
	void AddComponent(T* Comp) {
		static_assert(std::is_base_of<UComponent, T>::value,
			"T must derive from Component");

		ContainComponents[T::StaticTypeID()] = Comp;

		UComponent* BaseComp = static_cast<UComponent*>(Comp);
		BaseComp->SetOwner(this);
		BaseComp->OnAttach();
	}

	template<typename T>
	T* GetComponent() const {
		static_assert(std::is_base_of<UComponent, T>::value,
			"T must derive from Component");

		uint32_t ID = T::StaticTypeID();
		if (!ContainComponents.Find(ID)) {
			return nullptr;
		}

		auto Pair = ContainComponents.Get(ID);
		return static_cast<T*>(Pair.Value);
	}

	template<typename T>
	void RemoveComponent() {
		static_assert(std::is_base_of<UComponent, T>::value,
			"T must derive from Component");

		uint32_t ID = T::StaticTypeID();
		if (ContainComponents.Find(ID)) {
			ContainComponents.Remove(ID);
		}
	}

	template<typename T>
	bool HasComponent() const {
		static_assert(std::is_base_of<UComponent, T>::value,
			"T must derive from Component");

		uint32_t ID = T::StaticTypeID();
		return ContainComponents.Find(ID) != nullptr;
	}

protected:
	FString Name_;

	// Actor Transform
	UTransformComponent* LocalTransform;

	// 组件存储（按类型索引）
	TMap<uint32_t, UComponent*> ContainComponents;

	// 父对象
	AActor* ParentActor;
	TArray<AActor*> ChildrenActors;
};