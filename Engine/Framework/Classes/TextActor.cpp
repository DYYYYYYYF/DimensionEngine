#include "TextActor.h"
#include "Systems/FontSystem.hpp"

ATextActor::ATextActor(const FString& Name) : AActor(Name) {
	TextComponent = CreateComponent<UTextComponent>("TextComponent");
	SetRootComponent(TextComponent);

	// 设置默认字体
	FString FontName = "Noto Sans CJK JP";
	UITextType Type = UITextType::eUI_Text_Type_System;
	SetTextFont(FontName, Type);

	SetEnableTick(true);
}

ATextActor::ATextActor(const FString& Name, UITextType type, const FString& fontName,
	int fontSize, const FString& textContent) : AActor(fontName) {
	TextComponent = CreateComponent<UTextComponent>("TextComponent");
	if (!TextComponent) {
		return;
	}

	// 设置根组件
	SetRootComponent(TextComponent);

	// 设置字体和内容
	SetTextFont(fontName, type, fontSize);
	SetText(textContent);

	// 开启Tick
	SetEnableTick(true);
}

ATextActor::~ATextActor() {
	if (!TextComponent) {
		return;
	}

	TextComponent->Destroy();
}

void ATextActor::SetFontSize(uint32_t FontSize) {
	if (!TextComponent) return;

}

void ATextActor::SetTextFont(IFont* Font) {
	if (!Font) return;

	IFont* CurrentFont = TextComponent->GetFont();
	if (CurrentFont) {
		FontSystem::Get().Release(CurrentFont);
	}

	TextComponent->SetFont(Font);
}

void ATextActor::SetTextFont(const FString& FontName, UITextType Type, uint32_t FontSize) {
	if (!TextComponent) return;

	// 如果字体和大小相同就忽略
	IFont* FontData = TextComponent->GetFont();
	if (FontData) {
		if (FontData->GetFontName().Compare(FontName) == 0
			&& FontData->GetFontSize() == FontSize) {
			return;
		}
	}

	FontSystem& FontSystem = FontSystem::Get();
	FontData = FontSystem.Acquire(FontName, Type, FontSize);
	if (!FontData) {
		GLOG(Log::eError, "Unable to acquire font: '%s'. UIText can not be created.", FontName.CStr());
		return;
	}

	SetTextFont(FontData);
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
