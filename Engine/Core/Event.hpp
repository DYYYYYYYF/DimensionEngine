#pragma once

#include "Defines.hpp"
#include <functional>

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

typedef std::function<bool(unsigned short code, void* sender, void* listener_instance, SEventContext data)> PFN_OnEvent;

namespace Core {
	bool EventInitialize();
	void EventShutdown();

	DAPI bool EventRegister(unsigned short code, void* listener, PFN_OnEvent on_event);
	DAPI bool EventUnregister(unsigned short code, void* listener, PFN_OnEvent on_event);

	DAPI bool EventFire(unsigned short code, void* sender, SEventContext context);

	// System internal event codes. Application should use codes beyond 255.
	enum SystemEventCode {
		// Resized / resolution changed from the OS.
		/* Context usage:
		 * unsigned short width = data.data.u16[0];
		 * unsigned short height = data.data.u16[1];
		 */
		eEvent_Code_Resize = 0x00,

		// Shuts the application down on the next frame.
		eEvent_Code_Application_Quit = 0x01,

		// Keyboard key pressed
		/* Context usage:
		 * unsigned short key_code = data.data.u16[0];
		 */
		eEvent_Code_Key_Pressed = 0x02,

		// Keyboard key released
		/* Context usage:
		 * unsigned short key_code = data.data.u16[0];
		 */
		eEvent_Code_Key_Released = 0x03,

		// Mouse button pressed
		/* Context usage:
		 * unsigned short key_code = data.data.u16[0];
		 */
		eEvent_Code_Button_Pressed = 0x04,

		// Mouse button released
		/* Context usage:
		 * unsigned short key_code = data.data.u16[0];
		 */
		eEvent_Code_Button_Released = 0x05,

		// Mouse moved
		/* Context usage:
		 * short x = data.data.i16[0];
		 * short y = data.data.i16[1];
		 */
		eEvent_Code_Mouse_Moved = 0x06,

		//Mouse wheel
		/* Context usage:
		 * unsigned char z_delta = data.data.u8[0];
		 */
		eEvent_Code_Mouse_Wheel = 0x07,

		/**
		* @brief The hoverd-over object id, if there is one.
		* Context usage:
		* int id = data.data.u32[0];
		*/
		eEvent_Code_Object_Hover_ID_Changed = 0x08,

		/**
		* @breif An event fired by the renderer backend to indicate when any render
		* targets associated with the default window resource need to be refreshed.
		*/
		eEvent_Code_Default_Rendertarget_Refresh_Required = 0x09,

		/**
		* @breif Change the render mode for debugging purposes.
		*/
		eEvent_Code_Set_Render_Mode = 0x0A,

		eEvent_Code_Reload_Shader_Module = 0x10,

		eEvent_Code_Debug_0 = 0xFC,
		eEvent_Code_Debug_1 = 0xFB,
		eEvent_Code_Debug_2 = 0xFD,
		eEvent_Code_Debug_3 = 0xFE,

		eEvent_Code_Max = 0xFF
	};
}