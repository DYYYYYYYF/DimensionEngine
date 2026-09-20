#include "Game.h"

#include <Core/Controller.hpp>
#include <Core/Event.hpp>
#include <Core/DMemory.hpp>
#include <Systems/CameraSystem.h>
#include <Platform/File/JsonObject.h>
#include <Scene/World.h>
#include "UI/Console/GameConsole.h"
#include "GameLogic/UI/WindowDataPanel.h"
#include "GameLogic/UI/UISampleActor.h"

// TODO: Temp
#include "UI/Console/Keybinds.h"
#include "UI/Console/GameCommand.h"
#include "GameLogic/LogicActors/RotationCubeActor.h"
#include "GameLogic/BaseActors/CubeActor.h"
#include "Framework/Components/CameraComponent.h"
#include "Framework/Components/StaticMeshComponent.h"
#include "Framework/Classes/SkyboxActor.h"
#include "Framework/Classes/DirectionalLightActor.h"
#include "Framework/Classes/PointLightActor.h"
#include "Systems/JobSystem.hpp"

bool GameOnEvent(eEventCode code, void* sender, void* listender_inst, SEventContext context) {
	switch (code)
	{
        case eEventCode::Object_Hover_ID_Changed: 
        {
            //GameInst->HoveredObjectID = context.data.u32[0];
            return true;
        }break;
    }

	return false;
}

void LoadScene1(GameInstance* Game);
void LoadScene2(GameInstance* Game);
void LoadScene3(GameInstance* Game);
void LoadScene4(GameInstance* Game);

// 加载新场景前清理上一轮场景切换（LoadScene2）创建的 Actor，避免多次按 P 累积资源：
//  1) UnregisterComponents：从 RenderWorld 移除并销毁该 Actor 的 RenderProxy，本帧不再绘制。
//  2) 将 Actor 推入 PendingDestroyActors 延迟删除队列，不在此处立即 Destroy / DeleteObject /
//     减几何引用——几何体 / 材质 / Vulkan 实例资源的完整释放链（~AStaticMeshActor → Unload
//     → 引用计数递减 → MaterialSystem::DestroyMaterial → ReleaseInstanceResource）统一延迟到
//     DrawFrame 提交完当前帧命令缓冲后由 FlushDeferredDestroys 执行，避免渲染期仍引用
//     DescriptorSet / 纹理时被立即销毁，造成 InvalidCommandBuffer / 空 ImageView 崩溃。
static void ClearSceneActors(GameInstance* GameInst) {
	for (AActor* Actor : GameInst->SceneActors) {
		if (!Actor) continue;

		// 从 RenderWorld 摘除渲染收集，本帧起不再参与绘制
		Actor->UnregisterComponents();

		// 推入延迟删除队列，帧末由 FlushDeferredDestroys 走标准释放链
		GameInst->PendingDestroyActors.Push(Actor);
	}
	GameInst->SceneActors.Clear();
}

// 帧末延迟删除冲刷：渲染帧命令缓冲（EndFrame 提交）之后由主循环调用，对延迟删除队列中的
// Actor 走标准释放链（Destroy + World::RemoveActor + DeleteObject）。
// 析构链触发 UGeometry 引用计数递减 → 归零时销毁 UMaterialInstance（父材质引用 -1）→
// UMaterial 归零时 MaterialSystem::DestroyMaterial → ReleaseInstanceResource（内部 waitIdle
// 兜底，确保 in-flight 命令缓冲不再引用后才 freeDescriptorSets / 纹理），因此不会出现
// 渲染期立即释放描述符集/纹理导致的 InvalidCommandBuffer 与空 ImageView 校验错误。
void GameInstance::FlushDeferredDestroys() {
	if (PendingDestroyActors.Size() == 0) {
		return;
	}

	UWorld* GameWorld = GetWorld();
	for (AActor* Actor : PendingDestroyActors) {
		if (!Actor) continue;

		Actor->Destroy();
		if (GameWorld) GameWorld->RemoveActor(Actor);
		DeleteObject(Actor);
	}
	PendingDestroyActors.Clear();
}

