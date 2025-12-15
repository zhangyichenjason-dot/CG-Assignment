#pragma once
#include "Maths.h"
#include "Mesh.h"
#include "Material.h"
#include "PSO.h"
#include "Shaders.h"

//用于TerrainTile的草地实例数据结构
struct InstanceData
{
	Matrix worldMatrix;
};

// 方向光类（模拟太阳）
class DirectionalLight
{
public:
	Vec3 direction;      // 光线方向（从光源指向场景）
	Vec3 color;          // 光的颜色
	float intensity;     // 光照强度
	float ambientStrength; // 环境光强度

	DirectionalLight()
	{
		// 默认太阳方向：从右上方照射
		direction = Vec3(-0.5f, -1.0f, -0.5f);
		direction.normalize();
		color = Vec3(1.0f, 0.95f, 0.9f);  // 暖白色阳光
		intensity = 1.0f;
		ambientStrength = 0.2f;
	}

	// 设置太阳方向（使用角度）
	void setDirectionFromAngles(float azimuth, float elevation)
	{
		// azimuth: 水平角度（弧度）
		// elevation: 仰角（弧度）
		direction.x = cosf(elevation) * sinf(azimuth);
		direction.y = -sinf(elevation);  // 负号使光线向下
		direction.z = cosf(elevation) * cosf(azimuth);
		direction.normalize();
	}

	// 设置太阳从指定方向照射（更直观）
	// 例如：setLightFrom(1, 1, 1) 表示光从右上前方照射
	void setLightFrom(float x, float y, float z)
	{
		direction = Vec3(-x, -y, -z);  // 反向得到光线方向
		direction.normalize();
	}

	// 模拟日出日落（根据时间变化）
	void updateByTime(float timeOfDay)
	{
		// timeOfDay: 0.0 = 午夜, 0.5 = 正午, 1.0 = 午夜
		float sunAngle = (timeOfDay - 0.25f) * 2.0f * 3.14159f;

		direction.x = 0.0f;
		direction.y = -sinf(sunAngle);
		direction.z = -cosf(sunAngle);
		direction.normalize();

		// 根据太阳高度调整颜色（日出日落偏橙色）
		float sunHeight = -direction.y;
		if (sunHeight > 0)
		{
			float t = sunHeight;
			// 从橙色（低太阳）到白色（高太阳）
			color.x = 1.0f;
			color.y = 0.7f + 0.25f * t;
			color.z = 0.5f + 0.4f * t;
			intensity = 0.3f + 0.7f * t;
		}
		else
		{
			// 夜晚：月光
			color = Vec3(0.3f, 0.3f, 0.5f);
			intensity = 0.1f;
		}
	}

	// 获取用于着色器的光线方向（指向光源）
	Vec3 getLightDirectionForShader()
	{
		return direction * -1.0f;  // 着色器需要从表面指向光源的方向
	}

	// pitch (俯仰角): 0度=地平线, 90度=垂直向下
	// yaw   (偏航角): 0度=Z轴方向, 旋转360度围绕Y轴
	void setRotation(float pitchDegrees, float yawDegrees)
	{
		// 1. 将度数转换为弧度
		float pitchRad = pitchDegrees * (3.14159f / 180.0f);
		float yawRad = yawDegrees * (3.14159f / 180.0f);

		// 2. 计算方向向量
		// 假设 Y 是向上轴。
		// pitchRad 控制 Y 的分量 (高度)
		// yawRad 控制 X 和 Z 的分量 (水平旋转)

		// 这里的逻辑是：
		// 垂直分量 y = -sin(pitch)  (负号是因为光线是向下射的)
		// 水平投影长度 = cos(pitch)

		direction.y = -sinf(pitchRad);

		float horizontalScale = cosf(pitchRad);

		direction.x = horizontalScale * sinf(yawRad);
		direction.z = horizontalScale * cosf(yawRad);

		direction.normalize();
	}
};

// 天空球类
class Skybox
{
public:
	Mesh mesh;
	Material* material;
	std::string shaderName;
	bool initialized;

