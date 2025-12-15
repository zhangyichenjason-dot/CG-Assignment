#pragma once
#include <vector>
#include <fstream> 
#include <sstream> 
#include "Mesh.h"
#include "Material.h"
#include "Shaders.h"
#include "PSO.h"
#include "Animation.h"
#include "Environment.h"
#include "StateMechine.h" 



//静态模型类
class StaticModel
{
public:
	std::vector<Mesh*> meshes;
	std::vector<Material*> materials;
	bool hasTextures;

	StaticModel()
	{
		hasTextures = false;
	}

	void load(Core* core, std::string filename, Shaders* shaders, PSOManager* psos, MaterialManager* materialManager)
	{
		printf("\n=== Loading Static Model: %s ===\n", filename.c_str());
		fflush(stdout);

		GEMLoader::GEMModelLoader loader;
		std::vector<GEMLoader::GEMMesh> gemmeshes;
		loader.load(filename, gemmeshes);

		printf("Found %d meshes\n", (int)gemmeshes.size());
		fflush(stdout);

		
		std::string modelFolder = "Models/Textures";

		for (int i = 0; i < gemmeshes.size(); i++)
		{
			printf("\nMesh %d:\n", i);
			printf("  Material properties count: %d\n", (int)gemmeshes[i].material.properties.size());

			
			for (int j = 0; j < gemmeshes[i].material.properties.size(); j++)
			{
				printf("    Property[%d]: name='%s', value='%s'\n",
					j,
					gemmeshes[i].material.properties[j].name.c_str(),
					gemmeshes[i].material.properties[j].value.c_str());
			}
			fflush(stdout);

			Mesh* mesh = new Mesh();
			std::vector<STATIC_VERTEX> vertices;
			for (int j = 0; j < gemmeshes[i].verticesStatic.size(); j++)
			{
				STATIC_VERTEX v;
				memcpy(&v, &gemmeshes[i].verticesStatic[j], sizeof(STATIC_VERTEX));
				vertices.push_back(v);
			}
			mesh->init(core, vertices, gemmeshes[i].indices);
			meshes.push_back(mesh);

			
			Material* material = materialManager->createMaterial(gemmeshes[i].material, modelFolder);
			materials.push_back(material);
			if (material->hasTexture)
			{
				hasTextures = true;
				printf("  -> Model has textures\n");
			}
			else
			{
				printf("  -> No textures for this mesh\n");
			}
			fflush(stdout);
		}

		printf("\nModel loaded. hasTextures = %s\n", hasTextures ? "true" : "false");
		fflush(stdout);

		
		shaders->load(core, "StaticModelUntextured", "VS.txt", "PSUntextured.txt");
		shaders->load(core, "StaticModelTextured", "VS.txt", "PSTextured.txt");
		psos->createPSO(core, "StaticModelPSO", shaders->find("StaticModelUntextured")->vs, shaders->find("StaticModelUntextured")->ps, VertexLayoutCache::getStaticLayout());
		psos->createPSO(core, "StaticModelTexturedPSO", shaders->find("StaticModelTextured")->vs, shaders->find("StaticModelTextured")->ps, VertexLayoutCache::getStaticLayout());
	}
	//
	void updateWorld(Shaders* shaders, Matrix& w)
	{
		std::string shaderName = hasTextures ? "StaticModelTextured" : "StaticModelUntextured";
		shaders->updateConstantVS(shaderName, "staticMeshBuffer", "W", &w);
	}

