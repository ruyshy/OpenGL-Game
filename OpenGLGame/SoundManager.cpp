#include "pch.h"
#include "SoundManager.h"

#include <filesystem>

#if OPENGLGAME_HAS_FMOD
#include "fmod_errors.h"
#endif

namespace
{
	std::string NormalizeSoundPath(const std::string& filename)
	{
		if (filename.empty())
		{
			return std::string();
		}

		std::error_code errorCode;
		const std::filesystem::path path(filename);
		const std::filesystem::path normalized = std::filesystem::weakly_canonical(path, errorCode);
		return errorCode ? path.lexically_normal().string() : normalized.string();
	}
}

SoundManager& SoundManager::Get()
{
	static SoundManager instance;
	return instance;
}

bool SoundManager::Initialize(int maxChannels)
{
	if (mInitialized)
	{
		return true;
	}

#if OPENGLGAME_HAS_FMOD
	ClearError();

	if (!CheckResult(FMOD::System_Create(&mSystem), "Create FMOD system"))
	{
		return false;
	}

	if (!CheckResult(mSystem->init(maxChannels, FMOD_INIT_NORMAL, nullptr), "Initialize FMOD system"))
	{
		Shutdown();
		return false;
	}

	if (!CheckResult(mSystem->getMasterChannelGroup(&mMasterGroup), "Get master channel group"))
	{
		Shutdown();
		return false;
	}

	mInitialized = true;
	return true;
#else
	SetError("FMOD core headers were not found. Add fmod.h / fmod.hpp / fmod_errors.h to Library/include.");
	return false;
#endif
}

void SoundManager::Update()
{
#if OPENGLGAME_HAS_FMOD
	if (!mInitialized || mSystem == nullptr)
	{
		return;
	}

	CleanupStoppedChannels();
	const FMOD_RESULT result = mSystem->update();
	if (result != FMOD_OK)
	{
		CheckResult(result, "Update FMOD system");
	}
#endif
}

void SoundManager::Shutdown()
{
#if OPENGLGAME_HAS_FMOD
	if (mSystem == nullptr)
	{
		mInitialized = false;
		mMasterGroup = nullptr;
		mChannels.clear();
		return;
	}

	StopAll();
	ReleaseAllSounds();
	mMasterGroup = nullptr;

	mSystem->close();
	mSystem->release();
	mSystem = nullptr;
	mChannels.clear();
#endif

	mInitialized = false;
}

bool SoundManager::LoadSound(const std::string& key, const std::string& filename, bool loop, bool stream)
{
	if (key.empty() || filename.empty())
	{
		SetError("Sound key and filename must not be empty.");
		return false;
	}

#if OPENGLGAME_HAS_FMOD
	if (!mInitialized)
	{
		SetError("SoundManager must be initialized before loading sounds.");
		return false;
	}

	const std::string normalizedPath = NormalizeSoundPath(filename);
	if (!std::filesystem::exists(normalizedPath))
	{
		SetError("Sound file not found: " + filename);
		return false;
	}

	auto existingSound = mSounds.find(key);
	if (existingSound != mSounds.end())
	{
		existingSound->second->release();
		mSounds.erase(existingSound);
	}

	FMOD_MODE mode = FMOD_DEFAULT;
	mode |= stream ? FMOD_CREATESTREAM : FMOD_CREATECOMPRESSEDSAMPLE;
	mode |= loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;

	FMOD::Sound* sound = nullptr;
	if (!CheckResult(mSystem->createSound(normalizedPath.c_str(), mode, nullptr, &sound), "Load sound"))
	{
		return false;
	}

	mSounds[key] = sound;
	return true;
#else
	SetError("FMOD backend is unavailable in this build.");
	return false;
#endif
}

void SoundManager::UnloadSound(const std::string& key)
{
#if OPENGLGAME_HAS_FMOD
	auto soundIterator = mSounds.find(key);
	if (soundIterator == mSounds.end())
	{
		return;
	}

	for (auto channelIterator = mChannels.begin(); channelIterator != mChannels.end();)
	{
		FMOD::Sound* currentSound = nullptr;
		if (channelIterator->second != nullptr)
		{
			channelIterator->second->getCurrentSound(&currentSound);
		}

		if (currentSound == soundIterator->second)
		{
			if (channelIterator->second != nullptr)
			{
				channelIterator->second->stop();
			}

			channelIterator = mChannels.erase(channelIterator);
			continue;
		}

		++channelIterator;
	}

	soundIterator->second->release();
	mSounds.erase(soundIterator);
#else
	(void)key;
#endif
}