	Skybox()
	{
		material = nullptr;
		initialized = false;
	}

	STATIC_VERTEX addVertex(Vec3 p, Vec3 n, float tu, float tv)
	{
		STATIC_VERTEX v;
		v.pos = p;
		v.normal = n;
		Frame frame;
		frame.fromVector(n);
		v.tangent = frame.u;
		v.tu = tu;
		v.tv = tv;
		return v;
	}

	void init(Core* core, PSOManager* psos, Shaders* shaders, TextureManager* textureManager, MaterialManager* materialManager, std::string texturePath)
	{
		std::vector<STATIC_VERTEX> vertices;
		std::vector<unsigned int> indices;

		const int latitudeBands = 30;
		const int longitudeBands = 30;
		const float radius = 500.0f;

		// 生成顶点
		for (int latNumber = 0; latNumber <= latitudeBands; latNumber++)
		{
			float theta = latNumber * 3.14159265f / latitudeBands;
			float sinTheta = sinf(theta);
			float cosTheta = cosf(theta);

			for (int longNumber = 0; longNumber <= longitudeBands; longNumber++)
			{
				float phi = longNumber * 2.0f * 3.14159265f / longitudeBands;
				float sinPhi = sinf(phi);
				float cosPhi = cosf(phi);

				Vec3 position;
				position.x = radius * cosPhi * sinTheta;
				position.y = radius * cosTheta;
				position.z = radius * sinPhi * sinTheta;

				// 法线朝内
				Vec3 normal = position * -1.0f;
				normal.normalize();

				// UV映射
				float u = 1.0f - (float)longNumber / longitudeBands;
				float v = (float)latNumber / latitudeBands;

				vertices.push_back(addVertex(position, normal, u, v));
			}
		}

		// 生成索引
		for (int latNumber = 0; latNumber < latitudeBands; latNumber++)
		{
			for (int longNumber = 0; longNumber < longitudeBands; longNumber++)
			{
				int first = (latNumber * (longitudeBands + 1)) + longNumber;
				int second = first + longitudeBands + 1;

				indices.push_back(first);
				indices.push_back(first + 1);
				indices.push_back(second);

				indices.push_back(second);
				indices.push_back(first + 1);
				indices.push_back(second + 1);
			}
		}

		// 初始化网格
		mesh.init(core, vertices, indices);

		// 加载纹理
		printf("Loading skybox texture: %s\n", texturePath.c_str());
		fflush(stdout);

		material = new Material();
		material->diffuseTexture = textureManager->load(texturePath);

		if (material->diffuseTexture && material->diffuseTexture->textureResource)
		{
			material->hasTexture = true;
			printf("  -> Skybox texture loaded successfully!\n");
		}
		else
		{
			material->hasTexture = false;
			printf("  -> ERROR: Failed to load skybox texture!\n");
		}
		fflush(stdout);

		// 加载自发光天空球着色器
		shaders->load(core, "SkyboxEmissive", "VSSkyEmissive.txt", "PSSkyEmissive.txt");
		psos->createPSO(core, "SkyboxEmissivePSO",
			shaders->find("SkyboxEmissive")->vs,
			shaders->find("SkyboxEmissive")->ps,
			VertexLayoutCache::getStaticLayout());

		shaderName = "SkyboxEmissive";
		initialized = true;

		printf("Skybox initialization complete\n");
		fflush(stdout);
	}
	void draw(Core* core, PSOManager* psos, Shaders* shaders, Matrix& vp, Vec3 cameraPosition, TextureManager* textureManager, float emissiveIntensity = 1.0f)
	{
		if (!initialized)
		{
			printf("WARNING: Skybox not initialized\n");
			return;
		}

		if (material == nullptr || !material->hasTexture)
		{
			printf("WARNING: Skybox material/texture not ready\n");
			return;
		}

		// 天空球跟随相机位置
		Matrix W = Matrix::translation(cameraPosition);

		// 更新 VS 常量
		shaders->updateConstantVS(shaderName, "staticMeshBuffer", "VP", &vp);
		shaders->updateConstantVS(shaderName, "staticMeshBuffer", "W", &W);

		// 更新天空球自发光常量（PS b1)
		shaders->updateConstantPS(shaderName, "SkyboxBuffer", "emissiveIntensity", &emissiveIntensity);

		// 绑定纹理堆
		textureManager->bindHeap(core);

		// 应用着色器和 PSO
		shaders->apply(core, shaderName);
		psos->bind(core, "SkyboxEmissivePSO");

		// 绑定材质纹理
		material->bind(core);

		// 绘制网格
		mesh.draw(core);
	}


};

