#include "Event.hpp"
#include "DMemory.hpp"
#include "Containers/TArray.hpp"

std::vector<EngineEvent::SEventCodeEntry> EngineEvent::registered;
bool EngineEvent::IsInitialized;

bool EngineEvent::Initialize() {
	if (IsInitialized == true) {
		return false;
	}

	IsInitialized = false;

	registered.resize(MAX_MESSAGE_CODES);
	for (uint32_t i = 0; i < MAX_MESSAGE_CODES; ++i) {
		registered[i].events.reserve(32);
	}

	IsInitialized = true;
	return IsInitialized;
}

void EngineEvent::Shutdown() {
	if (!registered.empty()) {
		for (auto& re : registered) {
			for (auto& e : re.events) {
				e.callback = nullptr;
				e.listener = nullptr;
			}
			re.events.clear();
		}

		registered.clear();
	}
}

bool EngineEvent::Register(eEventCode code, void* listener, PFN_OnEvent on_event) {
	if (IsInitialized == false) {
		return false;
	}

	unsigned int Code = (unsigned int)code;
	size_t RegisterCount = registered[Code].events.size();
	for (size_t i = 0; i < RegisterCount; ++i) {
		if (registered[Code].events[i].listener == listener &&
			registered[Code].events[i].
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

	registered[Code].events.push_back(NewEvent);

	return true;
}

bool EngineEvent::Unregister(eEventCode code, void* listener, PFN_OnEvent on_event) {
	if (IsInitialized == false) {
		return false;
	}

	if (listener == nullptr) {
		return false;
	}

	unsigned int Code = (unsigned int)code;
	if (registered[Code].events.empty()) {
		// TODO: Warn
		LOG_WARN("Event code %d has no event callback.", code);
		return false;
	}

	size_t RegisterCount = registered[Code].events.size();
	for (size_t i = 0; i < RegisterCount; ++i) {
		const SRegisterEvent& event = registered[Code].events[i];
		if (event.listener == listener && registered[Code].events[i].
			callback.target<bool(unsigned short code, void* sender, void* listener_instance, SEventContext data)>() ==
			on_event.target<bool(unsigned short code, void* sender, void* listener_instance, SEventContext data)>()) {
			registered[Code].events.erase(registered[Code].events.begin() + i);
			return true;
		}
	}

	return false;
}

bool EngineEvent::Fire(eEventCode code, void* sender, SEventContext context) {
	if (IsInitialized == false) {
		return false;
	}

	unsigned int Code = (unsigned int)code;
	if (registered[Code].events.size() == 0) {
		return false;
	}

	size_t RegisterCount = registered[Code].events.size();
	bool SuccessAll = false;
	for (size_t i = 0; i < RegisterCount; ++i) {
		const SRegisterEvent& event =registered[Code].events[i];

		// Continue send to other listeners, but record false flag.
		if (!event.callback(code, sender, event.listener, context)) {
			SuccessAll = false;
		}
	}

	return SuccessAll;
}
