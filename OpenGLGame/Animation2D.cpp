#include "pch.h"
#include "Animation2D.h"

#include "Texture2D.h"
#include "VertexBuffer2D.h"

namespace
{
	const vec4 kEmptyFrame(0.0f, 0.0f, 1.0f, 1.0f);
}

Animation2D::Animation2D(const char* filename)
{
	LoadFromFile(filename);
}

Animation2D::Animation2D(const std::string& filename)
{
	LoadFromFile(filename);
}

bool Animation2D::LoadFromFile(const char* filename)
{
	return LoadFromFile(filename == nullptr ? std::string() : std::string(filename));
}

bool Animation2D::LoadFromFile(const std::string& filename)
{
	mFrames.clear();
	Reset();

	if (filename.empty())
	{
		std::cout << "ERROR::ANIMATION2D::EMPTY_FILENAME" << std::endl;
		return false;
	}

	std::ifstream file(filename);
	if (!file.is_open())
	{
		std::cout << "ERROR::ANIMATION2D::FILE_OPEN_FAILED: " << filename << std::endl;
		return false;
	}

	const bool loaded = LoadFromStream(file);
	if (!loaded)
	{
		std::cout << "ERROR::ANIMATION2D::NO_VALID_FRAMES: " << filename << std::endl;
	}

	return loaded;
}

void Animation2D::SetFrames(const std::vector<vec4>& newFrames)
{
	mFrames = newFrames;
	Reset();
}

void Animation2D::Update(double deltaTime)
{
	if (mPaused || mFinished || mFrames.size() <= 1)
	{
		return;
	}

	if (deltaTime <= 0.0 || mFrameDuration <= 0.0f)
	{
		return;
	}

	mAnimationCursor += deltaTime;

	while (mAnimationCursor >= mFrameDuration && !mFinished)
	{
		mAnimationCursor -= mFrameDuration;
		AdvanceFrame();
	}
}

void Animation2D::Apply(Texture2D& spriteTexture, VertexBufferObject2D& rectangle) const
{
	if (IsEmpty())
	{
		return;
	}

	const vec4 normalizedFrame = NormalizeFrame(mFrames[mCurrentFrameIndex], spriteTexture);
	UploadUV(normalizedFrame, rectangle);
}

void Animation2D::Play(Texture2D& spriteTexture, VertexBufferObject2D& rectangle, double deltaTime)
{
	Update(deltaTime);
	Apply(spriteTexture, rectangle);
}

void Animation2D::Reset()
{
	mAnimationCursor = 0.0;
	mCurrentFrameIndex = 0;
	mPaused = false;
	mFinished = false;
}

void Animation2D::Stop()
{
	Reset();
	mPaused = true;
}

void Animation2D::Pause()
{
	mPaused = true;
}

void Animation2D::Resume()
{
	if (!IsEmpty())
	{
		mPaused = false;
	}
}

void Animation2D::SetLooping(bool shouldLoop)
{
	mLooping = shouldLoop;
}

void Animation2D::SetAnimationSpeed(float newSpeed)
{
	if (newSpeed > 0.0f)
	{
		mFrameDuration = newSpeed;
	}
}

void Animation2D::SetFrameRate(float framesPerSecond)
{
	if (framesPerSecond > 0.0f)
	{
		mFrameDuration = 1.0f / framesPerSecond;
	}
}

void Animation2D::SetCurrentFrame(int frameIndex)
{
	if (IsEmpty())
	{
		mCurrentFrameIndex = 0;
		return;
	}

	mCurrentFrameIndex = std::clamp(frameIndex, 0, static_cast<int>(mFrames.size()) - 1);
	mAnimationCursor = 0.0;
	mFinished = false;
}

bool Animation2D::IsFinished() const
{
	return mFinished;
}

bool Animation2D::IsPaused() const
{
	return mPaused;
}

bool Animation2D::IsLooping() const
{
	return mLooping;
}

