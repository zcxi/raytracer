#ifndef RAYTRACER_LOADED_SCENE_H
#define RAYTRACER_LOADED_SCENE_H

#include "../Renderer.h"
#include "../Scene/Camera.h"
#include "../Scene/Scene.h"
#include "../Writer/ImageWriter.h"

#include <memory>
#include <string>

struct SceneLoadSummary {
    std::size_t objects;
    std::size_t lights;
    std::size_t materials;
    std::size_t textures;

    SceneLoadSummary()
        : objects(0), lights(0), materials(0), textures(0) {}
};

struct LoadedScene {
    std::unique_ptr<Scene> scene;
    std::unique_ptr<Camera> camera;
    RenderSettings renderSettings;
    ImageOutputSettings outputSettings;
    std::string outputPath;
    SceneLoadSummary summary;
};

#endif
