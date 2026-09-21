#include "SimpleEditorActor.h"

#include <Core/Controller.hpp>
#include <Core/DMemory.hpp>
#include <Core/Event.hpp>
#include <Systems/CameraSystem.h>
#include <Scene/World.h>

#include "UI/PanelWidget.h"
#include "UI/ImageWidget.h"
#include "Framework/Classes/TextActor.h"
#include "Game.h"
#include "GameLogic/UI/WindowDataPanel.h"
#include "GameLogic/UI/Console/GameConsole.h"

ASimpleEditorActor::ASimpleEditorActor(const FString& Name, GameInstance* InOwner)
	: AUIActor(Name), Owner(InOwner) {
	if (Owner) {
		WindowWidth = (float)Owner->GetWindowWidth();
		WindowHeight = (float)Owner->GetWindowHeight();
	}
	LogY = WindowHeight - LogPanelH;
	LogW = WindowWidth;
	HierarchyPanelH = LogY - HierarchyY - 10.0f;
	PropertiesX = WindowWidth - PropertiesMargin;
	BuildWidgetTree();
}

void ASimpleEditorActor::BuildWidgetTree() {
	RootPanel = new UPanelWidget();
	RootPanel->SetDesiredSize(-1.0f, -1.0f);

	// 顶部工具栏背景（半透明深色底板，与 3D 场景分离）
	ToolbarBG = new UImageWidget();
	ToolbarBG->SetDesiredSize(WindowWidth, ToolbarHeight);
	ToolbarBG->SetPosition(0.0f, 0.0f);
	ToolbarBG->SetColor(Vector4(0.10f, 0.11f, 0.13f, 0.92f));
	RootPanel->AddChild(ToolbarBG);

	// 工具栏按钮（Save / Play / Pause / Stop）
	for (int i = 0; i < 4; ++i) {
		UImageWidget* Btn = new UImageWidget();
		Btn->SetDesiredSize(ToolbarBtnW, ToolbarBtnH);
		Btn->SetPosition(ToolbarBtnX0 + i * ToolbarBtnGap, ToolbarBtnY);
		Btn->SetColor(Vector4(0.20f, 0.22f, 0.26f, 1.0f));
		RootPanel->AddChild(Btn);
		ToolbarButtonWidgets.Push(Btn);
		ToolbarButtons[i] = { ToolbarBtnX0 + i * ToolbarBtnGap, ToolbarBtnY, ToolbarBtnW, ToolbarBtnH, i };
	}

	// 层级面板背景（半透明深色底板）
	HierarchyBG = new UImageWidget();
	HierarchyBG->SetDesiredSize(HierarchyW, HierarchyPanelH);
	HierarchyBG->SetPosition(HierarchyX, HierarchyY);
	HierarchyBG->SetColor(Vector4(0.07f, 0.08f, 0.10f, 0.92f));
	RootPanel->AddChild(HierarchyBG);

	// 层级选中高亮条
	SelectionBar = new UImageWidget();
	SelectionBar->SetDesiredSize(HierarchyW - 4.0f, RowHeight);
	SelectionBar->SetPosition(HierarchyX + 2.0f, RowY(0));
	SelectionBar->SetColor(Vector4(0.22f, 0.45f, 0.90f, 0.85f));
	SelectionBar->SetVisible(false);
	RootPanel->AddChild(SelectionBar);

	// 属性面板背景（半透明深色底板）
	PropertiesBG = new UImageWidget();
	PropertiesBG->SetPosition(PropertiesX, HierarchyY);
	PropertiesBG->SetDesiredSize(PropertiesW, HierarchyPanelH);
	PropertiesBG->SetColor(Vector4(0.07f, 0.08f, 0.10f, 0.92f));
	RootPanel->AddChild(PropertiesBG);

	// 底部 Log 窗口背景（类似 UE Log/Debug 输出窗口）
	LogBG = new UImageWidget();
	LogBG->SetDesiredSize(LogW, LogPanelH);
	LogBG->SetPosition(LogX, LogY);
	LogBG->SetColor(Vector4(0.05f, 0.06f, 0.08f, 0.92f));
	RootPanel->AddChild(LogBG);

	// 底部 Log 窗口页签（Render / Keyboard / Console）
	for (int i = 0; i < 3; ++i) {
		UImageWidget* Tab = new UImageWidget();
		Tab->SetDesiredSize(LogTabW, LogTabH);
		Tab->SetPosition(LogX + LogTabX0 + i * LogTabGap, LogY + LogTabY);
		Tab->SetColor(Vector4(0.16f, 0.18f, 0.22f, 0.92f));
		RootPanel->AddChild(Tab);
		LogTabWidgets[i] = Tab;
	}

	SetRootWidget(RootPanel);
}

