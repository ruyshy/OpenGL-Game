#include "pch.h"
#include "Animation2D.h"

#include "Texture2D.h"
#include "VertexBuffer2D.h"

#include <filesystem>

namespace
{
	const vec4 kEmptyFrame(0.0f, 0.0f, 1.0f, 1.0f);

	std::string TrimLine(const std::string& line)
	{
		const auto first = line.find_first_not_of(" \t\r");
		if (first == std::string::npos)
		{
			return std::string();
		}

		const auto last = line.find_last_not_of(" \t\r");
		return line.substr(first, last - first + 1);
	}

	std::shared_ptr<Texture2D> CreateManagedTexture(const std::string& filename)
	{
		return std::shared_ptr<Texture2D>(
			new Texture2D(TextureSystem::Generate(filename.c_str())),
			[](Texture2D* texture)
			{
				if (texture != nullptr)
				{
					TextureSystem::Delete(*texture);
					delete texture;
				}
			});
	}
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
	mImageFrames.clear();
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

	const std::string baseDirectory = std::filesystem::path(filename).parent_path().string();
	const bool loaded = LoadFromStream(file, baseDirectory);
	if (!loaded)
	{
		std::cout << "ERROR::ANIMATION2D::NO_VALID_FRAMES: " << filename << std::endl;
	}

	return loaded;
}

void Animation2D::SetFrames(const std::vector<vec4>& newFrames)
{
	mFrames = newFrames;
	mImageFrames.clear();
	Reset();
}

bool Animation2D::SetImageFrames(const std::vector<std::string>& imageFilenames)
{
	mFrames.clear();
	mImageFrames.clear();
	Reset();

	for (const std::string& imageFilename : imageFilenames)
	{
		std::shared_ptr<Texture2D> texture;
		if (!TryLoadImageFrame(imageFilename, std::string(), texture))
		{
			mImageFrames.clear();
			return false;
		}

		mImageFrames.push_back(texture);
	}

	return !mImageFrames.empty();
}

void Animation2D::Update(double deltaTime)
{
	if (mPaused || mFinished || GetFrameCount() <= 1)
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

	if (!mImageFrames.empty())
	{
		spriteTexture = *mImageFrames[mCurrentFrameIndex];
		UploadUV(kEmptyFrame, rectangle);
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

	mCurrentFrameIndex = std::clamp(frameIndex, 0, GetFrameCount() - 1);
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
	return mFrames.empty() && mImageFrames.empty();
}

int Animation2D::GetCurrentFrameIndex() const
{
	return mCurrentFrameIndex;
}

int Animation2D::GetFrameCount() const
{
	return !mImageFrames.empty()
		? static_cast<int>(mImageFrames.size())
		: static_cast<int>(mFrames.size());
}

float Animation2D::GetAnimationSpeed() const
{
	return mFrameDuration;
}

const vec4& Animation2D::GetCurrentFrame() const
{
	return mFrames.empty() ? kEmptyFrame : mFrames[mCurrentFrameIndex];
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

bool Animation2D::LoadFromStream(std::istream& stream, const std::string& baseDirectory)
{
	std::string line;
	while (std::getline(stream, line))
	{
		vec4 frame;
		if (TryParseFrameLine(line, frame))
		{
			if (!mImageFrames.empty())
			{
				return false;
			}

			mFrames.push_back(frame);
			continue;
		}

		std::shared_ptr<Texture2D> texture;
		if (TryLoadImageFrame(line, baseDirectory, texture))
		{
			if (!mFrames.empty())
			{
				return false;
			}

			mImageFrames.push_back(texture);
		}
	}

	return !mFrames.empty() || !mImageFrames.empty();
}

bool Animation2D::TryParseFrameLine(const std::string& line, vec4& outFrame) const
{
	const std::string trimmed = TrimLine(line);
	if (trimmed.empty())
	{
		return false;
	}

	if (trimmed[0] == '#')
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

bool Animation2D::TryLoadImageFrame(const std::string& line, const std::string& baseDirectory, std::shared_ptr<Texture2D>& outTexture) const
{
	std::string trimmed = TrimLine(line);
	if (trimmed.empty() || trimmed[0] == '#')
	{
		return false;
	}

	if (trimmed.front() == '"' && trimmed.back() == '"' && trimmed.size() >= 2)
	{
		trimmed = trimmed.substr(1, trimmed.size() - 2);
	}

	std::filesystem::path resolvedPath(trimmed);
	if (resolvedPath.is_relative() && !baseDirectory.empty())
	{
		resolvedPath = std::filesystem::path(baseDirectory) / resolvedPath;
	}

	if (!std::filesystem::exists(resolvedPath))
	{
		return false;
	}

	outTexture = CreateManagedTexture(resolvedPath.string());
	return outTexture != nullptr && outTexture->ID != 0 && outTexture->width > 0 && outTexture->height > 0;
}

void Animation2D::AdvanceFrame()
{
	const int frameCount = GetFrameCount();
	if (frameCount <= 1)
	{
		return;
	}

	const int lastFrameIndex = frameCount - 1;
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

	const float normalizedWidth = frame.z / static_cast<float>(spriteTexture.width);
	const float normalizedHeight = frame.w / static_cast<float>(spriteTexture.height);
	const float normalizedX = frame.x / static_cast<float>(spriteTexture.width);
	const float normalizedY = 1.0f - ((frame.y + frame.w) / static_cast<float>(spriteTexture.height));

	return vec4(
		normalizedX,
		normalizedY,
		normalizedWidth,
		normalizedHeight);
}
