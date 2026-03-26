#pragma once

#include <Framework/Classes/StaticMeshActor.h>

class ACubeActor : public AStaticMeshActor {
public:
	ACubeActor();
	ACubeActor(const FString& Name);
	virtual ~ACubeActor();
};