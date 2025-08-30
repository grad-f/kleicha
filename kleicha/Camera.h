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

constexpr glm::vec3 WORLD_UP{ 0.0f, 1.0f, 0.0f };
constexpr float X_SENS{ 0.1f };
constexpr float Y_SENS{ 0.1f };
constexpr float MOVEMENT_FACTOR{ 4.0f };

class Camera {
public:
	Camera(const glm::vec3& pos, const VkExtent2D& windowExtent)
		: m_pos{ pos }
	{
		lastX = windowExtent.width / 2.0f;
		lastY = windowExtent.height / 2.0f;
		computeBasis();
	};
	glm::mat4 getViewMatrix() const {
		return utils::lookAt(m_pos, m_pos + m_gazeDir, WORLD_UP);
	}

	glm::vec3 get_world_pos() const {
		return m_pos;
	}

	void moveCameraPosition(CameraMoveFlags direction, float deltaTime);
	void updateEulerAngles(float xpos, float ypos);

private:

	glm::vec3 m_pos{};
	glm::vec3 m_gazeDir{};
	glm::vec3 m_u{};

	float lastX{ 0.0f };
	float lastY{ 0.0f };
	// euler angles
	float yaw{ -90.0f }; // rotate -90 degrees to look down the negative z by default.
	float pitch{ 0.0f };

	void computeBasis();
};

#endif // !CAMERA_H
