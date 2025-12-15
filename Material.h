#pragma once
#include "Texture.h"
#include "GEMLoader.h"
#include <string>

class Material
{
public:
	Texture* diffuseTexture;
	Texture* normalTexture;
	Texture* specularTexture;
	std::string name;
	bool hasTexture;

	Material()
	{
		diffuseTexture = nullptr;
		normalTexture = nullptr;
		specularTexture = nullptr;
		hasTexture = false;
	}

	void loadFromGEMMaterial(GEMLoader::GEMMaterial& gemMaterial, TextureManager* textureManager, std::string modelFolder)
	{
		printf("  Loading material properties...\n");
		fflush(stdout);

		// Load albedo/diffuse texture - try both property names
		GEMLoader::GEMMaterialProperty albedoProp = gemMaterial.find("albedo");
		if (albedoProp.value.empty())
		{
			albedoProp = gemMaterial.find("diffuse");
		}

		if (!albedoProp.value.empty())
		{
			// 智能路径拼接
			std::string texturePath;
			if (modelFolder.empty() || albedoProp.value.find('/') == 0 || albedoProp.value.find('\\') == 0)
			{
				// 如果 modelFolder 为空，或者纹理路径以斜杠开头，直接使用纹理路径
				texturePath = albedoProp.value;
			}
			else if (albedoProp.value.find("Models/") == 0 || albedoProp.value.find("Models\\") == 0)
			{
				// 如果纹理路径已经包含完整路径（以 Models/ 开头），直接使用
				texturePath = albedoProp.value;
			}
			else
			{
				// 否则，拼接 modelFolder 和纹理文件名
				texturePath = modelFolder + "/" + albedoProp.value;
			}

			printf("  Loading albedo/diffuse texture: %s\n", texturePath.c_str());
			fflush(stdout);

			diffuseTexture = textureManager->load(texturePath);
			if (diffuseTexture && diffuseTexture->textureResource)
			{
				hasTexture = true;
				printf("    -> Success! Texture loaded (width=%d, height=%d).\n",
					diffuseTexture->width, diffuseTexture->height);
			}
			else
			{
				printf("    -> FAILED to load texture!\n");
			}
			fflush(stdout);
		}
		else
		{
			printf("  No albedo/diffuse texture specified in material\n");
			fflush(stdout);
		}

		//同样的逻辑应用到 normal 纹理
		GEMLoader::GEMMaterialProperty normalProp = gemMaterial.find("nh");
		if (normalProp.value.empty())
		{
			normalProp = gemMaterial.find("normal");
		}

		if (!normalProp.value.empty())
		{
			std::string texturePath;
			if (modelFolder.empty() || normalProp.value.find('/') == 0 || normalProp.value.find('\\') == 0)
			{
				texturePath = normalProp.value;
			}
			else if (normalProp.value.find("Models/") == 0 || normalProp.value.find("Models\\") == 0)
			{
				texturePath = normalProp.value;
			}
			else
			{
				texturePath = modelFolder + "/" + normalProp.value;
			}

			printf("  Loading normal texture: %s\n", texturePath.c_str());
			fflush(stdout);
			normalTexture = textureManager->load(texturePath);
			if (normalTexture && normalTexture->textureResource)
			{
				printf("    -> Success!\n");
			}
			else
			{
				printf("    -> Failed!\n");
			}
			fflush(stdout);
		}

		//同样的逻辑应用到 specular 纹理
		GEMLoader::GEMMaterialProperty specularProp = gemMaterial.find("rmax");
		if (specularProp.value.empty())
		{
			specularProp = gemMaterial.find("specular");
		}

		if (!specularProp.value.empty())
		{
			std::string texturePath;
			if (modelFolder.empty() || specularProp.value.find('/') == 0 || specularProp.value.find('\\') == 0)
			{
				texturePath = specularProp.value;
			}
			else if (specularProp.value.find("Models/") == 0 || specularProp.value.find("Models\\") == 0)
			{
				texturePath = specularProp.value;
			}
			else
			{
				texturePath = modelFolder + "/" + specularProp.value;
			}

			printf("  Loading specular/rmax texture: %s\n", texturePath.c_str());
			fflush(stdout);
			specularTexture = textureManager->load(texturePath);
			if (specularTexture && specularTexture->textureResource)
			{
				printf("    -> Success!\n");
			}
			else
			{
				printf("    -> Failed!\n");
			}
			fflush(stdout);
		}
	}
	void bind(Core* core)
	{
		if (diffuseTexture && diffuseTexture->textureResource)
		{
			core->getCommandList()->SetGraphicsRootDescriptorTable(2, diffuseTexture->srvHandle);
		}
	}
};

class MaterialManager
{
public:
	std::vector<Material*> materials;
	TextureManager* textureManager;

	MaterialManager(TextureManager* _textureManager)
	{
		textureManager = _textureManager;
	}

	Material* createMaterial(GEMLoader::GEMMaterial& gemMaterial, std::string modelFolder)
	{
		Material* material = new Material();
		material->loadFromGEMMaterial(gemMaterial, textureManager, modelFolder);
		materials.push_back(material);
		return material;
	}

	void cleanup()
	{
		for (auto& mat : materials)
		{
			delete mat;
		}
		materials.clear();
	}

	~MaterialManager()
	{
		cleanup();
	}
};