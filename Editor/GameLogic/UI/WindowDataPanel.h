#pragma once
#include <Framework/Classes/TextActor.h>

class AWindowDataPanel : public ATextActor {
	DECLARE_CLASS_TYPE(AWindowDataPanel)

public:
	AWindowDataPanel(const FString& Name);
	virtual void Tick(float DeltaTime) override;

	// 由编辑器 Log 窗口锚定本面板位置：启用后 Tick 不再覆盖位置
	void SetAnchorPosition(const Vector3& Pos) { AnchorPosition = Pos; bUseAnchor = true; }

private:
	FString WindowDataContent;
	bool bUseAnchor = false;
	Vector3 AnchorPosition;
};