#pragma once

#include <engine/math/vector.h>
#include <engine/math/matrix.h>
#include <engine/math/frustum.h>


namespace scene {

enum class ECameraType
{
	kBase,
	kFps,
};

class Camera
{
public:
	Camera();
	virtual ~Camera() = default;

	void setLookAt(const math::Vector3& eye, const math::Vector3& target, const math::Vector3& up);
	void setPerspective(float fovYRadians, float aspect, float zNear, float zFar);

	void setPosition(const math::Vector3& eye);
	void setTarget(const math::Vector3& target);
	void setUp(const math::Vector3& up);

	void setAspect(float aspect);
	void setFovY(float fovYRadians);
	void setNearFar(float zNear, float zFar);

	// Rotate camera by yaw (world Y axis) and pitch (camera right axis), degrees
	void rotate(float yawDegrees, float pitchDegrees);

	// Move camera along its local axes: forward/right/up
	void move(float forward, float right, float up);

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

	virtual ECameraType getType() const { return ECameraType::kBase; }

	virtual void update(double dt) {}

private:
	void rebuild();
	// Rebuild target_ from eye_, yaw_, pitch_ and distance_
	void rebuildFromAngles();

	math::Vector3 eye_;     // Camera position in world space
	math::Vector3 target_;  // Point the camera is looking at in world space
	math::Vector3 up_;      // Up direction vector defining camera orientation

	float yaw_;    // Horizontal angle, radians, around world Y
	float pitch_;  // Vertical angle, radians, clamped to +-89 deg

	float fovYRadians_;
	float aspect_;
	float zNear_;
	float zFar_;

	math::Matrix4x4 view_;
	math::Matrix4x4 projection_;
	math::Matrix4x4 viewProjection_;
	math::Frustum frustum_;

	ECameraType type_;
};

}  // namespace scene
