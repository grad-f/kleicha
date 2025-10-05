#include "Scene.h"


bool Scene::find_scene_node(aiNode* pNode, const aiString& name, const glm::mat4& m4Transform, glm::mat4& m4RetTransform) {
    
    glm::mat4 m4NodeTransform{ glm::transpose(*reinterpret_cast<glm::mat4*>(&pNode->mTransformation)) * m4Transform };

    // check if this is the correct node
    if (strcmp(pNode->mName.data, name.data) == 0) {
        m4RetTransform = m4NodeTransform;
        return true;
    }

    // recursively check child nodes
    for (std::size_t i{ 0 }; i < pNode->mNumChildren; ++i) {
        bool bFoundNode{ find_scene_node(pNode->mChildren[i], name, m4NodeTransform, m4RetTransform)};

        if (bFoundNode)
            return true;
    }

    return false;
}

void Scene::load_scene_node(aiNode* pNode, const aiScene* pScene, std::vector<vkt::HostDrawData>& hostDraws, std::vector<vkt::DrawData>& draws, std::vector<vkt::Transform>& transforms, const glm::mat4& m4Transform) const {

    // compute this node's transformation (assimp stores matrices row-major, therefore we must transpose)
    vkt::Transform nodeTransform{.m_m4Model = m4Transform};

    if (pNode->mNumMeshes > 0) {
        nodeTransform.m_m4Model = glm::transpose(*reinterpret_cast<glm::mat4*>(&pNode->mTransformation)) * m4Transform;
        transforms.push_back(nodeTransform);
    }

    // traverse meshes of node
    for (std::size_t i{ 0 }; i < pNode->mNumMeshes; ++i) {
        // append node meshes to our draw data
        hostDraws.push_back(m_canonicalHostDrawData[pNode->mMeshes[i]]);

        vkt::DrawData drawData{};
        drawData.m_uiTransformIndex = static_cast<uint32_t>(transforms.size()) - 1;
        drawData.m_uiMaterialIndex = pScene->mMeshes[pNode->mMeshes[i]]->mMaterialIndex;
        draws.push_back(drawData);
    }

    // for each node, traverse its children
    for (std::size_t i{ 0 }; i < pNode->mNumChildren; ++i) {
        load_scene_node(pNode->mChildren[i], pScene, hostDraws, draws, transforms, nodeTransform.m_m4Model);
    }
}

void Scene::append_mesh(const vkt::Mesh& mesh) {

    // TO-DO: check if the mesh requires tangents to be computed

    // build mesh host draw data
    vkt::HostDrawData hDraw{};
    hDraw.m_iVertexOffset = static_cast<uint32_t>(m_unifiedVertices.size());
    hDraw.m_uiIndicesOffset = static_cast<uint32_t>(m_unifiedTriangles.size() * 3);
    hDraw.m_uiIndicesCount = static_cast<uint32_t>(mesh.tInd.size() * 3);

    m_canonicalHostDrawData.push_back(hDraw);

    // append mesh to unified vertices and indices
    m_unifiedVertices.reserve(m_unifiedVertices.size() + mesh.verts.size());
    m_unifiedTriangles.reserve(m_unifiedTriangles.size() + mesh.tInd.size());

    m_unifiedVertices.insert(m_unifiedVertices.end(), mesh.verts.begin(), mesh.verts.end());
    m_unifiedTriangles.insert(m_unifiedTriangles.end(), mesh.tInd.begin(), mesh.tInd.end());
}

bool Scene::load_scene(const char* filePath, std::vector<vkt::HostDrawData>& hostDraws, std::vector<vkt::DrawData>& draws, std::vector<vkt::PointLight>& pointLights, std::vector<vkt::Transform>& transforms,
    std::vector<vkt::Material>& materials, std::vector<vkt::Texture>& textures) {

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
        vkt::Texture texture{};
        aiGetMaterialTexture(pAiMaterial, aiTextureType_DIFFUSE, 0, &sTexture);
        material.m_uiAlbedoTexture = 1 + textures.size();
        if (!sTexture.Empty()) {
            texture.path = sPath + sTexture.data;
            texture.type = vkt::TextureType::ALBEDO;
            textures.push_back(texture);
        }

        aiGetMaterialTexture(pAiMaterial, aiTextureType_SPECULAR, 0, &sTexture);
        material.m_uiSpecularTexture = 1 + textures.size();
        if (!sTexture.Empty()) {
            texture.path = sPath + sTexture.data;
            texture.type = vkt::TextureType::SPECULAR;
            textures.push_back(texture);
        }

        aiGetMaterialTexture(pAiMaterial, aiTextureType_SHININESS, 0, &sTexture);
        material.m_uiRoughnessTexture = 1 + textures.size();
        if (!sTexture.Empty()) {
            texture.path = sPath + sTexture.data;
            texture.type = vkt::TextureType::ROUGHNESS;
            textures.push_back(texture);
        }

        materials.push_back(material);
    }

    for (std::size_t i{ 0 }; i < pScene->mNumLights; ++i) {
        const aiLight* pLight{ pScene->mLights[i] };

        glm::mat4 lightTransform{ 1.0f };
        bool bFoundNode{ find_scene_node(pScene->mRootNode, pLight->mName, lightTransform, lightTransform) };

        if (pLight->mType == aiLightSource_POINT && bFoundNode) {
            vkt::PointLight pointLight{};
            glm::vec4 lightPos{ pLight->mPosition.x, pLight->mPosition.y, pLight->mPosition.z, 1.0f };
            pointLight.m_v3Position = static_cast<glm::vec3>(lightTransform * lightPos);
            pointLight.m_v3Color = glm::vec3{ pLight->mColorDiffuse.r, pLight->mColorDiffuse.g, pLight->mColorDiffuse.b };

            pointLight.m_fFalloff.x = pLight->mAttenuationConstant == 0.0f ? 1.0f : pLight->mAttenuationConstant;

            pointLight.m_fFalloff.y = pLight->mAttenuationLinear / 2.0f;
            pointLight.m_fFalloff.z = pLight->mAttenuationQuadratic / 2.0f;

            pointLights.push_back(pointLight);
        }
    }

    // builds host draw data, gpu draw data and transforms
    load_scene_node(pScene->mRootNode, pScene, hostDraws, draws, transforms, glm::mat4{ 1.0f });
    aiReleaseImport(pScene);
    return true;
}
