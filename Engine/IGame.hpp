#pragma once

#include "Core/Engine.hpp"

struct SRenderPacket;

static IRenderer* Renderer = nullptr;

class IGame {
private:
	struct GameFrameData {
	public:
		std::vector<GeometryRenderData> WorldGeometries;
	};

	struct WindowRect {
	public:
		int Width;
		int Height;
	};

public:
    virtual ~IGame(){}
	virtual bool Boot(IRenderer* renderer) = 0;
	virtual void Shutdown() = 0;

	virtual bool Initialize() = 0;
	virtual bool Update(float delta_time) = 0;
	virtual bool Render(SRenderPacket* packet, float delta_time) = 0;

	virtual void OnResize(unsigned int width, unsigned int height) = 0;

public:
	// 基础属性
	int GetWindowWidth() const { return WindowSize.Width; }
	template<typename T>
	void SetWindowWidth(T W) { WindowSize.Width = (int)W; }
	int GetWindowHeight() const { return WindowSize.Height; }
	template<typename T>
	void SetWindowHeight(T H) { WindowSize.Height = (int)H; }
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

	std::string GetApplicationName() const { return ApplicationName; }
	void SetApplicationName(const std::string& Name) { ApplicationName = Name; }

	FontSystemConfig GetFontConfig() const { return FontConfig; }
	const std::vector<RenderViewConfig>& GetRenderviews() const { return Renderviews; };

public:
	float DeltaTime;
	GameFrameData FrameData;

protected:
	std::string ApplicationName;
	Vector2 WindowOffset;
	WindowRect WindowSize;
	FontSystemConfig FontConfig;
	std::vector<RenderViewConfig> Renderviews;

};
