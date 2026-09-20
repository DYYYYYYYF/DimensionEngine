#include "UIActor.h"
#include "UI/Widget.h"

AUIActor::AUIActor(const FString& Name) : AActor(Name) {
	UIWidgetComponent = CreateComponent<UUIWidgetComponent>("UIWidgetComponent");
	SetRootComponent(UIWidgetComponent);

	SetEnableTick(true);
}

AUIActor::~AUIActor() {
	if (UIWidgetComponent) {
		UIWidgetComponent->Destroy();
		UIWidgetComponent = nullptr;
	}
}

void AUIActor::SetRootWidget(UWidget* InRootWidget) {
	if (!UIWidgetComponent) return;

	UIWidgetComponent->SetRootWidget(InRootWidget);
}
