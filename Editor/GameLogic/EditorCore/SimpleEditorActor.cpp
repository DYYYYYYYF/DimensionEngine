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
	: AActor(Name), Owner(InOwner) {
	// UI 鏋舵瀯閲嶆瀯锛圓UIActor/UUIWidgetComponent -> UWidget/AActor 瀹夸富锛夊悗锛?
	// AActor 榛樿涓嶅惎鐢?Tick锛屽繀椤绘樉寮忓紑鍚細甯冨眬鍒锋柊锛圲pdateLayout ->
	// SetWorldViewportRect锛夈€佸眰绾?灞炴€?鏃ュ織/浜や簰鍏ㄩ儴渚濊禆姣忓抚 Tick 椹卞姩銆?
	SetEnableTick(true);
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

ASimpleEditorActor::~ASimpleEditorActor() {
	// 浠?World UI 鏍瑰垪琛ㄧЩ闄ゅ苟娉ㄩ攢娓叉煋浠ｇ悊锛圵orld 鍙兘宸茶閿€姣侊紝闇€鍒ょ┖锛?
	if (UWorld* WorldPtr = GetWorld()) {
		WorldPtr->RemoveUIWidgetRoot(RootPanel);
	}

	// Widget 鏍戞墍鏈夋潈褰掓湰 Actor锛圲Widget 鏋愭瀯閫掑綊娓呯悊瀛愯妭鐐癸級
	if (RootPanel) {
		delete RootPanel;
		RootPanel = nullptr;
	}
}

void ASimpleEditorActor::BuildWidgetTree() {
	RootPanel = new UPanelWidget();
	RootPanel->SetDesiredSize(-1.0f, -1.0f);

	// 椤堕儴宸ュ叿鏍忚儗鏅紙鍗婇€忔槑娣辫壊搴曟澘锛屼笌 3D 鍦烘櫙鍒嗙锛?
	ToolbarBG = new UImageWidget();
	ToolbarBG->SetDesiredSize(WindowWidth, ToolbarHeight);
	ToolbarBG->SetPosition(0.0f, 0.0f);
	ToolbarBG->SetColor(Vector4(0.10f, 0.11f, 0.13f, 0.92f));
	RootPanel->AddChild(ToolbarBG);

	// 宸ュ叿鏍忔寜閽紙Save / Play / Pause / Stop锛?
	for (int i = 0; i < 4; ++i) {
		UImageWidget* Btn = new UImageWidget();
		Btn->SetDesiredSize(ToolbarBtnW, ToolbarBtnH);
		Btn->SetPosition(ToolbarBtnX0 + i * ToolbarBtnGap, ToolbarBtnY);
		Btn->SetColor(Vector4(0.20f, 0.22f, 0.26f, 1.0f));
		RootPanel->AddChild(Btn);
		ToolbarButtonWidgets.Push(Btn);
		ToolbarButtons[i] = { ToolbarBtnX0 + i * ToolbarBtnGap, ToolbarBtnY, ToolbarBtnW, ToolbarBtnH, i };
	}

	// ---- 宸︿晶瀵硅薄鍒楄〃锛圲E 椋庢牸閲嶆瀯锛夛細閿氱偣 Panel 鍥哄畾宸︿晶 鈫?鍐呭 ScrollBox 鈫?Button 鍒楄〃椤?----
	// 1) 閿氱偣闈㈡澘锛欰nchorMin=(0,0) AnchorMax=(0,1) 閿氬畾灞忓箷宸︿晶骞跺瀭鐩存媺浼革紱
	//    Offsets=(Left=HierarchyX, Top=HierarchyY, Right=HierarchyX+HierarchyW,
	//    Bottom=-(LogPanelH+10))锛氬簳杈圭浉瀵圭埗搴曢儴涓婄Щ Log 绐楀彛 + 闂撮殭楂樺害锛?
	//    淇濊瘉闈㈡澘搴曞缁堜綅浜?Log 涓婃柟锛屽ぉ鐒朵笉涓?Log 閲嶅彔銆?
	HierarchyPanel = new UAnchorPanelWidget();
	HierarchyPanel->SetAnchors(Vector2(0.0f, 0.0f), Vector2(0.0f, 1.0f));
	HierarchyPanel->SetOffsets(HierarchyX, HierarchyY, HierarchyX + HierarchyW, -(LogPanelH + 10.0f));
	RootPanel->AddChild(HierarchyPanel);

	// 2) 闈㈡澘鑳屾櫙锛堥摵婊￠敋鐐归潰鏉匡級
	HierarchyBG = new UImageWidget();
	HierarchyBG->SetDesiredSize(-1.0f, -1.0f);
	HierarchyBG->SetColor(Vector4(0.07f, 0.08f, 0.10f, 0.92f));
	HierarchyPanel->AddChild(HierarchyBG);

	// 3) ScrollBox锛氫綅浜庨潰鏉挎爣棰橈紙30px锛変箣涓嬶紝绠＄悊婊氬姩鍋忕Щ涓庡彲瑙嗚鍓?
	HierarchyScrollBox = new UScrollBoxWidget();
	HierarchyScrollBox->SetContentStartY(30.0f);
	HierarchyScrollBox->SetContentMarginX(2.0f);
	HierarchyScrollBox->SetRowStep(RowHeight + RowGap);
	HierarchyScrollBox->SetRowHeight(RowHeight);
	HierarchyPanel->AddChild(HierarchyScrollBox);

	// 灞炴€ч潰鏉胯儗鏅紙鍗婇€忔槑娣辫壊搴曟澘锛?
	PropertiesBG = new UImageWidget();
	PropertiesBG->SetPosition(PropertiesX, HierarchyY);
	PropertiesBG->SetDesiredSize(PropertiesW, HierarchyPanelH);
	PropertiesBG->SetColor(Vector4(0.07f, 0.08f, 0.10f, 0.92f));
	RootPanel->AddChild(PropertiesBG);

	// 搴曢儴 Log 绐楀彛鑳屾櫙锛堢被浼?UE Log/Debug 杈撳嚭绐楀彛锛?
	LogBG = new UImageWidget();
	LogBG->SetDesiredSize(LogW, LogPanelH);
	LogBG->SetPosition(LogX, LogY);
	LogBG->SetColor(Vector4(0.05f, 0.06f, 0.08f, 0.92f));
	RootPanel->AddChild(LogBG);

	// 搴曢儴 Log 绐楀彛椤电锛圧ender / Keyboard / Console锛?
	for (int i = 0; i < 3; ++i) {
		UImageWidget* Tab = new UImageWidget();
		Tab->SetDesiredSize(LogTabW, LogTabH);
		Tab->SetPosition(LogX + LogTabX0 + i * LogTabGap, LogY + LogTabY);
		Tab->SetColor(Vector4(0.16f, 0.18f, 0.22f, 0.92f));
		RootPanel->AddChild(Tab);
		LogTabWidgets[i] = Tab;
	}

	// 涓ぎ瑙嗗彛瀹瑰櫒锛圲E5 椋庢牸锛氳鍙ｅ眳涓紝鍥涘懆涓?UI 闈㈡澘锛夈€?
	// 浣嶇疆/灏哄鐢?UpdateLayout 姣忓抚鎸夌獥鍙ｉ噸绠楋紙姝ゅ鍏堟寜鍒濆绐楀彛閾烘弧涓ぎ鍖哄煙锛夈€?
	// 娉ㄦ剰锛氫粎缁樺埗鏍囬鏉′笌鍥涜竟妗嗭紝娓叉煋鍖猴紙ViewportX/Y/W/H锛夊唴涓嶆斁缃换浣?UI 鐭╁舰锛?
	// 鍚﹀垯涓嶉€忔槑 UI 灞備細鐩栦綇涓嬫柟 Skybox/Deferred 鐨?3D 娓叉煋缁撴灉锛堣〃鐜颁负榛戝睆锛夈€?
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

	// 娉細RootPanel 鐢?CreateEditorOverlayTexts 娉ㄥ唽杩?World UI 鏍瑰垪琛紝
	// 鐢?UWorld 姣忓抚椹卞姩 娴嬮噺/甯冨眬/缁樺埗 骞舵彁浜ゆ覆鏌撱€?
}

void ASimpleEditorActor::CreateEditorOverlayTexts(UWorld* InWorld) {
	if (!InWorld) {
		return;
	}

	// 娉ㄥ唽 UI 鏍瑰埌 World锛歎World 姣忓抚椹卞姩 Widget 鏍戠敓鍛藉懆鏈熷苟鎻愪氦娓叉煋浠ｇ悊
	InWorld->AddUIWidgetRoot(RootPanel);

	// 灞傜骇闈㈡澘鏍囬
	// [TEST] 涓枃娓叉煋楠岃瘉锛歎TF-8 瀛楄妭杞箟锛堟簮鐮佸瓧闈㈤噺浼氱紪璇戜负 GBK 鎵ц瀛楃闆嗭級
	HierarchyTitle = NewObject<ATextActor>("EditorHierarchyTitle", UITextType::eUI_Text_Type_Bitmap,
		"Noto Sans 21px", 21, FString("\xe5\xb1\x82\xe7\xba\xa7\xe5\x88\x97\xe8\xa1\xa8 Hierarchy"));
	if (HierarchyTitle) {
		HierarchyTitle->SetActorLocation(Vector3(HierarchyX + 10.0f, HierarchyY + 6.0f, 0.0f));
		InWorld->AddActor(HierarchyTitle);
		EditorActors.Push(HierarchyTitle);
	}

	// 灞炴€ч潰鏉挎爣棰樹笌鍐呭
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

	// 涓ぎ瑙嗗彛鏍囬鏉★紙UE5 椋庢牸锛氳鍙ｅ悕绉?+ 瑙嗗浘妯″紡锛?
	ViewportTitle = NewObject<ATextActor>("EditorViewportTitle", UITextType::eUI_Text_Type_Bitmap,
		"Ubuntu Mono 21px", 20, FString("Viewport | Perspective"));
	if (ViewportTitle) {
		ViewportTitle->SetActorLocation(Vector3(ViewportX + 14.0f, ViewportY + 5.0f, 0.0f));
		InWorld->AddActor(ViewportTitle);
		EditorActors.Push(ViewportTitle);
	}

	// 宸ュ叿鏍忔寜閽爣绛?
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

	// 宸ュ叿鏍忕姸鎬佹枃鏈?
	ToolbarStatusText = NewObject<ATextActor>("EditorToolbarStatus", UITextType::eUI_Text_Type_Bitmap,
		"Ubuntu Mono 21px", 20, FString("Idle"));
	if (ToolbarStatusText) {
		ToolbarStatusText->SetActorLocation(Vector3(480.0f, 12.0f, 0.0f));
		InWorld->AddActor(ToolbarStatusText);
		EditorActors.Push(ToolbarStatusText);
	}

	// 搴曢儴 Log 绐楀彛椤电鏍囩锛圧ender / Keyboard / Console锛?
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

	// 灞傜骇琛屾枃鏈凡绉诲叆 ScrollBox 鍐呯殑 Button 鍒楄〃椤癸紙UButtonWidget 鑷甫鏂囧瓧缁樺埗锛夛紝
	// 涓嶅啀鍒涘缓鐙珛 ATextActor 琛屾枃鏈紙瑙?SyncHierarchyList锛夈€?

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
	// 蹇呴』璋冪敤鍩虹被 Tick锛欰Actor::Tick 浼氶┍鍔ㄧ粍浠?Tick銆?
	// UI 鏍圭敱 UWorld 姣忓抚鐩存帴椹卞姩锛堟祴閲?甯冨眬/缁樺埗/鎻愪氦浠ｇ悊锛夛紝涓嶅啀缁忕粍浠惰浆鍙戙€?
	AActor::Tick(DeltaTime);

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

	// ---- 涓ぎ瑙嗗彛鐭╁舰锛圲E5 椋庢牸锛氭覆鏌撳尯灞呬腑锛屽洓鍛ㄤ负 UI 闈㈡澘锛?----
	// 瑙嗗彛瀹瑰櫒 = 椤堕儴宸ュ叿鏍忎笅銆佸乏鍙抽潰鏉夸箣闂淬€佸簳閮?Log 涓婃柟鐨勫墿浣欏尯鍩燂紱
	// 娓叉煋鍖?= 瀹瑰櫒鍐呯缉锛堥《閮ㄨ鍑烘爣棰樻潯锛屽洓鍛ㄨ鍑鸿竟妗嗭級銆?
	// UI 浠呯粯鍒跺鍣ㄨ竟妗嗭紙鍚爣棰樻潯锛夛紝娓叉煋鍖轰腑澶暀缁?3D 鐢婚潰锛岄伩鍏嶄笉閫忔槑 UI 鐩栦綇鍦烘櫙銆?
	const float ContainerX = HierarchyX + HierarchyW + ViewportGap;
	const float ContainerY = HierarchyY;
	const float ContainerW = PropertiesX - ContainerX - ViewportGap;
	const float ContainerH = LogY - ContainerY - ViewportGap;

	if (ContainerW > 0.0f && ContainerH > 0.0f) {
		const float BorderTopH = ViewportTitleBarH;
		const float SideH = ContainerH - ViewportTitleBarH - ViewportBorder;
		if (SideH > 0.0f) {
			// 椤堕儴锛氭爣棰樻潯 + 涓婅竟妗嗭紙娓叉煋鍖轰笂鏂癸級
			if (ViewportBorderTop) {
				ViewportBorderTop->SetPosition(ContainerX, ContainerY);
				ViewportBorderTop->SetDesiredSize(ContainerW, BorderTopH);
			}
			// 搴曢儴锛氫笅杈规
			if (ViewportBorderBottom) {
				ViewportBorderBottom->SetPosition(ContainerX, ContainerY + ContainerH - ViewportBorder);
				ViewportBorderBottom->SetDesiredSize(ContainerW, ViewportBorder);
			}
			// 宸︿晶杈规锛堟爣棰樻潯涔嬩笅锛?
			if (ViewportBorderLeft) {
				ViewportBorderLeft->SetPosition(ContainerX, ContainerY + ViewportTitleBarH);
				ViewportBorderLeft->SetDesiredSize(ViewportBorder, SideH);
			}
			// 鍙充晶杈规锛堟爣棰樻潯涔嬩笅锛?
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

		// 鎺ㄩ€佽鍙ｇ煩褰㈢粰涓栫晫娓叉煋瑙嗗浘锛圫kybox / Deferred锛夛紝浣?3D 娓叉煋鎭板ソ钀藉湪涓ぎ鍖哄煙
		RenderViewSystem::Get().SetWorldViewportRect(Vector4(ViewportX, ViewportY, ViewportW, ViewportH));

		// 鍚屾鐩告満瀹介珮姣旓紙瑙嗛敟鍓旈櫎锛夛紝淇濇寔涓庤鍙ｄ竴鑷?
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

	// 閫変腑 actor 鍙兘宸茶鍦烘櫙鍒囨崲鍒犻櫎 鈫?澶辨晥鏃舵竻绌?
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

	// ---- ScrollBox锛氬悓姝ュ唴瀹规暟閲忥紙婊氬姩鑼冨洿闅忓唴瀹硅鏁板彉鍖栵級 ----
	HierarchyScrollBox->SetContentCount(N);
	HierarchyScrollBox->SetRowStep(RowHeight + RowGap);
	HierarchyScrollBox->SetRowHeight(RowHeight);
	HierarchyScrollBox->LayoutChildren();

	// ---- 瀛椾綋鎳掑姞杞斤紙涓?ATextActor 鍚屼竴瀛椾綋鏃忥級 ----
	// [FIX] 涓枃 Actor 鍚嶆覆鏌擄細浣跨敤鍚?CJK 瀛楀舰鐨勪綅鍥惧瓧浣?Noto Sans 21px
	//锛圔itmap 娓叉煋璺緞涓?Ubuntu Mono 鐩稿悓宸查獙璇佹甯革紱System 瀛椾綋 variant 娓叉煋瀛樺湪缂洪櫡锛?
	if (!HierarchyFont) {
		HierarchyFont = FontSystem::Get().Acquire("Noto Sans 21px",
			UITextType::eUI_Text_Type_Bitmap, 21);
	}

	// ---- 鎸夐挳姹狅細涓嶈冻鏃跺垱寤猴紙姣忎釜鎸夐挳缁戝畾鍥哄畾琛岀储寮曠殑鍥炶皟锛?----
	while ((int)HierarchyButtons.Size() < N) {
		UButtonWidget* Btn = new UButtonWidget();
		Btn->SetDesiredSize(-1.0f, RowHeight);
		Btn->SetFont(HierarchyFont);
		Btn->SetTextColor(Vector4(0.82f, 0.84f, 0.88f, 1.0f));
		const int RowIndex = (int)HierarchyButtons.Size();
		// OnClick 鍥炶皟锛氶┍鍔ㄩ€変腑楂樹寒 + 鍙充晶 Properties 闈㈡澘鍒锋柊
		Btn->SetOnClick([this, RowIndex](UButtonWidget* ClickedBtn) {
			OnHierarchyButtonClicked(RowIndex, ClickedBtn);
		});
		HierarchyScrollBox->AddChild(Btn);
		HierarchyButtons.Push(Btn);
	}

	// ---- 鎸夐挳姹犳敹缂╋細瓒呭嚭鍐呭鏁伴噺鐨勬寜閽殣钘?----
	for (int i = N; i < (int)HierarchyButtons.Size(); ++i) {
		HierarchyButtons[i]->SetVisible(false);
	}

	// ---- 鍚屾琛屾枃鏈笌閫変腑楂樹寒锛堥€変腑鎬佺敱 Button 鑷韩搴曡壊鍛堢幇锛?----
	for (int i = 0; i < N; ++i) {
		UButtonWidget* Btn = HierarchyButtons[i];
		if (!Btn) {
			continue;
		}
		Btn->SetVisible(true);
		Btn->SetText(DisplayActors[i]->GetName());
		Btn->SetSelected(i == SelectedRow);
	}

	// 婊氬姩瑁佸壀鐢?UScrollBoxWidget::OnDraw 澶勭悊锛氬彧缁樺埗钀藉湪鍙鐭╁舰鍐呯殑瀛愰」锛?
	// 瓒呭嚭鐨勮涓嶇粯鍒讹紝澶╃劧涓嶄笌涓嬫柟 Log 绐楀彛閲嶅彔銆?

	// [璇婃柇] 姣?120 甯у啓涓€娆℃寜閽睜/ScrollBox 鐘舵€佸埌涓棿浜х墿鐩綍锛岄獙璇佸垪琛ㄦ瀯寤轰笌甯冨眬
	++DiagFrame;
	if (true) {
		FILE* F = nullptr;
		fopen_s(&F, "D:/dongyf/DimensionEngine/Bin/hierarchy_diag.txt", "w");
		if (F) {
			fprintf(F, "N=%d SelectedRow=%d\n", N, SelectedRow);
			fprintf(F, "ScrollBox: children=%d pos=(%.1f,%.1f) size=(%.1f,%.1f) visible=%d scroll=%.1f max=%.1f\n",
				(int)HierarchyScrollBox->GetChildren().Size(),
				HierarchyScrollBox->GetPositionX(), HierarchyScrollBox->GetPositionY(),
				HierarchyScrollBox->GetWidth(), HierarchyScrollBox->GetHeight(),
				HierarchyScrollBox->IsVisible() ? 1 : 0,
				HierarchyScrollBox->GetScrollOffset(), HierarchyScrollBox->GetMaxScrollOffset());
			// [FIX-DIAG] Children 槽位 -> 按钮池索引映射：验证绘制顺序与命中顺序是否错位
			fprintf(F, "ChildrenMap: ");
			for (size_t k = 0; k < HierarchyScrollBox->GetChildren().Size() && k < 8; ++k) {
				UWidget* C = HierarchyScrollBox->GetChildren()[k];
				int BtnIdx = -1;
				for (int b = 0; b < (int)HierarchyButtons.Size(); ++b) {
					if (C == HierarchyButtons[b]) { BtnIdx = b; break; }
				}
				fprintf(F, "[%d]=%s ", (int)k, BtnIdx >= 0 ? "Btn%d" : "other");
				if (BtnIdx >= 0) {
					fprintf(F, "(idx=%d) ", BtnIdx);
				}
			}
			fprintf(F, "\n");
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
	fopen_s(&T, "D:/dongyf/DimensionEngine/Bin/click_trace.txt", "w");
	if (T) {
		fprintf(T, "OnHierarchyButtonClicked Row=%d Btn=%p DisplayActors.Size=%d\n", Row, (void*)Btn, (int)DisplayActors.Size());
		fclose(T);
	}
	if (Row < 0 || Row >= (int)DisplayActors.Size()) {
		return;
	}

	// 1) 鏇存柊閫変腑鐘舵€?
	SelectedActor = DisplayActors[Row];
	SelectedRow = Row;
	if (T) {
		fopen_s(&T, "D:/dongyf/DimensionEngine/Bin/click_trace.txt", "a");
		fprintf(T, "step1 selected=%s\n", SelectedActor ? SelectedActor->GetName().CStr() : "(null)");
		fclose(T);
	}

	// 2) 閫氳繃 Button 閫変腑鎬佸埛鏂伴珮浜紙鏇夸唬鏃?SelectionBar 鐭╁舰锛?
	for (int i = 0; i < (int)HierarchyButtons.Size(); ++i) {
		if (HierarchyButtons[i]) {
			HierarchyButtons[i]->SetSelected(i == Row);
		}
	}

	// 3) 鍥炶皟椹卞姩鍙充晶 Properties 闈㈡澘鍐呭鍒锋柊
	RefreshPropertiesText();
	if (T) {
		fopen_s(&T, "D:/dongyf/DimensionEngine/Bin/click_trace.txt", "a");
		fprintf(T, "done\n");
		fprintf(T, "ClickSnapshot Row=%d BtnPos=(%.1f,%.1f) BtnSize=(%.1f,%.1f)\n",
			Row, Btn->GetPositionX(), Btn->GetPositionY(), Btn->GetWidth(), Btn->GetHeight());
		for (int i = 0; i < (int)HierarchyButtons.Size() && i < 8; ++i) {
			UButtonWidget* B = HierarchyButtons[i];
			if (B) {
				fprintf(T, "  Btn[%d] pos=(%.1f,%.1f) size=(%.1f,%.1f) vis=%d sel=%d text=%s\n",
					i, B->GetPositionX(), B->GetPositionY(), B->GetWidth(), B->GetHeight(),
					B->IsVisible() ? 1 : 0, B->IsSelected() ? 1 : 0, B->GetText().CStr());
			}
		}
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

	// ---- Hierarchy ScrollBox锛氭粴杞湪灞傜骇闈㈡澘涓婃椂婊氬姩鍐呭 ----
	// 婊氳疆澧為噺鐢?GameOnWheel 浜嬩欢鍥炶皟鍐欏叆 Owner->WheelDelta锛堢浉鏈?dolly 鐢級銆?
	// 姝ゅ鎶㈠厛娑堣垂骞舵竻闆讹紝GameInstance::Update 涓浉鏈?dolly 渚夸笉鍐嶅搷搴旓紝
	// 瀹炵幇銆孒ierarchy 鍖哄煙婊氳疆 鈫?婊氬姩灞傜骇锛涗腑澶鍙ｆ粴杞?鈫?鐩告満缂╂斁銆嶇殑璺敱鍖哄垎銆?
	if (Owner && Owner->WheelDelta != 0 &&
		IsPointInRect((float)mx, (float)my, HierarchyX, HierarchyY, HierarchyW, HierarchyPanelH)) {
		const float RowStep = RowHeight + RowGap;
		// UE 璇箟锛氭粴杞悜涓婏紙WheelDelta>0锛夊唴瀹瑰悜涓嬬Щ鍔紙鏄剧ず鏇撮潬鍓嶇殑琛岋級锛?
		// 婊氳疆鍚戜笅锛圵heelDelta<0锛夊唴瀹瑰悜涓婄Щ鍔紙鏄剧ず鏇撮潬鍚庣殑琛岋級銆?
		// ScrollBox::ScrollBy 鐨?Delta>0 琛ㄧず鍚戜笅婊氬姩锛屾晠瀵?WheelDelta 鍙栬礋銆?
		if (HierarchyScrollBox) {
			HierarchyScrollBox->ScrollBy((float)(-Owner->WheelDelta) * RowStep);
		}
		Owner->WheelDelta = 0;
	}

	if (bLeft && !bPrevLeftDown) {
		bool bHandled = false;

		// 1) 宸ュ叿鏍忔寜閽?
		for (int i = 0; i < 4; ++i) {
			const ToolbarButtonRect& B = ToolbarButtons[i];
			if (IsPointInRect((float)mx, (float)my, B.X, B.Y, B.W, B.H)) {
				HandleToolbarClick(B.Id);
				bHandled = true;
				break;
			}
		}

		// 2) 搴曢儴 Log 绐楀彛椤电
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

		// 3) 灞傜骇闈㈡澘鐐瑰嚮锛氳蛋 Widget 鏍戣緭鍏ュ垎鍙戯紙Button OnClick 鍥炶皟椹卞姩閫変腑涓庨珮浜級銆?
		//    鍛戒腑/瑁佸壀瀹屽叏鐢?Widget 鏍戣嚜韬煩褰笌鍙鎬у喅瀹氾紝鏇夸唬鏃х殑
		//    銆屾暟缁勪笅鏍?+ 鍥哄畾鍧愭爣銆嶈鍛戒腑锛涙粴鍔ㄥ悗鐐瑰嚮涓嶅啀閿欎綅銆?
		if (!bHandled && (float)my >= HierarchyRowStartY &&
			IsPointInRect((float)mx, (float)my, HierarchyX, HierarchyY, HierarchyW, HierarchyPanelH)) {
			if (HierarchyPanel && HierarchyPanel->DispatchMouseClick((float)mx, (float)my)) {
				// Button 鍛戒腑锛歄nClick 鍥炶皟宸叉洿鏂伴€変腑鐘舵€?/ 楂樹寒 / 鍙充晶灞炴€у埛鏂?
			}
			else {
				// 鐐瑰嚮闈㈡澘绌虹櫧锛堣闂寸┖闅?/ 鏃犲唴瀹瑰尯锛夛細鍙栨秷閫変腑
				SelectedActor = nullptr;
				SelectedRow = -1;
				RefreshPropertiesText();
			}
			bHandled = true;
		}

		// 4) 涓ぎ瑙嗗彛绌虹櫧鐐瑰嚮 鈫?鍙栨秷閫変腑銆?
		//    涓?UE5 涓€鑷达細浠呯偣鍑绘覆鏌撹鍙ｅ尯鍩熷唴鐨勭┖鐧芥墠娓呯┖閫変腑锛?
		//    鐐瑰嚮鍥涘懆 UI 闈㈡澘锛堝伐鍏锋爮/灞傜骇/灞炴€?Log锛変笉浼氳鍙栨秷閫変腑銆?
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
	// Tab锛氬惊鐜€変腑
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

	// BackSpace锛氬彇娑堥€変腑
	const bool bBS = Controller::IsKeyDown(eKeys::BackSpace);
	if (bBS && !bPrevBackSpaceDown) {
		SelectedActor = nullptr;
		SelectedRow = -1;
	}
	bPrevBackSpaceDown = bBS;

	// 灞炴€х被鍒?
	if (Controller::IsKeyDown(eKeys::Num_1)) {
		EditCategory = EditorEditCategory::Location;
	}
	if (Controller::IsKeyDown(eKeys::Num_2)) {
		EditCategory = EditorEditCategory::Rotation;
	}
	if (Controller::IsKeyDown(eKeys::Num_3)) {
		EditCategory = EditorEditCategory::Scale;
	}
	// 灞炴€ц酱
	if (Controller::IsKeyDown(eKeys::X)) {
		EditAxis = 0;
	}
	if (Controller::IsKeyDown(eKeys::Y)) {
		EditAxis = 1;
	}
	if (Controller::IsKeyDown(eKeys::Z)) {
		EditAxis = 2;
	}

	// 鏁板€艰皟鏁?
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
		// 椤电楂樹寒锛氭縺娲婚〉钃濊壊锛岄潪婵€娲婚〉娣辩伆
		if (LogTabWidgets[i]) {
			LogTabWidgets[i]->SetColor(i == LogActiveTab
				? Vector4(0.22f, 0.45f, 0.90f, 0.92f)
				: Vector4(0.16f, 0.18f, 0.22f, 0.92f));
		}

		const bool bActive = (i == LogActiveTab);
		const Vector3 Target = bActive
			? Vector3(LogX + LogTextX, LogY + LogTextY, 0.0f)
			: Hidden;

		// 浠呮縺娲婚〉鏂囨湰鏄剧ず鍦?Log 绐楀彛鍐咃紝鍏朵綑绉诲嚭灞忓箷锛堝唴瀹瑰師鏍蜂繚鐣欙級
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