bool GameOnDebugEvent(eEventCode code, void* sender, void* listener_instance, SEventContext context) {
	GameInstance* GameInst = (GameInstance*)listener_instance;

	if (code == eEventCode::Debug_0) {
		LoadScene1(GameInst);
		return true;
	}
	else if (code == eEventCode::Debug_1) {
		LoadScene2(GameInst);
		return true;
	}
	else if (code == eEventCode::Debug_2) {
		LoadScene3(GameInst);
		return true;
	}
	else if (code == eEventCode::Debug_3) {
		LoadScene4(GameInst);
		return true;
	}

	return false;
}

bool GameInstance::Boot() {
	if (!IGame::Boot()) {
		return false;
	}

	GLOG(Log::eInfo, "Booting...");

	File MaterialAsset(EDITOR_CONFIG_PATH);
	if (!MaterialAsset.IsExist()) {
		return false;
	}

	JsonObject Content = JsonObject(MaterialAsset.ReadText());
	WindowSize.Width = (uint16_t)Content.ReadInt("Window.Width");
	WindowSize.Height = (uint16_t)Content.ReadInt("Window.Height");

	Keybind GameKeybind;
	GameKeybind.Setup(this);
	GameCommand GameCmd;
	GameCmd.Setup();

	// Configure fonts.
	BitmapFontConfig BmpFontConfig;
	BmpFontConfig.name = "Ubuntu Mono 21px";
	BmpFontConfig.resourceName = "UbuntuMono21px";
	BmpFontConfig.size = 21;

	SystemFontConfig SysFontConfig;
	SysFontConfig.defaultSize = 20;
	SysFontConfig.name = "Noto Sans";
	SysFontConfig.resourceName = "NotoSansCJK";

	FontConfig.autoRelease = false;
	FontConfig.defaultBitmapFontCount = 1;
	FontConfig.bitmapFontConfigs = (BitmapFontConfig*)Memory::Allocate(sizeof(BitmapFontConfig) * 1, MemoryType::eMemory_Type_Array);
	new (static_cast<BitmapFontConfig*>(FontConfig.bitmapFontConfigs)) BitmapFontConfig(BmpFontConfig);
	FontConfig.defaultSystemFontCount = 1;
	FontConfig.systemFontConfigs = (SystemFontConfig*)Memory::Allocate(sizeof(SystemFontConfig) * 1, MemoryType::eMemory_Type_Array);
	new (static_cast<SystemFontConfig*>(FontConfig.systemFontConfigs)) SystemFontConfig(SysFontConfig);
	FontConfig.maxBitmapFontCount = 100;
	FontConfig.maxSystemFontCount = 100;

	return true;
}

