#pragma once

#include "MeshComponent.h"

class UGeometry;

class ENGINE_API UStaticMeshComponent : public UMeshComponent {
public:
	DECLARE_CLASS_TYPE(UStaticMeshComponent)

public:
	UStaticMeshComponent(const FString& Name);
	virtual void DrawMesh() override;
	virtual bool CreateRenderProxy() override;
	virtual void UpdateRenderProxy() override;

	void SetMesh(TArray<UGeometry*> InMesh);

protected:
	void UpdateBounding();

protected:
	TArray<UGeometry*> Mesh;
};