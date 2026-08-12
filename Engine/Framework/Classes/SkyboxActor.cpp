#include "SkyboxActor.h"
#include "Framework/Components/SkyboxComponent.h"

ASkyboxActor::ASkyboxActor(const FString& Name) : AActor(Name) {
	SkyboxComponent = CreateComponent<USkyboxComponent>("SkyboxComponent");
	if (SkyboxComponent) {
		SetRootComponent(SkyboxComponent);
	}
}