	void draw(Core* core, PSOManager* psos, Shaders* shaders, Matrix& vp, TextureManager* textureManager)
	{
		std::string shaderName = hasTextures ? "StaticModelTextured" : "StaticModelUntextured";
		std::string psoName = hasTextures ? "StaticModelTexturedPSO" : "StaticModelPSO";

		shaders->updateConstantVS(shaderName, "staticMeshBuffer", "VP", &vp);

		if (hasTextures)
		{
			textureManager->bindHeap(core);
		}

		shaders->apply(core, shaderName);
		psos->bind(core, psoName);

		for (int i = 0; i < meshes.size(); i++)
		{
			if (hasTextures && materials[i]->hasTexture)
			{
				materials[i]->bind(core);
			}
			meshes[i]->draw(core);
		}
	}

	
	void drawLit(Core* core, PSOManager* psos, Shaders* shaders, Matrix& vp, Matrix& w, TextureManager* textureManager, DirectionalLight* light)
	{
		std::string shaderName = hasTextures ? "StaticModelLit" : "StaticModelLitUntextured";
		std::string psoName = hasTextures ? "StaticModelLitPSO" : "StaticModelLitUntexturedPSO";

		shaders->updateConstantVS(shaderName, "staticMeshBuffer", "VP", &vp);
		shaders->updateConstantVS(shaderName, "staticMeshBuffer", "W", &w);

		
		Vec3 lightDir = light->getLightDirectionForShader();
		shaders->updateConstantPS(shaderName, "LightBuffer", "lightDirection", &lightDir);
		shaders->updateConstantPS(shaderName, "LightBuffer", "lightIntensity", &light->intensity);
		shaders->updateConstantPS(shaderName, "LightBuffer", "lightColor", &light->color);
		shaders->updateConstantPS(shaderName, "LightBuffer", "ambientStrength", &light->ambientStrength);

		if (hasTextures)
		{
			textureManager->bindHeap(core);
		}

		shaders->apply(core, shaderName);
		psos->bind(core, psoName);

		for (int i = 0; i < meshes.size(); i++)
		{
			if (hasTextures && materials[i]->hasTexture)
			{
				materials[i]->bind(core);
			}
			meshes[i]->draw(core);
		}
	}

};

class AnimatedModel
{
public:
	std::vector<Mesh*> meshes;
	Animation animation;
	std::vector<Material*> materials;
	bool hasTextures;

	AnimatedModel()
	{
		hasTextures = false;
	}

	void load(Core* core, std::string filename, PSOManager* psos, Shaders* shaders, MaterialManager* materialManager)
	{
		printf("\n=== Loading Animated Model: %s ===\n", filename.c_str());
		fflush(stdout);

		GEMLoader::GEMModelLoader loader;
		std::vector<GEMLoader::GEMMesh> gemmeshes;
		GEMLoader::GEMAnimation gemanimation;
		loader.load(filename, gemmeshes, gemanimation);

		printf("Found %d meshes\n", (int)gemmeshes.size());
		fflush(stdout);

		std::string modelFolder = "Models/Textures";

		for (int i = 0; i < gemmeshes.size(); i++)
		{
			printf("\nMesh %d:\n", i);
			printf("  Material properties count: %d\n", (int)gemmeshes[i].material.properties.size());

			
			for (int j = 0; j < gemmeshes[i].material.properties.size(); j++)
			{
				printf("    Property[%d]: name='%s', value='%s'\n",
					j,
					gemmeshes[i].material.properties[j].name.c_str(),
					gemmeshes[i].material.properties[j].value.c_str());
			}
			fflush(stdout);

			Mesh* mesh = new Mesh();
			std::vector<ANIMATED_VERTEX> vertices;
			for (int j = 0; j < gemmeshes[i].verticesAnimated.size(); j++)
			{
				ANIMATED_VERTEX v;
				memcpy(&v, &gemmeshes[i].verticesAnimated[j], sizeof(ANIMATED_VERTEX));
				vertices.push_back(v);
			}
			mesh->init(core, vertices, gemmeshes[i].indices);
			meshes.push_back(mesh);

			
			Material* material = materialManager->createMaterial(gemmeshes[i].material, modelFolder);
			materials.push_back(material);
			if (material->hasTexture)
			{
				hasTextures = true;
				printf("  -> Model has textures\n");
			}
			else
			{
				printf("  -> No textures for this mesh\n");
			}
			fflush(stdout);
		}

		printf("\nModel loaded. hasTextures = %s\n", hasTextures ? "true" : "false");
		fflush(stdout);

		shaders->load(core, "AnimatedUntextured", "VSAnim.txt", "PSUntextured.txt");
		shaders->load(core, "AnimatedTextured", "VSAnim.txt", "PSTextured.txt");
		psos->createPSO(core, "AnimatedModelPSO", shaders->find("AnimatedUntextured")->vs, shaders->find("AnimatedUntextured")->ps, VertexLayoutCache::getAnimatedLayout());
		psos->createPSO(core, "AnimatedModelTexturedPSO", shaders->find("AnimatedTextured")->vs, shaders->find("AnimatedTextured")->ps, VertexLayoutCache::getAnimatedLayout());

		memcpy(&animation.skeleton.globalInverse, &gemanimation.globalInverse, 16 * sizeof(float));
		for (int i = 0; i < gemanimation.bones.size(); i++)
		{
			Bone bone;
			bone.name = gemanimation.bones[i].name;
			memcpy(&bone.offset, &gemanimation.bones[i].offset, 16 * sizeof(float));
			bone.parentIndex = gemanimation.bones[i].parentIndex;
			animation.skeleton.bones.push_back(bone);
		}
		for (int i = 0; i < gemanimation.animations.size(); i++)
		{
			std::string name = gemanimation.animations[i].name;
			AnimationSequence aseq;
			aseq.ticksPerSecond = gemanimation.animations[i].ticksPerSecond;
			for (int j = 0; j < gemanimation.animations[i].frames.size(); j++)
			{
				AnimationFrame frame;
				for (int index = 0; index < gemanimation.animations[i].frames[j].positions.size(); index++)
				{
					Vec3 p;
					Quaternion q;
					Vec3 s;
					memcpy(&p, &gemanimation.animations[i].frames[j].positions[index], sizeof(Vec3));
					frame.positions.push_back(p);
					memcpy(&q, &gemanimation.animations[i].frames[j].rotations[index], sizeof(Quaternion));
					frame.rotations.push_back(q);
					memcpy(&s, &gemanimation.animations[i].frames[j].scales[index], sizeof(Vec3));
					frame.scales.push_back(s);
				}
				aseq.frames.push_back(frame);
			}
			animation.animations.insert({ name, aseq });
		}
	}

