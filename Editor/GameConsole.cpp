#include "GameConsole.hpp"
#include <Core/Utils.hpp>
#include <Core/Console.hpp>
#include <Core/Controller.hpp>
#include <Containers/TString.hpp>
#include <Renderer/RendererFrontend.hpp>

bool DebugConsoleActor::Write(Log::Logger::Level level, const std::string& msg) {
	std::vector<std::string> SplitMessage = Utils::StringSplit(msg, '\n', true, false);
	for (size_t i = 0; i < SplitMessage.size(); ++i) {
		Lines.push_back(SplitMessage[i]);
	}

	Dirty = true;
	return true;
}

bool DebugConsoleActor::OnKey(eEventCode code, void* sender, void* listener_inst, SEventContext context) {
	if (!Visible) {
		return false;
	}

	if (code == eEventCode::Key_Pressed) {
		eKeys KeyCode = eKeys(context.data.u16[0]);
		bool IsShiftHeld = Controller::IsKeyDown(eKeys::LShift) || 
			Controller::IsKeyDown(eKeys::RShift) || 
			Controller::IsKeyDown(eKeys::Shift);

		if (KeyCode == eKeys::Enter) {
			uint32_t Length = (uint32_t)strlen(EntryControl->Text);
			if (Length > 0 && EntryControl->Text[0] != '\0') {
				// Execute the command and clear the text.
				if (!Console::ExecuteCommand(EntryControl->Text)) {
					// TODO: Handle the error.
				}

				// Clear text.
				EntryControl->SetText(" ");
			}
		}
		else if (KeyCode == eKeys::BackSpace) {
			uint32_t Length = (uint32_t)strlen(EntryControl->Text);
			if (Length > 0) {
				char* str = StringCopy(EntryControl->Text);
				str[Length - 1] = '\0';
				EntryControl->SetText(str);
				Memory::Free(str, Length + 1, MemoryType::eMemory_Type_String);
			}
		}
		else {
			char cKeyCode = static_cast<char>(KeyCode);
			if ((KeyCode >= eKeys::A) && (KeyCode <= eKeys::Z)) {
				// TODO: Check caps lock
				if (!IsShiftHeld) {
					cKeyCode += 32;
				}
			}
			else if (((KeyCode >= eKeys::Num_0) && (KeyCode <= eKeys::Num_9)) || 
                     KeyCode == eKeys::Minus) {
                // TODO：为什么数字0边上的'-'符号属于Minus，但是输入后无法转换为UTF8字符。
                if (KeyCode == eKeys::Minus){
                    cKeyCode = '-';
                }
                
				if (IsShiftHeld) {
					switch (KeyCode)
					{
					case eKeys::Num_0: cKeyCode = ')'; break;
					case eKeys::Num_1: cKeyCode = '!'; break;
					case eKeys::Num_2: cKeyCode = '@'; break;
					case eKeys::Num_3: cKeyCode = '#'; break;
					case eKeys::Num_4: cKeyCode = '$'; break;
					case eKeys::Num_5: cKeyCode = '%'; break;
					case eKeys::Num_6: cKeyCode = '^'; break;
					case eKeys::Num_7: cKeyCode = '&'; break;
					case eKeys::Num_8: cKeyCode = '*'; break;
                    case eKeys::Num_9: cKeyCode = '('; break;
					case eKeys::Insert: cKeyCode = '_'; break;
					}
				}
			}
			else {
				switch (KeyCode)
				{
				case eKeys::Space:
					cKeyCode = static_cast<char>(KeyCode);
					break;
				default:
					cKeyCode = 0;
					break;
				}
			}

			if (cKeyCode != 0) {
				uint32_t Length = (uint32_t)strlen(EntryControl->Text);
				char* NewText = (char*)Memory::Allocate(Length + 2, MemoryType::eMemory_Type_String);
				ASSERT(NewText);
				StringFormat(NewText, Length + 2, "%s%c", EntryControl->Text, cKeyCode);
				EntryControl->SetText(NewText);
				Memory::Free(NewText, Length + 1, MemoryType::eMemory_Type_String);
			}
		}
	}

	return true;
}

DebugConsoleActor::DebugConsoleActor(){
	DisplayLineCount = 10;
	LineOffset = 0;
	Visible = false;
	Dirty = false;
	Renderer = nullptr;
	TextControl = nullptr;
	EntryControl = nullptr;

	Console::RegisterConsumer(std::bind(&DebugConsoleActor::Write, this, std::placeholders::_1, std::placeholders::_2));
}

