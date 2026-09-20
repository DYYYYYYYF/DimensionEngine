#pragma once

#include <Defines.hpp>
#include <IGame.hpp>
#include <Math/MathTypes.hpp>
#include <Core/CPython.hpp>
#include <Core/Keymap.hpp>
#include <Containers/TArray.hpp>
#include "Framework/Classes/StaticMeshActor.h"
#include "Framework/Classes/TextActor.h"

#ifndef EIDTOR_MODE
#define EIDTOR_MODE
#endif

#define EDITOR_CONFIG_PATH FString::Format("%s%s", ROOT_PATH, "/Editor/Config.json")

class DebugConsoleActor;
class ACameraActor;
class ADirectionalLightActor;
class APointLightActor;

class GameInstance : public IGame {
public:
	GameInstance() :WorldCamera(nullptr), ConsoleKeymap(nullptr){}
	virtual ~GameInstance() {};

public:
	virtual bool Boot() override;
	virtual void Shutdown() override;
	virtual bool Initialize() override;
	virtual void BeginPlay() override;
	virtual bool Update(float delta_time) override;
	virtual void OnResize(unsigned int width, unsigned int height) override;

public:
	ACameraActor* WorldCamera;
	DebugConsoleActor* GameConsole;
	// 方向光 Actor：Initialize 中创建，Shutdown 时把光照配置写回 Editor/Config.json
	ADirectionalLightActor* DirectionalLightActor = nullptr;
	// 点光源 Actor 列表：数据源为 Editor/Config.json 的 PointLights 数组，
	// 每个元素对应一盏灯；Shutdown 时按同一顺序写回
	TArray<APointLightActor*> PointLightActors;

	// 场景切换（LoadScene2 / P 键）创建的 Actor：下次切换前清理，释放几何体/材质实例资源，
	// 避免多次按 P 累积 Actor 与 Vulkan 实例资源导致实例槽/UBO/描述符池耗尽后崩溃
	TArray<AActor*> SceneActors;

	// 延迟删除（deferred deletion）队列：ClearSceneActors 只做"摘除渲染收集 + 标记待删"，
	// 真正释放（Destroy / World::RemoveActor / DeleteObject 触发引用计数递减与实例资源释放）
	// 由主循环在 DrawFrame 提交完当前帧命令缓冲后调用 FlushDeferredDestroys 统一执行，
	// 避免渲染期（命令缓冲/绘制仍引用）立即销毁 DescriptorSet / 纹理导致 InvalidCommandBuffer 崩溃
	TArray<AActor*> PendingDestroyActors;

	// 帧末冲刷延迟删除队列：对 PendingDestroyActors 走标准释放链（Destroy + RemoveActor + DeleteObject）
	virtual void FlushDeferredDestroys() override;

	// TODO: temp
	Keymap* ConsoleKeymap;
	CPythonModule TestPython;
	// TODO: end temp

};
