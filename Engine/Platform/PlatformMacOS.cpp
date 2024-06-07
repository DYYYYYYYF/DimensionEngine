#include "Platform.hpp"

#ifdef DPLATFORM_MACOS

#include "Core/Input.hpp"
#include "Core/Event.hpp"
#include "Core/DThread.hpp"
#include "Core/DMutex.hpp"
#include "Renderer/Vulkan/VulkanPlatform.hpp"
#include "Renderer/Vulkan/VulkanContext.hpp"

#include <mach/mach_time.h>
#include <crt_externs.h>

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>

#include <pthread.h>
#include <errno.h>	// For error reporting

// For surface creation
#define VK_USE_PLATFORM_METAL_EXT
#include <vulkan/vulkan.hpp>

struct SInternalState {
	ApplicationDelegate* app_delegate;
	WindowDelegate* wnd_delegate;
	NSWindow* window;
	ContentView* view;
	CAMetalLayer* layer;

	vk::SurfaceKHR surface;
	bool quit_falgged;
};

enum MacOSModifierKeys {
	eMacOS_Modifier_Key_LShift = 0x01,
	eMacOS_Modifier_Key_RShift = 0x02,
	eMacOS_Modifier_Key_LCtrl = 0x04,
	eMacOS_Modifier_Key_RCtrl = 0x08,
	eMacOS_Modifier_Key_LOption = 0x10,
	eMacOS_Modifier_Key_ROption = 0x20,
	eMacOS_Modifier_Key_LCommand = 0x40,
	eMacOS_Modifier_Key_RCommand = 0x80
};

// Key translation
Keys TranslateKeyCode(uint32_t ns_keycode);
// Modifier key handling
void HandleModifierKeys(uint32_t ns_keycode, uint32_t modifier_flags);

@interface WindowDelegate : NSObject <NSWindowDelegate> {
	SPlatformState* state;
};

-(instancetype)initWithState:(SPlatformState*)onitState;

@end	// Window Delegate

@interface ContentView : NSView <NSTextInputClient> {
	NSWindow* window;
	NSTrackingArea* trackingArea;
	NSMutableAttributedString* markedText;
};

-(instancetype)initWithWindow:(NSWindow*)initWindow;

@end	// Content view

@implementation ContentView

- (instancetype)initWithWindow:(NSWindow*)initWindow {
	self = [super init];
	if (self != nil) {
		window = initWindow;
	}

	return self;
}

- (BOOL)canBecomeKeyView {
	return YES;
}

- (BOOL)acceptsFirstResponder {
	return YES;
}

- (BOOL)wantsUpdateLayer {
	return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent*)event {
	return YES;
}

- (void)mouseDown:(NSEvent*)event{
	input_process_button(BUTTON_LEFT, true);
}

- (void)mouseDragged : (NSEvent*)event {
	// Equivalent to moving the mouse for now.
	[self mouseMoved : event] ;
}

- (void)mouseUp : (NSEvent*)event {
	input_process_button(BUTTON_LEFT, false);
}

- (void)mouseMoved : (NSEvent*)event {
	const NSPoint pos = [event locationInWindow];
	input_process_mouse_move((short)pos.x, (short)pos.y);
}

- (void)rightMouseDown : (NSEvent*)event {
	input_process_button(BUTTON_RIGHT, true);
}

- (void)rightMouseDragged : (NSEvent*)event {
	// Equivalent to moving the mouse for now.
	[self mouseMoved : event] ;
}

- (void)rightMouseUp : (NSEvent*)event {
	input_process_button(BUTTON_RIGHT, false);
}

- (void)otherMouseDown : (NSEvent*)event {
	input_process_button(BUTTON_MIDDLE, true);
}

- (void)otherMouseDragged : (NSEvent*)event {
	// Equivalent to moving the mouse for now.
	[self mouseMoved : event] ;
}

- (void)otherMouseUp : (NSEvent*)event {
	input_process_button(BUTTON_MIDDLE, false);
}

// Handle modifier keys since they are only registered via modifier flags
- (void)flagsChanged : (NSEvent*)event {
	handle_modifier_keys([event keyCode], [event modifierFlags]);
}