bool GameInstance::Initialize() {
	GLOG(Log::eDebug, "GameInitialize() called.");
	File MaterialAsset(EDITOR_CONFIG_PATH);
	if (!MaterialAsset.IsExist()) {
		return false;
	}

	// Get transform
	JsonObject Content = JsonObject(MaterialAsset);
	Matrix4 Mat = Content.ReadMatrix4("Camera.Transform");
	
	// Load python script
	TestPython.SetPythonFile("recompile_shader");

	Vector3 Position = Content.ReadVector3("Camera.Position");
	Vector3 Rotation = Content.ReadVector3("Camera.Rotation");

	WorldCamera = CameraSystem::Get().GetMainCamera();
	if (WorldCamera) {
		UCameraComponent* CameraComp = WorldCamera->GetCameraComponent();
		if (CameraComp) {
			CameraComp->SetPosition(Position);
			CameraComp->SetEulerAngles(Rotation);
		}
	}
	

	// Create test ui text objects.
	ATextActor* TestText = NewObject<AWindowDataPanel>("Render information window.");
	if (TestText) {
		FString TestTextName = "Ubuntu Mono 21px";
		FString TestTextContent = "Test! \n Yooo!";
		TestText->SetFontSize(21);
		TestText->SetTextFont(TestTextName, UITextType::eUI_Text_Type_Bitmap);
		TestText->SetActorLocation(Vector3(350, 600, 0));
		World->AddActor(TestText);
	}

	// UI 系统演示：Widget 树（垂直盒 + 图片/纯色矩形）渲染
	UISampleActor* UISample = NewObject<UISampleActor>("UI System Sample");
	if (UISample) {
		World->AddActor(UISample);
	}

	// UI 系统演示说明文字（文本走 RenderViewUI 原有 FTextRenderProxy 路径）
	ATextActor* UITipText = NewObject<ATextActor>("UI system tip.", UITextType::eUI_Text_Type_System,
		"Noto Sans CJK JP", 20, "UI System Demo: VerticalBox + ImageWidget (top-left)");
	if (UITipText) {
		UITipText->SetActorLocation(Vector3(100, 40, 0));
		World->AddActor(UITipText);
	}

	FString TestSystemName = "Noto Sans CJK JP";
	FString TestSystemContent = "Keyboard map:\
		\nLoad models:\
		\n\tO: Scene1 P: Scene2\
		\n\tK: Scene3 L: Scene4\
		\nM: Watch memory usage.\
		\nF1: Default view.\
		\nF2: Normal view.\
		\nF3: Material view.\
		\nF4: Depth view.";
	ATextActor* TestSysText = NewObject<ATextActor>("Keyboard map texts.", UITextType::eUI_Text_Type_System, TestSystemName, 25, TestSystemContent);

	if (TestSysText) {
		TestSysText->SetActorLocation(Vector3(100, 600, 0));
		World->AddActor(TestSysText);
	}

	// Load console
	GameConsole = NewObject<DebugConsoleActor>("Debug console panel");
	if (GameConsole) {
		World->AddActor(GameConsole);
	}

	// Skybox
	ASkyboxActor* SkyboxActor = NewObject<ASkyboxActor>("SkyboxActor");
	if (SkyboxActor) {
		World->AddActor(SkyboxActor);
	}

	// 方向光 Actor：阴影通道与延迟光照通道的光照参数统一由它配置
	// （方向/颜色/强度/环境光/阴影偏置与光锥参数，构造时自动注册进 LightSystem）
	DirectionalLightActor = NewObject<ADirectionalLightActor>("DirectionalLight");
	if (DirectionalLightActor) {
		DirectionalLightActor->LoadFromConfig(Content);
		World->AddActor(DirectionalLightActor);
	}

	// 点光源 Actor：数量与参数完全由 Editor/Config.json 的 PointLights 数组决定，
	// 未配置该数组时场景中不存在点光源（延迟光照着色器内已无任何硬编码光源）
	JsonObject PointLightArray = Content.Read("PointLights");
	if (PointLightArray.IsArray()) {
		for (size_t PointLightIndex = 0; PointLightIndex < PointLightArray.Size(); ++PointLightIndex) {
			JsonObject PointLightConfig = PointLightArray.ArrayItemAt(PointLightIndex);
			FString ActorName("PointLight_%u", (unsigned int)PointLightIndex);

			APointLightActor* PointLightActor = NewObject<APointLightActor>(ActorName);
			if (!PointLightActor) {
				continue;
			}

			PointLightActor->LoadFromConfig(PointLightConfig);
			World->AddActor(PointLightActor);
			PointLightActors.Push(PointLightActor);
		}
	}

	// World meshes
	ARotationCubeActor* CubeMesh = NewObject<ARotationCubeActor>("TestCube");
	CubeMesh->SetActorLocation(Vector(0.0f, 0.0f, 0.0f));
	CubeMesh->SetActorRotation(Vector(0.0f));
	World->AddActor(CubeMesh);

	ARotationCubeActor* CubeMesh2 = NewObject<ARotationCubeActor>("TestCube2");
	CubeMesh2->SetActorLocation(Vector(10.0f, 0.0f, 0.0f));
	CubeMesh2->SetActorRotation(Vector(0.0f));
	CubeMesh2->SetWorldScale(Vector(0.5f));
	CubeMesh2->AttachTo(CubeMesh);
	World->AddActor(CubeMesh2);

	ARotationCubeActor* CubeMesh3 = NewObject<ARotationCubeActor>("TestCube3");
	CubeMesh3->SetActorLocation(Vector(15.0f, 0.0f, 0.0f));
	CubeMesh3->SetActorRotation(Vector(0.0f));
	CubeMesh3->SetWorldScale(Vector(0.3f));
	CubeMesh3->AttachTo(CubeMesh2);
	World->AddActor(CubeMesh3);

	// TODO: TEMP
	EngineEvent::Register(eEventCode::Debug_0, this, GameOnDebugEvent);
	EngineEvent::Register(eEventCode::Debug_1, this, GameOnDebugEvent);
	EngineEvent::Register(eEventCode::Debug_2, this, GameOnDebugEvent);
	EngineEvent::Register(eEventCode::Debug_3, this, GameOnDebugEvent);
	EngineEvent::Register(eEventCode::Object_Hover_ID_Changed, this, GameOnEvent);
	// TEMP

	// 优先初始化World
	// World->Actor->Component
	// Component中可能会初始化Geometry，后续在RegisterToWorld时需要这些数据
	World->Initialize();

	return true;
}

