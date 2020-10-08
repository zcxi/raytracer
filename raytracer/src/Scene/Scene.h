//
// Created by chenx on 2020-05-25.
//

#ifndef RAYTRACER_SCENE_H
#define RAYTRACER_SCENE_H


#include "Light/LightSource.h"
#include "Camera.h"
#include "../shapes/Shape.h"

class Scene {

    public:
        const std::vector<double> BACKGROUND_COLOR = {0,255,0};
        const int RAY_DEPTH_LIMIT = 100;

        Scene();

        Vec3 trace(const Vec3 &rayOrigin, const Vec3 &rayDirection, int depth) const;
        void addShapes(const std::vector<Shape*>& shapes);
        void addLights(const std::vector<LightSource*>& sources);

    private:
        std::vector<Shape*> shapes;
        std::vector<LightSource*> lightSources;
};


#endif //RAYTRACER_SCENE_H
