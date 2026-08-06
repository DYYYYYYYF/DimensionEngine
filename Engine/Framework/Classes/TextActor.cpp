#include "TextActor.h"
#include "Systems/FontSystem.hpp"

ATextActor::ATextActor() {
	ATextActor("TextActor");
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
	
	// 获取字体资产
	FontSystem& FontSystem = FontSystem::Get();
	IFont* FontData = FontSystem.Acquire(fontName, type, fontSize);
	if (!FontData) {
		GLOG(Log::eError, "Unable to acquire font: '%s'. UIText can not be created.", fontName.CStr());
		return;
	}

	// 设置字体和内容
	TextComponent->SetFont(FontData);
	TextComponent->SetText(textContent);

	// 初始化
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
