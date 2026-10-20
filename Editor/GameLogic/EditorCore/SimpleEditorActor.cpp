#include "SimpleEditorActor.h"

#include <Core/Controller.hpp>
#include <Core/DMemory.hpp>
#include <Core/Event.hpp>
#include <Systems/CameraSystem.h>
#include <Systems/RenderViewSystem.hpp>
#include <Systems/FontSystem.hpp>
#include <Scene/World.h>

#include "UI/PanelWidget.h"
#include "UI/ImageWidget.h"
#include "UI/ButtonWidget.h"
#include "UI/ScrollBoxWidget.h"
#include "UI/AnchorPanelWidget.h"
#include "Framework/Classes/TextActor.h"
#include "Framework/Components/CameraComponent.h"
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

	// ---- 左侧对象列表（UE 风格重构）：锚点 Panel 固定左侧 → 内套 ScrollBox → Button 列表项 ----
	// 1) 锚点面板：AnchorMin=(0,0) AnchorMax=(0,1) 锚定屏幕左侧并垂直拉伸；
	//    Offsets=(Left=HierarchyX, Top=HierarchyY, Right=HierarchyX+HierarchyW,
	//    Bottom=-(LogPanelH+10))：底边相对父底部上移 Log 窗口 + 间隙高度，
	//    保证面板底始终位于 Log 上方，天然不与 Log 重叠。
	HierarchyPanel = new UAnchorPanelWidget();
	HierarchyPanel->SetAnchors(Vector2(0.0f, 0.0f), Vector2(0.0f, 1.0f));
	HierarchyPanel->SetOffsets(HierarchyX, HierarchyY, HierarchyX + HierarchyW, -(LogPanelH + 10.0f));
	RootPanel->AddChild(HierarchyPanel);

	// 2) 面板背景（铺满锚点面板）
	HierarchyBG = new UImageWidget();
	HierarchyBG->SetDesiredSize(-1.0f, -1.0f);
	HierarchyBG->SetColor(Vector4(0.07f, 0.08f, 0.10f, 0.92f));
	HierarchyPanel->AddChild(HierarchyBG);

	// 3) ScrollBox：位于面板标题（30px）之下，管理滚动偏移与可视裁剪
	HierarchyScrollBox = new UScrollBoxWidget();
	HierarchyScrollBox->SetContentStartY(30.0f);
	HierarchyScrollBox->SetContentMarginX(2.0f);
	HierarchyScrollBox->SetRowStep(RowHeight + RowGap);
	HierarchyScrollBox->SetRowHeight(RowHeight);
	HierarchyPanel->AddChild(HierarchyScrollBox);

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

	// 中央视口容器（UE5 风格：视口居中，四周为 UI 面板）。
	// 位置/尺寸由 UpdateLayout 每帧按窗口重算（此处先按初始窗口铺满中央区域）。
	// 注意：仅绘制标题条与四边框，渲染区（ViewportX/Y/W/H）内不放置任何 UI 矩形，
	// 否则不透明 UI 层会盖住下方 Skybox/Deferred 的 3D 渲染结果（表现为黑屏）。
	ViewportBorderTop = new UImageWidget();
	ViewportBorderTop->SetDesiredSize(1.0f, 1.0f);
	ViewportBorderTop->SetPosition(0.0f, 0.0f);
	ViewportBorderTop->SetColor(Vector4(0.13f, 0.14f, 0.16f, 1.0f));
	RootPanel->AddChild(ViewportBorderTop);

	ViewportBorderBottom = new UImageWidget();
	ViewportBorderBottom->SetDesiredSize(1.0f, 1.0f);
	ViewportBorderBottom->SetPosition(0.0f, 0.0f);
	ViewportBorderBottom->SetColor(Vector4(0.13f, 0.14f, 0.16f, 1.0f));
	RootPanel->AddChild(ViewportBorderBottom);

	ViewportBorderLeft = new UImageWidget();
	ViewportBorderLeft->SetDesiredSize(1.0f, 1.0f);
	ViewportBorderLeft->SetPosition(0.0f, 0.0f);
	ViewportBorderLeft->SetColor(Vector4(0.13f, 0.14f, 0.16f, 1.0f));
	RootPanel->AddChild(ViewportBorderLeft);

	ViewportBorderRight = new UImageWidget();
	ViewportBorderRight->SetDesiredSize(1.0f, 1.0f);
	ViewportBorderRight->SetPosition(0.0f, 0.0f);
	ViewportBorderRight->SetColor(Vector4(0.13f, 0.14f, 0.16f, 1.0f));
	RootPanel->AddChild(ViewportBorderRight);

	SetRootWidget(RootPanel);
}

