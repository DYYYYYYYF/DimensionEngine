#pragma once

#include "Framework/Classes/UIActor.h"
#include "Containers/TArray.hpp"
#include "Math/MathTypes.hpp"

class GameInstance;
class UWorld;
class AActor;
class ATextActor;
class UImageWidget;
class UPanelWidget;
class UAnchorPanelWidget;
class UScrollBoxWidget;
class UButtonWidget;
class IFont;
class DebugConsoleActor;
class AWindowDataPanel;

// 工具栏按钮 id
enum class EditorToolbarButton : int {
	Save = 0,
	Play,
	Pause,
	Stop,
	Count
};

// 编辑器世界运行状态
enum class EditorPlayState : int {
	Idle = 0,   // 停止态：世界 actor 正常 tick
	Playing,    // 运行态：世界 actor 正常 tick（高亮 Play 按钮）
	Paused      // 暂停态：非编辑器 actor 的 tick 被冻结，编辑器 UI 仍响应
};

// 属性编辑类别
enum class EditorEditCategory : int {
	Location = 0,
	Rotation,
	Scale,
	Count
};

/**
 * 简易编辑器窗口 Actor（场景视图 UI 覆盖层）：
 * 复用引擎 UI 系统（AUIActor + UImageWidget + ATextActor）绘制
 * 顶部工具栏 / 层级大纲面板 / 属性细节面板，在 Tick 中处理
 * 鼠标点击命中（工具栏按钮 / 层级行选中）与键盘属性编辑。
 *
 * 相机操控（旋转 / 平移 / 缩放）由 GameInstance::Update 负责
 * （右键旋转 + 中键平移 + 滚轮缩放），本 Actor 只负责 UI 与交互。
 */
class ASimpleEditorActor : public AUIActor {
public:
	ASimpleEditorActor(const FString& Name, GameInstance* InOwner);
	virtual ~ASimpleEditorActor() = default;

	// GameInstance::Initialize 中调用：创建全部编辑器覆盖层文本 actor 并注册进 World
	void CreateEditorOverlayTexts(UWorld* InWorld);

	// 注册底部 Log 窗口的一个文本源（0=Render 1=Keyboard 2=Console）
	void RegisterLogTextActor(int Index, AWindowDataPanel* Actor, const FString& DisplayName);
	void RegisterLogTextActor(int Index, ATextActor* Actor, const FString& DisplayName);
	void RegisterLogTextActor(int Index, DebugConsoleActor* Actor, const FString& DisplayName);
	// 切换底部 Log 窗口激活页签
	void SetLogActiveTab(int Index);

	// 判断是否编辑器自持 actor（自身 + 全部覆盖层文本），Play/Pause 时跳过 tick 冻结
	bool IsEditorActor(const AActor* Actor) const;
	const TArray<AActor*>& GetEditorActors() const { return EditorActors; }

	// 中央视口矩形（窗口坐标系：x/y 偏移 + z/w 尺寸），供 GameInstance 将相机操控限制在渲染视口内
	Vector4 GetViewportRect() const { return Vector4(ViewportX, ViewportY, ViewportW, ViewportH); }

	virtual void Tick(float DeltaTime) override;

private:
	// Widget 树
	void BuildWidgetTree();
	// 布局随窗口尺寸更新
	void UpdateLayout();
	// 层级列表（过滤相机 / 编辑器自持 actor）
	void RebuildHierarchyList(UWorld* InWorld);
	// 每帧同步层级列表：滚动范围 / 按钮池 / 行文本 / 选中高亮
	void SyncHierarchyList();
	// 层级行 Button 点击回调（OnClick 驱动选中与右侧属性刷新）
	void OnHierarchyButtonClicked(int Row, UButtonWidget* Btn);
	// 每帧刷新属性面板文本
	void RefreshPropertiesText();
	// 每帧刷新工具栏状态
	void RefreshToolbar(float DeltaTime);
	// 每帧刷新底部 Log 窗口（页签高亮 + 文本位置）
	void RefreshLogWindow();

	// 输入处理
	void HandleMouseInput();
	void HandleKeyboardInput();
	void HandleToolbarClick(int ButtonId);
	bool IsPointInRect(float X, float Y, float RX, float RY, float RW, float RH) const;

	// 属性编辑
	void ApplyPropertyDelta(float Delta);

private:
	GameInstance* Owner = nullptr;

	// ---- Widget 树（构建于构造函数） ----
	UPanelWidget* RootPanel = nullptr;
	UImageWidget* ToolbarBG = nullptr;
	// 左侧对象列表：UE 风格 —— 锚点 Panel 固定左侧，内套 ScrollBox，ScrollBox 放 Button 列表项
	UAnchorPanelWidget* HierarchyPanel = nullptr;
	UImageWidget* HierarchyBG = nullptr;
	UScrollBoxWidget* HierarchyScrollBox = nullptr;
	TArray<UButtonWidget*> HierarchyButtons;   // 列表项按钮池（与 DisplayActors 一一对应）
	IFont* HierarchyFont = nullptr;            // 列表项文字字体（懒加载）
	UImageWidget* PropertiesBG = nullptr;
	// 中央视口容器（UE5 风格）：仅绘制标题条与四边框，渲染区中央不覆盖 UI，
	// 否则不透明矩形会盖住下方 Skybox/Deferred 的 3D 渲染结果（表现为黑屏）。
	UImageWidget* ViewportBorderTop = nullptr;    // 标题条 + 上边框
	UImageWidget* ViewportBorderBottom = nullptr; // 下边框
	UImageWidget* ViewportBorderLeft = nullptr;   // 左边框
	UImageWidget* ViewportBorderRight = nullptr;  // 右边框
	TArray<UImageWidget*> ToolbarButtonWidgets;