	void updateWorld(Shaders* shaders, Matrix& w)
	{
		std::string shaderName = hasTextures ? "AnimatedTextured" : "AnimatedUntextured";
		shaders->updateConstantVS(shaderName, "staticMeshBuffer", "W", &w);
	}

	void draw(Core* core, PSOManager* psos, Shaders* shaders, AnimationInstance* instance, Matrix& vp, Matrix& w, TextureManager* textureManager)
	{
		std::string shaderName = hasTextures ? "AnimatedTextured" : "AnimatedUntextured";
		std::string psoName = hasTextures ? "AnimatedModelTexturedPSO" : "AnimatedModelPSO";

		
		if (meshes.empty())
		{
			printf("ERROR: AnimatedModel has no meshes!\n");
			return;
		}

		if (instance == nullptr)
		{
			printf("ERROR: AnimationInstance is null!\n");
			return;
		}

		if (hasTextures)
		{
			textureManager->bindHeap(core);
		}

		// 先更新常量
		shaders->updateConstantVS(shaderName, "staticMeshBuffer", "W", &w);
		shaders->updateConstantVS(shaderName, "staticMeshBuffer", "VP", &vp);
		shaders->updateConstantVS(shaderName, "staticMeshBuffer", "bones", instance->matrices);

		// 然后应用着色器（在 PSO 之前）
		shaders->apply(core, shaderName);

		// 最后绑定 PSO
		psos->bind(core, psoName);

		for (int i = 0; i < meshes.size(); i++)
		{
			if (hasTextures && materials[i]->hasTexture)
			{
				materials[i]->bind(core);
			}
			meshes[i]->draw(core);
		}
	}
	
	void drawLit(Core* core, PSOManager* psos, Shaders* shaders, AnimationInstance* instance, Matrix& vp, Matrix& w, TextureManager* textureManager, DirectionalLight* light)
	{
		std::string shaderName = hasTextures ? "AnimatedLit" : "AnimatedLitUntextured";
		std::string psoName = hasTextures ? "AnimatedModelLitPSO" : "AnimatedModelLitUntexturedPSO";

		if (meshes.empty() || instance == nullptr)
		{
			return;
		}

		if (hasTextures)
		{
			textureManager->bindHeap(core);
		}

		shaders->updateConstantVS(shaderName, "staticMeshBuffer", "W", &w);
		shaders->updateConstantVS(shaderName, "staticMeshBuffer", "VP", &vp);
		shaders->updateConstantVS(shaderName, "staticMeshBuffer", "bones", instance->matrices);

		// 每次绘制前都更新光照常量
		Vec3 lightDir = light->getLightDirectionForShader();
		shaders->updateConstantPS(shaderName, "LightBuffer", "lightDirection", &lightDir);
		shaders->updateConstantPS(shaderName, "LightBuffer", "lightIntensity", &light->intensity);
		shaders->updateConstantPS(shaderName, "LightBuffer", "lightColor", &light->color);
		shaders->updateConstantPS(shaderName, "LightBuffer", "ambientStrength", &light->ambientStrength);

		shaders->apply(core, shaderName);
		psos->bind(core, psoName);

		for (int i = 0; i < meshes.size(); i++)
		{
			if (hasTextures && materials[i]->hasTexture)
			{
				materials[i]->bind(core);
			}
			meshes[i]->draw(core);
		}
	}
};
//可位移的动画模型类（用到动画模型类）
class MoveAnimatedModel
{
public:
	AnimatedModel* model;
	// 替换为状态机
	StateMachine stateMachine;
	Vec3 position;
	Vec3 scale;
	float speed;
	float rotationX;
	float rotationY;
	float rotationZ;
	