DebugConsoleActor::DebugConsoleActor(IRenderer* renderer) {
	DisplayLineCount = 10;
	LineOffset = 0;
	Visible = false;
	Dirty = false;
	Renderer = renderer;
	TextControl = nullptr;
	EntryControl = nullptr;

	Console::RegisterConsumer(std::bind(&DebugConsoleActor::Write, this, std::placeholders::_1, std::placeholders::_2));
}

DebugConsoleActor::~DebugConsoleActor() {
	if (TextControl) {
		TextControl->Destroy();
		DeleteObject(TextControl);
		TextControl = nullptr;
	}

	if (EntryControl) {
		EntryControl->Destroy();
		DeleteObject(EntryControl);
		EntryControl = nullptr;
	}

	Console::UnregisterConsumer(std::bind(&DebugConsoleActor::Write, this, std::placeholders::_1, std::placeholders::_2));
}

bool DebugConsoleActor::Initialize() {
	Actor::Initialize();

	// Create UI text control for rendering.
	TextControl = NewObject<UIText>();
	if (!TextControl->Create(Renderer, UITextType::eUI_Text_Type_system, "Noto Sans CJK JP", 26, "No Log.")) {
		GLOG(Log::eFatal, "Unable to create text control for debug console.");
		return false;
	}

	TextControl->SetLocation(Vector3(0.7f * Renderer->GetWidth(), 100, 0));

	// Create another ui text control for rendering typed text.
	EntryControl = NewObject<UIText>();
	if (!EntryControl->Create(Renderer, UITextType::eUI_Text_Type_system, "Noto Sans CJK JP", 26, "Press 'entry' to record command.")) {
		GLOG(Log::eFatal, "Unable to create entry control for debug console.");
		return false;
	}

	EntryControl->SetLocation(Vector3(0.7f * Renderer->GetWidth(), 100 + (31.0f * DisplayLineCount), 0.0f));

	EngineEvent::Register(eEventCode::Key_Pressed, nullptr,
		std::bind(
			&DebugConsoleActor::OnKey, this,
			std::placeholders::_1,
			std::placeholders::_2,
			std::placeholders::_3,
			std::placeholders::_4
		)
	);
	EngineEvent::Register(eEventCode::Key_Released, nullptr,
		std::bind(
			&DebugConsoleActor::OnKey, this,
			std::placeholders::_1,
			std::placeholders::_2,
			std::placeholders::_3,
			std::placeholders::_4
		)
	);

	return true;
}

void DebugConsoleActor::Tick(float DeltaTime) {
	if (!Dirty) {
		return;
	}

	TextControl->Tick(DeltaTime);
	EntryControl->Tick(DeltaTime);

	size_t LineCount = Lines.size();
	size_t MaxLines = DMIN(DisplayLineCount, LineCount);

	// Calculate the min line first, taking into account the line offset as well.
	size_t MinLine = DMAX(LineCount - MaxLines - LineOffset, 0);
	size_t MaxLine = MinLine + MaxLines - 1;

	std::string Buffer = " ";
	for (size_t i = MinLine; i < MaxLine; ++i) {
		// TODO: insert colour codes for the message type.

		// Copy line
		Buffer += Lines[i];
		// New line
		Buffer += "\n";
	}

	// Once the string is built, set the text.
	TextControl->SetText(Buffer.c_str());
	Dirty = false;
}

UIText* DebugConsoleActor::GetText() {
	return TextControl;
}

UIText* DebugConsoleActor::GetEntryText() {
	return EntryControl;
}

void DebugConsoleActor::MoveUp() {
	Dirty = true;
	int LineCount = (int)Lines.size();
	// Don't bother with trying an offset, just reset and boot out.
	if (LineCount <= DisplayLineCount) {
		LineOffset = 0;
		return;
	}

	LineOffset++;
	LineOffset = DMIN(LineOffset, LineCount - DisplayLineCount);
}

void DebugConsoleActor::MoveDown() {
	Dirty = true;
	size_t LineCount = Lines.size();
	// Don't bother with trying an offset, just reset and boot out.
	if (LineCount < DisplayLineCount) {
		LineOffset = 0;
		return;
	}

	LineOffset--;
	LineOffset = DMAX(LineOffset, 0);
}

void DebugConsoleActor::MoveToTop() {
	Dirty = true;
	int LineCount = (int)Lines.size();
	// Don't bother with trying an offset, just reset and boot out.
	if (LineCount <= DisplayLineCount) {
		LineOffset = 0;
		return;
	}

	LineOffset = LineCount - DisplayLineCount;
}

void DebugConsoleActor::MoveToBottom() {
	Dirty = true;
	LineOffset = 0;
}
