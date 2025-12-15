#pragma once
#include <string>
#include <cstdio>
#include "Animation.h"
#include "Maths.h"

// 状态机类--用于管理不同动作之间的切换，支持动作融合
class StateMachine
{
public:
	// 用于输出混合结果的实例，传给 AnimatedModel::draw 使用
	AnimationInstance outputInstance;

private:
	Animation* animationData;

	// 两个实例用于混合：current是当前动作，next是目标动作
	AnimationInstance currentInstance;
	AnimationInstance nextInstance;

	std::string currentStateName;
	std::string nextStateName;

	// 混合控制变量
	bool isBlending;
	float blendTime;
	float blendDuration;

	// 循环控制变量
	bool currentLoop;
	bool nextLoop;

public:
	StateMachine()
	{
		animationData = nullptr;
		isBlending = false;
		blendTime = 0.0f;
		blendDuration = 0.2f;
		// 默认循环
		currentLoop = true;
		nextLoop = true;
	}

	// 初始化
	void init(Animation* _animationData)
	{
		animationData = _animationData;
		if (animationData)
		{
			currentInstance.init(animationData, 0);
			nextInstance.init(animationData, 0);
			outputInstance.init(animationData, 0);
		}
	}

	// 切换到新状态（动作）
	// newState: 动作名称
	// duration: 过渡时间（秒），默认0.2秒
	// loop: 是否循环播放，默认true。设为false则播放一次后停在最后一帧
	void changeState(const std::string& newState, float duration = 0.2f, bool loop = true)
	{
		if (animationData && !animationData->hasAnimation(newState))
		{
			printf("WARNING: Animation '%s' not found! State change ignored.\n", newState.c_str());
			return;
		}

		// 如果已经是该状态且不在混合中
		if (currentStateName == newState && !isBlending)
		{
			// 更新循环属性（允许运行中改变循环模式）
			currentLoop = loop;
			return;
		}

		if (isBlending)
		{
			currentInstance = nextInstance;
			currentStateName = nextStateName;
			currentLoop = nextLoop; // 继承之前的循环设置
		}

		nextStateName = newState;
		nextLoop = loop; // 记录新动作的循环设置
		nextInstance.resetAnimationTime();

		if (currentStateName.empty())
		{
			currentStateName = newState;
			currentLoop = loop; // 设置当前动作的循环属性
			currentInstance.update(currentStateName, 0.0f);
			nextInstance = currentInstance;
			outputInstance = currentInstance;
			isBlending = false;
		}
		else
		{
			isBlending = true;
			blendTime = 0.0f;
			blendDuration = duration;
		}
	}

	void update(float dt)
	{
		if (!animationData) return;

		// 1. 更新当前动作
		if (!currentStateName.empty())
		{
			currentInstance.update(currentStateName, dt);

			// 根据 loop 属性决定是否重置
			if (currentInstance.animationFinished())
			{
				if (currentLoop) {
					currentInstance.resetAnimationTime();
				}
				// else: 不重置，AnimationInstance 内部机制会保持在结束状态（最后一帧）
			}
		}

		// 2. 如果在混合，更新目标动作
		if (isBlending)
		{
			nextInstance.update(nextStateName, dt);

			// 混合目标也要处理循环
			if (nextInstance.animationFinished())
			{
				if (nextLoop) {
					nextInstance.resetAnimationTime();
				}
			}

			blendTime += dt;

			if (blendTime >= blendDuration)
			{
				isBlending = false;
				currentStateName = nextStateName;
				currentLoop = nextLoop; // 混合结束，应用新的循环设置
				currentInstance = nextInstance;
			}
		}

		calculateMatrices();
	}

	// 获取用于渲染的实例指针
	AnimationInstance* getRenderInstance()
	{
		return &outputInstance;
	}

	// 获取当前状态名称
	std::string getState() const
	{
		return isBlending ? nextStateName : currentStateName;
	}

	// Check if the current animation has finished用于播放一次动画后播放另一个动画（被打一下）
	bool isAnimationFinished()
	{
		if (isBlending) return false;
		return currentInstance.animationFinished();
	}

private:
	void calculateMatrices()
	{
		if (!isBlending)
		{
			outputInstance = currentInstance;
		}
		else
		{
			float t = blendTime / blendDuration;
			if (t < 0.0f) t = 0.0f;
			if (t > 1.0f) t = 1.0f;

			for (int i = 0; i < 256; i++)
			{
				float* mOut = outputInstance.matrices[i].m;
				float* m1 = currentInstance.matrices[i].m;
				float* m2 = nextInstance.matrices[i].m;

				for (int k = 0; k < 16; k++)
				{
					mOut[k] = m1[k] * (1.0f - t) + m2[k] * t;
				}
			}
		}
	}
};