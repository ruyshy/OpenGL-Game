#include "pch.h"
#include "Mesh3D.h"

#include "Shader.h"

Mesh3D::Mesh3D(std::vector<Vertex3D> vertices, std::vector<unsigned int> indices, std::vector<Texture3D> textures)
    : mVertices(std::move(vertices)), mIndices(std::move(indices)), mTextures(std::move(textures))
{
    setupMesh();
}

Mesh3D::~Mesh3D()
{
    release();
}

Mesh3D::Mesh3D(Mesh3D&& other) noexcept
    : mVertices(std::move(other.mVertices))
    , mIndices(std::move(other.mIndices))
    , mTextures(std::move(other.mTextures))
    , mVAO(other.mVAO)
    , mVBO(other.mVBO)
    , mEBO(other.mEBO)
{
    other.mVAO = 0;
    other.mVBO = 0;
    other.mEBO = 0;
}

Mesh3D& Mesh3D::operator=(Mesh3D&& other) noexcept
{
    if (this != &other)
    {
        release();
        mVertices = std::move(other.mVertices);
        mIndices = std::move(other.mIndices);
        mTextures = std::move(other.mTextures);
        mVAO = other.mVAO;
        mVBO = other.mVBO;
        mEBO = other.mEBO;
        other.mVAO = 0;
        other.mVBO = 0;
        other.mEBO = 0;
    }

    return *this;
}

void Mesh3D::Draw(const Shader& shader) const
{
    bool hasDiffuseTexture = false;

    for (unsigned int i = 0; i < mTextures.size(); ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        const std::string& type = mTextures[i].Type;

        if (type == "texture_diffuse")
        {
            hasDiffuseTexture = true;
            shader.setInt("material.diffuseMap", static_cast<int>(i));
            shader.setBool("material.useDiffuseMap", true);
        }

        glBindTexture(GL_TEXTURE_2D, mTextures[i].Id);
    }

    if (!hasDiffuseTexture)
    {
        shader.setBool("material.useDiffuseMap", false);
    }

    glBindVertexArray(mVAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mIndices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
}

bool Mesh3D::HasTextures() const
{
    return !mTextures.empty();
}

void Mesh3D::setupMesh()
{
    glGenVertexArrays(1, &mVAO);
    glGenBuffers(1, &mVBO);
    glGenBuffers(1, &mEBO);

    glBindVertexArray(mVAO);

    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mVertices.size() * sizeof(Vertex3D)), mVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(mIndices.size() * sizeof(unsigned int)), mIndices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), reinterpret_cast<void*>(offsetof(Vertex3D, Position)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), reinterpret_cast<void*>(offsetof(Vertex3D, Normal)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), reinterpret_cast<void*>(offsetof(Vertex3D, TexCoords)));

    glBindVertexArray(0);
}

void Mesh3D::release()
{
    if (mEBO != 0)
    {
        glDeleteBuffers(1, &mEBO);
        mEBO = 0;
    }

    if (mVBO != 0)
    {
        glDeleteBuffers(1, &mVBO);
        mVBO = 0;
    }

    if (mVAO != 0)
    {
        glDeleteVertexArrays(1, &mVAO);
        mVAO = 0;
    }
}
