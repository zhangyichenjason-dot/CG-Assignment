#pragma once
#include "Maths.h"
#include "GameObject.h"
enum class CameraMode
{
	THIRD_PERSON,  // 第三人称跟随相机
	STATIC,        // 静态相机
	ORBITING,      // 环绕相机（使用 cos/sin 公式）
	FIRST_PERSON   // 第一人称相机
};

class CameraManager
{
private:
	CameraMode currentMode;
	float aspectRatio;
	float fov;
	float nearPlane;
	float farPlane;

	// 第三人称相机参数
	Vec3 thirdPersonOffset;  // 相机相对玩家的偏移量
	float thirdPersonDistance;
	float thirdPersonHeight;
	float thirdPersonAngle;

	// 第一人称相机参数
	Vec3 firstPersonOffset;

	// 静态相机参数
	Vec3 staticPosition;
	Vec3 staticTarget;

	// 环绕相机参数
	float orbitRadius;
	float orbitHeight;
	Vec3 orbitCenter;

public:
	CameraManager()
	{
		currentMode = CameraMode::STATIC;
		aspectRatio = 1920.0f / 1080.0f;
		fov = 60.0f;
		nearPlane = 0.1f;
		farPlane = 1000.0f;

		// 默认第三人称参数
		thirdPersonAngle = 13.0f;
		thirdPersonDistance = 20.0f;
		thirdPersonHeight = 6.0f;
		thirdPersonOffset = Vec3(thirdPersonAngle, thirdPersonHeight, thirdPersonDistance);

		// 默认第一人称参数 (高度6.0f, 前向偏移2.0f)
		firstPersonOffset = Vec3(0, 4.0f, 4.8f);

		// 默认静态相机
		staticPosition = Vec3(50, 50, 50);
		staticTarget = Vec3(0, 0, 0);

		// 默认环绕相机
		orbitRadius = 11.0f;
		orbitHeight = 5.0f;
		orbitCenter = Vec3(0, 0, 0);
	}

	// 设置相机模式的接口
	void setMode(CameraMode mode)
	{
		currentMode = mode;
		printf("Camera mode switched to: %d\n", (int)mode);
		fflush(stdout);
	}

	// 配置第三人称相机
	void configThirdPerson(float angle, float distance, float height)
	{
		thirdPersonAngle = angle;
		thirdPersonDistance = distance;
		thirdPersonHeight = height;
		thirdPersonOffset = Vec3(angle, height, distance);
	}

	// 配置第一人称相机
	void configFirstPerson(float height, float forwardOffset)
	{
		firstPersonOffset = Vec3(0, height, forwardOffset);
	}

	// 配置静态相机
	void configStatic(Vec3 position, Vec3 target)
	{
		staticPosition = position;
		staticTarget = target;
	}

	// 配置环绕相机
	void configOrbiting(float radius, float height, Vec3 center)
	{
		orbitRadius = radius;
		orbitHeight = height;
		orbitCenter = center;
	}

	// 获取当前相机的 View-Projection 矩阵(相机)
	Matrix getViewProjection(float time, MoveAnimatedModel* player = nullptr)
	{
		Matrix p = Matrix::perspective(nearPlane, farPlane, aspectRatio, fov);
		Matrix v;
		Vec3 from;
		Vec3 target;

		switch (currentMode)
		{
		case CameraMode::THIRD_PERSON:
			if (player != nullptr)
			{
				// 第三人称相机跟随玩家
				// 相机位置 = 玩家位置 + 偏移
				from = player->position + thirdPersonOffset;
				target = player->position + Vec3(3, 10, 5);
			}
			else
			{
				// 没有玩家时回退到静态相机
				from = staticPosition;
				target = staticTarget;
			}
			v = Matrix::lookAt(from, target, Vec3(0, 1, 0));
			break;

		case CameraMode::FIRST_PERSON:
			if (player != nullptr)
			{
				// 第一人称相机：位于玩家头部，看向前方
				// 根据玩家的 rotationY 计算前方向量 (假设初始朝向为 -Z)
				float theta = player->rotationY;
				Vec3 forward(-sinf(theta), 0.0f, -cosf(theta));

				// 相机位置 = 玩家位置 + 高度偏移 + 前向偏移(避免穿模)
				from = player->position + Vec3(0, firstPersonOffset.y, 0) + forward * firstPersonOffset.z;
				
				// 目标点 = 相机位置 + 前方向量
				target = from + forward;
			}
			else
			{
				from = staticPosition;
				target = staticTarget;
			}
			v = Matrix::lookAt(from, target, Vec3(0, 1, 0));
			break;

		case CameraMode::STATIC:
			// 静态相机
			from = staticPosition;
			target = staticTarget;
			v = Matrix::lookAt(from, target, Vec3(0, 1, 0));
			break;

		case CameraMode::ORBITING:
			// 环绕相机使用 cos/sin 公式
			from = Vec3(
				orbitCenter.x + orbitRadius * cosf(time),
				orbitCenter.y + orbitHeight,
				orbitCenter.z + orbitRadius * sinf(time)
			);
			target = orbitCenter;
			v = Matrix::lookAt(from, target, Vec3(0, 1, 0));
			break;
		}

		return v * p;
	}

	// 获取当前模式
	CameraMode getMode() const
	{
		return currentMode;
	}
};