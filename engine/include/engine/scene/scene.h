#pragma once

#include <engine/scene/actor.h>

#include <string>
#include <memory>
#include <vector>


namespace scene {
class Camera;
}  // namespace scene


namespace scene
{

struct DirectionalLight
{
	math::Vector3 direction_to_light = { 0.36f, 0.48f, -0.8f };
	math::Vector3 color = { 1.0f, 1.0f, 1.0f };
	float intensity = 2.0f;
	bool enabled = true;
};


inline constexpr uint32_t kMaxPointLights = 4;

struct PointLight
{
	math::Vector3 position = { 0.0f, 0.0f, -1.0f };
	math::Vector3 color = { 1.0f, 1.0f, 1.0f };
	float intensity = 2.0f;
	float radius = 3.0f;
	bool enabled = true;
};


class Scene
{
public:
	Scene(const std::string& name);
	~Scene();

	void draw();
	void update(double dt, bool update_camera = true);

	const std::string& getName() const { return name_; }

	Camera& getCamera();
	const Camera& getCamera() const;

	void setCamera(std::unique_ptr<Camera> camera);

	DirectionalLight& getDirectionalLight() { return directional_light_; }
	const DirectionalLight& getDirectionalLight() const { return directional_light_; }

	std::vector<PointLight>& getPointLights() { return point_lights_; }
	const std::vector<PointLight>& getPointLights() const { return point_lights_; }

	void addActor(const std::shared_ptr<Actor>& actor);
	void removeActor(const std::shared_ptr<Actor>& actor);

	const std::vector<std::shared_ptr<StaticMeshActor>>& getStaticMeshActors() const;

private:
	const std::string name_;
	std::unique_ptr<Camera> camera_;
	DirectionalLight directional_light_;
	std::vector<PointLight> point_lights_;
	std::vector<std::shared_ptr<Actor>> actors_;
	std::vector<std::shared_ptr<StaticMeshActor>> static_mesh_actors_;
};

}  // namespace scene
