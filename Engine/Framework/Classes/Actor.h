#pragma once

#include "Framework/Object.h"
#include "Containers/TMap.hpp"
#include "Containers/FString.hpp"
#include "Framework/Components/SceneComponent.h"
#include <typeinfo>
#include <typeindex>

class UWorld;

class ENGINE_API AActor : public UObject, public TRequireClassType<AActor> {
	DECLARE_CLASS_TYPE(AActor)

public:
	AActor(const FString& Name);
	virtual ~AActor() { Destroy(); }

public:
	virtual void BeginPlay();
	virtual void RegisterComponents();
	virtual void Tick(float DeltaTime);
	virtual void UnregisterComponents();
	virtual void Destroy();

public:
	void SetActorLocation(const Vector3& Loc) { RootComponent->SetLocation(Loc); }
	Vector3 GetActorLocation() const { return RootComponent->GetLocation(); }

	void SetActorRotation(const Vector3& Rot) { RootComponent->SetRotation(Rot); }
	Vector3 GetActorRotation() const { return RootComponent->GetQuaternion().ToEuler(); }

	void SetActorQuaternion(const Quaternion& Quat) { RootComponent->SetQuaternion(Quat); }
	Quaternion GetActorQuaternion() const { return RootComponent->GetQuaternion(); }

	void SetWorldScale(const Vector3& Sca) { RootComponent->SetScale(Sca); }
	Vector3 GetWorldScale() const { return RootComponent->GetScale(); }

	void SetWorld(UWorld* InWorld) { World = InWorld; }
	UWorld* GetWorld() const { return World; }

	Matrix4 GetLocalTransform() const;
	Matrix4 GetWorldTransform() const;

	bool AttachTo(AActor* Own);
	bool AddChild(AActor* Child);

	void SetRootComponent(USceneComponent* Root) { RootComponent = Root; }
	USceneComponent* GetRootComponent() const { return RootComponent; }

	void SetName(const FString& Name) { Name_ = Name; }
	FString GetName() const { return Name_; }

	bool IsEnableTick() const { return IsEnableTick_; }
	void SetEnableTick(bool bEnable) { IsEnableTick_ = bEnable; }

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
	USceneComponent* RootComponent;

	// 组件存储（按类型索引）
	TMap<uint32_t, UComponent*> ContainComponents;

	UWorld* World = nullptr;

	// 父对象
	AActor* ParentActor;
	TArray<AActor*> ChildrenActors;

	bool IsEnableTick_;
};