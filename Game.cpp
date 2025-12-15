#include "Core.h"
#include "Window.h"
#include "Timer.h"
#include "Maths.h"
#include "Shaders.h"
#include "Mesh.h"
#include "PSO.h"
#include "GEMLoader.h"
#include "Animation.h"
#include "Texture.h"
#include "Material.h"
#include <random>
#include <d3dcompiler.h>
#include "Environment.h"
#include "GameObject.h"
#include "Camera.h"
#include "PlayerController.h"
#include "Audio.h" 
#pragma comment(lib, "d3dcompiler.lib")


#define WIDTH 1920
#define HEIGHT 1080

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
	// 初始化随机数种子
	//Initialize random number seed
	srand((unsigned int)time(NULL));

	// 分配并显示控制台窗口
	//Allocate and display console window
	AllocConsole();
	FILE* pFile = nullptr;
	freopen_s(&pFile, "CONOUT$", "w", stdout);
	freopen_s(&pFile, "CONOUT$", "w", stderr);


	Window window;
	window.create(WIDTH, HEIGHT, "My Window");

	fflush(stdout);

	Core core;
	core.init(window.hwnd, WIDTH, HEIGHT);

	Shaders shaders;
	PSOManager psos;

	TextureManager textureManager;
	textureManager.init(&core, 100);
	MaterialManager materialManager(&textureManager);

	// 初始化音频系统
	// Initialize audio system
	AudioSystem audioSystem;
	audioSystem.playBGM("models/2.mp3");
	audioSystem.setVolume(500); // 设置音量 (0-1000)// Set volume


	// 初始化天空球
	// Initialize skybox
	Skybox skybox;
	skybox.init(&core, &psos, &shaders, &textureManager, &materialManager, "Models/Textures/skybox2.png");

	// 远景层类
	// Distant layers
	DistantLayer distantLayer;
	DistantLayer distantLayer2;
	DistantLayer distantLayer3;
	DistantLayer distantLayer35;
	DistantLayer distantLayer4;
	// 设置参数
	// // Set parameters
	float distRadius = 450.0f; // 半径// Radius
	float texW = 576.0f;       // 图片原始宽// Texture original width
	float texH = 324.0f;       // 图片原始高// Texture original height
	int repeatCount = 10;      // 重复 10 次形成一圈// Repeat 10 times to form a circle
	// 初始化
	// Initialize
	distantLayer.init(&core, &psos, &shaders, &textureManager, &materialManager, "Models/Textures/m.png", distRadius, texW, texH, repeatCount);
	// 第二层远景（灌木）
	// Second distant layer (bushes)
	distantLayer2.init(&core, &psos, &shaders, &textureManager, &materialManager, "Models/Textures/m2.png", distRadius, texW, texH, repeatCount);
	// 第三层远景（云）
	// Third distant layer (clouds)
	distantLayer3.init(&core, &psos, &shaders, &textureManager, &materialManager, "Models/Textures/m3.png", distRadius, texW, texH, repeatCount);
	// 第三层远景2（云）
	// Third distant layer 2 (clouds)
	distantLayer35.init(&core, &psos, &shaders, &textureManager, &materialManager, "Models/Textures/m3.png", distRadius, texW, texH, repeatCount);
	// 第四层远景（山脉）
	// Fourth distant layer (mountains)
	distantLayer4.init(&core, &psos, &shaders, &textureManager, &materialManager, "Models/Textures/m4.png", distRadius, texW, texH, repeatCount);


	//小蘑菇
	// Small mushroom
	StaticModel staticModel;
	staticModel.load(&core, "Models/acacia_003.gem", &shaders, &psos, &materialManager);

	//路
	// Road
	StaticModel road;
	road.load(&core, "Models/road_009.gem", &shaders, &psos, &materialManager);
	//路边草
	// Grass beside the road
	StaticModel grass;
	grass.load(&core, "Models/ground_005.gem", &shaders, &psos, &materialManager);
	//小草
	// Small grass
	GrassPatch grassPatch;
	grassPatch.init(&core, &psos, &shaders, &textureManager); 
	//加载5种草的纹理 
	// Load 5 types of grass textures
	grassPatch.addTexture(&textureManager, "Models/Textures/grass.png");
	grassPatch.addTexture(&textureManager, "Models/Textures/grass2.png");
	grassPatch.addTexture(&textureManager, "Models/Textures/grass3.png");
	grassPatch.addTexture(&textureManager, "Models/Textures/grass4.png");
	grassPatch.addTexture(&textureManager, "Models/Textures/grass5.png");



	// 创建玩家模型和实例
	// Create player model and instance
	AnimatedModel animatedModel;
	animatedModel.load(&core, "Models/Duck-white.gem", &psos, &shaders, &materialManager);
	MoveAnimatedModel player;
	player.init(&animatedModel, Vec3(0, 0, -2), Vec3(0.1f, 0.1f, 0.1f));
	player.setSpeed(10.0f); // 设置移动速度// Set movement speed

	// 初始化玩家控制器
	// Initialize player controller
	PlayerController playerController;
	playerController.init(&player, &window);

	// 创建农民模型和实例
	// Create farmer model and instance
	AnimatedModel FamerAnimatedModel;
	FamerAnimatedModel.load(&core, "Models/Farmer-male.gem", &psos, &shaders, &materialManager);

	MoveAnimatedModel farmer;
	farmer.init(&FamerAnimatedModel, Vec3(0, 0, 15), Vec3(0.1f, 0.1f, 0.1f));
	farmer.playAnimation("run", 0.0f); // 设置农民的动画，因为名称和用于鸭子的不同 // Set farmer's animation, as the name is different from duck's
	farmer.setRotation(0, -3.14 / 2.0f); // X轴旋转90度，因为模型默认是躺着的 // Rotate 90 degrees on X-axis, as the model lies flat by default
	farmer.setSpeed(10.0f); // 设置移动速度 // Set movement speed

	//山羊障碍物
	// Goat obstacle
	AnimatedModel goatModel;
	goatModel.load(&core, "Models/Sheep-01.gem", &psos, &shaders, &materialManager);
	

	// 创建地形管理器
	// Create terrain manager
	TerrainManager terrainManager;
	terrainManager.init(&core, &road, &grass, &grassPatch, &goatModel, &staticModel, player.position);


	// 创建相机管理器实例
	// Create camera manager instance
	CameraManager cameraManager;
	cameraManager.setMode(CameraMode::THIRD_PERSON); // 默认设置为第三人称// Default to third-person


	// 创建方向光（太阳）
	// Create directional light (sun)
	DirectionalLight sunLight;
	sunLight.setLightFrom(1.0f, 1.0f, 0.5f); // 设置太阳照射方向// Set sun light direction
	sunLight.intensity = 0.7f;// 光照强度// Light intensity
	sunLight.ambientStrength = 0.28f;// 环境光强度// Ambient light strength


	// 加载带光照的着色器// Load lit shaders
	shaders.load(&core, "StaticModelLit", "VS.txt", "PSLit.txt");
	shaders.load(&core, "StaticModelLitUntextured", "VS.txt", "PSLitUnTextured.txt");
	psos.createPSO(&core, "StaticModelLitPSO",shaders.find("StaticModelLit")->vs,shaders.find("StaticModelLit")->ps,VertexLayoutCache::getStaticLayout());
	psos.createPSO(&core, "StaticModelLitUntexturedPSO",shaders.find("StaticModelLitUntextured")->vs,shaders.find("StaticModelLitUntextured")->ps,VertexLayoutCache::getStaticLayout());
	// 加载带光照的着色器（动画模型）// Load lit shaders (animated models)
	shaders.load(&core, "AnimatedLit", "VSAnim.txt", "PSLit.txt");
	shaders.load(&core, "AnimatedLitUntextured", "VSAnim.txt", "PSLitUnTextured.txt");
	psos.createPSO(&core, "AnimatedModelLitPSO",shaders.find("AnimatedLit")->vs,shaders.find("AnimatedLit")->ps,VertexLayoutCache::getAnimatedLayout());
	psos.createPSO(&core, "AnimatedModelLitUntexturedPSO",shaders.find("AnimatedLitUntextured")->vs,shaders.find("AnimatedLitUntextured")->ps,VertexLayoutCache::getAnimatedLayout());

	Timer timer;
	float t = 0;
	static float sunPitch = 45.0f;
	static float sunYaw = 45.0f;

	// 碰撞计数器和游戏状态// Collision counter and game state
	int collisionCount = 0;
	// 定义游戏状态枚举// Define game state enum
	enum GameState {
		PLAYING,
		GAMEOVER_WALK, // 农民走向玩家 // Farmer walks towards the player
		GAMEOVER_GRAB  // 农民抓起玩家 // Farmer grabs the player
	};
	GameState gameState = PLAYING;


	while (1)
	{

		core.beginFrame();
		float dt = timer.dt();
		window.checkInput();
		if (window.keys[VK_ESCAPE] == 1)
		{
			break;
		}

		// 相机模式切换逻辑// Camera mode switching logic
		// 按 1 切换到第一人称// Press 1 to switch to first-person
		if (window.keys['1'] && cameraManager.getMode() != CameraMode::FIRST_PERSON)
		{
			cameraManager.setMode(CameraMode::FIRST_PERSON);
		}
		// 按 2 切换到第三人称// Press 2 to switch to third-person
		if (window.keys['2'] && cameraManager.getMode() != CameraMode::THIRD_PERSON)
		{
			cameraManager.setMode(CameraMode::THIRD_PERSON);
		}
		// 按 3 切换到静态相机// Press 3 to switch to static camera
		if (window.keys['3'] && cameraManager.getMode() != CameraMode::STATIC)
		{
			cameraManager.setMode(CameraMode::STATIC);
		}

		t += dt;

		// ！！！！！游戏逻辑！！！！！// !!!!!Game Logic!!!!!!!
		if (gameState == PLAYING)
		{
			// 检测碰撞（假设玩家半径 5.5）// Check collisions (assuming player radius 5.5)
			int newHits = terrainManager.checkCollisions(player.position, 5.5f);
			if (newHits > 0)
			{
				collisionCount += newHits;
				printf("Total Collisions: %d\n", collisionCount);

				// 如果碰撞达到2次，进入游戏结束流程// If collisions reach 2, enter game over process
				if (collisionCount >= 2)
				{
					gameState = GAMEOVER_WALK;

					// 1. 玩家死亡// Player dies
					player.setSpeed(0.0f); 
					player.playAnimation("death", 0.2f, false); // 播放死亡动画，不循环// Play death animation, non-looping

					// 2. 农民切换到走路状态// Farmer switches to walking state
					farmer.setSpeed(0.0f); // 停止自动奔跑// Stop auto-running
					farmer.playAnimation("walk", 0.2f); // 切换到走路动画// Switch to walking animation
				}
				else
				{
					// Not the last life: Play hit animation (non-looping)// 不是最后一条命：播放受击动画（不循环）
					player.playAnimation("hit reaction", 0.1f, false);
				}
			}
			// Check if hit animation finished, then resume running// 检查受击动画是否完成，然后恢复奔跑
			if (player.stateMachine.getState() == "hit reaction" && player.stateMachine.isAnimationFinished())
			{
				player.playAnimation("run forward", 0.2f, true);
			}


			// 只有游戏未结束时，才允许玩家左右移动// Only allow player to move left/right when game is not over
			playerController.update(dt);
		}
		else if (gameState == GAMEOVER_WALK)
		{
			// 农民走向玩家// Farmer walks towards the player
			// 目标位置：玩家位置前方一点点（Z轴正方向是后方，所以加一点Z）// Target position: a bit in front of the player
			Vec3 targetPos = player.position;
			targetPos.z += 5.0f; // 停在玩家身后3米处// Stop 3 meters behind the player

			Vec3 dir = targetPos - farmer.position;
			float dist = dir.length();

			if (dist < 0.1f)
			{
				// 到达位置，开始抓取// Arrived at position, start grabbing
				gameState = GAMEOVER_GRAB;
				farmer.playAnimation("grab low", 0.2f, false); // 播放抓取动画// Play grab animation
			}
			else
			{
				// 移动农民// Move farmer
				dir = dir.normalize();
				float walkSpeed = 5.0f;
				farmer.position += dir * walkSpeed * dt;
			}
		}

		//相机// Camera
		Matrix vp = cameraManager.getViewProjection(t, &player);

		core.beginRenderPass();


		// 更新光照常量到着色器// Update lighting constants to shaders
		Vec3 lightDir = sunLight.getLightDirectionForShader();
		shaders.updateConstantPS("StaticModelLit", "LightBuffer", "lightDirection", &lightDir);
		shaders.updateConstantPS("StaticModelLit", "LightBuffer", "lightIntensity", &sunLight.intensity);
		shaders.updateConstantPS("StaticModelLit", "LightBuffer", "lightColor", &sunLight.color);
		shaders.updateConstantPS("StaticModelLit", "LightBuffer", "ambientStrength", &sunLight.ambientStrength);

		// 更新动画模型的光照常量// Update animated model lighting constants
		shaders.updateConstantPS("AnimatedLit", "LightBuffer", "lightDirection", &lightDir);
		shaders.updateConstantPS("AnimatedLit", "LightBuffer", "lightIntensity", &sunLight.intensity);
		shaders.updateConstantPS("AnimatedLit", "LightBuffer", "lightColor", &sunLight.color);
		shaders.updateConstantPS("AnimatedLit", "LightBuffer", "ambientStrength", &sunLight.ambientStrength);


		// 首先绘制天空球（获取相机位置）// First draw skybox (get camera position)
		Vec3 skyboxCenter = Vec3(0, 0, 0);
		// 第一人称和第三人称时，天空球和远景层都跟随玩家移动// In first-person and third-person, skybox and distant layers follow player movement
		if (cameraManager.getMode() == CameraMode::THIRD_PERSON || cameraManager.getMode() == CameraMode::FIRST_PERSON)
		{
			skyboxCenter = player.position;
		}
		float skyEmissive = 1.2f; // 增强天空球亮度// Enhance skybox brightness
		skybox.draw(&core, &psos, &shaders, vp, skyboxCenter, &textureManager, skyEmissive);

		// 绘制远景层// Draw distant layers
		float layerEmissive = 1.0f;
		// 位置调整：
		// 只需要平移到天空球中心，并稍微调整Y轴高度
		// 因为现在是360度圆环，不需要推到远处，它已经包围了相机
		Matrix distantLayerWorld = Matrix::translation(skyboxCenter + Vec3(0, -10.0f, 0));
		Matrix distantLayerWorld2 = Matrix::translation(skyboxCenter + Vec3(100, -38.0f, 0));// 第三层远景稍微高一点
		//绘制需要倒序进行，从最远的开始画起
		// 第四层远景（山脉）
		distantLayer4.draw(&core, &psos, &shaders, vp,distantLayerWorld, &textureManager, layerEmissive);
		// 第三层远景（云）
		//distantLayer3.draw(&core, &psos, &shaders, vp, distantLayerWorld2, &textureManager, layerEmissive+0.33f);
		// 第三层远景2（云）
		//distantLayer35.draw(&core, &psos, &shaders, vp, distantLayerWorld, &textureManager, layerEmissive + 0.35f);
		// 第二层远景
		distantLayer2.draw(&core, &psos, &shaders, vp, distantLayerWorld, &textureManager, layerEmissive);
		// 第一层远景
		distantLayer.draw(&core, &psos, &shaders, vp, distantLayerWorld, &textureManager, layerEmissive);
		


		// 更新并绘制地形// Update and draw  terrain
		terrainManager.update(player.position, dt);
		terrainManager.drawLit(&core, &psos, &shaders, vp, &textureManager, &sunLight, dt);


		// 画静态模型 - 传入 sunLight 指针// Draw static model - pass in sunLight
		Matrix W;
		W = Matrix::scaling(Vec3(0.01f, 0.01f, 0.01f)) * Matrix::translation(Vec3(5, 0, 0));
		staticModel.drawLit(&core, &psos, &shaders, vp, W, &textureManager, &sunLight);

		// 画另一个静态模型// Draw another static model
		W = Matrix::scaling(Vec3(0.01f, 0.01f, 0.01f)) * Matrix::translation(Vec3(10, 0, 0));
		staticModel.drawLit(&core, &psos, &shaders, vp, W, &textureManager, &sunLight);

		// 更新并绘制玩家// Update and draw player
		player.update(dt);
		player.drawLit(&core, &psos, &shaders, vp, &textureManager, &sunLight);

		// 更新并绘制农民// Update and draw farmer
		farmer.update(dt);
		farmer.drawLit(&core, &psos, &shaders, vp, &textureManager, &sunLight);

		core.finishFrame();
	}

	core.flushGraphicsQueue();

	// 清理控制台// Clean up console
	if (pFile) fclose(pFile);
	FreeConsole();
}