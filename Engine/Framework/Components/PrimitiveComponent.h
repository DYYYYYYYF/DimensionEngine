#pragma once

#include "Component.h"

/**
 * 1. 未来可能需要多继承自物理组件
 * 2. 主要存放一些配置相关信息
 */
class DAPI UPrimitiveComponent : public UComponent {
public:
	DECLARE_CLASS_TYPE(UPrimitiveComponent)

};