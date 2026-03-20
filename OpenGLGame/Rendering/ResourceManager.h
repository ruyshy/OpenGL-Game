#pragma once

#ifndef RESOURCEMANAGER_H_
#define RESOURCEMANAGER_H_

enum class Resource : unsigned int
{
	SHADER = 0x01,
	IMAGE_PNG,
	FONT,
	SOUND_WAV,
	SOUND_MP3,
};

class ResourceManager
{
public:
	static string GetResourceString(UINT ID, Resource resource);

	static LPVOID GetResourceLPVOID(UINT ID, Resource resource);
	static LPVOID GetResourceLPVOID(UINT ID, Resource resource, DWORD& size);

private:
	static LPCWSTR ResourceName(Resource resource);

private:
	static constexpr LPCWSTR mShader = L"SHADER";
	static constexpr LPCWSTR mImage = L"PNG";
	static constexpr LPCWSTR mFont = L"BINFONT";
	static constexpr LPCWSTR mSoundWave = L"WAVE";
	static constexpr LPCWSTR mSoundMp3 = L"MP3";
};

#endif // !RESOURCEMANAGER_H_
