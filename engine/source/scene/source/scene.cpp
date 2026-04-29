#include "scene.h"
#include "camera.h"
#include "actor.hpp"

#include <algorithm>


namespace scene {

Scene::Scene(const std::string& name)
	: name_(name)
	, camera_(std::make_unique<Camera>())
	, actors_{}
	, renderable_actors_{}
{
	auto actor = std::make_shared<RotatingActor>("main_actor", math::Vector3(0.0f, 1.0f, 0.0f));
	addActor(actor);
}


Scene::~Scene()
{

}


void Scene::draw()
{
}


void Scene::update(double dt)
{
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


void Scene::addActor(const std::shared_ptr<Actor>& actor)
{
	if (!actor)
	{
		return;
	}

	actors_.push_back(actor);

	if (auto renderable = std::dynamic_pointer_cast<RenderableActor>(actor))
	{
		renderable_actors_.push_back(renderable);
	}
}


void Scene::removeActor(const std::shared_ptr<Actor>& actor)
{
	auto actor_it = std::remove(actors_.begin(), actors_.end(), actor);
	actors_.erase(actor_it, actors_.end());

	if (auto renderable = std::dynamic_pointer_cast<RenderableActor>(actor))
	{
		auto rend_it = std::remove(renderable_actors_.begin(), renderable_actors_.end(), renderable);
		renderable_actors_.erase(rend_it, renderable_actors_.end());
	}
}


const std::vector<std::shared_ptr<RenderableActor>>& Scene::getRenderableActors() const
{
	return renderable_actors_;
}

}  // namespace scene