void ASimpleEditorActor::CreateEditorOverlayTexts(UWorld* World) {
	if (!World) {
		return;
	}

	// 层级面板标题
	HierarchyTitle = NewObject<ATextActor>("EditorHierarchyTitle", UITextType::eUI_Text_Type_System,
		"Noto Sans CJK JP", 20, FString("Hierarchy"));
	if (HierarchyTitle) {
		HierarchyTitle->SetActorLocation(Vector3(HierarchyX + 10.0f, HierarchyY + 6.0f, 0.0f));
		World->AddActor(HierarchyTitle);
		EditorActors.Push(HierarchyTitle);
	}

	// 属性面板标题与内容
	PropertiesTitle = NewObject<ATextActor>("EditorPropertiesTitle", UITextType::eUI_Text_Type_System,
		"Noto Sans CJK JP", 20, FString("Properties"));
	if (PropertiesTitle) {
		PropertiesTitle->SetActorLocation(Vector3(PropertiesX + 10.0f, HierarchyY + 8.0f, 0.0f));
		World->AddActor(PropertiesTitle);
		EditorActors.Push(PropertiesTitle);
	}

	PropertiesText = NewObject<ATextActor>("EditorPropertiesText", UITextType::eUI_Text_Type_System,
		"Noto Sans CJK JP", 20, FString(" "));
	if (PropertiesText) {
		PropertiesText->SetActorLocation(Vector3(PropertiesX + 10.0f, HierarchyY + 36.0f, 0.0f));
		World->AddActor(PropertiesText);
		EditorActors.Push(PropertiesText);
	}

	// 工具栏按钮标签
	static const char* Labels[4] = { "Save", "Play", "Pause", "Stop" };
	for (int i = 0; i < 4; ++i) {
		ToolbarBtnLabels[i] = NewObject<ATextActor>(
			FString::Format("EditorBtnLabel_%d", i), UITextType::eUI_Text_Type_System,
			"Noto Sans CJK JP", 20, FString(Labels[i]));
		if (ToolbarBtnLabels[i]) {
			ToolbarBtnLabels[i]->SetActorLocation(Vector3(
				ToolbarButtons[i].X + ToolbarBtnW * 0.5f - 20.0f,
				ToolbarButtons[i].Y + 6.0f, 0.0f));
			World->AddActor(ToolbarBtnLabels[i]);
			EditorActors.Push(ToolbarBtnLabels[i]);
		}
	}

	// 工具栏状态文本
	ToolbarStatusText = NewObject<ATextActor>("EditorToolbarStatus", UITextType::eUI_Text_Type_System,
		"Noto Sans CJK JP", 20, FString("Idle"));
	if (ToolbarStatusText) {
		ToolbarStatusText->SetActorLocation(Vector3(480.0f, 12.0f, 0.0f));
		World->AddActor(ToolbarStatusText);
		EditorActors.Push(ToolbarStatusText);
	}

	// 底部 Log 窗口页签标签（Render / Keyboard / Console）
	static const char* LogTabNames3[3] = { "Render", "Keyboard", "Console" };
	for (int i = 0; i < 3; ++i) {
		LogTabLabels[i] = NewObject<ATextActor>(
			FString::Format("EditorLogTab_%d", i), UITextType::eUI_Text_Type_System,
			"Noto Sans CJK JP", 18, FString(LogTabNames3[i]));
		if (LogTabLabels[i]) {
			LogTabLabels[i]->SetActorLocation(Vector3(
				LogX + LogTabX0 + i * LogTabGap + 14.0f,
				LogY + LogTabY + 4.0f, 0.0f));
			World->AddActor(LogTabLabels[i]);
			EditorActors.Push(LogTabLabels[i]);
		}
	}

	// 层级行文本（固定 40 个，位置按行索引固定；空行置于屏幕外）
	for (int i = 0; i < MaxHierarchyRows; ++i) {
		ATextActor* Row = NewObject<ATextActor>(
			FString::Format("EditorRowText_%d", i), UITextType::eUI_Text_Type_System,
			"Noto Sans CJK JP", 20, FString(" "));
		if (!Row) {
			continue;
		}
		Row->SetActorLocation(Vector3(-9999.0f, -9999.0f, 0.0f));
		World->AddActor(Row);
		RowTextActors.Push(Row);
		EditorActors.Push(Row);
	}

	EditorActors.Push(this);
}

