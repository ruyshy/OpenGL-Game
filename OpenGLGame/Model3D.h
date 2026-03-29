#pragma once

#ifndef MODEL3D_H_
#define MODEL3D_H_

#include "pch.h"
#include "Mesh3D.h"

struct aiMaterial;
struct aiMesh;
struct aiNode;
struct aiScene;

class Shader;

class Model3D
{
public:
    bool LoadFromFile(const std::string& path);
    void CreateDemoCube();
    void Draw(const Shader& shader) const;
    bool IsLoaded() const;

private:
    void processNode(aiNode* node, const aiScene* scene);
    Mesh3D processMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture3D> loadMaterialTextures(aiMaterial* material, int type, const std::string& typeName);
    unsigned int loadTextureFromFile(const std::string& path) const;

private:
    std::vector<Mesh3D> mMeshes;
    std::vector<Texture3D> mLoadedTextures;
    std::string mDirectory;
};

#endif
