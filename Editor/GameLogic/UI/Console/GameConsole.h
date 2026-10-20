#include <Core/Event.hpp>
#include "Containers/TArray.hpp"
#include <Platform/Thread/DMutex.hpp>
#include <Framework/Classes/Actor.h>
#include <Math/MathTypes.hpp>

class IFont;
class IRenderer;
class UTextComponent;

class DebugConsoleActor : public AActor {
public:
	DebugConsoleActor(const FString& Name);
	virtual ~DebugConsoleActor();

public:
	virtual bool Initialize() override;
	virtual void Tick(float DeltaTime) override;

	// 由编辑器 Log 窗口设置面板整体位置（日志文本 + 命令输入行）
	void SetPanelLocation(const Vector3& Pos);

	void MoveUp();
	void MoveDown();
	void MoveToTop();
	void MoveToBottom();

	bool IsVisible() const { return Visible; }
	void SetVisible(bool visiblable);

	bool OnKey(eEventCode code, void* sender, void* listener_inst, SEventContext context);

private:
	bool Write(Log::Logger::Level level, const std::string& msg);

private:
	int DisplayLineCount;
	int LineOffset;
	TArray<std::string> Lines;

	bool isContentDirty;
	bool Visible;

	IFont* ConsoleFont;
	UTextComponent* ConsolePanelComponent;	// Log text.
	UTextComponent* EntryPanelComponent;	// Command text.

	IRenderer* Renderer;

	Mutex MsgMutex;
};