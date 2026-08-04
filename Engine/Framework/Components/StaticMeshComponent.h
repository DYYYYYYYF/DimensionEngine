#pragma once

#include "MeshComponent.h"

class UGeometry;

class UStaticMeshComponent : public UMeshComponent {
public:
	DECLARE_CLASS_TYPE(UStaticMeshComponent)

public:
	UStaticMeshComponent();
	virtual void DrawMesh() override;

protected:
	UGeometry* Mesh = nullptr;
};