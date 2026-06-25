#ifndef RAYTRACER_JSON_SCENE_LOADER_H
#define RAYTRACER_JSON_SCENE_LOADER_H

#include "LoadedScene.h"

#include <string>

class JsonSceneLoader {
public:
    static LoadedScene load(const std::string& path);
    static void validate(const std::string& path);
};

#endif
