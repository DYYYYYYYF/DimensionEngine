#pragma once

#include "Defines.hpp"
#include <functional>

// This should be more than enough coeds
#define MAX_MESSAGE_CODES 16384

struct SEventContext {
	// 128 Bytes
	union {
		long long i64[64];
		size_t u64[2];
		double f64[2];

		int i32[4];
		unsigned int u32[4];
		float f32[4];

		short i16[8];
		unsigned short u16[8];

		signed char i8[16];
		unsigned char u8[16];

		char c[16];
	}data;
};

// System internal event codes. Application should use codes beyond 255.
enum class eEventCode : unsigned int {
	// Resized / resolution changed from the OS.
	/* Context usage:
	 * unsigned short width = data.data.u16[0];
	 * unsigned short height = data.data.u16[1];
	 */
	Resize = 0x00,

	// Shuts the application down on the next frame.
	Application_Quit = 0x01,

	// Keyboard key pressed
	/* Context usage:
	 * unsigned short key_code = data.data.u16[0];
	 */
	Key_Pressed = 0x02,

	// Keyboard key released
	/* Context usage:
	 * unsigned short key_code = data.data.u16[0];
	 */
	Key_Released = 0x03,

	// Mouse button pressed
	/* Context usage:
	 * unsigned short key_code = data.data.u16[0];
	 */
	Button_Pressed = 0x04,

	// Mouse button released
	/* Context usage:
	 * unsigned short key_code = data.data.u16[0];
	 */
	Button_Released = 0x05,

	// Mouse moved
	/* Context usage:
	 * short x = data.data.i16[0];
	 * short y = data.data.i16[1];
	 */
	Mouse_Moved = 0x06,

	//Mouse wheel
	/* Context usage:
	 * unsigned char z_delta = data.data.u8[0];
	 */
	Mouse_Wheel = 0x07,

	/**
	* @brief The hoverd-over object id, if there is one.
	* Context usage:
	* int id = data.data.u32[0];
	*/
	Object_Hover_ID_Changed = 0x08,

	/**
	* @breif An event fired by the renderer backend to indicate when any render
	* targets associated with the default window resource need to be refreshed.
	*/
	Default_Rendertarget_Refresh_Required = 0x09,

	/**
	* @breif Change the render mode for debugging purposes.
	*/
	Set_Render_Mode = 0x0A,

	Reload_Shader_Module = 0x10,

	Debug_0 = 0xFC,
	Debug_1 = 0xFB,
	Debug_2 = 0xFD,
	Debug_3 = 0xFE,

	Max = 0xFF
};

typedef std::function<bool(eEventCode code, void* sender, void* listener_instance, SEventContext data)> PFN_OnEvent;

class EngineEvent {
public:
	struct SRegisterEvent {
		void* listener;
		PFN_OnEvent callback;
	};

	struct SEventCodeEntry {
		std::vector<SRegisterEvent> events;
	};

public:
	static bool Initialize();
	static void Shutdown();

	static bool DAPI Register(eEventCode code, void* listener, PFN_OnEvent on_event);
	static bool DAPI Unregister(eEventCode code, void* listener, PFN_OnEvent on_event);
	static bool DAPI Fire(eEventCode code, void* sender, SEventContext context);

public:
	static std::vector<SEventCodeEntry> registered;
	static bool IsInitialized;

};
