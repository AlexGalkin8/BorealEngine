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

class Scene
{
public:
    Scene(const std::string& name);
    ~Scene();

    void draw();
    void update(double dt);

    const std::string& getName() const { return name_; }

    Camera& getCamera();
    const Camera& getCamera() const;

    void addActor(const std::shared_ptr<Actor>& actor);
    void removeActor(const std::shared_ptr<Actor>& actor);

    const std::vector<std::shared_ptr<StaticMeshActor>>& getStaticMeshActors() const;

private:
    const std::string name_;
    std::unique_ptr<scene::Camera> camera_;
    std::vector<std::shared_ptr<Actor>> actors_;
    std::vector<std::shared_ptr<StaticMeshActor>> static_mesh_actors_;
};

}  // namespace scene
