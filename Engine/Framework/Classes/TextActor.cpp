#include "TextActor.h"
#include "Systems/FontSystem.hpp"

ATextActor::ATextActor() : AActor() {
	TextComponent = CreateComponent<UTextComponent>();
}

ATextActor::ATextActor(const FString& Name) : AActor(Name) {
	TextComponent = CreateComponent<UTextComponent>();
}

ATextActor::ATextActor(UITextType type, const FString& fontName,
	int fontSize, const FString& textContent) : AActor() {
	TextComponent = CreateComponent<UTextComponent>();

	if (!TextComponent) {
		return;
	}

	FontSystem& FontSystem = FontSystem::Get();
	IFont* FontData = FontSystem.Acquire(fontName, type, fontSize);
	if (!FontData) {
		GLOG(Log::eError, "Unable to acquire font: '%s'. UIText can not be created.", fontName.CStr());
		return;
	}
	TextComponent->SetFont(FontData);

	if (!TextComponent->Initialize()) {
		GLOG(Log::Level::eError, "Load font %s failed. font type: %i", fontName.CStr(), (int)type);
		return;
	}
}

ATextActor::~ATextActor() {
	if (!TextComponent) {
		return;
	}

	TextComponent->Destroy();
}

void ATextActor::SetText(const FString& content) {
	if (!TextComponent) {
		return;
	}

	TextComponent->SetText(content);
}

FString ATextActor::GetText() const { 
	if (!TextComponent) {
		return FString();
	}

	return TextComponent->GetText();
}