// 草地十字交叉贴图类
class GrassPatch
{
public:

	Mesh mesh;
	std::vector<Material*> materials;
	std::string shaderName;
	float time;

	GrassPatch()
	{
		time = 0.0f;
	}

	STATIC_VERTEX addVertex(Vec3 p, Vec3 n, float tu, float tv)
	{
		STATIC_VERTEX v;
		v.pos = p;
		v.normal = n;
		Frame frame;
		frame.fromVector(n);
		v.tangent = frame.u;
		v.tu = tu;
		v.tv = tv;
		return v;
	}

	void init(Core* core, PSOManager* psos, Shaders* shaders, TextureManager* textureManager)
	{
		std::vector<STATIC_VERTEX> vertices;
		std::vector<unsigned int> indices;

		float halfWidth = 0.5f;
		float height = 1.0f;

		// === 第一个矩形（沿X轴方向） ===
		// 前面
		vertices.push_back(addVertex(Vec3(-halfWidth, 0, 0), Vec3(0, 0, 1), 0, 1));      // 左下
		vertices.push_back(addVertex(Vec3(halfWidth, 0, 0), Vec3(0, 0, 1), 1, 1));       // 右下
		vertices.push_back(addVertex(Vec3(-halfWidth, height, 0), Vec3(0, 0, 1), 0, 0)); // 左上
		vertices.push_back(addVertex(Vec3(halfWidth, height, 0), Vec3(0, 0, 1), 1, 0));  // 右上

		// 后面（双面渲染）
		vertices.push_back(addVertex(Vec3(halfWidth, 0, 0), Vec3(0, 0, -1), 0, 1));
		vertices.push_back(addVertex(Vec3(-halfWidth, 0, 0), Vec3(0, 0, -1), 1, 1));
		vertices.push_back(addVertex(Vec3(halfWidth, height, 0), Vec3(0, 0, -1), 0, 0));
		vertices.push_back(addVertex(Vec3(-halfWidth, height, 0), Vec3(0, 0, -1), 1, 0));

		// === 第二个矩形（沿Z轴方向，垂直于第一个） ===
		// 前面
		vertices.push_back(addVertex(Vec3(0, 0, -halfWidth), Vec3(1, 0, 0), 0, 1));
		vertices.push_back(addVertex(Vec3(0, 0, halfWidth), Vec3(1, 0, 0), 1, 1));
		vertices.push_back(addVertex(Vec3(0, height, -halfWidth), Vec3(1, 0, 0), 0, 0));
		vertices.push_back(addVertex(Vec3(0, height, halfWidth), Vec3(1, 0, 0), 1, 0));

		// 后面
		vertices.push_back(addVertex(Vec3(0, 0, halfWidth), Vec3(-1, 0, 0), 0, 1));
		vertices.push_back(addVertex(Vec3(0, 0, -halfWidth), Vec3(-1, 0, 0), 1, 1));
		vertices.push_back(addVertex(Vec3(0, height, halfWidth), Vec3(-1, 0, 0), 0, 0));
		vertices.push_back(addVertex(Vec3(0, height, -halfWidth), Vec3(-1, 0, 0), 1, 0));

		// 索引
		for (int i = 0; i < 4; i++)
		{
			int base = i * 4;
			indices.push_back(base + 0);
			indices.push_back(base + 1);
			indices.push_back(base + 2);
			indices.push_back(base + 1);
			indices.push_back(base + 3);
			indices.push_back(base + 2);
		}

		mesh.init(core, vertices, indices);

		// 加载草地着色器
		shaders->load(core, "Grass", "VSGrass.txt", "PSGrass.txt");
		shaderName = "Grass";

		psos->createPSO(core, "GrassPSO",
			shaders->find("Grass")->vs,
			shaders->find("Grass")->ps,
			VertexLayoutCache::getInstancedLayout()); 
		printf("GrassPatch initialized (waiting for textures)\n");
		fflush(stdout);
	}

