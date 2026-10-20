#include "UISampleActor.h"

#include "UI/PanelWidget.h"
#include "UI/ImageWidget.h"

UISampleActor::UISampleActor(const FString& Name) : AActor(Name) {
	BuildWidgetTree();
}

UISampleActor::~UISampleActor() {
	if (Root) {
		delete Root;
		Root = nullptr;
	}
}

void UISampleActor::BuildWidgetTree() {
	// 根节点：垂直盒，位于屏幕左上角区域（通过 Padding 定位）
	UVerticalBoxPanel* RootBox = new UVerticalBoxPanel();
	RootBox->SetDesiredSize(240.0f, -1.0f);
	RootBox->SetSpacing(8.0f);
	RootBox->SetPadding(60.0f, 60.0f);

	// 1) 引擎 Logo 图片（走 ResourceSystem 加载纹理）
	UImageWidget* Logo = new UImageWidget();
	Logo->SetTexture("DimensionEngine");
	Logo->SetDesiredSize(240.0f, 48.0f);
	RootBox->AddChild(Logo);

	// 2) 纯色矩形（未设置纹理，绑定白色纹理 + Color）
	UImageWidget* Red = new UImageWidget();
	Red->SetDesiredSize(240.0f, 32.0f);
	Red->SetColor(Vector4(0.90f, 0.25f, 0.25f, 1.0f));
	RootBox->AddChild(Red);

	UImageWidget* Green = new UImageWidget();
	Green->SetDesiredSize(240.0f, 32.0f);
	Green->SetColor(Vector4(0.25f, 0.85f, 0.25f, 1.0f));
	RootBox->AddChild(Green);

	UImageWidget* Blue = new UImageWidget();
	Blue->SetDesiredSize(240.0f, 32.0f);
	Blue->SetColor(Vector4(0.25f, 0.45f, 0.95f, 1.0f));
	RootBox->AddChild(Blue);

	UImageWidget* Yellow = new UImageWidget();
	Yellow->SetDesiredSize(240.0f, 24.0f);
	Yellow->SetColor(Vector4(0.95f, 0.80f, 0.20f, 1.0f));
	RootBox->AddChild(Yellow);

	// Widget 树归本 Actor 持有（析构释放）；若需渲染需注册进 World UI 根列表：
	// World->AddUIWidgetRoot(Root)，由 UWorld 每帧驱动。
	Root = RootBox;
}
