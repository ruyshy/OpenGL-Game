#include "pch.h"
#include "FontAsset.h"

FontAsset::~FontAsset()
{
    shutdown();
}

bool FontAsset::initialize(FreeTypeService& freeTypeService, const string& fontPath, unsigned int pixelHeight)
{
    shutdown();

    if (!freeTypeService.isReady())
    {
        return false;
    }

    if (FT_New_Face(freeTypeService.getLibrary(), fontPath.c_str(), 0, &_face) != 0)
    {
        cerr << "Failed to load font: " << fontPath << endl;
        shutdown();
        return false;
    }

    FT_Set_Pixel_Sizes(_face, 0, pixelHeight);
    return true;
}

void FontAsset::shutdown()
{
    releaseGlyphs();

    if (_face != nullptr)
    {
        FT_Done_Face(_face);
        _face = nullptr;
    }
}

const TextGlyph* FontAsset::getGlyph(wchar_t character)
{
    if (!loadGlyph(character))
    {
        return nullptr;
    }

    return &_glyphs.at(character);
}

vec2 FontAsset::measureText(wstring_view text, float scale)
{
    float width = 0.0f;
    float maxHeight = 0.0f;

    for (const wchar_t character : text)
    {
        const TextGlyph* glyph = getGlyph(character);
        if (glyph == nullptr)
        {
            continue;
        }

        width += static_cast<float>(glyph->advance >> 6) * scale;
        maxHeight = (std::max)(maxHeight, static_cast<float>(glyph->size.y) * scale);
    }

    return vec2(width, maxHeight);
}

bool FontAsset::isReady() const
{
    return _face != nullptr;
}

bool FontAsset::loadGlyph(wchar_t character)
{
    if (_face == nullptr)
    {
        return false;
    }

    if (_glyphs.contains(character))
    {
        return true;
    }

    if (FT_Load_Char(_face, static_cast<FT_ULong>(character), FT_LOAD_RENDER) != 0)
    {
        cerr << "Failed to load glyph for character code: " << static_cast<unsigned int>(character) << endl;
        return false;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        _face->glyph->bitmap.width,
        _face->glyph->bitmap.rows,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        _face->glyph->bitmap.buffer);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    TextGlyph glyph;
    glyph.textureID = texture;
    glyph.size = ivec2(_face->glyph->bitmap.width, _face->glyph->bitmap.rows);
    glyph.bearing = ivec2(_face->glyph->bitmap_left, _face->glyph->bitmap_top);
    glyph.advance = static_cast<unsigned int>(_face->glyph->advance.x);
    _glyphs.insert({ character, glyph });

    return true;
}

void FontAsset::releaseGlyphs()
{
    for (auto& [character, glyph] : _glyphs)
    {
        (void)character;
        if (glyph.textureID != 0)
        {
            glDeleteTextures(1, &glyph.textureID);
        }
    }

    _glyphs.clear();
}
