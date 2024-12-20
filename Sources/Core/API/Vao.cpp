#include "Vao.h"

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>

#include "Utils/Logger.h"
namespace Rastery {
CpuVao::SharedPtr CpuVao::createTriangle() {
    CpuVao::SharedPtr pVao = std::make_shared<CpuVao>();
    pVao->vertexData = {Vertex{.position = float3(1, 1, 0), .normal{}, .texCoord{}},
                        Vertex{.position = float3(-1, 1, 0), .normal{}, .texCoord{}},
                        Vertex{.position = float3(0, 0, 0), .normal{}, .texCoord{}}};
    return pVao;
}

CpuVao::SharedPtr createFromFile(const std::filesystem::path& p) {
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        p.string(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_PreTransformVertices);

    if (!scene || !scene->HasMeshes()) {
        logError("Importer error: {}", importer.GetErrorString());
        return nullptr;
    }

    auto pVao = std::make_shared<CpuVao>();

    uint32_t vertexOffset = 0;
    for (int j = 0; j < scene->mNumMeshes; j++) {
        aiMesh* mesh = scene->mMeshes[j];
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            Vertex vertex{};

            vertex.position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};

            if (mesh->HasNormals()) {
                vertex.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
            }

            if (mesh->HasTextureCoords(0)) {
                vertex.texCoord = {mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y};
            }
            pVao->vertexData.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
            aiFace& face = mesh->mFaces[i];
            for (unsigned int k = 0; k < face.mNumIndices; ++k) {
                pVao->indexData.push_back(vertexOffset + face.mIndices[k]);
            }
        }

        vertexOffset += mesh->mNumVertices;
    }

    return pVao;
}
}  // namespace Rastery