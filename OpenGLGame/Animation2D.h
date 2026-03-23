#pragma once

#ifndef ANIMATION2D_H_
#define ANIMATION2D_H_

#include "pch.h"

class Texture2D;
class VertexBufferObject2D;

class Animation2D
{
public:
	Animation2D() = default;
	explicit Animation2D(const char* filename);
	explicit Animation2D(const std::string& filename);
	~Animation2D() = default;

	bool LoadFromFile(const char* filename);
	bool LoadFromFile(const std::string& filename);
	void SetFrames(const std::vector<vec4>& newFrames);
	bool SetImageFrames(const std::vector<std::string>& imageFilenames);

	void Update(double deltaTime);
	void Apply(Texture2D& spriteTexture, VertexBufferObject2D& rectangle) const;
	void Play(Texture2D& spriteTexture, VertexBufferObject2D& rectangle, double deltaTime);

	void Reset();
	void Stop();
	void Pause();
	void Resume();
	void SetLooping(bool shouldLoop);
	void SetAnimationSpeed(float newSpeed);
	void SetFrameRate(float framesPerSecond);
	void SetCurrentFrame(int frameIndex);

	bool IsFinished() const;
	bool IsPaused() const;
	bool IsLooping() const;
	bool IsEmpty() const;

	int GetCurrentFrameIndex() const;
	int GetFrameCount() const;
	float GetAnimationSpeed() const;
	const vec4& GetCurrentFrame() const;
	const std::vector<vec4>& GetFrames() const;

	void play(Texture2D& spritetexture, VertexBufferObject2D& rectangle, double deltatime);
	void set_animation_speed(float newspeed);

private:
	bool LoadFromStream(std::istream& stream, const std::string& baseDirectory);
	bool TryParseFrameLine(const std::string& line, vec4& outFrame) const;
	bool TryLoadImageFrame(const std::string& line, const std::string& baseDirectory, std::shared_ptr<Texture2D>& outTexture) const;
	void AdvanceFrame();
	void UploadUV(const vec4& normalizedFrame, VertexBufferObject2D& rectangle) const;
	vec4 NormalizeFrame(const vec4& frame, const Texture2D& spriteTexture) const;

private:
	double mAnimationCursor = 0.0;
	int mCurrentFrameIndex = 0;
	float mFrameDuration = 0.05f;
	bool mLooping = true;
	bool mPaused = false;
	bool mFinished = false;
	std::vector<vec4> mFrames;
	std::vector<std::shared_ptr<Texture2D>> mImageFrames;
};

#endif // !ANIMATION2D_H_
