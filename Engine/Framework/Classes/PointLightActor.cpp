#include "PointLightActor.h"
#include "Core/EngineLogger.hpp"
#include "Framework/Components/PointLightComponent.h"
#include "Platform/File/JsonObject.h"
#include "Systems/LightSystem.h"

APointLightActor::APointLightActor(const FString& Name) : AActor(Name) {
	PointLightComponent = CreateComponent<UPointLightComponent>("PointLightComponent");
	if (PointLightComponent) {
		SetRootComponent(PointLightComponent);
		// 注册进光照系统：渲染通道据此把所有点光源打包上传到 GPU
		LightSystem::Get().RegisterPointLight(PointLightComponent);
	}
}

APointLightActor::~APointLightActor() {
	// 注销以避免渲染通道持有已销毁组件（LightSystem 内部只做指针比较，不会解引用悬空指针）
	LightSystem::Get().UnregisterPointLight(PointLightComponent);
}

bool APointLightActor::LoadFromConfig(const JsonObject& Config) {
	if (!PointLightComponent) {
		GLOG(Log::eWarn, "APointLightActor::LoadFromConfig: point light component is null.");
		return false;
	}

	// 位置：直接写进组件变换，光源位置即为 Actor 世界位置
	SetActorLocation(Config.ReadVector3("Position", PointLightComponent->GetWorldPosition()));
	PointLightComponent->SetColor(Config.ReadVector4("Color", PointLightComponent->GetColor()));
	PointLightComponent->SetIntensity(Config.ReadFloat("Intensity", PointLightComponent->GetIntensity()));
	PointLightComponent->SetAttenuation(
		Config.ReadVector3("Attenuation", PointLightComponent->GetAttenuation()));
	PointLightComponent->SetEnabled(Config.ReadBool("Enabled", PointLightComponent->IsEnabled()));

	const Vector3 WorldPosition = PointLightComponent->GetWorldPosition();
	GLOG(Log::eDebug, "Point light config loaded (pos=(%.2f, %.2f, %.2f), intensity=%.3f, enabled=%d).",
		WorldPosition.x, WorldPosition.y, WorldPosition.z,
		PointLightComponent->GetIntensity(), PointLightComponent->IsEnabled() ? 1 : 0);
	return true;
}

void APointLightActor::SaveToConfig(JsonObject& Config) const {
	if (!PointLightComponent) {
		return;
	}

	// 与 Camera/DirectionalLight 段一致：退出时把运行时最终值写回配置文件
	Config.WriteVector3("Position", PointLightComponent->GetWorldPosition());
	Config.WriteVector4("Color", PointLightComponent->GetColor());
	Config.WriteFloat("Intensity", PointLightComponent->GetIntensity());
	Config.WriteVector3("Attenuation", PointLightComponent->GetAttenuation());
	Config.WriteBool("Enabled", PointLightComponent->IsEnabled());
}