	void addTexture(TextureManager* textureManager, std::string texturePath)
	{
		Material* mat = new Material();
		mat->diffuseTexture = textureManager->load(texturePath);
		mat->hasTexture = (mat->diffuseTexture != nullptr);
		materials.push_back(mat);
		printf("GrassPatch added texture: %s\n", texturePath.c_str());
	}

	void drawInstanced(Core* core, PSOManager* psos, Shaders* shaders, Matrix& vp,
		TextureManager* textureManager, DirectionalLight* light, float dt,
		int type, ID3D12Resource* instanceBuffer, int instanceCount)
	{
		if (instanceCount == 0) return;

		time += dt;

		// 更新全局常量 (VP, Time, Light)
		// 注意：W 矩阵不再通过这里传递，而是通过 instanceBuffer
		shaders->updateConstantVS(shaderName, "staticMeshBuffer", "VP", &vp);
		shaders->updateConstantVS(shaderName, "staticMeshBuffer", "time", &time);

		float windStrength = 0.1f;
		float windSpeed = 0.1f;
		shaders->updateConstantVS(shaderName, "staticMeshBuffer", "windStrength", &windStrength);
		shaders->updateConstantVS(shaderName, "staticMeshBuffer", "windSpeed", &windSpeed);

		// 光照常量
		Vec3 lightDir = light->getLightDirectionForShader();
		shaders->updateConstantPS(shaderName, "LightBuffer", "lightDirection", &lightDir);
		shaders->updateConstantPS(shaderName, "LightBuffer", "lightIntensity", &light->intensity);
		shaders->updateConstantPS(shaderName, "LightBuffer", "lightColor", &light->color);
		shaders->updateConstantPS(shaderName, "LightBuffer", "ambientStrength", &light->ambientStrength);

		// 绑定纹理
		textureManager->bindHeap(core);
		if (type >= 0 && type < materials.size())
		{
			materials[type]->bind(core);
		}

		shaders->apply(core, shaderName);
		psos->bind(core, "GrassPSO");

		
		core->getCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		core->getCommandList()->IASetVertexBuffers(0, 1, &mesh.vbView);
		core->getCommandList()->IASetIndexBuffer(&mesh.ibView);

		
		D3D12_VERTEX_BUFFER_VIEW instanceView;
		instanceView.BufferLocation = instanceBuffer->GetGPUVirtualAddress();
		instanceView.StrideInBytes = sizeof(InstanceData);
		instanceView.SizeInBytes = sizeof(InstanceData) * instanceCount;
		core->getCommandList()->IASetVertexBuffers(1, 1, &instanceView);

		
		core->getCommandList()->DrawIndexedInstanced(mesh.numMeshIndices, instanceCount, 0, 0, 0);
	}
};

// 远景层类-使用多张部分透明png叠加成远景效果（森林-山脉）
class DistantLayer
{
public:
	Mesh mesh;
	Material* material;
	std::string shaderName;
	bool initialized;

	DistantLayer()
	{
		material = nullptr;
		initialized = false;
	}

