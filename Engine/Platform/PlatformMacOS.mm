#include "Platform.hpp"

#if defined(DPLATFORM_MACOS)

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
#include <vulkan/vulkan_metal.h>
#include <vulkan/vulkan_macos.h>

@class ApplicationDelegate;
@class WindowDelegate;
@class ContentView;

struct SInternalState {
    ApplicationDelegate* app_delegate;
	WindowDelegate* wnd_delegate;
    NSWindow* window;
	ContentView* view;
	CAMetalLayer* layer;

	vk::SurfaceKHR surface;
	bool quit_flagged;
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
enum class eKeys TranslateKeyCode(uint32_t ns_keycode);
// Modifier key handling
void HandleModifierKeys(uint32_t ns_keycode, uint32_t modifier_flags, SInternalState* state_ptr);

@interface WindowDelegate : NSObject <NSWindowDelegate> {
	struct SPlatformState* state;
};

-(instancetype)initWithState:(struct SPlatformState*)initState;

@end	// Window Delegate

@interface ContentView : NSView <NSTextInputClient> {
    struct SPlatformState* state;
	NSWindow* window;
	NSTrackingArea* trackingArea;
	NSMutableAttributedString* markedText;
};

-(instancetype)initWithWindow:(NSWindow*)initWindow :(struct SPlatformState*)initState;

@end	// Content view

@implementation ContentView

- (instancetype)initWithWindow:(NSWindow*)initWindow :(struct SPlatformState*)initState {
	self = [super init];
	if (self != nil) {
		window = initWindow;
        state = initState;
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
    Controller::ProcessButton(eButtons::Left, true);
}

- (void)mouseDragged : (NSEvent*)event {
	// Equivalent to moving the mouse for now.
	[self mouseMoved : event] ;
}

- (void)mouseUp : (NSEvent*)event {
    Controller::ProcessButton(eButtons::Left, false);
}

- (void)mouseMoved : (NSEvent*)event {
	const NSPoint pos = [event locationInWindow];

    // Invert Y on MacOS, since origin is bottom-left.
    // Also need to scale the mouse position by the device pixel ratio so screen lookups are correct.
    SInternalState* state_ptr = (SInternalState*)state->internalState;
    NSSize WindowSize = state_ptr->layer.drawableSize;
    unsigned short x = pos.x * state_ptr->layer.contentsScale;
    unsigned short y = WindowSize.height - (pos.y * state_ptr->layer.contentsScale);
    Controller::ProcessMouseMove(x, y);
}

- (void)rightMouseDown : (NSEvent*)event {
    Controller::ProcessButton(eButtons::Right, true);
}

- (void)rightMouseDragged : (NSEvent*)event {
	// Equivalent to moving the mouse for now.
	[self mouseMoved : event] ;
}

- (void)rightMouseUp : (NSEvent*)event {
    Controller::ProcessButton(eButtons::Right, false);
}

- (void)otherMouseDown : (NSEvent*)event {
    Controller::ProcessButton(eButtons::Middle, true);
}

- (void)otherMouseDragged : (NSEvent*)event {
	// Equivalent to moving the mouse for now.
	[self mouseMoved : event] ;
}

- (void)otherMouseUp : (NSEvent*)event {
    Controller::ProcessButton(eButtons::Middle, false);
}

// Handle modifier keys since they are only registered via modifier flags
- (void)flagsChanged : (NSEvent*)event {
    // TODO: Should get SInternalState point and pass here.
    HandleModifierKeys([event keyCode], (uint32_t)[event modifierFlags], nullptr);
}

- (void)keyDown : (NSEvent*)event {
    eKeys key = TranslateKeyCode((uint32_t)[event keyCode]);
    Controller::ProcessKey(key, true);
}

- (void)keyUp : (NSEvent*)event {
    eKeys key = TranslateKeyCode((uint32_t)[event keyCode]);
    Controller::ProcessKey(key, false);
}

- (void)scrollWheel : (NSEvent*)event {
    Controller::ProcessMouseWheel((char)[event scrollingDeltaY]);
}


- (void)insertText : (id)string replacementRange : (NSRange)replacementRange {}

- (void)setMarkedText : (id)string selectedRange : (NSRange)selectedRange replacementRange : (NSRange)replacementRange {}

- (void)unmarkText {}

// Defines a constant for empty ranges in NSTextInputClient
static const NSRange kEmptyRange = { NSNotFound, 0 };

- (NSRange)selectedRange { return kEmptyRange; }

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

@implementation WindowDelegate
- (instancetype)initWithState:(struct SPlatformState*)initState {
	self = [super init];

	if (self != nil) {
		state = initState;
    SInternalState* state_ptr = (SInternalState*)initState->internalState;
		state_ptr->quit_flagged = false;
	}
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidResize:) name:NSWindowDidResizeNotification object:nil];

