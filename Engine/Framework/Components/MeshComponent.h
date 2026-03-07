#pragma once

#include "PrimitiveComponent.h"
#include "Containers/TArray.hpp"

class Material;
class Texture;

/**
 * 这是一个纯虚类，主要用于定义一些列接口
 * 需要存放一些材质、纹理等资源，并且提供一个Draw接口供渲染系统调用
 */
class UMeshComponent : public UPrimitiveComponent {
public:
	DECLARE_CLASS_TYPE(UMeshComponent)

public:
	virtual void DrawMesh() = 0;

protected:
	TArray<Material*> Materials;	// 材质
	TArray<Texture*> Textures;		// 纹理
};