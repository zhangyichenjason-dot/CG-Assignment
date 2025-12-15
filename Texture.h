#pragma once
#include "Core.h"
#include <string>
#include <map>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

class Texture
{
public:
	ID3D12Resource* textureResource;
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandle;
	int width;
	int height;
	int channels;

	void load(Core* core, std::string filename)
	{
		unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 4);
		if (!data)
		{
			printf("Failed to load texture: %s\n", filename.c_str());
			return;
		}

		// Create texture resource
		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		core->device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&textureResource)
		);

		// Upload texture data
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
		footprint.Offset = 0;
		footprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		footprint.Footprint.Width = width;
		footprint.Footprint.Height = height;
		footprint.Footprint.Depth = 1;
		footprint.Footprint.RowPitch = (width * 4 + 255) & ~255;

		unsigned int uploadSize = footprint.Footprint.RowPitch * height;
		unsigned char* uploadData = new unsigned char[uploadSize];
		for (int y = 0; y < height; y++)
		{
			memcpy(uploadData + y * footprint.Footprint.RowPitch, data + y * width * 4, width * 4);
		}

		core->uploadResource(textureResource, uploadData, uploadSize, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &footprint);

		delete[] uploadData;
		stbi_image_free(data);
	}

	void createSRV(Core* core, ID3D12DescriptorHeap* srvHeap, int index)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();
		unsigned int descriptorSize = core->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		cpuHandle.ptr += index * descriptorSize;

		core->device->CreateShaderResourceView(textureResource, &srvDesc, cpuHandle);

		srvHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();
		srvHandle.ptr += index * descriptorSize;
	}

	void cleanup()
	{
		if (textureResource)
		{
			textureResource->Release();
			textureResource = nullptr;
		}
	}

	~Texture()
	{
		cleanup();
	}
};

class TextureManager
{
public:
	std::map<std::string, Texture*> textures;
	ID3D12DescriptorHeap* srvHeap;
	int nextTextureIndex;
	Core* core;

	void init(Core* _core, int maxTextures = 100)
	{
		core = _core;
		nextTextureIndex = 0;

		// Create SRV descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = maxTextures;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		core->device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&srvHeap));
	}

	Texture* load(std::string filename)
	{
		// Check if already loaded
		auto it = textures.find(filename);
		if (it != textures.end())
		{
			return it->second;
		}

		// Load new texture
		Texture* texture = new Texture();
		texture->load(core, filename);
		texture->createSRV(core, srvHeap, nextTextureIndex);
		nextTextureIndex++;

		textures[filename] = texture;
		return texture;
	}

	Texture* get(std::string filename)
	{
		auto it = textures.find(filename);
		if (it != textures.end())
		{
			return it->second;
		}
		return nullptr;
	}

	void bindHeap(Core* core)
	{
		ID3D12DescriptorHeap* heaps[] = { srvHeap };
		core->getCommandList()->SetDescriptorHeaps(1, heaps);
	}

	void cleanup()
	{
		for (auto& pair : textures)
		{
			delete pair.second;
		}
		textures.clear();
		if (srvHeap)
		{
			srvHeap->Release();
			srvHeap = nullptr;
		}
	}

	~TextureManager()
	{
		cleanup();
	}
};