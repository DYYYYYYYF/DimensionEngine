#include <Core/Event.hpp>
#include <Core/DMutex.hpp>
#include <Framework/Classes/Actor.h>

class ATextActor;
class IRenderer;

class DebugConsoleActor : public AActor {
public:
	DebugConsoleActor();
	virtual ~DebugConsoleActor();

public:
	virtual bool Initialize() override;
	virtual void Tick(float DeltaTime) override;

	ATextActor* GetText();
	ATextActor* GetEntryText();

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

	bool Dirty;
	bool Visible;

	ATextActor* TextControl;	// Log text.
	ATextActor* EntryControl;	// Command text.

	IRenderer* Renderer;

	Mutex MsgMutex;
};