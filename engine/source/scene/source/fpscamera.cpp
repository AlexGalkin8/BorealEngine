#include <engine/scene/fpscamera.h>


namespace scene {

FpsCamera::FpsCamera(const platform::IInput& input)
	: Camera()
	, input_(input)
{
}


void FpsCamera::update(double dt)
{
	const platform::FpsCameraSettings& settings = input_.getFpsCameraSettings();

	// --- Rotation ---
	float dx = 0.0f;
	float dy = 0.0f;
	input_.getMouseDelta(dx, dy);

	if (dx != 0.0f || dy != 0.0f)
	{
		rotate(dx * settings.mouse_sensitivity, dy * settings.mouse_sensitivity);
	}

	// --- Movement ---
	const float speed = settings.move_speed
		* (input_.isActionDown("sprint") ? settings.sprint_multiplier : 1.0f)
		* static_cast<float>(dt);

	float fwd   = 0.0f;
	float right = 0.0f;
	float up    = 0.0f;

	if (input_.isActionDown("move_forward"))  fwd   += speed;
	if (input_.isActionDown("move_backward")) fwd   -= speed;
	if (input_.isActionDown("move_right"))    right += speed;
	if (input_.isActionDown("move_left"))     right -= speed;
	if (input_.isActionDown("move_up"))       up    += speed;
	if (input_.isActionDown("move_down"))     up    -= speed;

	if (fwd != 0.0f || right != 0.0f || up != 0.0f)
	{
		move(fwd, right, up);
	}
}

}  // namespace scene
