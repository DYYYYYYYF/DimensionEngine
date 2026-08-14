#pragma once
#include "Widget.h"

class UPanelWidget : public UWidget {
public:
	// 重写布局逻辑。比如 VerticalBox 就会在这里计算每个子 Widget 的 Top 偏移
	virtual void OnLayout(float Left, float Top, float Right, float Bottom) override {
		// 实现具体的自动布局算法（横排、竖排、网格等）
	}
};