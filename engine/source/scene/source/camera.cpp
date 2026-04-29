#include "camera.h"

#include <engine/math/projection.h>
#include <engine/math/math_utils.h>


namespace scene {

Camera::Camera()
	: eye_(0.0f, 0.0f, -5.0f)
	, target_(0.0f, 0.0f, 0.0f)
	, up_(0.0f, 1.0f, 0.0f)
	, fovYRadians_(math::radians(90.0f))
	, aspect_(16.0f / 9.0f)
	, zNear_(0.1f)
	, zFar_(1000.0f)
	, view_(math::Matrix4x4::identity())
	, projection_(math::Matrix4x4::identity())
	, viewProjection_(math::Matrix4x4::identity())
{
	rebuild();
}


void Camera::setLookAt(const math::Vector3& eye, const math::Vector3& target, const math::Vector3& up)
{
	eye_ = eye;
	target_ = target;
	up_ = up;
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


void Camera::rebuild()
{
	view_ = math::lookAtLH(eye_, target_, up_);
	projection_ = math::perspectiveFovLH(fovYRadians_, aspect_, zNear_, zFar_);
	viewProjection_ = projection_ * view_;
	frustum_ = math::Frustum::fromViewProjection(viewProjection_);
}

}  // namespace scene
