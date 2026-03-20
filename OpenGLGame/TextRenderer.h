#pragma once

#ifndef TEXTRENDERER_H_
#define TEXTRENDERER_H_

#include "pch.h"
#include "Shader.h"

#include <ft2build.h>
#include FT_FREETYPE_H

struct TextGlyph
{
	unsigned int TextureID = 0;
	ivec2 Size = ivec2(0);
	ivec2 Bearing = ivec2(0);
	unsigned int Advance = 0;
};

class TextRenderer
{
public:
	TextRenderer();
	~TextRenderer();

	TextRenderer(const TextRenderer&) = delete;
	TextRenderer& operator=(const TextRenderer&) = delete;

	bool Initialize(int viewportWidth, int viewportHeight);
	bool LoadFont(const std::string& fontPath, unsigned int pixelHeight);
	void SetViewport(int viewportWidth, int viewportHeight);
	void RenderText(const std::string& utf8Text, float x, float y, float scale, const vec3& color);
	float MeasureTextWidth(const std::string& utf8Text, float scale = 1.0f);
	float GetLineHeight(float scale = 1.0f) const;
	bool IsReady() const;

private:
	bool EnsureGlyph(wchar_t codepoint);
	void ReleaseGlyphs();
	void ReleaseFontFace();
	std::wstring ToWideText(const std::string& utf8Text) const;

private:
	FT_Library mLibrary = nullptr;
	FT_Face mFace = nullptr;
	std::map<wchar_t, TextGlyph> mGlyphs;
	std::shared_ptr<Shader> mpShader = nullptr;
	unsigned int mVAO = 0;
	unsigned int mVBO = 0;
	unsigned int mPixelHeight = 0;
	float mBaseLineHeight = 0.0f;
	bool mInitialized = false;
};

#endif // !TEXTRENDERER_H_
