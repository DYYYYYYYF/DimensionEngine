#include "TextActor.h"

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

	if (!TextComponent->Load(type, fontName, fontSize, textContent)) {
		GLOG(Log::Level::eError, "Load font %s failed. font type: %i", fontName.CStr(), (int)type);
		return;
	}
}

ATextActor::~ATextActor() {
	if (!TextComponent) {
		return;
	}

	TextComponent->Unload();
}

void ATextActor::SetText(const FString& content) {
	if (!TextComponent) {
		return;
	}

	TextComponent->SetContent(content);
}

FString ATextActor::GetText() const { 
	if (!TextComponent) {
		return FString();
	}

	return TextComponent->GetContent();
}
