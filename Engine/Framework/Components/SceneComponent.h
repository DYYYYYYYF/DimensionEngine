#pragma once

#include "TransformComponent.h"

class ENGINE_API USceneComponent : public UTransformComponent {
	DECLARE_CLASS_TYPE(USceneComponent)

public:
	USceneComponent(const FString& Name) : UTransformComponent(Name) {}

};