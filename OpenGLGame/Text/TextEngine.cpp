#include "pch.h"
#include "TextEngine.h"

namespace
{
    const string kTextVertexShader = R"(#version 330 core
layout (location = 0) in vec4 vertex;

out vec2 TexCoords;

uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
)";

    const string kTextFragmentShader = R"(#version 330 core
in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D textTexture;
uniform vec3 textColor;

void main()
{
    float alpha = texture(textTexture, TexCoords).r;
    FragColor = vec4(textColor, alpha);
}
)";
}

TextEngine::TextEngine() = default;

TextEngine::~TextEngine()
{
    shutdown();
}

bool TextEngine::initialize(const string& fontPath, unsigned int pixelHeight)
{
    shutdown();

    if (!_freeTypeService.initialize())
    {
        return false;
    }

    if (!_fontAsset.initialize(_freeTypeService, fontPath, pixelHeight))
    {
        shutdown();
        return false;
    }

    _shader = make_shared<Shader>(kTextVertexShader, kTextFragmentShader);
    _shader->use();
    _shader->setInt("textTexture", 0);

    if (!initializeRenderObjects())
    {
        shutdown();
        return false;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    _isInitialized = true;
    return true;
}

void TextEngine::shutdown()
{
    if (_vbo != 0)
    {
        glDeleteBuffers(1, &_vbo);
        _vbo = 0;
    }

    if (_vao != 0)
    {
        glDeleteVertexArrays(1, &_vao);
        _vao = 0;
    }

    _shader.reset();
    _fontAsset.shutdown();
    _freeTypeService.shutdown();
    _isInitialized = false;
}

void TextEngine::setProjection(int width, int height)
{
    _projection = ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height));
}

vec2 TextEngine::measureText(wstring_view text, float scale)
{
    return _fontAsset.measureText(text, scale);
}

void TextEngine::render(const TextDrawCommand& command)
{
    renderText(command.text, command.baseline.x, command.baseline.y, command.style);
}

void TextEngine::renderText(wstring_view text, float x, float y, const TextStyle& style)
{
    if (!_isInitialized || _shader == nullptr)
    {
        return;
    }

    _shader->use();
    _shader->setMat4("projection", _projection);
    _shader->setVec3("textColor", style.color);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(_vao);

    float cursorX = x;
    for (const wchar_t character : text)
    {
        const TextGlyph* glyph = _fontAsset.getGlyph(character);
        if (glyph == nullptr)
        {
            continue;
        }

        const float xpos = cursorX + static_cast<float>(glyph->bearing.x) * style.scale;
        const float ypos = y - static_cast<float>(glyph->size.y - glyph->bearing.y) * style.scale;
        const float width = static_cast<float>(glyph->size.x) * style.scale;
        const float height = static_cast<float>(glyph->size.y) * style.scale;

        const float vertices[6][4] =
        {
            { xpos,         ypos + height, 0.0f, 0.0f },
            { xpos,         ypos,          0.0f, 1.0f },
            { xpos + width, ypos,          1.0f, 1.0f },
            { xpos,         ypos + height, 0.0f, 0.0f },
            { xpos + width, ypos,          1.0f, 1.0f },
            { xpos + width, ypos + height, 1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, glyph->textureID);
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        cursorX += static_cast<float>(glyph->advance >> 6) * style.scale;
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

bool TextEngine::isReady() const
{
    return _isInitialized;
}

bool TextEngine::initializeRenderObjects()
{
    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);

    if (_vao == 0 || _vbo == 0)
    {
        return false;
    }

    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return true;
}