- (void)keyDown : (NSEvent*)event {
	Keys key = TranslateKeyCode((uint32_t)[event keyCode]);
	input_process_key(key, true);
}

- (void)keyUp : (NSEvent*)event {
	Keys key = TranslateKeyCode((uint32_t)[event keyCode]);
	input_process_key(key, true);
}

- (void)scrollWheel : (NSEvent*)event {
	input_process_mouse_wheel((char)[event scrollingDeltaY]);
}


- (void)insertText : (id)string replacementRange : (NSRange)replacementRange {}

- (void)setMarkedText : (id)string selectedRange : (NSRange)selectedRange replacementRange : (NSRange)replacementRange {}

- (void)unmarkText {}

// Defines a constant for empty ranges in NSTextInputClient
static const NSRange kEmptyRange = { NSNotFound, 0 };

-(NSRange)selectedRange { return kEmptyRange; }

- (NSRange)markedRange { return kEmptyRange; }

- (BOOL)hasMarkedText { return false; }

- (nullable NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range actualRange : (nullable NSRangePointer)actualRange { return nil; }

- (NSArray<NSAttributedStringKey> *)validAttributesForMarkedText { return[NSArray array]; }

- (NSRect)firstRectForCharacterRange : (NSRange)range actualRange : (nullable NSRangePointer)actualRange { return NSMakeRect(0, 0, 0, 0); }

- (NSUInteger)characterIndexForPoint : (NSPoint)point { return 0; }

@end // ContentView

@interface ApplicationDelegate : NSObject <NSApplicationDelegate> {}

@end // ApplicationDelegate

@implementation ApplicationDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
	// Posting an empty event at start
	@autoreleasepool{

	NSEvent * event = [NSEvent otherEventWithType : NSEventTypeApplicationDefined
										location : NSMakePoint(0, 0)
								   modifierFlags : 0
									   timestamp : 0
									windowNumber : 0
										 context : nil
										 subtype : 0
										   data1 : 0
										   data2 : 0];
	[NSApp postEvent : event atStart : YES] ;

	} // autoreleasepool

	[NSApp stop : nil] ;
}

@end // ApplicationDelegate

- (instancetype)initWithState:(SPlatformState*)initState {
	self = [super init];

	if (self != nil) {
		state = initState;
		state_ptr->quit_flagged = false;
	}

	return self;
}

- (BOOL)windowShouldClose:(id)sender {
	state_ptr->quit_flagged = true;

	SEventContext Data = {};
	Core::EventFire(Core::eEvent_Code_Application_Quit, 0, Data);

	return YES;
}

- (void)windowDidChangeScreen:(NSNotification*)notification {
	SEventContext Context;
	CGSize viewSize = state_ptr->view.bounds.size;
	NSSize newDrawableSize = [state_ptr->view convertSizeToBacking : viewSize];
	state_ptr->handle.layer.drawableSize = newDrawableSize;
	state_ptr->handle.layer.contentsScale = state_ptr->view.window.backingScaleFactor;
	// Save off the device pixel ratio.
	state_ptr->device_pixel_ratio = state_ptr->handle.layer.contentsScale;

	Context.data.u16[0] = (u16)newDrawableSize.width;
	Context.data.u16[1] = (u16)newDrawableSize.height;
	Core::EventFire(Core::eEvent_Code_Resize, 0, Context);
}

- (void)windowDidMiniaturize:(NSNotification*)notification {
	// Send a size of 0, which tells the application it was minimized.
	SEventContext Context;
	Context.data.u16[0] = 0;
	Context.data.u16[1] = 0;
	Core::EventFire(Core::eEvent_Code_Resize, 0, Context);

	[state_ptr->window miniaturize : nil] ;
}

- (void)windowDidDeminiaturize:(NSNotification*)notification {
	SEventContext Context;
	CGSize viewSize = state_ptr->view.bounds.size;
	NSSize newDrawableSize = [state_ptr->view convertSizeToBacking : viewSize];
	state_ptr->handle.layer.drawableSize = newDrawableSize;
	state_ptr->handle.layer.contentsScale = state_ptr->view.window.backingScaleFactor;
	// Save off the device pixel ratio.
	state_ptr->device_pixel_ratio = state_ptr->handle.layer.contentsScale;

	Context.data.u16[0] = (u16)newDrawableSize.width;
	Context.data.u16[1] = (u16)newDrawableSize.height;
	Core::EventFire(Core::eEvent_Code_Resize, 0, Context);

	[state_ptr->window deminiaturize : nil] ;
}

