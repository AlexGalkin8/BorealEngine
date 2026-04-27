#pragma once

#include <engine/math/vector.h>
#include <engine/math/matrix.h>
#include <engine/math/frustum.h>


namespace scene {

class Camera
{
public:
	Camera();
	~Camera() = default;

	void setLookAt(const math::Vector3& eye, const math::Vector3& target, const math::Vector3& up);
	void setPerspective(float fovYRadians, float aspect, float zNear, float zFar);

	void setPosition(const math::Vector3& eye);
	void setTarget(const math::Vector3& target);
	void setUp(const math::Vector3& up);

	void setAspect(float aspect);
	void setFovY(float fovYRadians);
	void setNearFar(float zNear, float zFar);

	const math::Vector3& getPosition() const { return eye_; }
	const math::Vector3& getTarget() const { return target_; }
	const math::Vector3& getUp() const { return up_; }

	float getFovY() const { return fovYRadians_; }
	float getAspect() const { return aspect_; }
	float getNearPlane() const { return zNear_; }
	float getFarPlane() const { return zFar_; }

	const math::Matrix4x4& getViewMatrix() const { return view_; }
	const math::Matrix4x4& getProjectionMatrix() const { return projection_; }
	const math::Matrix4x4& getViewProjectionMatrix() const { return viewProjection_; }
	const math::Frustum& getFrustum() const { return frustum_; }

private:
	void rebuild();

	math::Vector3 eye_;
	math::Vector3 target_;
	math::Vector3 up_;

	float fovYRadians_;
	float aspect_;
	float zNear_;
	float zFar_;

	math::Matrix4x4 view_;
	math::Matrix4x4 projection_;
	math::Matrix4x4 viewProjection_;
	math::Frustum frustum_;
};

}  // namespace scene
