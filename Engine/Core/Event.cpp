#include "Event.hpp"
#include "DMemory.hpp"
#include "Containers/TArray.hpp"

namespace Core {
	struct SRegisterEvent {
		void* listener;
		PFN_on_event callback;
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
	static EventSystemState state;

}

bool Core::EventInitialize() {
	if (IsInitialized == true) {
		return false;
	}

	IsInitialized = false;
	Memory::Zero(&state, sizeof(state));

	state.registered.resize(MAX_MESSAGE_CODES);

	IsInitialized = true;
	return IsInitialized;
}

void Core::EventShutdown() {
	for (unsigned short i = 0; i < MAX_MESSAGE_CODES; ++i) {
		if (!state.registered.empty()) {
			state.registered.clear();
		}
	}
}

bool Core::EventRegister(unsigned short code, void* listener, PFN_on_event on_event) {
	if (IsInitialized == false) {
		return false;
	}

	size_t RegisterCount = state.registered[code].events.size();
	for (size_t i = 0; i < RegisterCount; ++i) {
		if (state.registered[code].events[i].listener == listener && 
			state.registered[code].events[i].callback == on_event) {
			LOG_WARN("The event callback has been registered.");
			return false;
		}
	}

	// If at this point, no duplication was found. proceed with registration
	SRegisterEvent NewEvent;
	NewEvent.listener = listener;
	NewEvent.callback = on_event;

	state.registered[code].events.push_back(NewEvent);

	return true;
}

bool Core::EventUnregister(unsigned short code, void* listener, PFN_on_event on_event) {
	if (IsInitialized == false) {
		return false;
	}

	if (state.registered[code].events.data() == nullptr) {
		// TODO: Warn
		return false;
	}

	size_t RegisterCount = state.registered[code].events.size();
	for (size_t i = 0; i < RegisterCount; ++i) {
		const SRegisterEvent& event = state.registered[code].events[i];
		if (event.listener == listener && event.callback == on_event) {
			state.registered[code].events.erase(state.registered[code].events.begin() + i);
			return true;
		}
	}

	return false;
}

bool Core::EventFire(unsigned short code, void* sender, SEventContext context) {
	if (IsInitialized == false) {
		return false;
	}

	if (Core::state.registered[code].events.size() == 0) {
		return false;
	}

	size_t RegisterCount = Core::state.registered[code].events.size();
	bool SuccessAll = false;
	for (size_t i = 0; i < RegisterCount; ++i) {
		const SRegisterEvent& event = Core::state.registered[code].events[i];

		// Continue send to other listeners, but record false flag.
		if (!event.callback(code, sender, event.listener, context)) {
			SuccessAll = false;
		}
	}

	return SuccessAll;
}