bool ASimpleEditorActor::IsEditorActor(const AActor* Actor) const {
	if (!Actor) {
		return false;
	}
	for (size_t i = 0; i < EditorActors.Size(); ++i) {
		if (EditorActors[i] == Actor) {
			return true;
		}
	}
	return false;
}

void ASimpleEditorActor::Tick(float DeltaTime) {
	// 必须调用基类 Tick：AActor::Tick 会驱动 UUIWidgetComponent::Tick，
	// 进而触发 Widget 树测量/布局/绘制与 RebuildGeometry 生成合批几何。
	// 此前未调用基类导致背景矩形几何从未构建（Log 中无 Material.UI.Batched 重建痕迹）。
	AUIActor::Tick(DeltaTime);

	UWorld* World = GetWorld();
	if (!World || !Owner) {
		return;
	}

	UpdateLayout();
	RebuildHierarchyList(World);
	HandleMouseInput();
	HandleKeyboardInput();
	RefreshHierarchyTexts();
	RefreshPropertiesText();
	RefreshToolbar(DeltaTime);
	RefreshLogWindow();
}

void ASimpleEditorActor::UpdateLayout() {
	if (!Owner) {
		return;
	}
	WindowWidth = (float)Owner->GetWindowWidth();
	WindowHeight = (float)Owner->GetWindowHeight();
	LogY = WindowHeight - LogPanelH;
	LogW = WindowWidth;
	HierarchyPanelH = LogY - HierarchyY - 10.0f;
	PropertiesX = WindowWidth - PropertiesMargin;

	if (ToolbarBG) {
		ToolbarBG->SetDesiredSize(WindowWidth, ToolbarHeight);
	}
	if (HierarchyBG) {
		HierarchyBG->SetDesiredSize(HierarchyW, HierarchyPanelH);
	}
	if (PropertiesBG) {
		PropertiesBG->SetPosition(PropertiesX, HierarchyY);
		PropertiesBG->SetDesiredSize(PropertiesW, HierarchyPanelH);
	}
	if (PropertiesTitle) {
		PropertiesTitle->SetActorLocation(Vector3(PropertiesX + 10.0f, HierarchyY + 8.0f, 0.0f));
	}
	if (PropertiesText) {
		PropertiesText->SetActorLocation(Vector3(PropertiesX + 10.0f, HierarchyY + 36.0f, 0.0f));
	}
	if (LogBG) {
		LogBG->SetDesiredSize(LogW, LogPanelH);
		LogBG->SetPosition(LogX, LogY);
	}
	for (int i = 0; i < 3; ++i) {
		if (LogTabWidgets[i]) {
			LogTabWidgets[i]->SetPosition(LogX + LogTabX0 + i * LogTabGap, LogY + LogTabY);
		}
		if (LogTabLabels[i]) {
			LogTabLabels[i]->SetActorLocation(Vector3(
				LogX + LogTabX0 + i * LogTabGap + 14.0f,
				LogY + LogTabY + 4.0f, 0.0f));
		}
	}
}

void ASimpleEditorActor::RebuildHierarchyList(UWorld* World) {
	DisplayActors.Clear();
	if (!World) {
		return;
	}

	const TArray<AActor*>& WorldActors = World->GetWorldActors();
	AActor* MainCam = CameraSystem::Get().GetMainCamera();
	for (size_t i = 0; i < WorldActors.Size(); ++i) {
		AActor* A = WorldActors[i];
		if (!A || A == this || IsEditorActor(A)) {
			continue;
		}
		if (A == MainCam) {
			continue;
		}
		DisplayActors.Push(A);
	}

	// 选中 actor 可能已被场景切换删除 → 失效时清空
	bool bStillExists = false;
	if (SelectedActor) {
		for (size_t i = 0; i < DisplayActors.Size(); ++i) {
			if (DisplayActors[i] == SelectedActor) {
				bStillExists = true;
				break;
			}
		}
		if (!bStillExists) {
			SelectedActor = nullptr;
		}
	}

	SelectedRow = -1;
	if (SelectedActor) {
		for (size_t i = 0; i < DisplayActors.Size(); ++i) {
			if (DisplayActors[i] == SelectedActor) {
				SelectedRow = (int)i;
				break;
			}
		}
	}
}

