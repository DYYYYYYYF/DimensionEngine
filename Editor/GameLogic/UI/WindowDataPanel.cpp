#include "WindowDataPanel.h"
#include <Systems/CameraSystem.h>
#include <Core/Controller.hpp>
#include <Core/Metrics.hpp>
#include <Scene/World.h>
#include <Math/MathTypes.hpp>
#include <Utils/RenderDataHelper.h>

AWindowDataPanel::AWindowDataPanel(const FString& Name) : ATextActor(Name) {
	WindowDataContent = "No information.";
	SetText(WindowDataContent);
}

void AWindowDataPanel::Tick(float DeltaTime) {
	ATextActor::Tick(DeltaTime);

	// Text
	ACameraActor* WorldCamera = CameraSystem::Get().GetMainCamera();
	if (!WorldCamera) return;

	Vector3 Pos = WorldCamera->GetActorLocation();
	Vector3 Rot = WorldCamera->GetActorRotation();

	// Mouse state
	bool LeftDown = Controller::IsButtonDown(eButtons::Left);
	bool RightDown = Controller::IsButtonDown(eButtons::Right);
	int MouseX, MouseY;
	Controller::GetMousePosition(MouseX, MouseY);

	// Convert to NDC.
	Vector2f WindowSize = URenderDataHelper::GetRTSize();
	float MouseX_NDC = RangeConvertfloat((float)MouseX, 0.0f, WindowSize.x, -1.0f, 1.0f);
	float MouseY_NDC = RangeConvertfloat((float)MouseY, 0.0f, WindowSize.y, -1.0f, 1.0f);

	double FPS, FrameTime;
	Metrics::Frame(&FPS, &FrameTime);

	// NOTE: starting at a reasonable default to avoid too many realloc.
	uint32_t DrawCount = (uint32_t)GetWorld()->GetVisibleGeometryCount();

	// 更新文本
	FString FPSText = FString::Format("\
	Camera Pos: [%.3f %.3f %.3f]\tCamera Rot: [%.3f %.3f %.3f]\n\
	FPS: %d\tDelta time: %.2f\tL=%s R=%s\tNDC: x=%.2f, y=%.2f\n\
	Drawn Count: %-5u",
		Pos.x, Pos.y, Pos.z,
		Rot.x, Rot.y, Rot.z,
		(int)FPS, (float)FrameTime,
		LeftDown ? "Y" : "N", RightDown ? "Y" : "N",
		MouseX_NDC, MouseY_NDC,
		DrawCount
	);

	SetText(FPSText);
	SetActorLocation(Vector3(450, WindowSize.y - 200, 0));
}