void ASimpleEditorActor::CreateEditorOverlayTexts(UWorld* InWorld) {
	if (!InWorld) {
		return;
	}

	// 层级面板标题
	// [TEST] 中文渲染验证：UTF-8 字节转义（源码字面量会编译为 GBK 执行字符集）
	HierarchyTitle = NewObject<ATextActor>("EditorHierarchyTitle", UITextType::eUI_Text_Type_Bitmap,
		"Noto Sans 21px", 21, FString("\xe5\xb1\x82\xe7\xba\xa7\xe5\x88\x97\xe8\xa1\xa8 Hierarchy"));
	if (HierarchyTitle) {
		HierarchyTitle->SetActorLocation(Vector3(HierarchyX + 10.0f, HierarchyY + 6.0f, 0.0f));
		InWorld->AddActor(HierarchyTitle);
		EditorActors.Push(HierarchyTitle);
	}

	// 属性面板标题与内容
	PropertiesTitle = NewObject<ATextActor>("EditorPropertiesTitle", UITextType::eUI_Text_Type_Bitmap,
		"Ubuntu Mono 21px", 20, FString("Properties"));
	if (PropertiesTitle) {
		PropertiesTitle->SetActorLocation(Vector3(PropertiesX + 10.0f, HierarchyY + 8.0f, 0.0f));
		InWorld->AddActor(PropertiesTitle);
		EditorActors.Push(PropertiesTitle);
	}

	PropertiesText = NewObject<ATextActor>("EditorPropertiesText", UITextType::eUI_Text_Type_Bitmap,
		"Ubuntu Mono 21px", 20, FString(" "));
	if (PropertiesText) {
		PropertiesText->SetActorLocation(Vector3(PropertiesX + 10.0f, HierarchyY + 36.0f, 0.0f));
		InWorld->AddActor(PropertiesText);
		EditorActors.Push(PropertiesText);
	}

	// 中央视口标题条（UE5 风格：视口名称 + 视图模式）
	ViewportTitle = NewObject<ATextActor>("EditorViewportTitle", UITextType::eUI_Text_Type_Bitmap,
		"Ubuntu Mono 21px", 20, FString("Viewport | Perspective"));
	if (ViewportTitle) {
		ViewportTitle->SetActorLocation(Vector3(ViewportX + 14.0f, ViewportY + 5.0f, 0.0f));
		InWorld->AddActor(ViewportTitle);
		EditorActors.Push(ViewportTitle);
	}

	// 工具栏按钮标签
	static const char* Labels[4] = { "Save", "Play", "Pause", "Stop" };
	for (int i = 0; i < 4; ++i) {
		ToolbarBtnLabels[i] = NewObject<ATextActor>(
			FString::Format("EditorBtnLabel_%d", i), UITextType::eUI_Text_Type_Bitmap,
			"Ubuntu Mono 21px", 20, FString(Labels[i]));
		if (ToolbarBtnLabels[i]) {
			ToolbarBtnLabels[i]->SetActorLocation(Vector3(
				ToolbarButtons[i].X + ToolbarBtnW * 0.5f - 20.0f,
				ToolbarButtons[i].Y + 6.0f, 0.0f));
			InWorld->AddActor(ToolbarBtnLabels[i]);
			EditorActors.Push(ToolbarBtnLabels[i]);
		}
	}

	// 工具栏状态文本
	ToolbarStatusText = NewObject<ATextActor>("EditorToolbarStatus", UITextType::eUI_Text_Type_Bitmap,
		"Ubuntu Mono 21px", 20, FString("Idle"));
	if (ToolbarStatusText) {
		ToolbarStatusText->SetActorLocation(Vector3(480.0f, 12.0f, 0.0f));
		InWorld->AddActor(ToolbarStatusText);
		EditorActors.Push(ToolbarStatusText);
	}

	// 底部 Log 窗口页签标签（Render / Keyboard / Console）
	static const char* LogTabNames3[3] = { "Render", "Keyboard", "Console" };
	for (int i = 0; i < 3; ++i) {
		LogTabLabels[i] = NewObject<ATextActor>(
			FString::Format("EditorLogTab_%d", i), UITextType::eUI_Text_Type_Bitmap,
			"Ubuntu Mono 21px", 18, FString(LogTabNames3[i]));
		if (LogTabLabels[i]) {
			LogTabLabels[i]->SetActorLocation(Vector3(
				LogX + LogTabX0 + i * LogTabGap + 14.0f,
				LogY + LogTabY + 4.0f, 0.0f));
			InWorld->AddActor(LogTabLabels[i]);
			EditorActors.Push(LogTabLabels[i]);
		}
	}

	// 层级行文本已移入 ScrollBox 内的 Button 列表项（UButtonWidget 自带文字绘制），
	// 不再创建独立 ATextActor 行文本（见 SyncHierarchyList）。

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

	UWorld* WorldPtr = GetWorld();
	if (!WorldPtr || !Owner) {
		return;
	}

	UpdateLayout();
	RebuildHierarchyList(WorldPtr);
	HandleMouseInput();
	HandleKeyboardInput();
	SyncHierarchyList();
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

	// ---- 中央视口矩形（UE5 风格：渲染区居中，四周为 UI 面板） ----
	// 视口容器 = 顶部工具栏下、左右面板之间、底部 Log 上方的剩余区域；
	// 渲染区 = 容器内缩（顶部让出标题条，四周让出边框）。
	// UI 仅绘制容器边框（含标题条），渲染区中央留给 3D 画面，避免不透明 UI 盖住场景。
	const float ContainerX = HierarchyX + HierarchyW + ViewportGap;
	const float ContainerY = HierarchyY;
	const float ContainerW = PropertiesX - ContainerX - ViewportGap;
	const float ContainerH = LogY - ContainerY - ViewportGap;

	if (ContainerW > 0.0f && ContainerH > 0.0f) {
		const float BorderTopH = ViewportTitleBarH;
		const float SideH = ContainerH - ViewportTitleBarH - ViewportBorder;
		if (SideH > 0.0f) {
			// 顶部：标题条 + 上边框（渲染区上方）
			if (ViewportBorderTop) {
				ViewportBorderTop->SetPosition(ContainerX, ContainerY);
				ViewportBorderTop->SetDesiredSize(ContainerW, BorderTopH);
			}
			// 底部：下边框
			if (ViewportBorderBottom) {
				ViewportBorderBottom->SetPosition(ContainerX, ContainerY + ContainerH - ViewportBorder);
				ViewportBorderBottom->SetDesiredSize(ContainerW, ViewportBorder);
			}
			// 左侧边框（标题条之下）
			if (ViewportBorderLeft) {
				ViewportBorderLeft->SetPosition(ContainerX, ContainerY + ViewportTitleBarH);
				ViewportBorderLeft->SetDesiredSize(ViewportBorder, SideH);
			}
			// 右侧边框（标题条之下）
			if (ViewportBorderRight) {
				ViewportBorderRight->SetPosition(ContainerX + ContainerW - ViewportBorder, ContainerY + ViewportTitleBarH);
				ViewportBorderRight->SetDesiredSize(ViewportBorder, SideH);
			}
		}
	}

	if (ContainerW > 100.0f && ContainerH > 100.0f) {
		ViewportX = ContainerX + ViewportBorder;
		ViewportY = ContainerY + ViewportTitleBarH;
		ViewportW = ContainerW - ViewportBorder * 2.0f;
		ViewportH = ContainerH - ViewportTitleBarH - ViewportBorder;

		if (ViewportTitle) {
			ViewportTitle->SetActorLocation(Vector3(ContainerX + 14.0f, ContainerY + 5.0f, 0.0f));
		}

		// 推送视口矩形给世界渲染视图（Skybox / Deferred），使 3D 渲染恰好落在中央区域
		RenderViewSystem::Get().SetWorldViewportRect(Vector4(ViewportX, ViewportY, ViewportW, ViewportH));

		// 同步相机宽高比（视锥剔除），保持与视口一致
		ACameraActor* MainCam = CameraSystem::Get().GetMainCamera();
		if (MainCam && MainCam->GetCameraComponent()) {
			MainCam->GetCameraComponent()->SetAspectRatio(ViewportW / ViewportH);
		}
	}
}

