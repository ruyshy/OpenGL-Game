#pragma once

#ifndef TEXTENGINE_H_
#define TEXTENGINE_H_

#include "FontAsset.h"
#include "Shader.h"
#include "TextCommon.h"

class TextEngine
{
public:
    TextEngine();
    ~TextEngine();

    TextEngine(const TextEngine&) = delete;
    TextEngine& operator=(const TextEngine&) = delete;

    bool initialize(const string& fontPath, unsigned int pixelHeight);
    void shutdown();

    void setProjection(int width, int height);
    vec2 measureText(wstring_view text, float scale = 1.0f);

    void render(const TextDrawCommand& command);
    void renderText(wstring_view text, float x, float y, const TextStyle& style);

    bool isReady() const;

private:
    bool initializeRenderObjects();

private:
    FreeTypeService _freeTypeService;
    FontAsset _fontAsset;
    shared_ptr<Shader> _shader;
    GLuint _vao = 0;
    GLuint _vbo = 0;
    mat4 _projection = mat4(1.0f);
    bool _isInitialized = false;
};

#endif // !TEXTENGINE_H_