	MoveAnimatedModel()
	{
		position = Vec3(0, 0, 0);
		scale = Vec3(0.1f, 0.1f, 0.1f);
		speed = 1.0f;
		rotationX = 0.0f;
		rotationY = 0.0f;
		rotationZ = 0.0f;
		model = nullptr;
	}

	void init(AnimatedModel* _model, Vec3 startPosition, Vec3 _scale)
	{
		model = _model;
		position = startPosition;
		scale = _scale;
		//初始化状态机//Initialize state machine
		stateMachine.init(&model->animation);
		if (model->animation.hasAnimation("run forward"))
		{
			stateMachine.changeState("run forward", 0.0f);
		}
	}

	void update(float dt)
	{
		// 更新状态机// Update state machine
		stateMachine.update(dt * 0.5f);

		// 向前移动（Z轴正方向）// Move forward (positive Z direction)
		position.z -= speed * dt;
	}

	// 增加 loop 参数，默认为 true// Add loop parameter, default to true
	void playAnimation(std::string name, float blendTime = 0.2f, bool loop = true)
	{
		stateMachine.changeState(name, blendTime, loop);
	}

	void draw(Core* core, PSOManager* psos, Shaders* shaders, Matrix& vp, TextureManager* textureManager)
	{
		// 构建世界矩阵：缩放 -> 旋转 -> 平移
		Matrix W = Matrix::scaling(scale) *
			Matrix::rotateX(rotationX) *
			Matrix::rotateY(rotationY) *
			Matrix::rotateZ(rotationZ) *
			Matrix::translation(position);

		// 传入状态机的渲染实例
		model->draw(core, psos, shaders, stateMachine.getRenderInstance(), vp, W, textureManager);
	}
	
	void drawLit(Core* core, PSOManager* psos, Shaders* shaders, Matrix& vp, TextureManager* textureManager, DirectionalLight* light)
	{
		Matrix W = Matrix::scaling(scale) *
			Matrix::rotateX(rotationX) *
			Matrix::rotateY(rotationY) *
			Matrix::rotateZ(rotationZ) *
			Matrix::translation(position);

		// 传入状态机的渲染实例
		model->drawLit(core, psos, shaders, stateMachine.getRenderInstance(), vp, W, textureManager, light);
	}

	// 设置起点和缩放的接口
	void setTransform(Vec3 startPosition, Vec3 _scale)
	{
		position = startPosition;
		scale = _scale;
	}

	// 设置速度
	void setSpeed(float _speed)
	{
		speed = _speed;
	}

	// 设置旋转角度（弧度）
	void setRotation(int xyz, float _rotation)
	{
		if (xyz == 0) rotationX = _rotation;
		else if (xyz == 1) rotationY = _rotation;
		else if (xyz == 2) rotationZ = _rotation;
	}
};
//专门的障碍物类-用来处理障碍物逻辑-本质是动画模型（比如前方是正在吃草的山羊）
class Obstacle {
public:
	AnimatedModel* model;
	StateMachine stateMachine;
	Vec3 position;
	Vec3 scale;
	float rotationY;
	bool hit;              
	float collisionRadius; 

	Obstacle()
	{
		model = nullptr;
		position = Vec3(0, 0, 0);
		scale = Vec3(0.1f, 0.1f, 0.1f);
		rotationY = 0.0f;
		hit = false;
		collisionRadius = 0.5f;
	}

	void init(AnimatedModel* _model, Vec3 _pos, float _rotY, float _scale)
	{
		model = _model;
		position = _pos;
		rotationY = _rotY;
		scale = Vec3(_scale, _scale, _scale);
		hit = false;            
		collisionRadius = 0.8f; 

		if (model)
		{
			
			stateMachine.init(&model->animation);
			stateMachine.changeState("eating", 0.0f);

			
			float randomOffset = ((float)rand() / RAND_MAX) * 5.0f;
			stateMachine.update(randomOffset);
		}
	}

	void update(float dt)
	{
		if (!model) return;
		
		stateMachine.update(dt);
	}

	
	void playAnimation(std::string name, float blendTime = 0.2f, bool loop = true)
	{
		stateMachine.changeState(name, blendTime, loop);
	}