	return self;
}

- (BOOL)windowShouldClose:(id)sender {
    SInternalState* state_ptr = (SInternalState*)state->internalState;
	state_ptr->quit_flagged = true;

	SEventContext Data = {};
	EngineEvent::Fire(eEventCode::Application_Quit, 0, Data);

	return YES;
}

- (void)windowDidChangeScreen:(NSNotification*)notification {
    SInternalState* state_ptr = (SInternalState*)state->internalState;
	SEventContext Context;
	CGSize viewSize = state_ptr->view.bounds.size;
	NSSize newDrawableSize = [state_ptr->view convertSizeToBacking : viewSize];
	state_ptr->layer.drawableSize = newDrawableSize;
	state_ptr->layer.contentsScale = state_ptr->view.window.backingScaleFactor;

	Context.data.u16[0] = (unsigned short)newDrawableSize.width;
	Context.data.u16[1] = (unsigned short)newDrawableSize.height;
	EngineEvent::Fire(eEventCode::Resize, 0, Context);
}


- (void)windowDidResize:(NSNotification *)notification {
    SInternalState* state_ptr = (SInternalState*)state->internalState;
    SEventContext Context;
    CGSize viewSize = state_ptr->view.bounds.size;
    NSSize newDrawableSize = [state_ptr->view convertSizeToBacking : viewSize];
    state_ptr->layer.drawableSize = newDrawableSize;
    state_ptr->layer.contentsScale = state_ptr->view.window.backingScaleFactor;

    Context.data.u16[0] = (unsigned short)newDrawableSize.width;
    Context.data.u16[1] = (unsigned short)newDrawableSize.height;
    EngineEvent::Fire(eEventCode::Resize, 0, Context);
}

- (void)windowDidMiniaturize:(NSNotification*)notification {
    SInternalState* state_ptr = (SInternalState*)state->internalState;

	// Send a size of 0, which tells the application it was minimized.
	SEventContext Context;
	Context.data.u16[0] = 0;
	Context.data.u16[1] = 0;
	EngineEvent::Fire(eEventCode::Resize, 0, Context);

	[state_ptr->window miniaturize : nil] ;
}

- (void)windowDidDeminiaturize:(NSNotification*)notification {
  SInternalState* state_ptr = (SInternalState*)state->internalState;
	SEventContext Context;

	CGSize viewSize = state_ptr->view.bounds.size;
	NSSize newDrawableSize = [state_ptr->view convertSizeToBacking : viewSize];
	state_ptr->layer.drawableSize = newDrawableSize;
	state_ptr->layer.contentsScale = state_ptr->view.window.backingScaleFactor;

	Context.data.u16[0] = (unsigned short)newDrawableSize.width;
	Context.data.u16[1] = (unsigned short)newDrawableSize.height;
	EngineEvent::Fire(eEventCode::Resize, 0, Context);

	[state_ptr->window deminiaturize : nil] ;
}

@end // WindowDelegate

