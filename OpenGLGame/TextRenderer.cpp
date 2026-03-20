#include "pch.h"
#include "TextRenderer.h"

#include "Encoding.h"

namespace
{
	const char* kTextVertexShader = R"(
		#version 330 core
		layout (location = 0) in vec4 vertex;

		out vec2 TexCoords;

		uniform mat4 projection;

		void main()
		{
			gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
			TexCoords = vertex.zw;
		}
	)";

	const char* kTextFragmentShader = R"(
		#version 330 core
		in vec2 TexCoords;
		out vec4 FragColor;

		uniform sampler2D text;
		uniform vec3 textColor;

		void main()
		{
			float alpha = texture(text, TexCoords).r;
			FragColor = vec4(textColor, alpha);
		}
	)";
}

TextRenderer::TextRenderer()
{
}

TextRenderer::~TextRenderer()
{
	ReleaseGlyphs();
	ReleaseFontFace();

	if (mVBO != 0)
	{
		glDeleteBuffers(1, &mVBO);
	}

	if (mVAO != 0)
	{
		glDeleteVertexArrays(1, &mVAO);
	}

	if (mLibrary != nullptr)
	{
		FT_Done_FreeType(mLibrary);
		mLibrary = nullptr;
	}
}

bool TextRenderer::Initialize(int viewportWidth, int viewportHeight)
{
	if (mInitialized)
	{
		SetViewport(viewportWidth, viewportHeight);
		return true;
	}

	if (FT_Init_FreeType(&mLibrary) != 0)
	{
		std::cout << "ERROR::TEXT::FREETYPE_INIT_FAILED" << std::endl;
		return false;
	}

	mpShader = std::make_shared<Shader>(std::string(kTextVertexShader), std::string(kTextFragmentShader));
	mpShader->use();
	mpShader->setInt("text", 0);

	glGenVertexArrays(1, &mVAO);
	glGenBuffers(1, &mVBO);

	glBindVertexArray(mVAO);
	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	SetViewport(viewportWidth, viewportHeight);
	mInitialized = true;
	return true;
}

bool TextRenderer::LoadFont(const std::string& fontPath, unsigned int pixelHeight)
{
	if (!mInitialized)
	{
		return false;
	}

	ReleaseGlyphs();
	ReleaseFontFace();

	if (FT_New_Face(mLibrary, fontPath.c_str(), 0, &mFace) != 0)
	{
		std::cout << "ERROR::TEXT::FONT_LOAD_FAILED: " << fontPath << std::endl;
		return false;
	}

	if (FT_Set_Pixel_Sizes(mFace, 0, pixelHeight) != 0)
	{
		std::cout << "ERROR::TEXT::FONT_SIZE_SET_FAILED: " << fontPath << std::endl;
		ReleaseFontFace();
		return false;
	}

	mPixelHeight = pixelHeight;
	mBaseLineHeight = static_cast<float>(mFace->size->metrics.height >> 6);

	for (wchar_t codepoint = 32; codepoint < 127; ++codepoint)
	{
		EnsureGlyph(codepoint);
	}
	return true;
}

void TextRenderer::SetViewport(int viewportWidth, int viewportHeight)
{
	if (mpShader == nullptr)
	{
		return;
	}

	mpShader->use();
	mpShader->setMat4("projection", ortho(0.0f, static_cast<float>(viewportWidth), 0.0f, static_cast<float>(viewportHeight)));
}

