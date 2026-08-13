#include "GameConsole.h"
#include <Core/Utils.hpp>
#include <Core/Console.hpp>
#include <Core/Controller.hpp>
#include <Rendering/Renderer.hpp>
#include <Framework/Classes/TextActor.h>
#include "Systems/FontSystem.hpp"

DebugConsoleActor::DebugConsoleActor(const FString& Name) : AActor(Name) {
	DisplayLineCount = 10;
	LineOffset = 0;
	Visible = false;
	isContentDirty = false;
	Renderer = IRenderer::GetRenderer();

	// 获取字体资产
	FontSystem& FontSystem = FontSystem::Get();
	ConsoleFont = FontSystem.Acquire("Noto Sans CJK JP", UITextType::eUI_Text_Type_System, 26);
	if (!ConsoleFont) {
		GLOG(Log::eWarn, "Unable to acquire font: 'Noto Sans CJK JP'. UIText can not be created.");
		return;
	}

	ConsolePanelComponent = CreateComponent<UTextComponent>("ConsolePanel");
	if (!ConsolePanelComponent) {
		GLOG(Log::eWarn, "DebugConsoleActor Unable to create text component ConsolePanel.");
	}
	EntryPanelComponent = CreateComponent<UTextComponent>("EntryPanel");
	if (!EntryPanelComponent) {
		GLOG(Log::eWarn, "DebugConsoleActor Unable to create text component EntryPanel.");
	}

	SetRootComponent(ConsolePanelComponent);
	
	// 设置字体和内容
	ConsolePanelComponent->SetFont(ConsoleFont);
	ConsolePanelComponent->SetText("No Log.");
	ConsolePanelComponent->SetLocation(Vector3(0.7f * Renderer->GetWidth(), 100, 0.0f));

	EntryPanelComponent->SetFont(ConsoleFont);
	EntryPanelComponent->SetText("Press ' ~ ' to record command.");
	EntryPanelComponent->SetLocation(Vector3(0.7f * Renderer->GetWidth(), 100 + (31.0f * DisplayLineCount), 0.0f));


	SetEnableTick(true);
	Console::RegisterConsumer(std::bind(&DebugConsoleActor::Write, this, std::placeholders::_1, std::placeholders::_2));
}

DebugConsoleActor::~DebugConsoleActor() {
	FontSystem::Get().Release(ConsoleFont);
	Console::UnregisterConsumer(std::bind(&DebugConsoleActor::Write, this, std::placeholders::_1, std::placeholders::_2));
}

bool DebugConsoleActor::Write(Log::Logger::Level level, const std::string& msg) {
	std::vector<std::string> SplitMessage = Utils::StringSplit(msg, '\n', true, false);
	for (size_t i = 0; i < SplitMessage.size(); ++i) {
		MutexGuard Gurad(MsgMutex);
		Lines.push_back(SplitMessage[i]);
	}

	isContentDirty = true;
	return true;
}

void DebugConsoleActor::SetVisible(bool visiblable) {
	Visible = visiblable;
	if (Visible) {
		EntryPanelComponent->SetText(" ");
	}
	else {
		EntryPanelComponent->SetText("Press ' ~ ' to record command.");
	}
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
			FString Content = EntryPanelComponent->GetText();
			uint32_t Length = (uint32_t)Content.Length();
			if (Length > 0 && Content[0] != '\0') {
				// Execute the command and clear the text.
				if (!Console::ExecuteCommand(Content.SubStr(0, Length - 1).CStr())) {
					// TODO: Handle the error.
				}

				// Clear text.
				EntryPanelComponent->SetText(" ");
			}
		}
		else if (KeyCode == eKeys::BackSpace) {
			FString Content = EntryPanelComponent->GetText();
			uint32_t Length = (uint32_t)Content.Length();
			if (Length > 0) {
				Content = Content.SubStr(0, Content.Length() - 1);
				EntryPanelComponent->SetText(Content);
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
				FString Content = EntryPanelComponent->GetText();
				FString NewContent = FString::Format("%s%c|", Content.SubStr(0, Content.Length() - 1).CStr(), cKeyCode);
				EntryPanelComponent->SetText(NewContent);
			}
		}
	}

	return true;
}

bool DebugConsoleActor::Initialize() {
	AActor::Initialize();

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
	AActor::Tick(DeltaTime);

	/*if (!isContentDirty) {
		return;
	}*/

	size_t LineCount = Lines.size();
	size_t MaxLines = DMIN(DisplayLineCount, LineCount);

	// Calculate the min line first, taking into account the line offset as well.
	size_t MinLine = DMAX(LineCount - MaxLines - LineOffset, 0);
	size_t MaxLine = MinLine + MaxLines;

	std::string Buffer = " ";
	for (size_t i = MinLine; i < MaxLine; ++i) {
		// TODO: insert colour codes for the message type.

		// Copy line
		Buffer += Lines[i];
		// New line
		Buffer += "\n";
	}

	// Once the string is built, set the text.
	ConsolePanelComponent->SetText(Buffer.c_str());
	isContentDirty = false;
}

void DebugConsoleActor::MoveUp() {
	isContentDirty = true;
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
	isContentDirty = true;
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
	isContentDirty = true;
	int LineCount = (int)Lines.size();
	// Don't bother with trying an offset, just reset and boot out.
	if (LineCount <= DisplayLineCount) {
		LineOffset = 0;
		return;
	}

	LineOffset = LineCount - DisplayLineCount;
}

void DebugConsoleActor::MoveToBottom() {
	isContentDirty = true;
	LineOffset = 0;
}
