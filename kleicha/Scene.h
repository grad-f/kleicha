#ifndef SCENE_H
#define SCENE_H

#include "Types.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class Scene {
public:
	bool load_scene(const char* filePath, std::vector<vkt::HostDrawData>& hostDraws, std::vector<vkt::DrawData>& draws, std::vector<vkt::PointLight>& pointLights, std::vector<vkt::Transform>& transforms, std::vector<vkt::Material>& materials, std::vector<vkt::Texture>& textures);
	std::vector<vkt::Vertex> m_unifiedVertices{};
	std::vector<glm::uvec3> m_unifiedTriangles{};
private:
	void load_scene_node(aiNode* pNode, const aiScene* pScene, std::vector<vkt::HostDrawData>& hostDraws, std::vector<vkt::DrawData>& draws, std::vector<vkt::Transform>& transforms, const glm::mat4& m4Transform) const;
	bool find_scene_node(aiNode* pNode, const aiString& name, const glm::mat4& m4Transform, glm::mat4& m4RetTransform);
	void append_mesh(const vkt::Mesh& mesh);


	std::vector<vkt::HostDrawData> m_canonicalHostDrawData{};
};
#endif // !SCENE_H
