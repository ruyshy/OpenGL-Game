#pragma once

#ifndef TEXTCOMMON_H_
#define TEXTCOMMON_H_

#include "pch.h"

struct TextGlyph
{
    GLuint textureID = 0;
    ivec2 size = ivec2(0);
    ivec2 bearing = ivec2(0);
    unsigned int advance = 0;
};

struct TextStyle
{
    float scale = 1.0f;
    vec3 color = vec3(1.0f);
};

struct TextDrawCommand
{
    wstring_view text;
    vec2 baseline = vec2(0.0f);
    TextStyle style;
};

#endif // !TEXTCOMMON_H_