	void drawLit(Core* core, PSOManager* psos, Shaders* shaders, Matrix& vp, TextureManager* textureManager, DirectionalLight* light)
	{
		if (!model) return;

		Matrix W = Matrix::scaling(scale) *
			Matrix::rotateY(rotationY) *
			Matrix::translation(position);

		
		model->drawLit(core, psos, shaders, stateMachine.getRenderInstance(), vp, W, textureManager, light);
	}
};
//地块类-包含障碍物和草地实例化逻辑
class TerrainTile
{
public:
	Vec3 position;
	float length;  // 地块沿Z轴的长度

	// 障碍物
	std::vector<Obstacle> obstacles;

	// 装饰物（蘑菇）
	struct Decoration {
		Vec3 position;
		float scale;
		float rotationY;
	};
	std::vector<Decoration> decorations;

	// 硬件实例化资源管理
	static const int MAX_GRASS_TYPES = 5;
	ID3D12Resource* instanceBuffers[MAX_GRASS_TYPES];
	int instanceCounts[MAX_GRASS_TYPES];

	// 记录当前 Buffer 的容量（能装多少个实例）
	int bufferCapacities[MAX_GRASS_TYPES];
	bool buffersCreated;

	TerrainTile()
	{
		position = Vec3(0, 0, 0);
		length = 20.0f;
		buffersCreated = false;
		for (int i = 0; i < MAX_GRASS_TYPES; i++) {
			instanceBuffers[i] = nullptr;
			instanceCounts[i] = 0;
			bufferCapacities[i] = 0; // 初始化容量为0
		}
	}

	// 析构函数
	~TerrainTile()
	{
		cleanupBuffers();
	}
	void cleanupBuffers()
	{
		for (int i = 0; i < MAX_GRASS_TYPES; i++) {
			if (instanceBuffers[i]) {
				// 但如果在运行时手动调用 cleanup，必须先 flush
				instanceBuffers[i]->Release();
				instanceBuffers[i] = nullptr;
			}
			instanceCounts[i] = 0;
			bufferCapacities[i] = 0;
		}
		buffersCreated = false;
	}

	// 碰撞检测逻辑
	int checkCollisions(Vec3 playerPos, float playerRadius)
	{
		int hits = 0;
		for (auto& obs : obstacles)
		{
			if (obs.hit) continue;
			float dx = playerPos.x - obs.position.x;
			float dz = playerPos.z - obs.position.z;
			float distSq = dx * dx + dz * dz;
			float minDist = playerRadius + obs.collisionRadius;
			if (distSq < minDist * minDist)
			{
				obs.hit = true;
				hits++;
				printf("Collision Detected! Obstacle at Z: %.2f\n", obs.position.z);
			}
		}
		return hits;
	}

	// 智能生成草（复用 Buffer + 安全扩容）
	void generateGrass(Core* core)
	{
		// 1. CPU 端计算数据 (保持不变)
		std::vector<InstanceData> cpuInstances[MAX_GRASS_TYPES];

		
		float minVal = -4.0f;
		float maxVal = 4.0f;

		auto randomOffset = [minVal, maxVal]() {
			float r = (float)rand() / RAND_MAX;
			return r * (maxVal - minVal) + minVal;
			};
		auto randomType = []() { return rand() % MAX_GRASS_TYPES; };

		auto addGrass = [&](float xBase, float zBase) {
			float offsetX = randomOffset();
			float offsetZ = randomOffset();
			int type = randomType();
			Vec3 finalPos = position + Vec3(xBase + offsetX, 0.0f, zBase + offsetZ);
			float scale = 5.0f;
			Matrix W = Matrix::scaling(Vec3(scale, scale, scale)) * Matrix::translation(finalPos);
			InstanceData data;
			data.worldMatrix = W;
			cpuInstances[type].push_back(data);
			};

		//右侧
		for (int x = -30; x <= -0; x++) {
			for (int z = -6; z <= 6; z++) {
				addGrass(x * 5.0f - 17.0f, z * 10.0f);
			}
		}
		//左侧
		for (int x = 0; x <= 5; x++) {
			for (int z = -6; z <= 6; z++) {
				addGrass(x * 5.0f + 17.0f, z * 10.0f);
			}
		}

		// 2. 智能更新 GPU Buffer
		for (int i = 0; i < MAX_GRASS_TYPES; i++)
		{
			int newCount = (int)cpuInstances[i].size();
			instanceCounts[i] = newCount;

			if (newCount > 0)
			{
				// 情况 A: 现有的 Buffer 太小，或者还没有 Buffer
				if (instanceBuffers[i] == nullptr || newCount > bufferCapacities[i])
				{
					// 安全措施：如果旧 Buffer 存在，必须等待 GPU 用完它才能删除
					if (instanceBuffers[i] != nullptr)
					{
						core->flushGraphicsQueue(); // 强制同步 GPU，防止崩溃
						instanceBuffers[i]->Release();
						instanceBuffers[i] = nullptr;
					}

					// 计算新容量：预留 1.5 倍空间，防止下次稍微多一点草又要重新分配
					int newCapacity = (int)(newCount * 1.5f);
					if (newCapacity < 64) newCapacity = 64; // 最小分配大小
					bufferCapacities[i] = newCapacity;

					int bufferSize = newCapacity * sizeof(InstanceData);

					// 创建新 Buffer
					D3D12_HEAP_PROPERTIES heapProps = {};
					heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
					heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
					heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
					heapProps.CreationNodeMask = 1;
					heapProps.VisibleNodeMask = 1;

					D3D12_RESOURCE_DESC bufferDesc = {};
					bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
					bufferDesc.Width = bufferSize;
					bufferDesc.Height = 1;
					bufferDesc.DepthOrArraySize = 1;
					bufferDesc.MipLevels = 1;
					bufferDesc.SampleDesc.Count = 1;
					bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
					bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

					core->device->CreateCommittedResource(
						&heapProps, D3D12_HEAP_FLAG_NONE,
						&bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
						nullptr, IID_PPV_ARGS(&instanceBuffers[i]));
				}

				// 情况 B: Buffer 够大，直接更新数据（这是最快且最安全的路径）
				// Map -> Copy -> Unmap
				void* pData;
				// 注意：这里只拷贝实际需要的数量 (newCount)，而不是整个容量
				instanceBuffers[i]->Map(0, nullptr, &pData);
				memcpy(pData, cpuInstances[i].data(), newCount * sizeof(InstanceData));
				instanceBuffers[i]->Unmap(0, nullptr);
			}
		}
		buffersCreated = true;
	}