int SoundManager::PlaySound(const std::string& key, float volume, bool paused)
{
#if OPENGLGAME_HAS_FMOD
	if (!mInitialized)
	{
		SetError("SoundManager must be initialized before playing sounds.");
		return -1;
	}

	auto soundIterator = mSounds.find(key);
	if (soundIterator == mSounds.end())
	{
		SetError("Sound key is not loaded: " + key);
		return -1;
	}

	FMOD::Channel* channel = nullptr;
	if (!CheckResult(mSystem->playSound(soundIterator->second, nullptr, paused, &channel), "Play sound"))
	{
		return -1;
	}

	if (channel != nullptr)
	{
		channel->setVolume(std::clamp(volume, 0.0f, 1.0f));
	}

	const int channelId = mNextChannelId++;
	mChannels[channelId] = channel;
	return channelId;
#else
	(void)key;
	(void)volume;
	(void)paused;
	SetError("FMOD backend is unavailable in this build.");
	return -1;
#endif
}

bool SoundManager::StopChannel(int channelId)
{
#if OPENGLGAME_HAS_FMOD
	auto channelIterator = mChannels.find(channelId);
	if (channelIterator == mChannels.end() || channelIterator->second == nullptr)
	{
		return false;
	}

	channelIterator->second->stop();
	mChannels.erase(channelIterator);
	return true;
#else
	(void)channelId;
	return false;
#endif
}

void SoundManager::StopAll()
{
#if OPENGLGAME_HAS_FMOD
	for (auto& [channelId, channel] : mChannels)
	{
		(void)channelId;
		if (channel != nullptr)
		{
			channel->stop();
		}
	}

	mChannels.clear();
#endif
}

bool SoundManager::SetChannelPaused(int channelId, bool paused)
{
#if OPENGLGAME_HAS_FMOD
	auto channelIterator = mChannels.find(channelId);
	if (channelIterator == mChannels.end() || channelIterator->second == nullptr)
	{
		return false;
	}

	return CheckResult(channelIterator->second->setPaused(paused), "Set channel paused");
#else
	(void)channelId;
	(void)paused;
	return false;
#endif
}

bool SoundManager::SetChannelVolume(int channelId, float volume)
{
#if OPENGLGAME_HAS_FMOD
	auto channelIterator = mChannels.find(channelId);
	if (channelIterator == mChannels.end() || channelIterator->second == nullptr)
	{
		return false;
	}

	return CheckResult(channelIterator->second->setVolume(std::clamp(volume, 0.0f, 1.0f)), "Set channel volume");
#else
	(void)channelId;
	(void)volume;
	return false;
#endif
}

bool SoundManager::SetMasterVolume(float volume)
{
#if OPENGLGAME_HAS_FMOD
	if (!mInitialized || mMasterGroup == nullptr)
	{
		return false;
	}

	return CheckResult(mMasterGroup->setVolume(std::clamp(volume, 0.0f, 1.0f)), "Set master volume");
#else
	(void)volume;
	return false;
#endif
}

bool SoundManager::IsInitialized() const
{
	return mInitialized;
}

bool SoundManager::IsBackendAvailable() const
{
	return OPENGLGAME_HAS_FMOD == 1;
}

bool SoundManager::HasSound(const std::string& key) const
{
#if OPENGLGAME_HAS_FMOD
	return mSounds.find(key) != mSounds.end();
#else
	(void)key;
	return false;
#endif
}

const std::string& SoundManager::GetLastError() const
{
	return mLastError;
}

#if OPENGLGAME_HAS_FMOD
bool SoundManager::CheckResult(int result, const char* action)
{
	if (result == FMOD_OK)
	{
		return true;
	}

	SetError(std::string(action) + " failed: " + FMOD_ErrorString(static_cast<FMOD_RESULT>(result)));
	return false;
}

void SoundManager::CleanupStoppedChannels()
{
	for (auto channelIterator = mChannels.begin(); channelIterator != mChannels.end();)
	{
		bool isPlaying = false;
		if (channelIterator->second != nullptr)
		{
			channelIterator->second->isPlaying(&isPlaying);
		}

		if (!isPlaying)
		{
			channelIterator = mChannels.erase(channelIterator);
			continue;
		}

		++channelIterator;
	}
}

void SoundManager::ReleaseAllSounds()
{
	for (auto& [key, sound] : mSounds)
	{
		(void)key;
		if (sound != nullptr)
		{
			sound->release();
		}
	}

	mSounds.clear();
}
#endif

void SoundManager::SetError(const std::string& message)
{
	mLastError = message;
	std::cout << "ERROR::SOUNDMANAGER::" << message << std::endl;
}

void SoundManager::ClearError()
{
	mLastError.clear();
}
