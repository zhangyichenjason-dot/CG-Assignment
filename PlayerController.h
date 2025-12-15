#pragma once
#include "GameObject.h"
#include "Window.h"
// 玩家控制器类
class PlayerController
{
public:
	MoveAnimatedModel* player;
	Window* window;
	float maxOffset = 3.5f;       // 左右移动的固定偏移距离
	float transitionSpeed = 8.0f; // 平滑过渡的速度

	void init(MoveAnimatedModel* _player, Window* _window)

	{
		player = _player;
		window = _window;
	}

	void update(float dt)
	{
		float targetX = 0.0f;

		// 检测键盘输入
		if (window->keys[VK_LEFT])
		{
			targetX = maxOffset; // 左移目标位置
		}
		else if (window->keys[VK_RIGHT])
		{
			targetX = -maxOffset;  // 右移目标位置
		}

		// 松开按键时 targetX 默认为 0 (回到中心)
		// 平滑过渡：使用插值让当前X逐渐接近目标X
		// 公式: current += (target - current) * speed * dt
		float currentX = player->position.x;
		float diff = targetX - currentX;

		// 更新玩家位置
		player->position.x += diff * transitionSpeed * dt;
	}
};