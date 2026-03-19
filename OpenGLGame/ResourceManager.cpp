#include "pch.h"
#include "ResourceManager.h"


string ResourceManager::GetResourceString(UINT ID, Resource resource)
{
	std::vector<char> output;

	LPCWSTR resouce_name = ResourceName(resource);

	if (resouce_name == NULL)
		return "";

	HMODULE module = nullptr;

	auto resourceHandle = FindResource(module, MAKEINTRESOURCE(ID), resouce_name);
	if (resourceHandle != nullptr)
	{
		auto dataHandle = ::LoadResource(module, resourceHandle);
		if (dataHandle != nullptr)
		{
			auto resourceSize = ::SizeofResource(module, resourceHandle);
			if (resourceSize != 0)
			{
				auto firstByte = reinterpret_cast<const char*>(::LockResource(dataHandle));
				if (firstByte != nullptr)
				{
					output.resize(resourceSize);
					std::copy(firstByte, firstByte + resourceSize, output.begin());
				}
			}
		}
	}
	auto string = std::string(output.begin(), output.end());

	return string;
}

LPVOID ResourceManager::GetResourceLPVOID(UINT ID, Resource resource)
{
	LPCWSTR resouce_name = ResourceName(resource);

	if (resouce_name == NULL)
		return NULL;

	HMODULE module = nullptr;

	auto resourceHandle = FindResource(module, MAKEINTRESOURCE(ID), resouce_name);

	if (resourceHandle != nullptr)
	{
		auto dataHandle = ::LoadResource(module, resourceHandle);

		if (dataHandle != nullptr)
		{
			auto resourceSize = ::SizeofResource(module, resourceHandle);

			if (resourceSize != 0)
			{
				LPVOID firstByte = ::LockResource(dataHandle);
				return firstByte;
			}
		}
	}

	return NULL;
}

LPVOID ResourceManager::GetResourceLPVOID(UINT ID, Resource resource, DWORD& size)
{
	LPCWSTR resouce_name = ResourceName(resource);
	size = 0;

	if (resouce_name == NULL)
		return NULL;

	HMODULE module = nullptr;

	auto resourceHandle = FindResource(module, MAKEINTRESOURCE(ID), resouce_name);

	if (resourceHandle != nullptr)
	{
		auto dataHandle = ::LoadResource(module, resourceHandle);

		if (dataHandle != nullptr)
		{
			auto resourceSize = ::SizeofResource(module, resourceHandle);

			if (resourceSize != 0)
			{
				size = resourceSize;
				LPVOID firstByte = ::LockResource(dataHandle);
				return firstByte;
			}
		}
	}

	return NULL;
}

LPCWSTR ResourceManager::ResourceName(Resource resource)
{
	LPCWSTR resouce_name = NULL;
	switch (resource)
	{
	case Resource::SHADER:
		resouce_name = mShader;
		break;
	case Resource::IMAGE_PNG:
		resouce_name = mImage;
		break;
	case Resource::FONT:
		resouce_name = mFont;
		break;
	case Resource::SOUND_WAV:
		resouce_name = mSoundWave;
		break;
	case Resource::SOUND_MP3:
		resouce_name = mSoundMp3;
		break;
	default:
		break;
	}
	return resouce_name;
}
