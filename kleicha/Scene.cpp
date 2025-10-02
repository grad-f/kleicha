#include "Scene.h"
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

bool Scene::append_mesh(const vkt::Mesh& mesh) {

    // TO-DO: check if the mesh requires tangents to be computed

    // build mesh host draw data
    vkt::HostDrawData hDraw{};
    hDraw.m_uiDrawId = m_canonicalHostDrawData.size();
    hDraw.m_iVertexOffset = m_unifiedVertices.size();
    hDraw.m_uiIndicesOffset = m_unifiedTriangles.size() * 3;
    hDraw.m_uiIndicesCount = mesh.tInd.size() * 3;

    m_canonicalHostDrawData.push_back(hDraw);

    // append mesh to unified vertices and indices
    m_unifiedVertices.resize(m_unifiedVertices.size() + mesh.verts.size());
    m_unifiedTriangles.resize(m_unifiedTriangles.size() + mesh.tInd.size());

    m_unifiedVertices.insert(m_unifiedVertices.end(), mesh.verts.begin(), mesh.verts.end());
    m_unifiedTriangles.insert(m_unifiedTriangles.end(), mesh.tInd.begin(), mesh.tInd.end());
}

bool Scene::load_scene(const char* filePath, std::vector<vkt::DrawData>& draws, std::vector<vkt::Transform>& transforms,
    std::vector<vkt::Material>& materials, std::vector<std::string>& texturePaths) {

    const aiScene* pScene{ aiImportFile(filePath,
       aiProcess_GenSmoothNormals |
       aiProcess_CalcTangentSpace |
       aiProcess_Triangulate |
       aiProcess_ImproveCacheLocality |
       aiProcess_SortByPType) };

    if (!pScene) {
        aiReleaseImport(pScene);
        return false;
    }

    // load mesh data
    for (std::size_t i{ 0 }; i < pScene->mNumMeshes; ++i) {
        const aiMesh* pAiMesh{ pScene->mMeshes[i] };

        vkt::Mesh mesh{};
        mesh.verts.resize(pAiMesh->mNumVertices);
        mesh.tInd.resize(pAiMesh->mNumFaces);

        for (std::size_t j{ 0 }; j < pAiMesh->mNumVertices; ++j) {
            mesh.verts[j].m_v3Position = glm::vec3{ pAiMesh->mVertices[j].x, pAiMesh->mVertices[j].y, pAiMesh->mVertices[j].z };
            mesh.verts[j].m_v3Normal = glm::vec3{ pAiMesh->mNormals[j].x, pAiMesh->mNormals[j].y, pAiMesh->mNormals[j].z };
            mesh.verts[j].m_v2UV = glm::vec2{ pAiMesh->mTextureCoords[0][j].x, pAiMesh->mTextureCoords[0][j].y };
        }

        for (std::size_t j{ 0 }; j < pAiMesh->mNumFaces; ++j) {
            mesh.tInd[j].x = pAiMesh->mFaces[j].mIndices[0];
            mesh.tInd[j].y = pAiMesh->mFaces[j].mIndices[1];
            mesh.tInd[j].z = pAiMesh->mFaces[j].mIndices[2];
        }
        append_mesh(mesh);
    }

    for (std::size_t i{ 0 }; i < pScene->mNumMaterials; ++i) {
        const aiMaterial* pAiMaterial{ pScene->mMaterials[i] };

        vkt::Material material{};

        std::string sPath{ filePath };
        std::size_t pos{ sPath.find_last_of("/\\") };
        // handle case where files are in the project folder
        if (pos == std::string::npos)
            sPath = "";
        else
            sPath = sPath.substr(0, pos + 1);

        aiString sTexture{};
        aiGetMaterialTexture(pAiMaterial, aiTextureType_DIFFUSE, 0, &sTexture);
        material.m_uiAlbedoTexture = 1 + texturePaths.size();
        texturePaths.push_back(sPath + sTexture.data);

        aiGetMaterialTexture(pAiMaterial, aiTextureType_SPECULAR, 0, &sTexture);
        material.m_uiSpecularTexture = 1 + texturePaths.size();
        texturePaths.push_back(sPath + sTexture.data);

        aiGetMaterialTexture(pAiMaterial, aiTextureType_SHININESS, 0, &sTexture);
        material.m_uiRoughnessTexture = 1 + texturePaths.size();
        texturePaths.push_back(sPath + sTexture.data);

        materials.push_back(material);
    }

    // TO-DO: Traverse nodes -- we now have the data we need to construct the host draw data as we traverse them

    return true;
}
