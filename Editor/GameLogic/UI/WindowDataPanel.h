#pragma once
#include <Framework/Classes/TextActor.h>

class AWindowDataPanel : public ATextActor {
	DECLARE_CLASS_TYPE(AWindowDataPanel)

public:
	AWindowDataPanel(const FString& Name);
	virtual void Tick(float DeltaTime) override;

private:
	FString WindowDataContent;
};