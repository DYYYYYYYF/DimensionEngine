#pragma once

#include "StaticMeshComponent.h"

class ENGINE_API USkyboxComponent : public UStaticMeshComponent {
	DECLARE_CLASS_TYPE(USkyboxComponent)

public:
	USkyboxComponent(const FString& Name);

	virtual bool CreateRenderProxy() override;
	virtual void UpdateRenderProxy() override;

protected:
	UGeometry* SkyboxGeometry;
};