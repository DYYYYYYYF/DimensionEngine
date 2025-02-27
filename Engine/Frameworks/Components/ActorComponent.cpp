#include "ActorComponent.h"
#include "Frameworks/Classes/Actor.h"

bool ActorComponent::AttachTo(Actor* Own) {
	if (Own) {
		Owner = Own;
		return true;
	}

	LOG_WARN("Invalid pointer.");
	return false;
}
