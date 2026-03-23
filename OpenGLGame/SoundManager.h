#pragma once

#ifndef SOUNDMANAGER_H_
#define SOUNDMANAGER_H_

#include "pch.h"

#if defined(__has_include)
#if __has_include("fmod.hpp")
#define OPENGLGAME_HAS_FMOD 1
#include "fmod.hpp"
#else
#define OPENGLGAME_HAS_FMOD 0
namespace FMOD
{
	class System;
	class Sound;
	class Channel;
	class ChannelGroup;
}
#endif
#else
#define OPENGLGAME_HAS_FMOD 0
namespace FMOD
{
	class System;
	class Sound;
	class Channel;
	class ChannelGroup;
}
#endif

class SoundManager
{
public:
	static SoundManager& Get();

	bool Initialize(int maxChannels = 256);
	void Update();
	void Shutdown();

	bool LoadSound(const std::string& key, const std::string& filename, bool loop = false, bool stream = false);
	void UnloadSound(const std::string& key);
	int PlaySound(const std::string& key, float volume = 1.0f, bool paused = false);
	bool StopChannel(int channelId);
	void StopAll();
	bool SetChannelPaused(int channelId, bool paused);
	bool SetChannelVolume(int channelId, float volume);
	bool SetMasterVolume(float volume);

	bool IsInitialized() const;
	bool IsBackendAvailable() const;
	bool HasSound(const std::string& key) const;
	const std::string& GetLastError() const;

private:
	SoundManager() = default;
	~SoundManager() = default;
	SoundManager(const SoundManager&) = delete;
	SoundManager& operator=(const SoundManager&) = delete;

#if OPENGLGAME_HAS_FMOD
	bool CheckResult(int result, const char* action);
	void CleanupStoppedChannels();
	void ReleaseAllSounds();
#endif
	void SetError(const std::string& message);
	void ClearError();

private:
	bool mInitialized = false;
	std::string mLastError;
	int mNextChannelId = 1;

#if OPENGLGAME_HAS_FMOD
	FMOD::System* mSystem = nullptr;
	FMOD::ChannelGroup* mMasterGroup = nullptr;
	std::map<std::string, FMOD::Sound*> mSounds;
	std::map<int, FMOD::Channel*> mChannels;
#endif
};

#endif // !SOUNDMANAGER_H_