void GameInstance::BeginPlay() {
	if (!World) return;
	World->BeginPlay();
}

void GameInstance::Shutdown() {
	// 序列化数据（必须在 World 销毁前完成：相机与方向光 Actor 的组件此时仍然有效）
	File MaterialAsset(EDITOR_CONFIG_PATH);
	if (MaterialAsset.IsExist()) {
		UCameraComponent* CameraComp = WorldCamera ? WorldCamera->GetCameraComponent() : nullptr;
		if (!CameraComp) {
			GLOG(Log::eError, "GameInstance::Shutdown: camera component is nullptr.");
		}

		JsonObject Content = JsonObject(MaterialAsset);
		Content.WriteInt("Window.Width", (int)WindowSize.Width);
		Content.WriteInt("Window.Height", (int)WindowSize.Height);
		if (CameraComp) {
			Content.WriteVector3("Camera.Position", CameraComp->GetPosition());
			Content.WriteVector3("Camera.Rotation", CameraComp->GetEulerAngles());
		}

		// 方向光配置（方向/颜色/强度/环境光/阴影）写回 Editor/Config.json，下次启动即可复用
		if (DirectionalLightActor) {
			DirectionalLightActor->SaveToConfig(Content);
		}

		// 点光源配置写回：按创建顺序组装 PointLights 数组，下次启动即按此重建
		JsonObject PointLightArray(JsonObject::Type::eArray);
		for (APointLightActor* PointLightActor : PointLightActors) {
			if (!PointLightActor) {
				continue;
			}

			JsonObject PointLightConfig(JsonObject::Type::eObject);
			PointLightActor->SaveToConfig(PointLightConfig);
			PointLightArray.ArrayPush(PointLightConfig);
		}
		Content.Write("PointLights", PointLightArray);

		Content.SaveToFile(MaterialAsset);
	}

	if (World) {
		// Actor示例由World清空
		World->Destroy();
	}

	// TODO: TEMP
	EngineEvent::Unregister(eEventCode::Debug_0, this, GameOnDebugEvent);
	EngineEvent::Unregister(eEventCode::Debug_1, this, GameOnDebugEvent);
	EngineEvent::Unregister(eEventCode::Debug_2, this, GameOnDebugEvent);
	EngineEvent::Unregister(eEventCode::Debug_3, this, GameOnDebugEvent);
	EngineEvent::Unregister(eEventCode::Object_Hover_ID_Changed, this, GameOnEvent);
	// TEMP
}

