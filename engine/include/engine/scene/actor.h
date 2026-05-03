#pragma once

#include <engine/math/transform.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace scene {

class Actor;

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
	std::string resource_path;
	EMeshPrimitive primitive = EMeshPrimitive::kTriangle;
	uint32_t mesh_index = 0;
	uint32_t primitive_index = 0;

	bool hasResourcePath() const { return !resource_path.empty(); }
};


struct MeshSlot
{
	MeshHandle mesh;
	MaterialHandle material;
};


class ActorComponent
{
public:
	virtual ~ActorComponent() = default;

	Actor* getOwner() { return owner_; }
	const Actor* getOwner() const { return owner_; }

protected:
	friend class Actor;
	void setOwner(Actor* owner) { owner_ = owner; }

private:
	Actor* owner_ = nullptr;
};


class MeshRendererComponent final : public ActorComponent
{
public:
	std::vector<MeshSlot>& getMeshSlots() { return mesh_slots_; }
	const std::vector<MeshSlot>& getMeshSlots() const { return mesh_slots_; }

	MeshSlot& addMeshSlot()
	{
		mesh_slots_.emplace_back();
		return mesh_slots_.back();
	}

	void clearMeshSlots() { mesh_slots_.clear(); }
	bool hasMeshSlots() const { return !mesh_slots_.empty(); }

private:
	std::vector<MeshSlot> mesh_slots_;
};


class Actor
{
public:
	explicit Actor(const std::string& name = "actor")
		: name_(name)
		, transform_(math::Transform::identity())
	{
	}

	virtual ~Actor() = default;

	virtual void update(double dt) = 0;

	math::Transform& getTransform() { return transform_; }
	const math::Transform& getTransform() const { return transform_; }

	const std::string& getName() const { return name_; }

	void addComponent(std::unique_ptr<ActorComponent> component)
	{
		if (!component)
		{
			return;
		}

		component->setOwner(this);
		components_.push_back(std::move(component));
	}

	const std::vector<std::unique_ptr<ActorComponent>>& getComponents() const { return components_; }

protected:
	std::string name_;
	math::Transform transform_;
	std::vector<std::unique_ptr<ActorComponent>> components_;
};


class StaticMeshActor : public Actor
{
public:
	explicit StaticMeshActor(const std::string& name = "static_mesh_actor")
		: Actor(name)
	{
		auto mesh_renderer_component = std::make_unique<MeshRendererComponent>();
		mesh_renderer_component_ = mesh_renderer_component.get();
		addComponent(std::move(mesh_renderer_component));
	}

	void update(double dt) override
	{
		(void)dt;
	}

	bool isVisible() const { return visible_; }
	void setVisible(bool visible) { visible_ = visible; }

	MeshRendererComponent& getMeshRenderer() { return *mesh_renderer_component_; }
	const MeshRendererComponent& getMeshRenderer() const { return *mesh_renderer_component_; }

	MeshHandle& getMesh()
	{
		auto& slots = mesh_renderer_component_->getMeshSlots();
		if (slots.empty())
		{
			slots.emplace_back();
		}
		return slots.front().mesh;
	}

	const MeshHandle& getMesh() const
	{
		const auto& slots = mesh_renderer_component_->getMeshSlots();
		if (slots.empty())
		{
			static MeshHandle empty_mesh{};
			return empty_mesh;
		}
		return slots.front().mesh;
	}

	MaterialHandle& getMaterial()
	{
		auto& slots = mesh_renderer_component_->getMeshSlots();
		if (slots.empty())
		{
			slots.emplace_back();
		}
		return slots.front().material;
	}

	const MaterialHandle& getMaterial() const
	{
		const auto& slots = mesh_renderer_component_->getMeshSlots();
		if (slots.empty())
		{
			static MaterialHandle empty_material{};
			return empty_material;
		}
		return slots.front().material;
	}

private:
	MeshRendererComponent* mesh_renderer_component_ = nullptr;
	bool visible_ = true;
};

} // namespace scene