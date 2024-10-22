#include "Platform.hpp"

#if defined(DPLATFORM_WINDOWS)

#include "Core/Input.hpp"
#include "Core/Event.hpp"
#include "Core/DThread.hpp"
#include "Core/DMutex.hpp"
#include "Renderer/Vulkan/VulkanPlatform.hpp"
#include "Renderer/Vulkan/VulkanContext.hpp"

#include <windows.h>
#include <windowsx.h>
#include <vulkan/vulkan_win32.h>

struct SInternalState {
	HINSTANCE h_instance;
	HWND hwnd;
	vk::SurfaceKHR surface;
};

static double ClockFrequency;
static LARGE_INTEGER StartTime;

LRESULT CALLBACK win32_process_message(HWND hwnd, UINT32 msg, WPARAM w_param, LPARAM l_param);

bool Platform::PlatformStartup(SPlatformState* platform_state, const char* application_name,
	int x, int y, int width, int height) {
	
	platform_state->internalState = malloc(sizeof(SInternalState));
	SInternalState* state = (SInternalState*)platform_state->internalState;

	state->h_instance = GetModuleHandleA(0);

	// Setup & register window class
	HICON icon = LoadIcon(state->h_instance, IDI_APPLICATION);
	WNDCLASSA wc;
	memset(&wc, 0, sizeof(wc));
	wc.style = CS_DBLCLKS;						// Get double clicks
	wc.lpfnWndProc = win32_process_message;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = state->h_instance;
	wc.hIcon = icon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);	// NULL; // Mange cursor
	wc.hbrBackground = NULL;					// Transparent
	wc.lpszClassName = "dimension_window_class";

	if (!RegisterClassA(&wc)) {
		MessageBoxA(0, "Register window failed", "Error", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}

	// Create window
	unsigned int ClientX = x;
	unsigned int ClientY = y;
	unsigned int ClientWidth = width;
	unsigned int ClientHeight = height;

	unsigned int WindowX = ClientX;
	unsigned int WindowY = ClientY;
	unsigned int WindowWidth = ClientWidth;
	unsigned int WindowHeight = ClientHeight;

	unsigned int WindowStyle = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
	unsigned int WindowExStyle = WS_EX_APPWINDOW;

	WindowStyle |= WS_MAXIMIZEBOX;
	WindowStyle |= WS_MINIMIZEBOX;
	WindowStyle |= WS_THICKFRAME;

	// Obtain the size of border
	RECT BorderRect = { 0, 0, 0, 0 };
	AdjustWindowRectEx(&BorderRect, WindowStyle, 0, WindowExStyle);

	WindowX += BorderRect.left;
	WindowY += BorderRect.top;

	WindowWidth += BorderRect.right - BorderRect.left;
	WindowHeight += BorderRect.bottom - BorderRect.top;

	HWND Handle = CreateWindowExA(WindowExStyle, "dimension_window_class", application_name, 
		WindowStyle, WindowX, WindowY, WindowWidth, WindowHeight, 0, 0, state->h_instance, 0);

	if (Handle == 0) {
		MessageBoxA(NULL, "Create window failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
		return false;
	}
	else {
		state->hwnd = Handle;
	}

	// Display
	bool ShouldActivate = true;	// TODO: it will should not accept input if false
	int DisplayWindowCommandFlags = ShouldActivate ? SW_SHOW : SW_SHOWNOACTIVATE;
	//int DisplayWindowCommandFlags = ShouldActivate ? SW_SHOWMAXIMIZED : SW_MAXIMIZE;
	ShowWindow(state->hwnd, DisplayWindowCommandFlags);

	// Clock setup
	LARGE_INTEGER Frequency;
	QueryPerformanceFrequency(&Frequency);
	ClockFrequency = 1.0 / (double)Frequency.QuadPart;
	QueryPerformanceCounter(&StartTime);

	return true;
}

void Platform::PlatformShutdown(SPlatformState* platform_state) {
	SInternalState* state = (SInternalState*)platform_state;
	if (state) {
		DestroyWindow(state->hwnd);
		state->hwnd = 0;
	}
}

bool Platform::PlatformPumpMessage(SPlatformState* platform_state) {
	MSG message;
	while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&message);
		DispatchMessageA(&message);
	}

	return true;
}