bool GameInstance::Update(float delta_time) {
	if (World) {
		World->Tick(delta_time);
	}

	// Controller
	int px, py, cx, cy;
	Controller::GetMousePosition(cx, cy);
	Controller::GetPreviousMousePosition(px, py);
	float MouseMoveSpeed = 0.005f;
	UCameraComponent* CameraComp = WorldCamera->GetCameraComponent();
	if (!CameraComp) {
		return false;
	}

	if (Controller::IsButtonDown(eButtons::Right)) {
		if (cx != px) {
			CameraComp->RotateYaw((px - cx) * MouseMoveSpeed);
		}

		if (cy != py) {
			CameraComp->RotatePitch((py - cy) * MouseMoveSpeed);
		}
	}

	return true;
}

void GameInstance::OnResize(unsigned int width, unsigned int height) {
	WindowSize = { (uint16_t)width, (uint16_t)height };
}

void LoadScene1(GameInstance* GameInst) {
	
}

void LoadScene2(GameInstance* GameInst) {
	// 加载新场景前，先等待上一轮异步资源加载 job 全部收尾（队列清空 + 完成回调已执行）：
	// 疯狂按 P 时，若场景切换快于 eResource_Load worker 的加载速度，同资源 load job 会
	// 持续重复提交并在队列中无限积压，最终吃光内存触发
	// "RequiredSize < 4294967295U is null!" 分配器断言崩溃。等待收尾后再清理旧 Actor、
	// 创建新 Actor，确保任何时刻在途的资源 load job 只有"当前场景"这一批。
	// 注意：此处必须位于 ClearSceneActors 之前——等待期间上一轮 Actor 仍存活且未
	// UnregisterComponents，job 完成回调（LoadJobSuccess）访问 Actor 完全安全；
	// 若在清理之后等待，回调将触碰已摘除渲染的 Actor，存在悬垂风险。
	JobSystem::WaitForIdle(JobType::eResource_Load);

	// 加载新场景前清理上一轮场景切换创建的 Actor（地面砖 + 模型），
	// 释放几何体引用与 Vulkan 实例资源，避免多次按 P 累积导致实例槽/UBO/描述符池耗尽崩溃
	ClearSceneActors(GameInst);

	// 立即冲刷延迟删除队列：ClearSceneActors 只把旧 Actor 推入 PendingDestroy（几何缓冲仍占用
	// freelist），真正释放要等帧末 FlushDeferredDestroys。若此处不提前冲刷，下面提交的新一批
	// load job 会在 worker 线程立刻创建 sponza 几何，而旧 sponza 几何仍占着渲染缓冲 freelist，
	// 双份几何短暂共存即可挤爆 128MB 缓冲，出现 "Freelist find block, no block with enough
	// free space found" 几何创建失败、返回 nullptr 后解引用崩溃（engine.dll 0xc0000005）。
	// 此处处于事件处理阶段（DrawFrame 之前），上一帧命令缓冲已提交完毕，提前释放安全。
	GameInst->FlushDeferredDestroys();

	// ---- 地面：由多块立方体拼接而成 ----
	// 基础立方体为 10x10x10，每块地面在 X/Z 放大 20 倍、Y 向不缩放 -> 200 x 10 x 200：
	//   * Y 向不缩放，厚度 = 一个小立方体的边长（10）
	//   * 4 x 4 = 16 块拼出 800 x 800 的地面，足够放下三个模型（含 sponza 约 229 x 372 的占地）
	//     且模型/建筑投出的阴影也能完整落在地面上（阴影水平延伸约 130 单位）
	//   * 地面顶面整体下沉一个小立方体边长（y = -10），避免把原点处默认加载的三个立方体切开
	// 注意：必须用不带动画的 ACubeActor；ARotationCubeActor 会在 Tick 中持续自转，不能当地面
	const float GroundTopY = -10.0f;
	const float GroundTileSizeXZ = 200.0f;
	const float GroundTileHeight = 10.0f;
	const int   GroundTileCount = 4;

	for (int ix = 0; ix < GroundTileCount; ++ix) {
		for (int iz = 0; iz < GroundTileCount; ++iz) {
			const float PosX = (static_cast<float>(ix) - (GroundTileCount - 1) * 0.5f) * GroundTileSizeXZ;
			const float PosZ = (static_cast<float>(iz) - (GroundTileCount - 1) * 0.5f) * GroundTileSizeXZ;

			ACubeActor* GroundTile = NewObject<ACubeActor>(FString::Format("GroundTile_%d_%d", ix, iz));
			GroundTile->SetActorLocation(Vector(PosX, GroundTopY - GroundTileHeight * 0.5f, PosZ));
			GroundTile->SetActorRotation(Vector(0.0f));
			GroundTile->SetWorldScale(Vector(GroundTileSizeXZ / 10.0f, GroundTileHeight / 10.0f, GroundTileSizeXZ / 10.0f));
			GameInst->GetWorld()->AddActor(GroundTile);
			GameInst->SceneActors.Push(GroundTile);
		}
	}

	// 模型贴地微调量：让模型底面刚好落在地面顶面之上，避免与地面共面产生 Z-Fighting
	const float ModelLiftEpsilon = 0.05f;

	// ---- 模型 1: sponza（左后方） ----
	// 模型原始包围盒 min.y = -126.443，缩放 0.1 -> 原点距底面 12.6443，需抬高
	AStaticMeshActor* Model1 = NewObject<AStaticMeshActor>("sponza");
	Model1->LoadFromResource("sponza");
	Model1->SetActorLocation(Vector(-200.0f, GroundTopY + 126.443f * 0.1f + ModelLiftEpsilon, -10.0f));
	Model1->SetActorRotation(Vector(0.0f, 90.0f, 0.0f));
	Model1->SetWorldScale(Vector(0.1f));
	GameInst->GetWorld()->AddActor(Model1);
	GameInst->SceneActors.Push(Model1);

	// ---- 模型 2: bunny（右前方） ----
	// 模型原始包围盒 min.y = 0，缩放 5 -> 原点即底面
	AStaticMeshActor* Model2 = NewObject<AStaticMeshActor>("bunny");
	Model2->LoadFromResource("bunny");
	Model2->SetActorLocation(Vector(25.0f, GroundTopY + ModelLiftEpsilon, 5.0f));
	Model2->SetActorRotation(Vector(0.0f, 90.0f, 0.0f));
	Model2->SetWorldScale(Vector(5.0f));
	GameInst->GetWorld()->AddActor(Model2);
	// NOTE: bunny 此前漏 Push 进 SceneActors，导致 ClearSceneActors 每轮都无法清理它，
	// 其子网格/实例资源持续累积直到耗尽。此处与其他 Actor 保持一致登记进清理列表。
	GameInst->SceneActors.Push(Model2);

	// ---- 模型 3: falcon（正前方） ----
	// 模型原始包围盒 min.y = -0.201，缩放 1 -> 需抬高 0.201
	AStaticMeshActor* Model3 = NewObject<AStaticMeshActor>("falcon");
	Model3->LoadFromResource("falcon");
	Model3->SetActorLocation(Vector(0.0f, GroundTopY + 0.201f + ModelLiftEpsilon, 15.0f));
	GameInst->GetWorld()->AddActor(Model3);
	GameInst->SceneActors.Push(Model3);

	// [诊断] 每次切换场景后输出内存统计，用于定位疯狂按 P 时全局池被吃空的泄漏点
	Memory::ShowMemoryUsage();
}

void LoadScene3(GameInstance* GameInst) {
	
}

void LoadScene4(GameInstance* GameInst) {
	
}