void ASimpleEditorActor::RefreshHierarchyTexts() {
	const int N = (int)DisplayActors.Size();
	for (int i = 0; i < MaxHierarchyRows; ++i) {
		ATextActor* Row = RowTextActors[i];
		if (!Row) {
			continue;
		}
		if (i < N) {
			if (Row->GetActorLocation().x < -1000.0f) {
				Row->SetActorLocation(Vector3(HierarchyX + 14.0f, RowY(i) + 4.0f, 0.0f));
			}
			Row->SetText(DisplayActors[i]->GetName());
		}
		else {
			Row->SetText(FString(" "));
			if (Row->GetActorLocation().x > -1000.0f) {
				Row->SetActorLocation(Vector3(-9999.0f, -9999.0f, 0.0f));
			}
		}
	}

	// 选中高亮条
	if (SelectionBar) {
		if (SelectedRow >= 0) {
			SelectionBar->SetVisible(true);
			SelectionBar->SetPosition(HierarchyX + 2.0f, RowY(SelectedRow));
		}
		else {
			SelectionBar->SetVisible(false);
		}
	}
}

void ASimpleEditorActor::RefreshPropertiesText() {
	if (!PropertiesText) {
		return;
	}
	if (!SelectedActor) {
		PropertiesText->SetText(FString(
			"  No actor selected.\n\n"
			"  Click a row in the Hierarchy panel\n"
			"  or press Tab to cycle actors."));
		return;
	}

	const Vector3 Loc = SelectedActor->GetActorLocation();
	const Vector3 Rot = SelectedActor->GetActorRotation();
	const Vector3 Sca = SelectedActor->GetWorldScale();
	const char* CatMark[3] = { "  ", "  ", "  " };
	CatMark[(int)EditCategory] = "> ";

	PropertiesText->SetText(FString::Format(
		" %s\n\n"
		" %s[1] Location  X=%.2f Y=%.2f Z=%.2f\n"
		" %s[2] Rotation  X=%.2f Y=%.2f Z=%.2f\n"
		" %s[3] Scale     X=%.2f Y=%.2f Z=%.2f\n\n"
		" 1/2/3 category    X/Y/Z axis\n"
		" PageUp/PageDown   value\n"
		" Shift=10  Ctrl=0.1\n"
		" Tab cycle   BackSpace deselect",
		SelectedActor->GetName().CStr(),
		CatMark[0], Loc.x, Loc.y, Loc.z,
		CatMark[1], Rot.x, Rot.y, Rot.z,
		CatMark[2], Sca.x, Sca.y, Sca.z));
}

void ASimpleEditorActor::RefreshToolbar(float DeltaTime) {
	if (SaveFlashTime > 0.0f) {
		SaveFlashTime -= DeltaTime;
	}

	const Vector4 BtnDefault(0.20f, 0.22f, 0.26f, 1.0f);
	const Vector4 BtnGreen(0.15f, 0.55f, 0.28f, 1.0f);
	const Vector4 BtnYellow(0.70f, 0.60f, 0.15f, 1.0f);

	for (int i = 0; i < 4; ++i) {
		if (ToolbarButtonWidgets.Size() > (size_t)i && ToolbarButtonWidgets[i]) {
			ToolbarButtonWidgets[i]->SetColor(BtnDefault);
		}
	}

	FString Status = "Idle | right-drag orbit | middle-drag pan | wheel zoom";
	const EditorPlayState State = Owner ? Owner->GetEditorPlayState() : EditorPlayState::Idle;
	switch (State) {
	case EditorPlayState::Playing:
		if (ToolbarButtonWidgets.Size() > 1 && ToolbarButtonWidgets[1]) {
			ToolbarButtonWidgets[1]->SetColor(BtnGreen);
		}
		Status = "Playing - world actors ticking";
		break;
	case EditorPlayState::Paused:
		if (ToolbarButtonWidgets.Size() > 2 && ToolbarButtonWidgets[2]) {
			ToolbarButtonWidgets[2]->SetColor(BtnYellow);
		}
		Status = "Paused - world ticks frozen (editor UI alive)";
		break;
	default:
		break;
	}

	if (SaveFlashTime > 0.0f) {
		if (ToolbarButtonWidgets.Size() > 0 && ToolbarButtonWidgets[0]) {
			ToolbarButtonWidgets[0]->SetColor(BtnGreen);
		}
		Status = "Scene saved to Editor/Config.json";
	}

	if (ToolbarStatusText) {
		ToolbarStatusText->SetText(Status);
	}
}