void* Platform::PlatformAllocate(size_t size, bool aligned) {
	return malloc(size);
}

void Platform::PlatformFree(void* block, bool aligned) {
	free(block);
}

void* Platform::PlatformZeroMemory(void* block, size_t size) {
	return memset(block, 0, size);
}

void* Platform::PlatformCopyMemory(void* dst, const void* src, size_t size) {
	return memcpy(dst, src, size);
}

void* Platform::PlatformSetMemory(void* dst, int val, size_t size) {
	return memset(dst, val, size);
}

void Platform::PlatformConsoleWrite(const char* message, unsigned char color) {
	HANDLE ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	static unsigned char levels[6] = { 64, 4, 6, 2, 1, 8 };
	SetConsoleTextAttribute(ConsoleHandle, levels[color]);
	OutputDebugStringA(message);
	unsigned long long Length = strlen(message);
	LPDWORD NumberWritten = 0;
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message, (DWORD)Length, NumberWritten, 0);
	SetConsoleTextAttribute(ConsoleHandle, 8);
}

void Platform::PlatformConsoleWriteError(const char* message, unsigned char color) {
	HANDLE ConsoleHandle = GetStdHandle(STD_ERROR_HANDLE);
	static unsigned char levels[6] = { 64, 4, 6, 2, 1, 8 };
	SetConsoleTextAttribute(ConsoleHandle, levels[color]);
	OutputDebugStringA(message);
	unsigned long long Length = strlen(message);
	LPDWORD NumberWritten = 0;
	WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), message, (DWORD)Length, NumberWritten, 0);
	SetConsoleTextAttribute(ConsoleHandle, 8);
}

double Platform::PlatformGetAbsoluteTime() {
	LARGE_INTEGER CurrentTime;
	QueryPerformanceCounter(&CurrentTime);
	return (double)CurrentTime.QuadPart * ClockFrequency;
}

void Platform::PlatformSleep(size_t ms) {
	Sleep((DWORD)ms);
}

int Platform::GetProcessorCount() {
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);
	LOG_INFO("%i processor cores detected.", SystemInfo.dwNumberOfProcessors);
	return SystemInfo.dwNumberOfProcessors;
}

// NOTE: Begin Threads
bool Thread::Create(PFN_thread_start start_func, void* params, bool auto_detach) {
	if (!start_func) {
		return false;
	}

	InternalData = CreateThread(
		0,
		0,													// Default stack size
		(LPTHREAD_START_ROUTINE)start_func, params,			// Function ptr
		0,													// Params to pass to thread
		(DWORD*)&ThreadID);

	LOG_DEBUG("Starting process on thread id: %#x.", ThreadID);
	if (!InternalData) {
		return false;
	}

	if (auto_detach) {
		CloseHandle((HANDLE)InternalData);
	}

	return true;
}

void Thread::Destroy() {
	if (InternalData != nullptr) {
		DWORD ExitCode;
		GetExitCodeThread(InternalData, &ExitCode);
		//if (ExitCode == STILL_ACTIVE) {
		//	TerminateThread(InternalData, 0);	// 0 = failure
		//}
		CloseHandle((HANDLE)InternalData);
		InternalData = nullptr;
		ThreadID = 0;
	}
}

void Thread::Detach() {
	if (InternalData == nullptr) {
		return;
	}

	CloseHandle((HANDLE)InternalData);
	InternalData = nullptr;
}

void Thread::Cancel() {
	if (InternalData == nullptr) {
		return;
	}

	TerminateThread((HANDLE)InternalData, 0);
	InternalData = nullptr;
}

bool Thread::IsActive() const {
	if (InternalData == nullptr) {
		return false;
	}

	DWORD ExitCode = WaitForSingleObject((HANDLE)InternalData, 0);
	if (ExitCode == WAIT_TIMEOUT) {
		return true;
	}
	return false;
}

void Thread::Sleep(size_t ms) {
	Platform::PlatformSleep(ms); 
}

size_t Thread::GetThreadID() {
	return (size_t)GetCurrentThreadId();
}
// NOTE: End Threads

// NOTE: Begin mutexs
bool Mutex::Create() {
	InternalData = CreateMutex(0, 0, 0);
	if (InternalData == nullptr) {
		LOG_FATAL("Unable to create mutex.");
		return false;
	}

	return true;
}

