#include "Camera.h"

void Camera::moveCameraPosition(CameraMoveFlags direction, float deltaTime) {
	float velocity{ MOVEMENT_FACTOR * deltaTime };

	if (direction == FORWARD)
		m_pos += velocity * m_gazeDir;
	else if (direction == BACKWARD)
		m_pos -= velocity * m_gazeDir;
	else if (direction == RIGHT)
		m_pos += velocity * m_u;
	else if (direction == LEFT)
		m_pos -= velocity * m_u;
}

void Camera::updateEulerAngles(float xpos, float ypos) {
	// compute difference from last cursor displacement and current
	float xDiff{ xpos - lastX };
	float yDiff{ lastY - ypos };

	// update last cursor displacement for next callback
	lastX = xpos;
	lastY = ypos;

	// scale difference -- controls sensitivity.
	xDiff *= X_SENS;
	yDiff *= Y_SENS;

	// adjust euler angles using the difference
	yaw += xDiff;
	pitch += yDiff;

	// limit pitch
	if (pitch > 89.0f)
		pitch = 89.0f;
	else if (pitch < -89.0f)
		pitch = -89.0f;

	computeBasis();
}

void Camera::computeBasis() {
	// compute gaze direction using euler angles
	m_gazeDir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	m_gazeDir.y = sin(glm::radians(pitch));
	m_gazeDir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	m_gazeDir = glm::normalize(m_gazeDir);
	m_u = glm::normalize(glm::cross(m_gazeDir, WORLD_UP));
}