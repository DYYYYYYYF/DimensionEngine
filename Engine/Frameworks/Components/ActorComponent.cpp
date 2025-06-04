#include "ActorComponent.h"
#include "Frameworks/Classes/Actor.h"

bool ActorComponent::AttachTo(Actor* Own) {
	if (Own) {
		Owner = Own;
		return true;
	}

	GLOG(Log::eWarn, "Invalid pointer.");
	return false;
}