	// ---- 工具栏按钮命中矩形 ----
	struct ToolbarButtonRect { float X = 0, Y = 0, W = 0, H = 0; int Id = 0; };
	ToolbarButtonRect ToolbarButtons[4];

	// ---- 文本 actor（CreateEditorOverlayTexts 创建并注册） ----
	ATextActor* HierarchyTitle = nullptr;
	ATextActor* PropertiesTitle = nullptr;
	ATextActor* PropertiesText = nullptr;
	ATextActor* ToolbarStatusText = nullptr;
	ATextActor* ViewportTitle = nullptr;   // 中央视口标题条（UE5 风格视口名称）
	ATextActor* ToolbarBtnLabels[4] = { nullptr, nullptr, nullptr, nullptr };
	TArray<AActor*> EditorActors;

	// ---- 层级列表状态 ----
	TArray<AActor*> DisplayActors;
	AActor* SelectedActor = nullptr;
	int SelectedRow = -1;

	// ---- 属性编辑状态 ----
	EditorEditCategory EditCategory = EditorEditCategory::Location;
	int EditAxis = 0;

	// ---- 底部 Log 窗口状态（类似 UE Log/Debug 输出窗口） ----
	UImageWidget* LogBG = nullptr;
	UImageWidget* LogTabWidgets[3] = { nullptr, nullptr, nullptr };
	ATextActor* LogTabLabels[3] = { nullptr, nullptr, nullptr };
	FString LogTabNames[3];
	int LogActiveTab = 0;

	// 底部 Log 窗口的文本源（3 个页签各一）
	struct LogTextSource {
		AWindowDataPanel* RenderPanel = nullptr; // Render 页：渲染统计面板
		ATextActor* Text = nullptr;              // Keyboard 页：普通文本
		DebugConsoleActor* Console = nullptr;    // Console 页：调试控制台
	};
	LogTextSource LogSources[3];

	// ---- 布局参数 ----
	static constexpr float ToolbarHeight = 44.0f;
	static constexpr float ToolbarBtnW = 92.0f;
	static constexpr float ToolbarBtnH = 32.0f;
	static constexpr float ToolbarBtnX0 = 12.0f;
	static constexpr float ToolbarBtnY = 6.0f;
	static constexpr float ToolbarBtnGap = 104.0f;
	static constexpr float HierarchyX = 10.0f;
	static constexpr float HierarchyY = 52.0f;
	static constexpr float HierarchyW = 300.0f;
	static constexpr float PropertiesW = 380.0f;
	static constexpr float PropertiesMargin = 390.0f;
	static constexpr int MaxHierarchyRows = 40;
	static constexpr float RowHeight = 26.0f;
	static constexpr float RowGap = 2.0f;

	// ---- Log 窗口布局参数 ----
	static constexpr float LogPanelH = 200.0f;      // 底部 Log 窗口高度
	static constexpr float LogTabW = 96.0f;         // 页签宽
	static constexpr float LogTabH = 28.0f;         // 页签高
	static constexpr float LogTabX0 = 12.0f;        // 首个页签 x 偏移
	static constexpr float LogTabY = 6.0f;          // 页签 y 偏移（相对 LogBG 顶）
	static constexpr float LogTabGap = 104.0f;      // 页签 x 间距
	static constexpr float LogTextX = 16.0f;        // 文本内容 x 偏移（相对 LogBG 左）
	static constexpr float LogTextY = 44.0f;        // 文本内容 y 偏移（相对 LogBG 顶）

	// ---- 中央视口（场景视图）布局参数（UE5 风格：视口居中、四周 UI） ----
	static constexpr float ViewportGap = 8.0f;        // 视口容器与侧边/底部面板的间距
	static constexpr float ViewportTitleBarH = 30.0f; // 视口标题条高度
	static constexpr float ViewportBorder = 3.0f;     // 视口容器内缩边框（渲染区到容器边缘）

	float WindowWidth = 1280.0f;
	float WindowHeight = 720.0f;
	float HierarchyPanelH = 658.0f;
	float PropertiesX = 890.0f;
	float HierarchyRowStartY = HierarchyY + 30.0f;
	float LogX = 0.0f;
	float LogY = 520.0f;
	float LogW = 1280.0f;

	// 中央视口矩形（窗口坐标系：x/y 偏移 + w/h 尺寸），每帧由 UpdateLayout 计算并推送给世界渲染视图
	float ViewportX = 0.0f;
	float ViewportY = 0.0f;
	float ViewportW = 0.0f;
	float ViewportH = 0.0f;

	// ---- 输入状态 ----
	bool bPrevLeftDown = false;
	bool bPrevTabDown = false;
	bool bPrevBackSpaceDown = false;

	// ---- 工具栏反馈 ----
	float SaveFlashTime = 0.0f;

	// ---- 诊断 ----
	int DiagFrame = 0;
};