void ASimpleEditorActor::RebuildHierarchyList(UWorld* InWorld) {
	DisplayActors.Clear();
	if (!InWorld) {
		return;
	}

	const TArray<AActor*>& WorldActors = InWorld->GetWorldActors();
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

void ASimpleEditorActor::SyncHierarchyList() {
	const int N = (int)DisplayActors.Size();
	if (!HierarchyScrollBox) {
		return;
	}

	// ---- ScrollBox：同步内容数量（滚动范围随内容行数变化） ----
	HierarchyScrollBox->SetContentCount(N);
	HierarchyScrollBox->SetRowStep(RowHeight + RowGap);
	HierarchyScrollBox->SetRowHeight(RowHeight);
	HierarchyScrollBox->LayoutChildren();

	// ---- 字体懒加载（与 ATextActor 同一字体族） ----
	// [FIX] 中文 Actor 名渲染：使用含 CJK 字形的位图字体 Noto Sans 21px
	//（Bitmap 渲染路径与 Ubuntu Mono 相同已验证正常；System 字体 variant 渲染存在缺陷）
	if (!HierarchyFont) {
		HierarchyFont = FontSystem::Get().Acquire("Noto Sans 21px",
			UITextType::eUI_Text_Type_Bitmap, 21);
	}

	// ---- 按钮池：不足时创建（每个按钮绑定固定行索引的回调） ----
	while ((int)HierarchyButtons.Size() < N) {
		UButtonWidget* Btn = new UButtonWidget();
		Btn->SetDesiredSize(-1.0f, RowHeight);
		Btn->SetFont(HierarchyFont);
		Btn->SetTextColor(Vector4(0.82f, 0.84f, 0.88f, 1.0f));
		const int RowIndex = (int)HierarchyButtons.Size();
		// OnClick 回调：驱动选中高亮 + 右侧 Properties 面板刷新
		Btn->SetOnClick([this, RowIndex](UButtonWidget* ClickedBtn) {
			OnHierarchyButtonClicked(RowIndex, ClickedBtn);
		});
		HierarchyScrollBox->AddChild(Btn);
		HierarchyButtons.Push(Btn);
	}

	// ---- 按钮池收缩：超出内容数量的按钮隐藏 ----
	for (int i = N; i < (int)HierarchyButtons.Size(); ++i) {
		HierarchyButtons[i]->SetVisible(false);
	}

	// ---- 同步行文本与选中高亮（选中态由 Button 自身底色呈现） ----
	for (int i = 0; i < N; ++i) {
		UButtonWidget* Btn = HierarchyButtons[i];
		if (!Btn) {
			continue;
		}
		Btn->SetVisible(true);
		Btn->SetText(DisplayActors[i]->GetName());
		Btn->SetSelected(i == SelectedRow);
	}

	// 滚动裁剪由 UScrollBoxWidget::OnDraw 处理：只绘制落在可视矩形内的子项，
	// 超出的行不绘制，天然不与下方 Log 窗口重叠。

	// [诊断] 每 120 帧写一次按钮池/ScrollBox 状态到中间产物目录，验证列表构建与布局
	++DiagFrame;
	if ((DiagFrame % 120) == 0) {
		FILE* F = nullptr;
		fopen_s(&F, "C:/Users/dongyf/AppData/Roaming/Tencent/Marvis/User/oAN1i2WRMFSrJ52wfk3jOkgo9DVc/workspace/conv_d5042d99ddef4e598969646c91ea7532/temp/hierarchy_diag.txt", "w");
		if (F) {
			fprintf(F, "N=%d SelectedRow=%d\n", N, SelectedRow);
			fprintf(F, "ScrollBox: children=%d pos=(%.1f,%.1f) size=(%.1f,%.1f) visible=%d scroll=%.1f max=%.1f\n",
				(int)HierarchyScrollBox->GetChildren().Size(),
				HierarchyScrollBox->GetPositionX(), HierarchyScrollBox->GetPositionY(),
				HierarchyScrollBox->GetWidth(), HierarchyScrollBox->GetHeight(),
				HierarchyScrollBox->IsVisible() ? 1 : 0,
				HierarchyScrollBox->GetScrollOffset(), HierarchyScrollBox->GetMaxScrollOffset());
			for (int i = 0; i < N && i < 5; ++i) {
				UButtonWidget* Btn = HierarchyButtons[i];
				if (Btn) {
					fprintf(F, "Btn[%d] text=%s pos=(%.1f,%.1f) size=(%.1f,%.1f) visible=%d sel=%d font=%d\n",
						i, Btn->GetText().CStr(), Btn->GetPositionX(), Btn->GetPositionY(),
						Btn->GetWidth(), Btn->GetHeight(), Btn->IsVisible() ? 1 : 0,
						Btn->IsSelected() ? 1 : 0, Btn->GetFont() ? 1 : 0);
				}
			}
			fclose(F);
		}
	}
}

void ASimpleEditorActor::OnHierarchyButtonClicked(int Row, UButtonWidget* Btn) {
	FILE* T = nullptr;
	fopen_s(&T, "C:/Users/dongyf/AppData/Roaming/Tencent/Marvis/User/oAN1i2WRMFSrJ52wfk3jOkgo9DVc/workspace/conv_d5042d99ddef4e598969646c91ea7532/temp/click_trace.txt", "w");
	if (T) {
		fprintf(T, "OnHierarchyButtonClicked Row=%d Btn=%p DisplayActors.Size=%d\n", Row, (void*)Btn, (int)DisplayActors.Size());
		fclose(T);
	}
	if (Row < 0 || Row >= (int)DisplayActors.Size()) {
		return;
	}

	// 1) 更新选中状态
	SelectedActor = DisplayActors[Row];
	SelectedRow = Row;
	if (T) {
		fopen_s(&T, "C:/Users/dongyf/AppData/Roaming/Tencent/Marvis/User/oAN1i2WRMFSrJ52wfk3jOkgo9DVc/workspace/conv_d5042d99ddef4e598969646c91ea7532/temp/click_trace.txt", "a");
		fprintf(T, "step1 selected=%s\n", SelectedActor ? SelectedActor->GetName().CStr() : "(null)");
		fclose(T);
	}

	// 2) 通过 Button 选中态刷新高亮（替代旧 SelectionBar 矩形）
	for (int i = 0; i < (int)HierarchyButtons.Size(); ++i) {
		if (HierarchyButtons[i]) {
			HierarchyButtons[i]->SetSelected(i == Row);
		}
	}

	// 3) 回调驱动右侧 Properties 面板内容刷新
	RefreshPropertiesText();
	if (T) {
		fopen_s(&T, "C:/Users/dongyf/AppData/Roaming/Tencent/Marvis/User/oAN1i2WRMFSrJ52wfk3jOkgo9DVc/workspace/conv_d5042d99ddef4e598969646c91ea7532/temp/click_trace.txt", "a");
		fprintf(T, "done\n");
		fclose(T);
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

	// ---- Hierarchy ScrollBox：滚轮在层级面板上时滚动内容 ----
	// 滚轮增量由 GameOnWheel 事件回调写入 Owner->WheelDelta（相机 dolly 用）。
	// 此处抢先消费并清零，GameInstance::Update 中相机 dolly 便不再响应，
	// 实现「Hierarchy 区域滚轮 → 滚动层级；中央视口滚轮 → 相机缩放」的路由区分。
	if (Owner && Owner->WheelDelta != 0 &&
		IsPointInRect((float)mx, (float)my, HierarchyX, HierarchyY, HierarchyW, HierarchyPanelH)) {
		const float RowStep = RowHeight + RowGap;
		// UE 语义：滚轮向上（WheelDelta>0）内容向下移动（显示更靠前的行），
		// 滚轮向下（WheelDelta<0）内容向上移动（显示更靠后的行）。
		// ScrollBox::ScrollBy 的 Delta>0 表示向下滚动，故对 WheelDelta 取负。
		if (HierarchyScrollBox) {
			HierarchyScrollBox->ScrollBy((float)(-Owner->WheelDelta) * RowStep);
		}
		Owner->WheelDelta = 0;
	}

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

		// 3) 层级面板点击：走 Widget 树输入分发（Button OnClick 回调驱动选中与高亮）。
		//    命中/裁剪完全由 Widget 树自身矩形与可见性决定，替代旧的
		//    「数组下标 + 固定坐标」行命中；滚动后点击不再错位。
		if (!bHandled && (float)my >= HierarchyRowStartY &&
			IsPointInRect((float)mx, (float)my, HierarchyX, HierarchyY, HierarchyW, HierarchyPanelH)) {
			if (HierarchyPanel && HierarchyPanel->DispatchMouseClick((float)mx, (float)my)) {
				// Button 命中：OnClick 回调已更新选中状态 / 高亮 / 右侧属性刷新
			}
			else {
				// 点击面板空白（行间空隙 / 无内容区）：取消选中
				SelectedActor = nullptr;
				SelectedRow = -1;
				RefreshPropertiesText();
			}
			bHandled = true;
		}

		// 4) 中央视口空白点击 → 取消选中。
		//    与 UE5 一致：仅点击渲染视口区域内的空白才清空选中；
		//    点击四周 UI 面板（工具栏/层级/属性/Log）不会误取消选中。
		if (!bHandled) {
			if (IsPointInRect((float)mx, (float)my, ViewportX, ViewportY, ViewportW, ViewportH)) {
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