void ASimpleEditorActor::HandleMouseInput() {
	int mx = 0, my = 0;
	Controller::GetMousePosition(mx, my);
	const bool bLeft = Controller::IsButtonDown(eButtons::Left);

	if (bLeft && !bPrevLeftDown) {
		bool bHandled = false;

		// 1) 工具栏按钮
		for (int i = 0; i < 4; ++i) {
			const ToolbarButtonRect& B = ToolbarButtons[i];
			if (IsPointInRect((float)mx, (float)my, B.X, B.Y, B.W, B.H)) {
				HandleToolbarClick(B.Id);
				bHandled = true;
				break;
			}
		}

		// 2) 底部 Log 窗口页签
		if (!bHandled && (float)my >= LogY) {
			for (int i = 0; i < 3; ++i) {
				const float TX = LogX + LogTabX0 + i * LogTabGap;
				if (IsPointInRect((float)mx, (float)my, TX, LogY + LogTabY, LogTabW, LogTabH)) {
					SetLogActiveTab(i);
					bHandled = true;
					break;
				}
			}
		}

		// 3) 层级面板行命中
		if (!bHandled && IsPointInRect((float)mx, (float)my, HierarchyX, HierarchyY, HierarchyW, HierarchyPanelH)) {
			const float LocalY = (float)my - HierarchyRowStartY;
			int Row = (int)(LocalY / (RowHeight + RowGap));
			if (Row >= 0 && Row < (int)DisplayActors.Size()) {
				SelectedActor = DisplayActors[Row];
				SelectedRow = Row;
			}
			else {
				SelectedActor = nullptr;
				SelectedRow = -1;
			}
			bHandled = true;
		}

		// 4) 场景视图区域（空白）点击 → 取消选中
		if (!bHandled) {
			const bool bInProps = IsPointInRect((float)mx, (float)my, PropertiesX, HierarchyY, PropertiesW, HierarchyPanelH);
			const bool bInToolbar = (float)my < ToolbarHeight;
			if (!bInProps && !bInToolbar) {
				SelectedActor = nullptr;
				SelectedRow = -1;
			}
		}
	}

	bPrevLeftDown = bLeft;
}

void ASimpleEditorActor::HandleKeyboardInput() {
	// Tab：循环选中
	const bool bTab = Controller::IsKeyDown(eKeys::Tab);
	if (bTab && !bPrevTabDown) {
		if (DisplayActors.Size() > 0) {
			int Next = SelectedRow + 1;
			if (Next >= (int)DisplayActors.Size()) {
				Next = 0;
			}
			SelectedActor = DisplayActors[Next];
			SelectedRow = Next;
		}
	}
	bPrevTabDown = bTab;

	// BackSpace：取消选中
	const bool bBS = Controller::IsKeyDown(eKeys::BackSpace);
	if (bBS && !bPrevBackSpaceDown) {
		SelectedActor = nullptr;
		SelectedRow = -1;
	}
	bPrevBackSpaceDown = bBS;

	// 属性类别
	if (Controller::IsKeyDown(eKeys::Num_1)) {
		EditCategory = EditorEditCategory::Location;
	}
	if (Controller::IsKeyDown(eKeys::Num_2)) {
		EditCategory = EditorEditCategory::Rotation;
	}
	if (Controller::IsKeyDown(eKeys::Num_3)) {
		EditCategory = EditorEditCategory::Scale;
	}
	// 属性轴
	if (Controller::IsKeyDown(eKeys::X)) {
		EditAxis = 0;
	}
	if (Controller::IsKeyDown(eKeys::Y)) {
		EditAxis = 1;
	}
	if (Controller::IsKeyDown(eKeys::Z)) {
		EditAxis = 2;
	}

	// 数值调整
	float Step = 1.0f;
	if (Controller::IsKeyDown(eKeys::Control)) {
		Step = 0.1f;
	}
	else if (Controller::IsKeyDown(eKeys::Shift)) {
		Step = 10.0f;
	}
	if (Controller::IsKeyDown(eKeys::PageUp)) {
		ApplyPropertyDelta(Step);
	}
	if (Controller::IsKeyDown(eKeys::PageDown)) {
		ApplyPropertyDelta(-Step);
	}
}

