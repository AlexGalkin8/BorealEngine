#pragma once

#include <engine/scene/actor.h>
#include <engine/math/quaternion.h>
#include <engine/math/math_utils.h>

namespace scene {

class RotatingActor final : public StaticMeshActor
{
public:
    RotatingActor(
        std::string name = "rotating_actor",
        const math::Vector3& axis = math::Vector3(0.0f, 0.0f, 1.0f),
        float angular_speed_radians = math::constants::kTwoPi * 0.25f)
        : StaticMeshActor(std::move(name))
        , axis_(axis)
        , angular_speed_radians_(angular_speed_radians)
        , angle_(0.0f)
    {
        getMesh().primitive = EMeshPrimitive::kTriangle;
        getMaterial().name = "default";
    }

    void update(double dt) override
    {
        angle_ += angular_speed_radians_ * static_cast<float>(dt);
        transform_.rotation = math::Quaternion::fromAxisAngle(axis_, angle_);
    }

    void setAxis(const math::Vector3& axis) { axis_ = axis; }
    void setAngularSpeed(float radians_per_second) { angular_speed_radians_ = radians_per_second; }

private:
    math::Vector3 axis_;
    float angular_speed_radians_;
    float angle_;
};

} // namespace scene