// Platform.hpp
bool Platform::PlatformStartup(SPlatformState* platform_state, const char* application_name,
	int x, int y, int width, int height){
    platform_state->internalState = malloc(sizeof(SInternalState));
    SInternalState* state_ptr = (SInternalState*)platform_state->internalState;
    if (state_ptr == nullptr){
        LOG_ERROR("Platform internal state is null!");
        return false;
    }


	@autoreleasepool{

	[NSApplication sharedApplication] ;

	// App delegate creation
	state_ptr->app_delegate = [[ApplicationDelegate alloc] init];
	if (!state_ptr->app_delegate) {
		LOG_ERROR("Failed to create application delegate")
		return false;
	}
	[NSApp setDelegate : state_ptr->app_delegate];

	// Window delegate creation
	state_ptr->wnd_delegate = [[WindowDelegate alloc]initWithState:platform_state];
	if (!state_ptr->wnd_delegate) {
		LOG_ERROR("Failed to create window delegate")
		return false;
	}

	// Window creation
	state_ptr->window = [[NSWindow alloc]
		initWithContentRect:NSMakeRect(x, y, width, height)
		styleMask : NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
		backing : NSBackingStoreBuffered
		defer : NO];
	if (!state_ptr->window) {
		LOG_ERROR("Failed to create window");
		return false;
	}

	// View creation
	state_ptr->view = [[ContentView alloc]initWithWindow:state_ptr->window : platform_state];
	[state_ptr->view setWantsLayer : YES] ;

	// Layer creation
	state_ptr->layer = [CAMetalLayer layer];
	if (!state_ptr->layer) {
		LOG_ERROR("Failed to create layer for view");
	}


	// Setting window properties
	[state_ptr->window setLevel : NSNormalWindowLevel];
	[state_ptr->window setContentView : state_ptr->view] ;
	[state_ptr->window makeFirstResponder : state_ptr->view] ;
	[state_ptr->window setTitle : @(application_name)] ;
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
	state_ptr->layer.bounds = state_ptr->view.bounds;
	// It's important to set the drawableSize to the actual backing pixels. When rendering
	// full-screen, we can skip the macOS compositor if the size matches the display size.
	state_ptr->layer.drawableSize = [state_ptr->view convertSizeToBacking : state_ptr->view.bounds.size];

	// In its implementation of vkGetPhysicalDeviceSurfaceCapabilitiesKHR, MoltenVK takes into
	// consideration both the size (in points) of the bounds, and the contentsScale of the
	// CAMetalLayer from which the Vulkan surface was created.
	// See also https://github.com/KhronosGroup/MoltenVK/issues/428
	state_ptr->layer.contentsScale = state_ptr->view.window.backingScaleFactor;
	LOG_DEBUG("contentScale: %f", state_ptr->layer.contentsScale);

	[state_ptr->view setLayer : state_ptr->layer] ;

	// This is set to NO by default, but is also important to ensure we can bypass the compositor
	// in full-screen mode
	// See "Direct to Display" http://metalkit.org/2017/06/30/introducing-metal-2.html.
	state_ptr->layer.opaque = YES;

	// Fire off a resize event to make sure the framebuffer is the right size.
	// Again, this should be the actual backing framebuffer size (taking into account pixel density).
	SEventContext context;
	context.data.u16[0] = (unsigned short)state_ptr->layer.drawableSize.width;
	context.data.u16[1] = (unsigned short)state_ptr->layer.drawableSize.height;
	EngineEvent::Fire(eEventCode::Resize, 0, context);

	return true;

	} // autoreleasepool
  return true;
}

void Platform::PlatformShutdown(SPlatformState* platform_state){
  SInternalState* state_ptr = (SInternalState*)platform_state->internalState;
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
  SInternalState* state_ptr = (SInternalState*)platform_state->internalState;
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
	printf("\033[%sm%s\033[0m", colour_strings[color], message);
}

void Platform::PlatformConsoleWriteError(const char* message, unsigned char color){
	// FATAL,ERROR,WARN,INFO,DEBUG,TRACE
	const char* colour_strings[] = { "0;41", "1;31", "1;33", "1;32", "1;34", "1;30" };
	printf("\033[%sm%s\033[0m", colour_strings[color], message);
}

