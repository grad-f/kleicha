#ifndef SCENE_H
#define SCENE_H

#include "Types.h"

class Scene {
public:
	bool load_scene(const char* filePath, std::vector<vkt::DrawData>& draws, std::vector<vkt::Transform>& transforms, std::vector<vkt::Material>& materials, std::vector<std::string>& texturePaths);
private:

	bool append_mesh(const vkt::Mesh& mesh);

	std::vector<vkt::Vertex> m_unifiedVertices{};
	std::vector<glm::uvec3> m_unifiedTriangles{};
	std::vector<vkt::HostDrawData> m_canonicalHostDrawData{};
};
#endif // !SCENE_H