	// 生成障碍物
	void generateObstacles(AnimatedModel* model, int obstacleType)
	{
		obstacles.clear();
		if (!model) return;

		// obstacleType: 0=None, 1=Left, 2=Right
		if (obstacleType == 0) return;

		Obstacle obs;
		bool isLeft = (obstacleType == 1);
		float xPos = isLeft ? 5.0f : -5.3f;
		float rotY = isLeft ? 1.57f : -1.57f;
		
		// Z轴偏移仍然保留一点随机性，增加视觉变化
		float zOffset = ((float)rand() / RAND_MAX - 0.5f) * (length * 0.6f);
		Vec3 obsPos = position + Vec3(xPos, -0.8f, zOffset);
		obs.init(model, obsPos, rotY, 0.11f);
		obstacles.push_back(obs);
	}

	// 生成装饰物
	void generateDecorations(int count)
	{
		decorations.clear();
		
		for (int i = 0; i < count; i++)
		{
			Decoration dec;
			// 随机位置，避开道路 (道路宽约 10，左右各 5)
			// 在 +/- 8 到 +/- 25 之间
			bool isLeft = (rand() % 2 == 0);
			float xOffset = 18.0f + ((float)rand() / RAND_MAX) * 17.0f;
			float x = isLeft ? xOffset : -xOffset;

			// Z轴随机
			float zOffset = ((float)rand() / RAND_MAX - 0.5f) * length;

			dec.position = position + Vec3(x, -0.8f, zOffset);

			// 随机缩放 (参考 Game.cpp 中的 0.01f)
			float randomScale = 0.007f + ((float)rand() / RAND_MAX) * 0.01f; 
			dec.scale = randomScale;

			dec.rotationY = ((float)rand() / RAND_MAX) * 6.28f;

			decorations.push_back(dec);
		}
	}

	// 更新 
	void update(float dt)
	{
		for (auto& obs : obstacles) obs.update(dt);
	}

	void setPosition(Vec3 pos) { position = pos; }

	// 绘制 (不带光照)
	void draw(Core* core, PSOManager* psos, Shaders* shaders, Matrix& vp,
		StaticModel* road, StaticModel* grass, TextureManager* textureManager)
	{
		Matrix W;
		shaders->updateConstantVS("StaticModelUntextured", "staticMeshBuffer", "VP", &vp);
		W = Matrix::scaling(Vec3(0.07f, 0.01f, 0.04f)) * Matrix::translation(position + Vec3(0, -0.8f, 0));
		road->updateWorld(shaders, W);
		road->draw(core, psos, shaders, vp, textureManager);

		W = Matrix::scaling(Vec3(0.01f, 0.01f, 0.06f)) * Matrix::translation(position + Vec3(-20.0f, -0.7f, 0));
		grass->updateWorld(shaders, W);
		grass->draw(core, psos, shaders, vp, textureManager);

		W = Matrix::scaling(Vec3(0.01f, 0.01f, 0.06f)) * Matrix::translation(position + Vec3(15.0f, -0.7f, 0));
		grass->updateWorld(shaders, W);
		grass->draw(core, psos, shaders, vp, textureManager);
	}