double Platform::PlatformGetAbsoluteTime(){
	mach_timebase_info_data_t clock_timebase;
	mach_timebase_info(&clock_timebase);

	size_t mach_absolute = mach_absolute_time();

	size_t nanos = (double)(mach_absolute * (size_t)clock_timebase.numer) / (double)clock_timebase.denom;
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
		sleep((unsigned int)ms / 1000);
	}
	usleep((ms % 1000) * 1000);
#endif
}

int Platform::GetProcessorCount(){
	return (int)[[NSProcessInfo processInfo]processorCount];
}

// Vulkan
bool PlatformCreateVulkanSurface(SPlatformState* plat_state, VulkanContext* context) {
	// Simple cold-cast to then know type
	SInternalState* state = (SInternalState*)plat_state->internalState;

	VkMetalSurfaceCreateInfoEXT info = { VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT};
	info.pLayer = state->layer;

	VkSurfaceKHR MetalSurface;
	if (vkCreateMetalSurfaceEXT(context->Instance, &info, nullptr, &MetalSurface) != VK_SUCCESS) {
		LOG_FATAL("Create surface failed.");
		return false;
	}

	state->surface = vk::SurfaceKHR(MetalSurface);
	context->Surface = state->surface;

	return true;
}

void GetPlatformRequiredExtensionNames(std::vector<const char*>& array){
	array.push_back("VK_EXT_metal_surface");
	array.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
}

// NOTE: Begin Threads
bool Thread::Create(PFN_thread_start start_func, void* params, bool auto_detach) {
	if (!start_func) {
		return false;
	}

	// pthread_create uses a function pointer that returns void*, so cold-cast to this type.
	int Result = pthread_create((pthread_t*)&ThreadID, 0, (void* (*)(void*))start_func, params);
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
		InternalData = Platform::PlatformAllocate(sizeof(size_t), false);
		*(size_t*)InternalData = ThreadID;
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

	Cancel();
}

void Thread::Detach() {
	if (InternalData == nullptr) {
		return;
	}

	int Result = pthread_detach((pthread_t)InternalData);
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

	int Result = pthread_cancel((pthread_t)InternalData);
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

	return (size_t)pthread_self();
}
// NOTE: End Threads