void TextRenderer::RenderText(const std::string& utf8Text, float x, float y, float scale, const vec3& color)
{
	if (!IsReady())
	{
		return;
	}

	const std::wstring wideText = ToWideText(utf8Text);
	const float startX = x;

	mpShader->use();
	mpShader->setVec3("textColor", color);

	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(mVAO);

	for (const wchar_t codepoint : wideText)
	{
		if (codepoint == L'\r')
		{
			continue;
		}

		if (codepoint == L'\n')
		{
			x = startX;
			y -= GetLineHeight(scale);
			continue;
		}

		if (!EnsureGlyph(codepoint))
		{
			continue;
		}

		const TextGlyph& glyph = mGlyphs[codepoint];
		const float xpos = x + static_cast<float>(glyph.Bearing.x) * scale;
		const float ypos = y - static_cast<float>(glyph.Size.y - glyph.Bearing.y) * scale;
		const float w = static_cast<float>(glyph.Size.x) * scale;
		const float h = static_cast<float>(glyph.Size.y) * scale;

		const float vertices[6][4] =
		{
			{ xpos,     ypos + h, 0.0f, 0.0f },
			{ xpos,     ypos,     0.0f, 1.0f },
			{ xpos + w, ypos,     1.0f, 1.0f },

			{ xpos,     ypos + h, 0.0f, 0.0f },
			{ xpos + w, ypos,     1.0f, 1.0f },
			{ xpos + w, ypos + h, 1.0f, 0.0f }
		};

		if (glyph.TextureID != 0 && glyph.Size.x > 0 && glyph.Size.y > 0)
		{
			glBindTexture(GL_TEXTURE_2D, glyph.TextureID);
			glBindBuffer(GL_ARRAY_BUFFER, mVBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		x += static_cast<float>(glyph.Advance >> 6) * scale;
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

float TextRenderer::MeasureTextWidth(const std::string& utf8Text, float scale)
{
	if (!IsReady())
	{
		return 0.0f;
	}

	float currentLineWidth = 0.0f;
	float widestLine = 0.0f;
	const std::wstring wideText = ToWideText(utf8Text);

	for (const wchar_t codepoint : wideText)
	{
		if (codepoint == L'\r')
		{
			continue;
		}

		if (codepoint == L'\n')
		{
			widestLine = std::max(widestLine, currentLineWidth);
			currentLineWidth = 0.0f;
			continue;
		}

		if (!EnsureGlyph(codepoint))
		{
			continue;
		}

		currentLineWidth += static_cast<float>(mGlyphs[codepoint].Advance >> 6) * scale;
	}

	return std::max(widestLine, currentLineWidth);
}

float TextRenderer::GetLineHeight(float scale) const
{
	return mBaseLineHeight * scale;
}

bool TextRenderer::IsReady() const
{
	return mInitialized && mFace != nullptr && mpShader != nullptr;
}

bool TextRenderer::EnsureGlyph(wchar_t codepoint)
{
	if (mGlyphs.contains(codepoint))
	{
		return true;
	}

	if (mFace == nullptr)
	{
		return false;
	}

	if (FT_Load_Char(mFace, static_cast<FT_ULong>(codepoint), FT_LOAD_RENDER) != 0)
	{
		std::cout << "WARN::TEXT::GLYPH_LOAD_FAILED: " << static_cast<unsigned int>(codepoint) << std::endl;
		return false;
	}

	TextGlyph glyph;
	glyph.Size = ivec2(mFace->glyph->bitmap.width, mFace->glyph->bitmap.rows);
	glyph.Bearing = ivec2(mFace->glyph->bitmap_left, mFace->glyph->bitmap_top);
	glyph.Advance = static_cast<unsigned int>(mFace->glyph->advance.x);

	if (glyph.Size.x > 0 && glyph.Size.y > 0)
	{
		GLint previousUnpackAlignment = 4;
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		glGenTextures(1, &glyph.TextureID);
		glBindTexture(GL_TEXTURE_2D, glyph.TextureID);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RED,
			glyph.Size.x,
			glyph.Size.y,
			0,
			GL_RED,
			GL_UNSIGNED_BYTE,
			mFace->glyph->bitmap.buffer);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);
	}

	mGlyphs[codepoint] = glyph;
	return true;
}

void TextRenderer::ReleaseGlyphs()
{
	for (auto& [codepoint, glyph] : mGlyphs)
	{
		(void)codepoint;
		if (glyph.TextureID != 0)
		{
			glDeleteTextures(1, &glyph.TextureID);
		}
	}

	mGlyphs.clear();
}

void TextRenderer::ReleaseFontFace()
{
	if (mFace != nullptr)
	{
		FT_Done_Face(mFace);
		mFace = nullptr;
	}

	mPixelHeight = 0;
	mBaseLineHeight = 0.0f;
}

std::wstring TextRenderer::ToWideText(const std::string& utf8Text) const
{
	return util::encoding::to_wide(utf8Text, 65001);
}
