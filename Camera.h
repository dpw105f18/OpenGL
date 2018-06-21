#pragma once
#include "include/glm/detail/type_vec4.hpp"

class Camera
{
public:
	float FieldOfView() const { return m_FieldOfView; }
	float Near() const { return m_Near; }
	float Far() const { return m_Far; }
	glm::vec4 Position() const { return m_position; }
	glm::vec4 Target() const { return m_target; }
	glm::vec4 Up() const { return m_up; }
	void SetPosition(const glm::vec4& position) { m_position = position; };
	void SetFar(float newfar) { m_Far = newfar; };
	static Camera Default() { return { }; }

private:
	Camera() = default;
	float m_FieldOfView = 45.0f*(3.141592f / 180.0f); // 45 degrees in radians
	float m_Near = 0.1f;
	float m_Far = 50.0f;
	glm::vec4 m_position = { 0.0f, 2.0f, -4.0f,	1.0f };
	glm::vec4 m_target = { 0.0f, 0.0f, 0.0f, 1.0f };
	glm::vec4 m_up = { 0.0f, 1.0f, 0.0f, 0.0f };
};
