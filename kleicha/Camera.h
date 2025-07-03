#ifndef CAMERA_H
#define CAMERA_H

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "Utils.h"

enum CameraMoveFlags {
	FORWARD,
	BACKWARD,
	RIGHT,
	LEFT
};

class Camera {
public:

	Camera(const glm::vec3& pos, const glm::vec3& lookAt, const glm::vec3& up)
		: m_pos{ pos }, m_lookAt{ lookAt }, m_up{ up }
	{
		m_gazeDir = glm::normalize(lookAt - pos);
		m_u = glm::normalize(glm::cross(m_gazeDir, up));
	};

	void moveCameraPosition(CameraMoveFlags direction, float deltaTime) {
		float velocity{ 2.5f * deltaTime };

		if (direction == FORWARD)
			m_pos += velocity * m_gazeDir;
		else if (direction == BACKWARD)
			m_pos -= velocity * m_gazeDir;
		else if (direction == RIGHT)
			m_pos += velocity * m_u;
		else if (direction == LEFT)
			m_pos -= velocity * m_u;
	}

	glm::mat4 getViewMatrix() {
		return utils::lookAt(m_pos, m_pos + m_gazeDir, m_up);
	}

private:
	glm::vec3 m_pos{};
	glm::vec3 m_lookAt{};
	glm::vec3 m_up{};
	glm::vec3 m_gazeDir{};

	glm::vec3 m_u{};
	glm::vec3 m_v{};
	glm::vec3 m_w{};
};

#endif // !CAMERA_H