@end // WindowDelegate

// Platform.hpp
bool Platform::PlatformStartup(SPlatformState* platform_state, const char* application_name,
	int x, int y, int width, int height){
	state_ptr->device_pixel_ratio = 1.0f;

	@autoreleasepool{

	[NSApplication sharedApplication] ;

	// App delegate creation
	state_ptr->app_delegate = [[ApplicationDelegate alloc]init];
	if (!state_ptr->app_delegate) {
		KERROR("Failed to create application delegate")
		return false;
	}
	[NSApp setDelegate : state_ptr->app_delegate];

	// Window delegate creation
	state_ptr->wnd_delegate = [[WindowDelegate alloc]initWithState:state];
	if (!state_ptr->wnd_delegate) {
		KERROR("Failed to create window delegate")
		return false;
	}

	// Window creation
	state_ptr->window = [[NSWindow alloc]
		initWithContentRect:NSMakeRect(typed_config->x, typed_config->y, typed_config->width, typed_config->height)
		styleMask : NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
		backing : NSBackingStoreBuffered
		defer : NO];
	if (!state_ptr->window) {
		KERROR("Failed to create window");
		return false;
	}

	// View creation
	state_ptr->view = [[ContentView alloc]initWithWindow:state_ptr->window];
	[state_ptr->view setWantsLayer : YES] ;

	// Layer creation
	state_ptr->handle.layer = [CAMetalLayer layer];
	if (!state_ptr->handle.layer) {
		KERROR("Failed to create layer for view");
	}


	// Setting window properties
	[state_ptr->window setLevel : NSNormalWindowLevel];
	[state_ptr->window setContentView : state_ptr->view] ;
	[state_ptr->window makeFirstResponder : state_ptr->view] ;
	[state_ptr->window setTitle : @(typed_config->application_name)] ;
	[state_ptr->window setDelegate : state_ptr->wnd_delegate] ;
	[state_ptr->window setAcceptsMouseMovedEvents : YES] ;
	[state_ptr->window setRestorable : NO] ;

	if (![[NSRunningApplication currentApplication]isFinishedLaunching] )
		[NSApp run];

	// Making the app a proper UI app since we're unbundled
	[NSApp setActivationPolicy : NSApplicationActivationPolicyRegular] ;

	// Putting window in front on launch
	[NSApp activateIgnoringOtherApps : YES] ;
	[state_ptr->window makeKeyAndOrderFront : nil] ;

	// Handle content scaling for various fidelity displays (i.e. Retina)
	state_ptr->handle.layer.bounds = state_ptr->view.bounds;
	// It's important to set the drawableSize to the actual backing pixels. When rendering
	// full-screen, we can skip the macOS compositor if the size matches the display size.
	state_ptr->handle.layer.drawableSize = [state_ptr->view convertSizeToBacking : state_ptr->view.bounds.size];

	// In its implementation of vkGetPhysicalDeviceSurfaceCapabilitiesKHR, MoltenVK takes into
	// consideration both the size (in points) of the bounds, and the contentsScale of the
	// CAMetalLayer from which the Vulkan surface was created.
	// See also https://github.com/KhronosGroup/MoltenVK/issues/428
	state_ptr->handle.layer.contentsScale = state_ptr->view.window.backingScaleFactor;
	KDEBUG("contentScale: %f", state_ptr->handle.layer.contentsScale);
	// Save off the device pixel ratio.
	state_ptr->device_pixel_ratio = state_ptr->handle.layer.contentsScale;

	[state_ptr->view setLayer : state_ptr->handle.layer] ;

	// This is set to NO by default, but is also important to ensure we can bypass the compositor
	// in full-screen mode
	// See "Direct to Display" http://metalkit.org/2017/06/30/introducing-metal-2.html.
	state_ptr->handle.layer.opaque = YES;

	// Fire off a resize event to make sure the framebuffer is the right size.
	// Again, this should be the actual backing framebuffer size (taking into account pixel density).
	event_context context;
	context.data.u16[0] = (u16)state_ptr->handle.layer.drawableSize.width;
	context.data.u16[1] = (u16)state_ptr->handle.layer.drawableSize.height;
	event_fire(EVENT_CODE_RESIZED, 0, context);

	return true;

	} // autoreleasepool
  return true;
}