bool Animation2D::IsEmpty() const
{
	return mFrames.empty();
}

int Animation2D::GetCurrentFrameIndex() const
{
	return mCurrentFrameIndex;
}

int Animation2D::GetFrameCount() const
{
	return static_cast<int>(mFrames.size());
}

float Animation2D::GetAnimationSpeed() const
{
	return mFrameDuration;
}

const vec4& Animation2D::GetCurrentFrame() const
{
	return IsEmpty() ? kEmptyFrame : mFrames[mCurrentFrameIndex];
}

const std::vector<vec4>& Animation2D::GetFrames() const
{
	return mFrames;
}

void Animation2D::play(Texture2D& spritetexture, VertexBufferObject2D& rectangle, double deltatime)
{
	Play(spritetexture, rectangle, deltatime);
}

void Animation2D::set_animation_speed(float newspeed)
{
	SetAnimationSpeed(newspeed);
}

bool Animation2D::LoadFromStream(std::istream& stream)
{
	std::string line;
	while (std::getline(stream, line))
	{
		vec4 frame;
		if (TryParseFrameLine(line, frame))
		{
			mFrames.push_back(frame);
		}
	}

	return !mFrames.empty();
}

bool Animation2D::TryParseFrameLine(const std::string& line, vec4& outFrame) const
{
	std::string trimmed = line;
	trimmed.erase(std::remove(trimmed.begin(), trimmed.end(), '\r'), trimmed.end());

	const auto first = trimmed.find_first_not_of(" \t");
	if (first == std::string::npos)
	{
		return false;
	}

	if (trimmed[first] == '#')
	{
		return false;
	}

	std::stringstream ss(trimmed);
	std::string token;
	std::array<float, 4> values = {};
	int index = 0;

	while (std::getline(ss, token, ','))
	{
		if (index >= static_cast<int>(values.size()))
		{
			return false;
		}

		try
		{
			values[index++] = std::stof(token);
		}
		catch (const std::exception&)
		{
			return false;
		}
	}

	if (index != 4)
	{
		return false;
	}

	outFrame = vec4(values[0], values[1], values[2], values[3]);
	return true;
}

void Animation2D::AdvanceFrame()
{
	if (mFrames.size() <= 1)
	{
		return;
	}

	const int lastFrameIndex = static_cast<int>(mFrames.size()) - 1;
	if (mCurrentFrameIndex < lastFrameIndex)
	{
		++mCurrentFrameIndex;
		return;
	}

	if (mLooping)
	{
		mCurrentFrameIndex = 0;
		return;
	}

	mFinished = true;
	mPaused = true;
	mCurrentFrameIndex = lastFrameIndex;
	mAnimationCursor = 0.0;
}

void Animation2D::UploadUV(const vec4& normalizedFrame, VertexBufferObject2D& rectangle) const
{
	const vec2 uv[4] =
	{
		vec2(normalizedFrame.x, normalizedFrame.y),
		vec2(normalizedFrame.x, normalizedFrame.y + normalizedFrame.w),
		vec2(normalizedFrame.x + normalizedFrame.z, normalizedFrame.y),
		vec2(normalizedFrame.x + normalizedFrame.z, normalizedFrame.y + normalizedFrame.w)
	};

	glBindVertexArray(rectangle.VAO);
	glBindBuffer(GL_ARRAY_BUFFER, rectangle.UVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(uv), uv);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

vec4 Animation2D::NormalizeFrame(const vec4& frame, const Texture2D& spriteTexture) const
{
	if (spriteTexture.width <= 0 || spriteTexture.height <= 0)
	{
		return vec4(0.0f);
	}

	return vec4(
		frame.x / static_cast<float>(spriteTexture.width),
		frame.y / static_cast<float>(spriteTexture.height),
		frame.z / static_cast<float>(spriteTexture.width),
		frame.w / static_cast<float>(spriteTexture.height));
}
