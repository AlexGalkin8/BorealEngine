#include "scene.h"
#include "camera.h"

#include <algorithm>


namespace scene {

Scene::Scene(const std::string& name)
	: name_(name)
	, camera_(std::make_unique<Camera>())
	, actors_{}
	, static_mesh_actors_{}
{
}


Scene::~Scene()
{
}


void Scene::draw()
{
}


void Scene::update(double dt)
{
	if (camera_)
	{
		camera_->update(dt);
	}

	for (auto& actor : actors_)
	{
		actor->update(dt);
	}
}


Camera& Scene::getCamera()
{
	return *camera_;
}


const Camera& Scene::getCamera() const
{
	return *camera_;
}


void Scene::setCamera(std::unique_ptr<Camera> camera)
{
	if (camera)
	{
		camera_ = std::move(camera);
	}
}


void Scene::addActor(const std::shared_ptr<Actor>& actor)
{
	if (!actor)
	{
		return;
	}

	actors_.push_back(actor);

	if (auto static_mesh_actor = std::dynamic_pointer_cast<StaticMeshActor>(actor))
	{
		static_mesh_actors_.push_back(static_mesh_actor);
	}
}


void Scene::removeActor(const std::shared_ptr<Actor>& actor)
{
	auto actor_it = std::remove(actors_.begin(), actors_.end(), actor);
	actors_.erase(actor_it, actors_.end());

	if (auto static_mesh_actor = std::dynamic_pointer_cast<StaticMeshActor>(actor))
	{
		auto mesh_it = std::remove(static_mesh_actors_.begin(), static_mesh_actors_.end(), static_mesh_actor);
		static_mesh_actors_.erase(mesh_it, static_mesh_actors_.end());
	}
}


const std::vector<std::shared_ptr<StaticMeshActor>>& Scene::getStaticMeshActors() const
{
	return static_mesh_actors_;
}

}  // namespace scene
