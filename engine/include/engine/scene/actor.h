#pragma once

#include <engine/math/transform.h>

#include <string>

namespace scene {

struct MaterialHandle
{
    std::string name;
};


enum class EMeshPrimitive
{
    kTriangle = 0,
};


struct MeshHandle
{
    EMeshPrimitive primitive = EMeshPrimitive::kTriangle;
};


class Actor
{
public:
    explicit Actor(std::string name = "actor")
        : name_(std::move(name))
        , transform_(math::Transform::identity())
    {
    }

    virtual ~Actor() = default;

    virtual void update(double dt) = 0;

    math::Transform& getTransform() { return transform_; }
    const math::Transform& getTransform() const { return transform_; }

    const std::string& getName() const { return name_; }

protected:
    std::string name_;
    math::Transform transform_;
};


class RenderableActor : public Actor
{
public:
    explicit RenderableActor(std::string name = "renderable_actor")
        : Actor(std::move(name))
    {
    }

    virtual ~RenderableActor() = default;

    bool isVisible() const { return visible_; }
    void setVisible(bool visible) { visible_ = visible; }

private:
    bool visible_ = true;
};


class StaticMeshActor : public RenderableActor
{
public:
    explicit StaticMeshActor(std::string name = "static_mesh_actor")
        : RenderableActor(std::move(name))
    {
    }

    void update(double dt) override
    {
        (void)dt;
    }

    MeshHandle& getMesh() { return mesh_; }
    const MeshHandle& getMesh() const { return mesh_; }

    MaterialHandle& getMaterial() { return material_; }
    const MaterialHandle& getMaterial() const { return material_; }

private:
    MeshHandle mesh_{};
    MaterialHandle material_{};
};

} // namespace scene