// NOTE: Begin mutexs
bool Mutex::Create() {
	// Initialize
	pthread_mutexattr_t mutex_attr;
	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_t mutex;
	int result = pthread_mutex_init(&mutex, &mutex_attr);
	if (result != 0) {
		LOG_ERROR("Mutex creation failure!");
		return false;
	}

	// Save off the mutex handle.
	InternalData = Platform::PlatformAllocate(sizeof(pthread_mutex_t), false);
	*(pthread_mutex_t*)InternalData = mutex;

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

enum class eKeys TranslateKeyCode(uint32_t ns_keycode) {
	// https://boredzo.org/blog/wp-content/uploads/2007/05/IMTx-virtual-keycodes.pdf
	// https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
	switch (ns_keycode) {
	case 0x52:
		return eKeys::Numpad_0;
	case 0x53:
		return eKeys::Numpad_1;
	case 0x54:
		return eKeys::Numpad_2;
	case 0x55:
		return eKeys::Numpad_3;
	case 0x56:
		return eKeys::Numpad_4;
	case 0x57:
		return eKeys::Numpad_5;
	case 0x58:
		return eKeys::Numpad_6;
	case 0x59:
		return eKeys::Numpad_7;
	case 0x5B:
		return eKeys::Numpad_8;
	case 0x5C:
		return eKeys::Numpad_9;

/*
	case 0x12:
		return eKeys_1;
	case 0x13:
		return eKeys_2;
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
*/

	case 0x00:
		return eKeys::A;
	case 0x0B:
		return eKeys::B;
	case 0x08:
		return eKeys::C;
	case 0x02:
		return eKeys::D;
	case 0x0E:
		return eKeys::E;
	case 0x03:
		return eKeys::F;
	case 0x05:
		return eKeys::G;
	case 0x04:
		return eKeys::H;
	case 0x22:
		return eKeys::I;
	case 0x26:
		return eKeys::J;
	case 0x28:
		return eKeys::K;
	case 0x25:
		return eKeys::L;
	case 0x2E:
		return eKeys::I;
	case 0x2D:
		return eKeys::N;
	case 0x1F:
		return eKeys::O;
	case 0x23:
		return eKeys::P;
	case 0x0C:
		return eKeys::Q;
	case 0x0F:
		return eKeys::R;
	case 0x01:
		return eKeys::S;
	case 0x11:
		return eKeys::T;
	case 0x20:
		return eKeys::U;
	case 0x09:
		return eKeys::V;
	case 0x0D:
		return eKeys::W;
	case 0x07:
		return eKeys::X;
	case 0x10:
		return eKeys::Y;
	case 0x06:
		return eKeys::Z;

	case 0x27:
		return eKeys::Apostrophe;
	case 0x2A:
		return eKeys::Backslash;
	case 0x2B:
		return eKeys::Comma;
	case 0x18:
		return eKeys::Equal; // Equal/Plus
	case 0x32:
		return eKeys::Grave;
	case 0x21:
		return eKeys::LBracket;
	case 0x1B:
		return eKeys::Minus;
	case 0x2F:
		return eKeys::Period;
	case 0x1E:
		return eKeys::Rbracket;
	case 0x29:
		return eKeys::Semicolon;
	case 0x2C:
		return eKeys::Slash;
	case 0x0A:
		return eKeys::Max; // ?

	case 0x33:
		return eKeys::BackSpace;
	case 0x39:
		return eKeys::Capital;
	case 0x75:
		return eKeys::DELETE;
	case 0x7D:
		return eKeys::Down;
	case 0x77:
		return eKeys::End;
	case 0x24:
		return eKeys::Enter;
	case 0x35:
		return eKeys::Escape;
	case 0x7A:
		return eKeys::F1;
	case 0x78:
		return eKeys::F2;
	case 0x63:
		return eKeys::F3;
	case 0x76:
		return eKeys::F4;
	case 0x60:
		return eKeys::F5;
	case 0x61:
		return eKeys::F6;
	case 0x62:
		return eKeys::F7;
	case 0x64:
		return eKeys::F8;
	case 0x65:
		return eKeys::F9;
	case 0x6D:
		return eKeys::F10;
	case 0x67:
		return eKeys::F11;
	case 0x6F:
		return eKeys::F12;
	case 0x69:
		return eKeys::Print;
	case 0x6B:
		return eKeys::F14;
	case 0x71:
		return eKeys::F15;
	case 0x6A:
		return eKeys::F16;
	case 0x40:
		return eKeys::F17;
	case 0x4F:
		return eKeys::F18;
	case 0x50:
		return eKeys::F19;
	case 0x5A:
		return eKeys::F20;
	case 0x73:
		return eKeys::Home;
	case 0x72:
		return eKeys::Insert;
	case 0x7B:
		return eKeys::Left;
	case 0x3B:
		return eKeys::LControl;
	case 0x38:
		return eKeys::LShift;
	case 0x37:
		return eKeys::LSuper;
	case 0x6E:
		return eKeys::Max; // Menu
	case 0x79:
		return eKeys::Max; // Page down
	case 0x74:
		return eKeys::Max; // Page up
	case 0x7C:
		return eKeys::Right;
	case 0x3E:
		return eKeys::RControl;
	case 0x3C:
		return eKeys::RShift;
	case 0x36:
		return eKeys::RSuper;
	case 0x31:
		return eKeys::Space;
	case 0x30:
		return eKeys::Tab;
	case 0x7E:
		return eKeys::Up;

	case 0x45:
		return eKeys::Numpad_Add;
	case 0x4B:
		return eKeys::Numpad_Divide;
	case 0x4C:
		return eKeys::Enter;
	case 0x51:
		return eKeys::Numpad_Equal;
	case 0x43:
		return eKeys::Numpad_Mutiply;
	case 0x4E:
		return eKeys::Numpad_Subtract;

	default:
		return eKeys::Max;
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
	unsigned int ns_keycode,
	unsigned int ns_key_mask,
	unsigned int ns_l_keycode,
	unsigned int ns_r_keycode,
	unsigned int k_l_keycode,
	unsigned int k_r_keycode,
	unsigned int modifier_flags,
	unsigned int l_mod,
	unsigned int r_mod,
	unsigned int l_mask,
	unsigned int r_mask,
  SInternalState* state_ptr) {
	if (modifier_flags & ns_key_mask) {
		// Check left variant
		if (modifier_flags & l_mask) {
			//if (!(state_ptr->modifier_key_states & l_mod)) {
			//	state_ptr->modifier_key_states |= l_mod;
				// Report the keypress
                Controller::ProcessKey(eKeys(k_l_keycode), true);
			//}
		}

		// Check right variant
		if (modifier_flags & r_mask) {
			//if (!(state_ptr->modifier_key_states & r_mod)) {
			//	state_ptr->modifier_key_states |= r_mod;
				// Report the keypress
                Controller::ProcessKey(eKeys(k_r_keycode), true);
			//}
		}
	}
	else {
		if (ns_keycode == ns_l_keycode) {
			//if (state_ptr->modifier_key_states & l_mod) {
			//	state_ptr->modifier_key_states &= ~(l_mod);
				// Report the release.
                Controller::ProcessKey(eKeys(k_l_keycode), false);
			//}
		}

		if (ns_keycode == ns_r_keycode) {
			//if (state_ptr->modifier_key_states & r_mod) {
			//	state_ptr->modifier_key_states &= ~(r_mod);
				// Report the release.
                Controller::ProcessKey(eKeys(k_r_keycode), false);
			//}
		}
	}
}

void HandleModifierKeys(uint32_t ns_keycode, uint32_t modifier_flags, SInternalState* state_ptr) {
	// Shift
	HandleModifierKey(
		ns_keycode,
		NSEventModifierFlagShift,
		0x38,
		0x3C,
        (unsigned int)eKeys::LShift,
        (unsigned int)eKeys::RShift,
		modifier_flags,
		eMacOS_Modifier_Key_LShift,
		eMacOS_Modifier_Key_RShift,
		MACOS_LSHIFT_MASK,
		MACOS_RSHIFT_MASK, state_ptr);

	  LOG_INFO("modifier flags keycode: %u", ns_keycode);

	// Ctrl
	HandleModifierKey(
		ns_keycode,
		NSEventModifierFlagControl,
		0x3B,
		0x3E,
        (unsigned int)eKeys::LControl,
        (unsigned int)eKeys::RControl,
		modifier_flags,
        eMacOS_Modifier_Key_LCtrl,
        eMacOS_Modifier_Key_RCtrl,
		MACOS_LCTRL_MASK,
		MACOS_RCTRL_MASK, state_ptr);

	// Alt/Option
  /*
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
    */

	// Command/Super
	HandleModifierKey(
		ns_keycode,
		NSEventModifierFlagCommand,
		0x37,
		0x36,
		(unsigned int)eKeys::LSuper,
        (unsigned int)eKeys::RSuper,
		modifier_flags,
        eMacOS_Modifier_Key_LCommand,
        eMacOS_Modifier_Key_RCommand,
		MACOS_LCOMMAND_MASK,
		MACOS_RCOMMAND_MASK, state_ptr);

	// Caps lock - handled a bit differently than other keys.
	if (ns_keycode == 0x39) {
		if (modifier_flags & NSEventModifierFlagCapsLock) {
			// Report as a keypress. This notifies the system
			// that caps lock has been turned on.
            Controller::ProcessKey(eKeys::Capital, true);
		}
		else {
			// Report as a release. This notifies the system
			// that caps lock has been turned off.
			Controller::ProcessKey(eKeys::Capital, false);
		}
	}
}


#endif
