#pragma once

#include <engine/scene/camera.h>
#include <engine/platform/input.h>


namespace scene {

class FpsCamera final : public Camera
{
public:
	explicit FpsCamera(const platform::IInput& input);
	~FpsCamera() = default;

	void update(double dt) override;

	ECameraType getType() const override { return ECameraType::kFps; }

private:
	const platform::IInput& input_;
};

}  // namespace scene