	// 绘制 (带光照)
	void drawLit(Core* core, PSOManager* psos, Shaders* shaders, Matrix& vp,
		StaticModel* road, StaticModel* grass, GrassPatch* grassPatch, StaticModel* decorationModel,
		TextureManager* textureManager, DirectionalLight* light, float dt)
	{
		Matrix W;
		// 1. 路
		W = Matrix::scaling(Vec3(0.07f, 0.01f, 0.04f)) * Matrix::translation(position + Vec3(0, -0.8f, 0));
		road->drawLit(core, psos, shaders, vp, W, textureManager, light);

		// 2. 路边草
		W = Matrix::scaling(Vec3(0.03f, 0.01f, 0.06f)) * Matrix::translation(position + Vec3(-29.9f, -0.7f, 0));
		grass->drawLit(core, psos, shaders, vp, W, textureManager, light);

		W = Matrix::scaling(Vec3(0.02f, 0.01f, 0.06f)) * Matrix::translation(position + Vec3(20.0f, -0.7f, 0));
		grass->drawLit(core, psos, shaders, vp, W, textureManager, light);

		// 3. 实例化草阵
		if (buffersCreated)
		{
			for (int i = 0; i < MAX_GRASS_TYPES; i++)
			{
				if (instanceCounts[i] > 0 && instanceBuffers[i])
				{
					grassPatch->drawInstanced(core, psos, shaders, vp, textureManager, light, dt, i, instanceBuffers[i], instanceCounts[i]);
				}
			}
		}

		// 4. 障碍物
		for (auto& obs : obstacles)
		{
			obs.drawLit(core, psos, shaders, vp, textureManager, light);
		}

		// 5. 装饰物
		if (decorationModel)
		{
			for (auto& dec : decorations)
			{
				Matrix W = Matrix::scaling(Vec3(dec.scale, dec.scale, dec.scale)) *
					Matrix::rotateY(dec.rotationY) *
					Matrix::translation(dec.position);
				decorationModel->drawLit(core, psos, shaders, vp, W, textureManager, light);
			}
		}
	}
};
//地形管理类-负责地块的生成和回收
class TerrainManager
{
private:
	std::vector<TerrainTile*> tiles;
	int numTiles;
	float tileLength;
	StaticModel* roadModel;
	StaticModel* grassModel;
	GrassPatch* grassPatchModel;
	AnimatedModel* obstacleModel;
	StaticModel* decorationModel; // 新增装饰物模型指针
	Core* corePtr; // 需要保存 Core 指针用于地块生成

	// 配置结构体
	struct TileConfig {
		int obstacleType; // 0: None, 1: Left, 2: Right
		int decorationCount;
	};
	std::vector<TileConfig> levelConfigs;
	int currentConfigIndex;

public:
	TerrainManager()
	{
		numTiles = 10;
		tileLength = 35.0f;
		roadModel = nullptr;
		grassModel = nullptr;
		grassPatchModel = nullptr;
		obstacleModel = nullptr;
		decorationModel = nullptr;
		corePtr = nullptr;
		currentConfigIndex = 0;
	}

	// 析构函数：清理内存
	~TerrainManager()
	{
		for (auto tile : tiles)
		{
			delete tile;
		}
		tiles.clear();
	}

	// 加载关卡配置
	void loadLevelConfig(std::string filename)
	{
		std::ifstream file(filename);
		if (!file.is_open())
		{
			printf("Warning: Could not open %s. Using random generation.\n", filename.c_str());
			return;
		}

		levelConfigs.clear();
		std::string line;
		while (std::getline(file, line))
		{
			if (line.empty() || line[0] == '#') continue;
			std::stringstream ss(line);
			std::string obsStr;
			int decCount;
			ss >> obsStr >> decCount;

			TileConfig config;
			if (obsStr == "LEFT") config.obstacleType = 1;
			else if (obsStr == "RIGHT") config.obstacleType = 2;
			else config.obstacleType = 0; // NONE or others

			config.decorationCount = decCount;
			levelConfigs.push_back(config);
		}
		printf("Loaded %d tile configs from %s\n", (int)levelConfigs.size(), filename.c_str());
		currentConfigIndex = 0;
	}

