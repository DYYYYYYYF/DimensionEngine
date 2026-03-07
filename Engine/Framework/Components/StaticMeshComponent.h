#pragma once

#include "MeshComponent.h"

class Geometry;

class UStaticMeshComponent : public UMeshComponent {
public:
	DECLARE_CLASS_TYPE(UStaticMeshComponent)

public:
	UStaticMeshComponent();
	virtual void DrawMesh() override;

protected:
	Geometry* Mesh = nullptr;
};