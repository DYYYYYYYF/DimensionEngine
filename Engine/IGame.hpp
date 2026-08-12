#pragma once

#include "Core/Engine.hpp"
#include "Scene/World.h"

struct SRenderPacket;

class IGame {
private:
	struct WindowRect {
	public:
		uint16_t Width;
		uint16_t Height;
	};

public:
    virtual ~IGame(){}
	virtual bool Boot() {
		World = NewObject<UWorld>();
		if (!World) {
			return false;
		}

		return true;
	};

	virtual void Shutdown() = 0;

	virtual bool Initialize() = 0;
	virtual bool Update(float delta_time) = 0;
	virtual bool Render(SRenderPacket* packet, float delta_time) = 0;

	virtual void OnResize(unsigned int width, unsigned int height) = 0;

	void Add(AActor* Actor) { World->AddActor(Actor); }

public:
	// 基础属性
	uint16_t GetWindowWidth() const { return WindowSize.Width; }
	template<typename T>
	void SetWindowWidth(T W) { WindowSize.Width = (uint16_t)W; }
	uint16_t GetWindowHeight() const { return WindowSize.Height; }
	template<typename T>
	void SetWindowHeight(T H) { WindowSize.Height = (uint16_t)H; }
	WindowRect GetWindowSize() const { return WindowSize; }
	void SetWindowSize(const WindowRect& Rect) { WindowSize = Rect; }

	float GetWindowOffsetX() const { return (float)WindowOffset.x; }
	template<typename T>
	void SetWindowOffsetX(T X) { WindowOffset.x = (float)X; }
	float GetWindowOffsetY() const { return (float)WindowOffset.y; }
	template<typename T>
	void SetWindowOffsetY(T Y) { WindowOffset.y = (float)Y; }
	Vector2 GetWindowOffset() const { return WindowOffset; }
	void SetWindowOffset(const Vector2& Offset) { WindowOffset = Offset; }

	const std::string& GetApplicationName() const { return ApplicationName; }
	void SetApplicationName(const std::string& Name) { ApplicationName = Name; }

	const FontSystemConfig& GetFontConfig() const { return FontConfig; }

	void SetRenderviewConfigPath(const FString& Path) { RenderviewConfigPath = Path; }
	const FString& GetRenderviewConfigPath() const { return RenderviewConfigPath; }

	UWorld* GetWorld() const { return World; }

public:
	float DeltaTime;

protected:
	std::string ApplicationName;
	Vector2 WindowOffset;
	WindowRect WindowSize;
	FontSystemConfig FontConfig;
	FString RenderviewConfigPath;

	UWorld* World;
};
