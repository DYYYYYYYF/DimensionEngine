#include "Platform.hpp"

#if DPLATFORM_WINDOWS

#include "core/Input.hpp"
#include "Core/Event.hpp"
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
}

void Platform::PlatformConsoleWriteError(const char* message, unsigned char color) {
	HANDLE ConsoleHandle = GetStdHandle(STD_ERROR_HANDLE);
	static unsigned char levels[6] = { 64, 4, 6, 2, 1, 8 };
	SetConsoleTextAttribute(ConsoleHandle, levels[color]);
	OutputDebugStringA(message);
	unsigned long long Length = strlen(message);
	LPDWORD NumberWritten = 0;
	WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), message, (DWORD)Length, NumberWritten, 0);
}

double Platform::PlatformGetAbsoluteTime() {
	LARGE_INTEGER CurrentTime;
	QueryPerformanceCounter(&CurrentTime);
	return (double)CurrentTime.QuadPart * ClockFrequency;
}

void Platform::PlatformSleep(int ms) {
	Sleep(ms);
}

LRESULT CALLBACK win32_process_message(HWND hwnd, UINT32 msg, WPARAM w_param, LPARAM l_param) {
	switch (msg) {
		case WM_ERASEBKGND:
			return 1;
		case WM_CLOSE:
			SEventContext Context = SEventContext();
			Core::EventFire(Core::SystemEventCode::eEvent_Code_Application_Quit, 0, Context);
			return 1;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_SIZE: {
			RECT Rect;
			GetClientRect(hwnd, &Rect);
			unsigned int Width = Rect.right - Rect.left;
			unsigned int Height = Rect.bottom - Rect.top;
			break;
		}
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP: {
			// Key pressed/released
			bool pressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
			Keys key = Keys(w_param);

			// Pass to the input subsystem for processing.
			Core::InputProcessKey(key, pressed);
		} break;
		case WM_MOUSEMOVE: {
			int PositionX = GET_X_LPARAM(l_param);
			int PositionY = GET_Y_LPARAM(l_param);

			// Pass over to the input subsystem
			Core::InputProcessMouseMove(PositionX, PositionY);
		}break;
		case WM_MOUSEWHEEL: {
			int DeltaZ = GET_WHEEL_DELTA_WPARAM(w_param);
			if (DeltaZ != 0) {
				DeltaZ = (DeltaZ < 0) ? -1 : 1;
				Core::InputProcessMouseWheel(DeltaZ);
			}
		}break;
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP: {
			bool pressed = msg == WM_LBUTTONDOWN || WM_RBUTTONDOWN || WM_MBUTTONDOWN;
			Buttons MouseButton = eButton_Max;
			switch (msg) {
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
				MouseButton = eButton_Left;
				break;
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
				MouseButton = eButton_Middle;
				break;
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
				MouseButton = eButton_Right;
				break;
			}

			// Pass over mouse button to input subsystem.
			if (MouseButton != eButton_Max) {
				Core::InputProcessButton(MouseButton, pressed);
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
		UL_FATAL("Create surface failed.");
		return false;
	}

	state->surface = vk::SurfaceKHR(Win32Surface);
	context->Surface = state->surface;

	return true;
}

#endif	//DPLATFORM_WINDOWS