void Mutex::Destroy() {
	if (InternalData == nullptr) {
		return;
	}

	CloseHandle((HANDLE)InternalData);
	InternalData = nullptr;
}

bool Mutex::Lock() {
	if (InternalData == nullptr) {
		return false;
	}

	DWORD Result = WaitForSingleObject((HANDLE)InternalData, INFINITE);
	switch (Result)
	{
	// The thread got ownership of mutex
	case WAIT_OBJECT_0:
		return true;
	// The thread got ownership of an obandoned mutex
	case WAIT_ABANDONED:
		LOG_FATAL("Mutex lock faield.");
		return false;
	}
	return true;
}

bool Mutex::UnLock() {
	if (InternalData == nullptr) {
		return false;
	}

	int Result = ReleaseMutex((HANDLE)InternalData);
	return Result != 0;	// 0 is failed.
}

// NOTE: End mutexs.

LRESULT CALLBACK win32_process_message(HWND hwnd, UINT32 msg, WPARAM w_param, LPARAM l_param) {
	switch (msg) {
		case WM_ERASEBKGND:
			return 1;
		case WM_CLOSE:
			SEventContext Context = SEventContext();
			EngineEvent::Fire(eEventCode::Application_Quit, 0, Context);
			return 1;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_SIZE: {
			RECT Rect;
			GetClientRect(hwnd, &Rect);
			unsigned int Width = Rect.right - Rect.left;
			unsigned int Height = Rect.bottom - Rect.top;

			// Fire the event. The application layer should pick this up, but not handle it
			// as it should not be visible to other parts of the application.
			SEventContext Context = SEventContext();
			Context.data.u16[0] = (unsigned short)Width;
			Context.data.u16[1] = (unsigned short)Height;
			EngineEvent::Fire(eEventCode::Resize, 0, Context);
			break;
		}
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP: {
			// Key pressed/released
			bool pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
			eKeys key = eKeys(w_param);

			// Pass to the input subsystem for processing.
			Controller::ProcessKey(key, pressed);
		} break;
		case WM_MOUSEMOVE: {
			int PositionX = GET_X_LPARAM(l_param);
			int PositionY = GET_Y_LPARAM(l_param);

			// Pass over to the input subsystem
			Controller::ProcessMouseMove(PositionX, PositionY);
		}break;
		case WM_MOUSEWHEEL: {
			int DeltaZ = GET_WHEEL_DELTA_WPARAM(w_param);
			if (DeltaZ != 0) {
				DeltaZ = (DeltaZ < 0) ? -1 : 1;
				Controller::ProcessMouseWheel(DeltaZ);
			}
		}break;
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP: {
			bool pressed = msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN;
			eButtons MouseButton = eButtons::Max;
			switch (msg) {
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
				MouseButton = eButtons::Left;
				break;
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
				MouseButton = eButtons::Middle;
				break;
			case WM_RBUTTONDOWN:
				MouseButton = eButtons::Right;
				break;
			case WM_RBUTTONUP:
				MouseButton = eButtons::Right;
				break;
			}

			// Pass over mouse button to input subsystem.
			if (MouseButton != eButtons::Max) {
				Controller::ProcessButton(MouseButton, pressed);
			}

		}break;
	} 

	return DefWindowProc(hwnd, msg, w_param, l_param);
}


/*
	Vulkan platform
*/
void GetPlatformRequiredExtensionNames(std::vector<const char*>& array) {
	array.push_back("VK_KHR_win32_surface");
}

bool PlatformCreateVulkanSurface(SPlatformState* plat_state, VulkanContext* context) {
	// Simple cold-cast to then know type
	SInternalState* state = (SInternalState*)plat_state->internalState;

	VkWin32SurfaceCreateInfoKHR info = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	info.hinstance = state->h_instance;
	info.hwnd = state->hwnd;

	VkSurfaceKHR Win32Surface;
	if (vkCreateWin32SurfaceKHR(context->Instance, &info, nullptr, &Win32Surface) != VK_SUCCESS) {
		LOG_FATAL("Create surface failed.");
		return false;
	}

	state->surface = vk::SurfaceKHR(Win32Surface);
	context->Surface = state->surface;

	return true;
}

#endif	//DPLATFORM_WINDOWS