void Platform::PlatformShutdown(SPlatformState* platform_state){
	if (state_ptr) {
		@autoreleasepool{

			[state_ptr->window orderOut : nil] ;

			[state_ptr->window setDelegate : nil] ;
			[state_ptr->wnd_delegate release] ;

			[state_ptr->view release] ;
			state_ptr->view = nil;

			[state_ptr->window close] ;
			state_ptr->window = nil;

			[NSApp setDelegate : nil] ;
			[state_ptr->app_delegate release] ;
			state_ptr->app_delegate = nil;

		} // autoreleasepool
	}
	state_ptr = 0;
}

bool Platform::PlatformPumpMessage(SPlatformState* platform_state){
	if (state_ptr) {
		@autoreleasepool{

		NSEvent * event;

		for (;;) {
			event = [NSApp
				nextEventMatchingMask : NSEventMaskAny
				untilDate : [NSDate distantPast]
				inMode : NSDefaultRunLoopMode
				dequeue : YES];

			if (!event)
				break;

			[NSApp sendEvent : event] ;
		}

		} // autoreleasepool

		platform_update_watches();

		return !state_ptr->quit_flagged;
	}
	return true;
}

void* Platform::PlatformAllocate(size_t size, bool aligned){
	return malloc(size);
}

void Platform::PlatformFree(void* block, bool aligned){
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

void Platform::PlatformConsoleWrite(const char* message, unsigned char color){
	// FATAL,ERROR,WARN,INFO,DEBUG,TRACE
	const char* colour_strings[] = { "0;41", "1;31", "1;33", "1;32", "1;34", "1;30" };
	printf("\033[%sm%s\033[0m", colour_strings[colour], message);
}

void Platform::PlatformConsoleWriteError(const char* message, unsigned char color){
	// FATAL,ERROR,WARN,INFO,DEBUG,TRACE
	const char* colour_strings[] = { "0;41", "1;31", "1;33", "1;32", "1;34", "1;30" };
	printf("\033[%sm%s\033[0m", colour_strings[colour], message);
}

double Platform::PlatformGetAbsoluteTime(){
	mach_timebase_info_data_t clock_timebase;
	mach_timebase_info(&clock_timebase);

	u64 mach_absolute = mach_absolute_time();

	u64 nanos = (f64)(mach_absolute * (u64)clock_timebase.numer) / (f64)clock_timebase.denom;
	return nanos / 1.0e9; // Convert to seconds
}

void Platform::PlatformSleep(size_t ms){
#if _POSIX_C_SOURCE >= 199309L
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000 * 1000;
	nanosleep(&ts, 0);
#else
	if (ms >= 1000) {
		sleep(ms / 1000);
	}
	usleep((ms % 1000) * 1000);
#endif
}

int Platform::GetProcessorCount(){
	return [[NSProcessInfo processInfo]processorCount];
}

// TODO: Check usage
void platform_get_handle_info(u64* out_size, void* memory) {

	*out_size = sizeof(macos_handle_info);
	if (!memory) {
		return;
	}

	kcopy_memory(memory, &state_ptr->handle, *out_size);
}

f32 platform_device_pixel_ratio(void) {
	return state_ptr->device_pixel_ratio;
}

// Vulkan
bool PlatformCreateVulkanSurface(SPlatformState* plat_state, VulkanContext* context) {
	// Simple cold-cast to then know type
	SInternalState* state = (SInternalState*)plat_state->internalState;

	VkMetalSurfaceCreateInfoEXT info = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	info.pLayer = state->layer;

	VkSurfaceKHR MetalSurface;
	if (vkCreateMetalSurfaceEXT(context->Instance, &info, context->Allocator, &MetalSurface) != VK_SUCCESS) {
		LOG_FATAL("Create surface failed.");
		return false;
	}

	state->surface = vk::SurfaceKHR(MetalSurface);
	context->Surface = state->surface;

	return true;
}

void GetPlatformRequiredExtensionNames(std::vector<const char*>& array){
	array.push_back("VK_KHR_metal_surface");
	array.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
}

// NOTE: Begin Threads
bool Thread::Create(PFN_thread_start start_func, void* params, bool auto_detach) {
	if (!start_func) {
		return false;
	}

	// pthread_create uses a function pointer that returns void*, so cold-cast to this type.
	i32 Result = pthread_create((pthread_t*)&ThreadID, 0, (void* (*)(void*))start_func, params);
	if (Result != 0) {
		switch (Result) {
		case EAGAIN:
			LOG_ERROR("Failed to create thread: insufficient resources to create another thread.");
			return false;
		case EINVAL:
			LOG_ERROR("Failed to create thread: invalid settings were passed in attributes..");
			return false;
		default:
			LOG_ERROR("Failed to create thread: an unhandled error has occurred. errno=%i", Result);
			return false;
		}
	}
	LOG_DEBUG("Starting process on thread id: %#x", ThreadID);

	// Only save off the handle if not auto-detaching.
	if (!auto_detach) {
		InternalData = platform_allocate(sizeof(u64), false);
		InternalData = ThreadID;
	}
	else {
		// If immediately detaching, make sure the operation is a success.
		Result = pthread_detach((pthread_t)ThreadID);
		if (Result != 0) {
			switch (Result) {
			case EINVAL:
				LOG_ERROR("Failed to detach newly-created thread: thread is not a joinable thread.");
				return false;
			case ESRCH:
				LOG_ERROR("Failed to detach newly-created thread: no thread with the id %#x could be found.", ThreadID);
				return false;
			default:
				LOG_ERROR("Failed to detach newly-created thread: an unknown error has occurred. errno=%i", Result);
				return false;
			}
		}
	}

	return true;
}

void Thread::Destroy() {
	if (InternalData == nullptr) {
		return;
	}

	kthread_cancel(InternalData);
}

void Thread::Detach() {
	if (InternalData == nullptr) {
		return;
	}

	int Result = pthread_detach(InternalData);
	if (Result != 0) {
		switch (Result) {
		case EINVAL:
			LOG_ERROR("Failed to detach thread: thread is not a joinable thread.");
			break;
		case ESRCH:
			LOG_ERROR("Failed to detach thread: no thread with the id %#x could be found.", ThreadID);
			break;
		default:
			LOG_ERROR("Failed to detach thread: an unknown error has occurred. errno=%i", Result);
			break;
		}
	}
	Platform::PlatformFree(InternalData, false);
	InternalData = nullptr;
}

void Thread::Cancel() {
	if (InternalData == nullptr) {
		return;
	}

	int Result = pthread_cancel(InternalData);
	if (Result != 0) {
		switch (Result) {
		case ESRCH:
			LOG_ERROR("Failed to cancel thread: no thread with the id %#x could be found.", ThreadID);
			break;
		default:
			LOG_ERROR("Failed to cancel thread: an unknown error has occurred. errno=%i", Result);
			break;
		}
	}
	Platform::PlatformFree(InternalData, false);
	InternalData = nullptr;
	ThreadID = 0;
}

bool Thread::IsActive() const {
	if (InternalData == nullptr) {
		return false;
	}

	return true;
}

void Thread::Sleep(size_t ms) {
	Platform::PlatformSleep(ms); 
}

size_t Thread::GetThreadID() {

	return (u64)pthread_self();
}
// NOTE: End Threads

// NOTE: Begin mutexs
bool Mutex::Create() {
	// Initialize
	pthread_mutexattr_t mutex_attr;
	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_t mutex;
	i32 result = pthread_mutex_init(&mutex, &mutex_attr);
	if (result != 0) {
		LOG_ERROR("Mutex creation failure!");
		return false;
	}

	// Save off the mutex handle.
	InternalData = Platform::PlatformAllocate(sizeof(pthread_mutex_t), false);
	InternalData = mutex;

	return true;
}

void Mutex::Destroy() {
	if (InternalData == nullptr) {
		return;
	}

	int result = pthread_mutex_destroy((pthread_mutex_t*)InternalData);
	switch (result) {
	case 0:
		// KTRACE("Mutex destroyed.");
		break;
	case EBUSY:
		LOG_ERROR("Unable to destroy mutex: mutex is locked or referenced.");
		break;
	case EINVAL:
		LOG_ERROR("Unable to destroy mutex: the value specified by mutex is invalid.");
		break;
	default:
		LOG_ERROR("An handled error has occurred while destroy a mutex: errno=%i", result);
		break;
	}

	Platform::PlatformFree(InternalData, false);
	InternalData = 0;
}

bool Mutex::Lock() {
	if (InternalData == nullptr) {
		return false;
	}

	// Lock
	int result = pthread_mutex_lock((pthread_mutex_t*)InternalData);
	switch (result) {
	case 0:
		// Success, everything else is a failure.
		// KTRACE("Obtained mutex lock.");
		return true;
	case EOWNERDEAD:
		LOG_ERROR("Owning thread terminated while mutex still active.");
		return false;
	case EAGAIN:
		LOG_ERROR("Unable to obtain mutex lock: the maximum number of recursive mutex locks has been reached.");
		return false;
	case EBUSY:
		LOG_ERROR("Unable to obtain mutex lock: a mutex lock already exists.");
		return false;
	case EDEADLK:
		LOG_ERROR("Unable to obtain mutex lock: a mutex deadlock was detected.");
		return false;
	default:
		LOG_ERROR("An handled error has occurred while obtaining a mutex lock: errno=%i", result);
		return false;
	}

	return true;
}

bool Mutex::UnLock() {
	if (InternalData == nullptr) {
		return false;
	}

	int Result = pthread_mutex_unlock((pthread_mutex_t*)InternalData);
	switch (Result) {
	case 0:
		// KTRACE("Freed mutex lock.");
		return true;
	case EOWNERDEAD:
		LOG_ERROR("Unable to unlock mutex: owning thread terminated while mutex still active.");
		return false;
	case EPERM:
		LOG_ERROR("Unable to unlock mutex: mutex not owned by current thread.");
		return false;
	default:
		LOG_ERROR("An handled error has occurred while unlocking a mutex lock: errno=%i", Result);
		return false;
	}
  return false;
}

// NOTE: End mutexs.

Keys TranslateKeyCode(uint32_t ns_keycode) {
	// https://boredzo.org/blog/wp-content/uploads/2007/05/IMTx-virtual-keycodes.pdf
	// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
	switch (ns_keycode) {
	case 0x52:
		return KEY_NUMPAD0;
	case 0x53:
		return KEY_NUMPAD1;
	case 0x54:
		return KEY_NUMPAD2;
	case 0x55:
		return KEY_NUMPAD3;
	case 0x56:
		return KEY_NUMPAD4;
	case 0x57:
		return KEY_NUMPAD5;
	case 0x58:
		return KEY_NUMPAD6;
	case 0x59:
		return KEY_NUMPAD7;
	case 0x5B:
		return KEY_NUMPAD8;
	case 0x5C:
		return KEY_NUMPAD9;

	case 0x12:
		return KEY_1;
	case 0x13:
		return KEY_2;
	case 0x14:
		return KEY_3;
	case 0x15:
		return KEY_4;
	case 0x17:
		return KEY_5;
	case 0x16:
		return KEY_6;
	case 0x1A:
		return KEY_7;
	case 0x1C:
		return KEY_8;
	case 0x19:
		return KEY_9;
	case 0x1D:
		return KEY_0;

	case 0x00:
		return KEY_A;
	case 0x0B:
		return KEY_B;
	case 0x08:
		return KEY_C;
	case 0x02:
		return KEY_D;
	case 0x0E:
		return KEY_E;
	case 0x03:
		return KEY_F;
	case 0x05:
		return KEY_G;
	case 0x04:
		return KEY_H;
	case 0x22:
		return KEY_I;
	case 0x26:
		return KEY_J;
	case 0x28:
		return KEY_K;
	case 0x25:
		return KEY_L;
	case 0x2E:
		return KEY_M;
	case 0x2D:
		return KEY_N;
	case 0x1F:
		return KEY_O;
	case 0x23:
		return KEY_P;
	case 0x0C:
		return KEY_Q;
	case 0x0F:
		return KEY_R;
	case 0x01:
		return KEY_S;
	case 0x11:
		return KEY_T;
	case 0x20:
		return KEY_U;
	case 0x09:
		return KEY_V;
	case 0x0D:
		return KEY_W;
	case 0x07:
		return KEY_X;
	case 0x10:
		return KEY_Y;
	case 0x06:
		return KEY_Z;

	case 0x27:
		return KEY_APOSTROPHE;
	case 0x2A:
		return KEY_BACKSLASH;
	case 0x2B:
		return KEY_COMMA;
	case 0x18:
		return KEY_EQUAL; // Equal/Plus
	case 0x32:
		return KEY_GRAVE;
	case 0x21:
		return KEY_LBRACKET;
	case 0x1B:
		return KEY_MINUS;
	case 0x2F:
		return KEY_PERIOD;
	case 0x1E:
		return KEY_RBRACKET;
	case 0x29:
		return KEY_SEMICOLON;
	case 0x2C:
		return KEY_SLASH;
	case 0x0A:
		return KEYS_MAX_KEYS; // ?

	case 0x33:
		return KEY_BACKSPACE;
	case 0x39:
		return KEY_CAPITAL;
	case 0x75:
		return KEY_DELETE;
	case 0x7D:
		return KEY_DOWN;
	case 0x77:
		return KEY_END;
	case 0x24:
		return KEY_ENTER;
	case 0x35:
		return KEY_ESCAPE;
	case 0x7A:
		return KEY_F1;
	case 0x78:
		return KEY_F2;
	case 0x63:
		return KEY_F3;
	case 0x76:
		return KEY_F4;
	case 0x60:
		return KEY_F5;
	case 0x61:
		return KEY_F6;
	case 0x62:
		return KEY_F7;
	case 0x64:
		return KEY_F8;
	case 0x65:
		return KEY_F9;
	case 0x6D:
		return KEY_F10;
	case 0x67:
		return KEY_F11;
	case 0x6F:
		return KEY_F12;
	case 0x69:
		return KEY_PRINT;
	case 0x6B:
		return KEY_F14;
	case 0x71:
		return KEY_F15;
	case 0x6A:
		return KEY_F16;
	case 0x40:
		return KEY_F17;
	case 0x4F:
		return KEY_F18;
	case 0x50:
		return KEY_F19;
	case 0x5A:
		return KEY_F20;
	case 0x73:
		return KEY_HOME;
	case 0x72:
		return KEY_INSERT;
	case 0x7B:
		return KEY_LEFT;
	case 0x3A:
		return KEY_LALT;
	case 0x3B:
		return KEY_LCONTROL;
	case 0x38:
		return KEY_LSHIFT;
	case 0x37:
		return KEY_LSUPER;
	case 0x6E:
		return KEYS_MAX_KEYS; // Menu
	case 0x47:
		return KEY_NUMLOCK;
	case 0x79:
		return KEYS_MAX_KEYS; // Page down
	case 0x74:
		return KEYS_MAX_KEYS; // Page up
	case 0x7C:
		return KEY_RIGHT;
	case 0x3D:
		return KEY_RALT;
	case 0x3E:
		return KEY_RCONTROL;
	case 0x3C:
		return KEY_RSHIFT;
	case 0x36:
		return KEY_RSUPER;
	case 0x31:
		return KEY_SPACE;
	case 0x30:
		return KEY_TAB;
	case 0x7E:
		return KEY_UP;

	case 0x45:
		return KEY_ADD;
	case 0x41:
		return KEY_DECIMAL;
	case 0x4B:
		return KEY_DIVIDE;
	case 0x4C:
		return KEY_ENTER;
	case 0x51:
		return KEY_NUMPAD_EQUAL;
	case 0x43:
		return KEY_MULTIPLY;
	case 0x4E:
		return KEY_SUBTRACT;

	default:
		return KEYS_MAX_KEYS;
	}
}


// Bit masks for left and right versions of these keys.
#define MACOS_LSHIFT_MASK (1 << 1)
#define MACOS_RSHIFT_MASK (1 << 2)
#define MACOS_LCTRL_MASK (1 << 0)
#define MACOS_RCTRL_MASK (1 << 13)
#define MACOS_LCOMMAND_MASK (1 << 3)
#define MACOS_RCOMMAND_MASK (1 << 4)
#define MACOS_LALT_MASK (1 << 5)
#define MACOS_RALT_MASK (1 << 6)

static void HandleModifierKey(
	u32 ns_keycode,
	u32 ns_key_mask,
	u32 ns_l_keycode,
	u32 ns_r_keycode,
	u32 k_l_keycode,
	u32 k_r_keycode,
	u32 modifier_flags,
	u32 l_mod,
	u32 r_mod,
	u32 l_mask,
	u32 r_mask) {
	if (modifier_flags & ns_key_mask) {
		// Check left variant
		if (modifier_flags & l_mask) {
			if (!(state_ptr->modifier_key_states & l_mod)) {
				state_ptr->modifier_key_states |= l_mod;
				// Report the keypress
				input_process_key(k_l_keycode, true);
			}
		}

		// Check right variant
		if (modifier_flags & r_mask) {
			if (!(state_ptr->modifier_key_states & r_mod)) {
				state_ptr->modifier_key_states |= r_mod;
				// Report the keypress
				input_process_key(k_r_keycode, true);
			}
		}
	}
	else {
		if (ns_keycode == ns_l_keycode) {
			if (state_ptr->modifier_key_states & l_mod) {
				state_ptr->modifier_key_states &= ~(l_mod);
				// Report the release.
				input_process_key(k_l_keycode, false);
			}
		}

		if (ns_keycode == ns_r_keycode) {
			if (state_ptr->modifier_key_states & r_mod) {
				state_ptr->modifier_key_states &= ~(r_mod);
				// Report the release.
				input_process_key(k_r_keycode, false);
			}
		}
	}
}

void HandleModifierKeys(uint32_t ns_keycode, uint32_t modifier_flags) {
	// Shift
	HandleModifierKey(
		ns_keycode,
		NSEventModifierFlagShift,
		0x38,
		0x3C,
		KEY_LSHIFT,
		KEY_RSHIFT,
		modifier_flags,
		MACOS_MODIFIER_KEY_LSHIFT,
		MACOS_MODIFIER_KEY_RSHIFT,
		MACOS_LSHIFT_MASK,
		MACOS_RSHIFT_MASK);

	KTRACE("modifier flags keycode: %u", ns_keycode);

	// Ctrl
	HandleModifierKey(
		ns_keycode,
		NSEventModifierFlagControl,
		0x3B,
		0x3E,
		KEY_LCONTROL,
		KEY_RCONTROL,
		modifier_flags,
		MACOS_MODIFIER_KEY_LCTRL,
		MACOS_MODIFIER_KEY_RCTRL,
		MACOS_LCTRL_MASK,
		MACOS_RCTRL_MASK);

	// Alt/Option
	HandleModifierKey(
		ns_keycode,
		NSEventModifierFlagOption,
		0x3A,
		0x3D,
		KEY_LALT,
		KEY_RALT,
		modifier_flags,
		MACOS_MODIFIER_KEY_LOPTION,
		MACOS_MODIFIER_KEY_ROPTION,
		MACOS_LALT_MASK,
		MACOS_RALT_MASK);

	// Command/Super
	HandleModifierKey(
		ns_keycode,
		NSEventModifierFlagCommand,
		0x37,
		0x36,
		KEY_LSUPER,
		KEY_RSUPER,
		modifier_flags,
		MACOS_MODIFIER_KEY_LCOMMAND,
		MACOS_MODIFIER_KEY_RCOMMAND,
		MACOS_LCOMMAND_MASK,
		MACOS_RCOMMAND_MASK);

	// Caps lock - handled a bit differently than other keys.
	if (ns_keycode == 0x39) {
		if (modifier_flags & NSEventModifierFlagCapsLock) {
			// Report as a keypress. This notifies the system
			// that caps lock has been turned on.
			input_process_key(KEY_CAPITAL, true);
		}
		else {
			// Report as a release. This notifies the system
			// that caps lock has been turned off.
			input_process_key(KEY_CAPITAL, false);
		}
	}
}


#endif