	// 获取下一个配置
	TileConfig getNextConfig()
	{
		if (levelConfigs.empty())
		{
			// 如果没有配置文件，回退到随机逻辑
			TileConfig config;
			config.obstacleType = (rand() % 2 == 0) ? (rand() % 2 + 1) : 0; // 50% chance for obstacle
			config.decorationCount = (rand() % 100 > 30) ? (rand() % 2 + 1) : 0; // 70% chance for decorations
			return config;
		}

		TileConfig config = levelConfigs[currentConfigIndex];
		currentConfigIndex = (currentConfigIndex + 1) % levelConfigs.size();
		return config;
	}

	// init 增加 Core* 参数
	void init(Core* core, StaticModel* road, StaticModel* grass, GrassPatch* grassPatch, AnimatedModel* _obstacleModel, StaticModel* _decorationModel, Vec3 playerStartPosition)
	{
		corePtr = core; // 保存指针
		roadModel = road;
		grassModel = grass;
		grassPatchModel = grassPatch;
		obstacleModel = _obstacleModel;
		decorationModel = _decorationModel;

		// 加载配置
		loadLevelConfig("level.txt");

		// 清理旧数据（如果有）
		for (auto tile : tiles) delete tile;
		tiles.clear();

		int tilesBehand = 2;
		int tilesAhead = numTiles - tilesBehand;

		for (int i = 0; i < numTiles; i++)
		{
			// 使用 new 创建对象
			TerrainTile* tile = new TerrainTile();
			tile->length = tileLength;

			float zPos = playerStartPosition.z + (tilesBehand - i) * tileLength;
			tile->setPosition(Vec3(playerStartPosition.x, playerStartPosition.y, zPos));

			// 传入 core 生成草地实例缓冲区
			tile->generateGrass(corePtr);

			// 获取配置
			TileConfig config = getNextConfig();

			// 生成障碍物
			tile->generateObstacles(obstacleModel, config.obstacleType);

			// 生成装饰物
			tile->generateDecorations(config.decorationCount);

			tiles.push_back(tile);
		}

		printf("TerrainManager initialized with %d tiles (Hardware Instancing Enabled)\n", numTiles);
	}

	int checkCollisions(Vec3 playerPos, float playerRadius)
	{
		int totalHits = 0;
		for (auto tile : tiles) // tile 是指针
		{
			totalHits += tile->checkCollisions(playerPos, playerRadius);
		}
		return totalHits;
	}

	void update(Vec3 playerPosition, float dt)
	{
		// 更新所有地块上的障碍物动画
		for (auto tile : tiles)
		{
			tile->update(dt);
		}

		// 地块回收逻辑
		if (tiles.empty()) return;

		// 找到玩家身后最远的地块（列表第一个）
		float backTileZ = tiles[0]->position.z;

		// 如果玩家已经远离身后的地块
		if (playerPosition.z < backTileZ - tileLength * 1.5f)
		{
			// 取出第一个地块（指针）
			TerrainTile* recycledTile = tiles[0];
			tiles.erase(tiles.begin());

			// 计算新位置（在最前方）
			float newZ = tiles.back()->position.z - tileLength;
			recycledTile->setPosition(Vec3(tiles.back()->position.x, tiles.back()->position.y, newZ));

			// 重新生成内容（这里不需要 delete 再 new，只需重置数据）
			
			// 获取下一个配置
			TileConfig config = getNextConfig();

			// 重新生成草（会重建 GPU 缓冲区）
			recycledTile->generateGrass(corePtr);
			// 重新生成障碍物
			recycledTile->generateObstacles(obstacleModel, config.obstacleType);
			// 重新生成装饰物
			recycledTile->generateDecorations(config.decorationCount);

			tiles.push_back(recycledTile);

			// printf("Tile recycled: z=%.2f -> z=%.2f\n", backTileZ, newZ);
		}
	}

	void drawLit(Core* core, PSOManager* psos, Shaders* shaders, Matrix& vp, TextureManager* textureManager, DirectionalLight* light, float dt)
	{
		if (roadModel == nullptr || grassModel == nullptr || grassPatchModel == nullptr)
		{
			return;
		}

		// 绘制所有地块
		for (auto tile : tiles)
		{
			tile->drawLit(core, psos, shaders, vp, roadModel, grassModel, grassPatchModel, decorationModel, textureManager, light, dt);
		}
	}

	void setNumTiles(int num) { numTiles = num; }
	void setTileLength(float length) { tileLength = length; }
};