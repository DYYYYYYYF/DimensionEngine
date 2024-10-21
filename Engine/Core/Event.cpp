#include "Event.hpp"
#include "DMemory.hpp"
#include "Containers/TArray.hpp"

namespace Core {
	struct SRegisterEvent {
		void* listener;
		PFN_OnEvent callback;
	};

	struct SEventCodeEntry {
		std::vector<SRegisterEvent> events;
	};

	// This should be more than enough coeds
#define MAX_MESSAGE_CODES 16384

	struct EventSystemState {
		std::vector<SEventCodeEntry> registered;
	};

	// Event system internal state
	static bool IsInitialized = false;
	static EventSystemState EventState = {};

}

bool Core::EventInitialize() {
	if (IsInitialized == true) {
		return false;
	}

	IsInitialized = false;
	Memory::Zero(&EventState, sizeof(EventState));

	EventState.registered.resize(MAX_MESSAGE_CODES);
	for (uint32_t i = 0; i < MAX_MESSAGE_CODES; ++i) {
		EventState.registered[i].events.reserve(32);
	}

	IsInitialized = true;
	return IsInitialized;
}

void Core::EventShutdown() {
	if (!EventState.registered.empty()) {
		for (auto& re : EventState.registered) {
			for (auto& e : re.events) {
				e.callback = nullptr;
				e.listener = nullptr;
			}
			re.events.clear();
		}

		EventState.registered.clear();
	}
}

bool Core::EventRegister(unsigned short code, void* listener, PFN_OnEvent on_event) {
	if (IsInitialized == false) {
		return false;
	}

	size_t RegisterCount = EventState.registered[code].events.size();
	for (size_t i = 0; i < RegisterCount; ++i) {
		if (EventState.registered[code].events[i].listener == listener && 
			EventState.registered[code].events[i].
				callback.target<bool(unsigned short code, void* sender, void* listener_instance, SEventContext data)>() == 
				on_event.target<bool(unsigned short code, void* sender, void* listener_instance, SEventContext data)>()) {
			LOG_WARN("The event callback has been registered.");
			return false;
		}
	}

	// If at this point, no duplication was found. proceed with registration
	SRegisterEvent NewEvent;
	NewEvent.listener = listener;
	NewEvent.callback = on_event;

	EventState.registered[code].events.push_back(NewEvent);

	return true;
}

bool Core::EventUnregister(unsigned short code, void* listener, PFN_OnEvent on_event) {
	if (IsInitialized == false) {
		return false;
	}

	if (listener == nullptr) {
		return false;
	}

	if (EventState.registered[code].events.empty()) {
		// TODO: Warn
		LOG_WARN("Event code %d has no event callback.", code);
		return false;
	}

	size_t RegisterCount = EventState.registered[code].events.size();
	for (size_t i = 0; i < RegisterCount; ++i) {
		const SRegisterEvent& event = EventState.registered[code].events[i];
		if (event.listener == listener && EventState.registered[code].events[i].
			callback.target<bool(unsigned short code, void* sender, void* listener_instance, SEventContext data)>() ==
			on_event.target<bool(unsigned short code, void* sender, void* listener_instance, SEventContext data)>()) {
			EventState.registered[code].events.erase(EventState.registered[code].events.begin() + i);
			return true;
		}
	}

	return false;
}

bool Core::EventFire(unsigned short code, void* sender, SEventContext context) {
	if (IsInitialized == false) {
		return false;
	}

	if (Core::EventState.registered[code].events.size() == 0) {
		return false;
	}

	size_t RegisterCount = Core::EventState.registered[code].events.size();
	bool SuccessAll = false;
	for (size_t i = 0; i < RegisterCount; ++i) {
		const SRegisterEvent& event = Core::EventState.registered[code].events[i];

		// Continue send to other listeners, but record false flag.
		if (!event.callback(code, sender, event.listener, context)) {
			SuccessAll = false;
		}
	}

	return SuccessAll;
}