	STATIC_VERTEX addVertex(Vec3 p, Vec3 n, float tu, float tv)
	{
		STATIC_VERTEX v;
		v.pos = p;
		v.normal = n;
		Frame frame;
		frame.fromVector(n);
		v.tangent = frame.u;
		v.tu = tu;
		v.tv = tv;
		return v;
	}

	
	void init(Core* core, PSOManager* psos, Shaders* shaders, TextureManager* textureManager, MaterialManager* materialManager, std::string texturePath, float radius, float textureWidth, float textureHeight, int repeatCount)
	{
		std::vector<STATIC_VERTEX> vertices;
		std::vector<unsigned int> indices;

		// 1. 计算圆周长
		float circumference = 2.0f * 3.14159265f * radius;

		// 2. 计算单张图片在世界空间中的宽度
		float tileWidthWorld = circumference / (float)repeatCount;

		// 3. 根据图片原始比例计算高度 (保持宽高比)
		// Height / Width = texHeight / texWidth
		// Height = Width * (texHeight / texWidth)
		float height = tileWidthWorld * (textureHeight / textureWidth);

		printf("DistantLayer Info: Radius=%.1f, Repeats=%d, Calculated Height=%.1f\n", radius, repeatCount, height);

		// 4. 生成 360 度圆柱面
		int segments = 72; // 分段数，越高越圆滑 (72段 = 每5度一段)
		float angleStep = 2.0f * 3.14159265f / segments;

		for (int i = 0; i <= segments; i++)
		{
			float theta = i * angleStep;

			// 计算位置 (XZ平面)
			float x = radius * cosf(theta);
			float z = radius * sinf(theta);

			// 法线指向圆心 (反向)
			Vec3 normal = Vec3(-x, 0, -z);
			normal.normalize();

			
			float u = ((float)i / segments) * (float)repeatCount;

		
			// 防止垂直方向的纹理采样越界（Wrap模式下顶部采样到底部导致黑线）
			float vTop = 0.01f;
			float vBottom = 0.99f;

			// 添加一列顶点 (上和下)
			vertices.push_back(addVertex(Vec3(x, height, z), normal, u, vTop));    // 上
			vertices.push_back(addVertex(Vec3(x, 0, z), normal, u, vBottom));      // 下
		}

		// 生成索引
		for (int i = 0; i < segments; i++)
		{
			int base = i * 2;
			indices.push_back(base);
			indices.push_back(base + 1);
			indices.push_back(base + 2);

			indices.push_back(base + 1);
			indices.push_back(base + 3);
			indices.push_back(base + 2);
		}

		// 初始化网格
		mesh.init(core, vertices, indices);

		// 加载纹理
		printf("Loading distant layer texture: %s\n", texturePath.c_str());
		fflush(stdout);
		material = new Material();
		material->diffuseTexture = textureManager->load(texturePath);

		// 设置纹理寻址模式为 WRAP (重复)，这对于平铺至关重要
		// 注意：在 DX12 中这通常在采样器描述符中设置。
		// 假设你的 TextureManager 或默认采样器已经支持 Wrap 模式，或者默认就是 Wrap。
		// 如果默认是 Clamp，你可能需要去 PSO/RootSignature 确认采样器设置。
		// 通常游戏引擎默认采样器都是 Wrap。

		if (material->diffuseTexture && material->diffuseTexture->textureResource)
		{
			material->hasTexture = true;
		}
		else
		{
			material->hasTexture = false;
		}

		// 加载着色器和PSO
		shaders->load(core, "DistantLayerEmissive", "VSSkyEmissive.txt", "PSSkyEmissive.txt");
		psos->createTransparentPSO(core, "DistantLayerEmissivePSO",
			shaders->find("DistantLayerEmissive")->vs,
			shaders->find("DistantLayerEmissive")->ps,
			VertexLayoutCache::getStaticLayout());

		shaderName = "DistantLayerEmissive";
		initialized = true;
	}

	// 绘制函数保持不变
	void draw(Core* core, PSOManager* psos, Shaders* shaders, Matrix& vp, Matrix& w, TextureManager* textureManager, float emissiveIntensity = 1.0f)
	{
		if (!initialized) return;

		shaders->updateConstantVS(shaderName, "staticMeshBuffer", "VP", &vp);
		shaders->updateConstantVS(shaderName, "staticMeshBuffer", "W", &w);
		shaders->updateConstantPS(shaderName, "SkyboxBuffer", "emissiveIntensity", &emissiveIntensity);

		textureManager->bindHeap(core);
		shaders->apply(core, shaderName);
		psos->bind(core, "DistantLayerEmissivePSO");

		if (material && material->hasTexture)
		{
			material->bind(core);
		}

		mesh.draw(core);
	}
};
