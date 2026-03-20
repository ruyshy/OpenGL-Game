#pragma once

#ifndef FONTASSET_H_
#define FONTASSET_H_

#include "FreeTypeService.h"
#include "TextCommon.h"

class FontAsset
{
public:
    FontAsset() = default;
    ~FontAsset();

    FontAsset(const FontAsset&) = delete;
    FontAsset& operator=(const FontAsset&) = delete;

    bool initialize(FreeTypeService& freeTypeService, const string& fontPath, unsigned int pixelHeight);
    void shutdown();

    const TextGlyph* getGlyph(wchar_t character);
    vec2 measureText(wstring_view text, float scale);

    bool isReady() const;

private:
    bool loadGlyph(wchar_t character);
    void releaseGlyphs();

private:
    FT_Face _face = nullptr;
    map<wchar_t, TextGlyph> _glyphs;
};

#endif // !FONTASSET_H_
