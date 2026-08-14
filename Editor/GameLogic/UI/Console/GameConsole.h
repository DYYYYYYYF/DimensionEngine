#include <Core/Event.hpp>
#include <Platform/Thread/DMutex.hpp>
#include <Framework/Classes/Actor.h>

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
	std::vector<std::string> Lines;

	bool isContentDirty;
	bool Visible;

	IFont* ConsoleFont;
	UTextComponent* ConsolePanelComponent;	// Log text.
	UTextComponent* EntryPanelComponent;	// Command text.

	IRenderer* Renderer;

	Mutex MsgMutex;
};