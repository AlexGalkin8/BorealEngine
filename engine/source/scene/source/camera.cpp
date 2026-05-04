#include "camera.h"

#include <engine/math/projection.h>
#include <engine/math/math_utils.h>
#include <engine/math/quaternion.h>


namespace scene {

Camera::Camera()
	: eye_(0.0f, 0.0f, -5.0f)
	, target_(0.0f, 0.0f, 0.0f)
	, up_(0.0f, 1.0f, 0.0f)
	, yaw_(0.0f)
	, pitch_(0.0f)
	, fovYRadians_(math::radians(90.0f))
	, aspect_(16.0f / 9.0f)
	, zNear_(0.1f)
	, zFar_(1000.0f)
	, view_(math::Matrix4x4::identity())
	, projection_(math::Matrix4x4::identity())
	, viewProjection_(math::Matrix4x4::identity())
{
	// Derive initial yaw/pitch from default eye/target
	const math::Vector3 fwd = math::normalize(target_ - eye_);
	pitch_ = std::asin(math::clamp(fwd.y, -1.0f, 1.0f));
	yaw_   = std::atan2(fwd.x, fwd.z);
	rebuild();
}


void Camera::setLookAt(const math::Vector3& eye, const math::Vector3& target, const math::Vector3& up)
{
	eye_    = eye;
	target_ = target;
	up_     = up;

	// Derive yaw/pitch from the new forward vector
	const math::Vector3 fwd = math::normalize(target_ - eye_);
	pitch_ = std::asin(math::clamp(fwd.y, -1.0f, 1.0f));
	yaw_   = std::atan2(fwd.x, fwd.z);

	rebuild();
}


void Camera::setPerspective(float fovYRadians, float aspect, float zNear, float zFar)
{
	fovYRadians_ = fovYRadians;
	aspect_ = aspect;
	zNear_ = zNear;
	zFar_ = zFar;
	rebuild();
}


void Camera::setPosition(const math::Vector3& eye)
{
	eye_ = eye;
	rebuild();
}


void Camera::setTarget(const math::Vector3& target)
{
	target_ = target;
	rebuild();
}


void Camera::setUp(const math::Vector3& up)
{
	up_ = up;
	rebuild();
}


void Camera::setAspect(float aspect)
{
	aspect_ = aspect;
	rebuild();
}


void Camera::setFovY(float fovYRadians)
{
	fovYRadians_ = fovYRadians;
	rebuild();
}


void Camera::setNearFar(float zNear, float zFar)
{
	zNear_ = zNear;
	zFar_ = zFar;
	rebuild();
}


void Camera::move(float forward, float right, float up)
{
	// Forward along the actual view direction (free camera, not grounded)
	const math::Vector3 fwd_dir = math::normalize(math::Vector3(
		std::cos(pitch_) * std::sin(yaw_),
		std::sin(pitch_),
		std::cos(pitch_) * std::cos(yaw_)));
	const math::Vector3 right_dir = math::normalize(math::Vector3(std::cos(yaw_), 0.0f, -std::sin(yaw_)));
	const math::Vector3 world_up(0.0f, 1.0f, 0.0f);

	const math::Vector3 delta = fwd_dir * forward + right_dir * right + world_up * up;
	eye_    = eye_    + delta;
	target_ = target_ + delta;
	rebuild();
}


void Camera::rebuild()
{
	view_ = math::lookAtLH(eye_, target_, up_);
	projection_ = math::perspectiveFovLH(fovYRadians_, aspect_, zNear_, zFar_);
	viewProjection_ = projection_ * view_;
	frustum_ = math::Frustum::fromViewProjection(viewProjection_);
}


void Camera::rotate(float yawDegrees, float pitchDegrees)
{
	constexpr float kMaxPitch = math::constants::kHalfPi - 0.001f;

	yaw_   += math::radians(yawDegrees);
	pitch_ -= math::radians(pitchDegrees);
	pitch_  = math::clamp(pitch_, -kMaxPitch, kMaxPitch);

	rebuildFromAngles();
}


void Camera::rebuildFromAngles()
{
	// Reconstruct forward from yaw/pitch (LH: +Z is forward, +Y is up)
	const float fwd_x = std::cos(pitch_) * std::sin(yaw_);
	const float fwd_y = std::sin(pitch_);
	const float fwd_z = std::cos(pitch_) * std::cos(yaw_);

	const float dist = (target_ - eye_).length();
	target_ = eye_ + math::Vector3(fwd_x, fwd_y, fwd_z) * dist;
	rebuild();
}

}  // namespace scene
