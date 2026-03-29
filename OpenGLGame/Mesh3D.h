#pragma once

#ifndef MESH3D_H_
#define MESH3D_H_

#include "pch.h"

class Shader;

struct Vertex3D
{
    glm::vec3 Position = glm::vec3(0.0f);
    glm::vec3 Normal = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec2 TexCoords = glm::vec2(0.0f);
};

struct Texture3D
{
    unsigned int Id = 0;
    std::string Type;
    std::string Path;
};

class Mesh3D
{
public:
    Mesh3D() = default;
    Mesh3D(std::vector<Vertex3D> vertices, std::vector<unsigned int> indices, std::vector<Texture3D> textures);
    ~Mesh3D();

    Mesh3D(const Mesh3D&) = delete;
    Mesh3D& operator=(const Mesh3D&) = delete;
    Mesh3D(Mesh3D&& other) noexcept;
    Mesh3D& operator=(Mesh3D&& other) noexcept;

    void Draw(const Shader& shader) const;
    bool HasTextures() const;

private:
    void setupMesh();
    void release();

private:
    std::vector<Vertex3D> mVertices;
    std::vector<unsigned int> mIndices;
    std::vector<Texture3D> mTextures;
    unsigned int mVAO = 0;
    unsigned int mVBO = 0;
    unsigned int mEBO = 0;
};

#endif