void ASimpleEditorActor::HandleToolbarClick(int ButtonId) {
	if (!Owner) {
		return;
	}
	switch ((EditorToolbarButton)ButtonId) {
	case EditorToolbarButton::Save: {
		Owner->SaveSceneConfig();
		SaveFlashTime = 0.8f;
	} break;
	case EditorToolbarButton::Play:
		Owner->SetEditorPlayState(EditorPlayState::Playing);
		break;
	case EditorToolbarButton::Pause:
		Owner->SetEditorPlayState(EditorPlayState::Paused);
		break;
	case EditorToolbarButton::Stop:
		Owner->SetEditorPlayState(EditorPlayState::Idle);
		break;
	default:
		break;
	}
}

bool ASimpleEditorActor::IsPointInRect(float X, float Y, float RX, float RY, float RW, float RH) const {
	return X >= RX && X <= RX + RW && Y >= RY && Y <= RY + RH;
}

void ASimpleEditorActor::ApplyPropertyDelta(float Delta) {
	if (!SelectedActor) {
		return;
	}

	Vector3 V;
	switch (EditCategory) {
	case EditorEditCategory::Location:
		V = SelectedActor->GetActorLocation();
		break;
	case EditorEditCategory::Rotation:
		V = SelectedActor->GetActorRotation();
		break;
	case EditorEditCategory::Scale:
		V = SelectedActor->GetWorldScale();
		break;
	default:
		return;
	}

	if (EditAxis == 0) {
		V.x += Delta;
	}
	else if (EditAxis == 1) {
		V.y += Delta;
	}
	else {
		V.z += Delta;
	}

	switch (EditCategory) {
	case EditorEditCategory::Location:
		SelectedActor->SetActorLocation(V);
		break;
	case EditorEditCategory::Rotation:
		SelectedActor->SetActorRotation(V);
		break;
	case EditorEditCategory::Scale:
		SelectedActor->SetWorldScale(V);
		break;
	default:
		break;
	}
}

void ASimpleEditorActor::RegisterLogTextActor(int Index, ATextActor* Actor, const FString& DisplayName) {
	if (Index < 0 || Index >= 3) {
		return;
	}
	LogSources[Index].Text = Actor;
	LogTabNames[Index] = DisplayName;
}

void ASimpleEditorActor::RegisterLogTextActor(int Index, AWindowDataPanel* Actor, const FString& DisplayName) {
	if (Index < 0 || Index >= 3) {
		return;
	}
	LogSources[Index].RenderPanel = Actor;
	LogTabNames[Index] = DisplayName;
}

void ASimpleEditorActor::RegisterLogTextActor(int Index, DebugConsoleActor* Actor, const FString& DisplayName) {
	if (Index < 0 || Index >= 3) {
		return;
	}
	LogSources[Index].Console = Actor;
	LogTabNames[Index] = DisplayName;
}

void ASimpleEditorActor::SetLogActiveTab(int Index) {
	if (Index < 0 || Index >= 3) {
		return;
	}
	LogActiveTab = Index;
}

void ASimpleEditorActor::RefreshLogWindow() {
	const Vector3 Hidden(-9999.0f, -9999.0f, 0.0f);

	for (int i = 0; i < 3; ++i) {
		// 页签高亮：激活页蓝色，非激活页深灰
		if (LogTabWidgets[i]) {
			LogTabWidgets[i]->SetColor(i == LogActiveTab
				? Vector4(0.22f, 0.45f, 0.90f, 0.92f)
				: Vector4(0.16f, 0.18f, 0.22f, 0.92f));
		}

		const bool bActive = (i == LogActiveTab);
		const Vector3 Target = bActive
			? Vector3(LogX + LogTextX, LogY + LogTextY, 0.0f)
			: Hidden;

		// 仅激活页文本显示在 Log 窗口内，其余移出屏幕（内容原样保留）
		const LogTextSource& Src = LogSources[i];
		if (Src.RenderPanel) {
			Src.RenderPanel->SetAnchorPosition(Target);
			Src.RenderPanel->SetActorLocation(Target);
		}
		if (Src.Text) {
			Src.Text->SetActorLocation(Target);
		}
		if (Src.Console) {
			Src.Console->SetPanelLocation(Target);
		}
	}
}
