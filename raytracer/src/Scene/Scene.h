//
// Created by chenx on 2020-05-25.
//

#ifndef RAYTRACER_SCENE_H
#define RAYTRACER_SCENE_H


#include "Light/LightSource.h"
#include "../shapes/Shape.h"
#include "../Math/Ray.h"
#include <memory>

class Scene {

    public:
        static constexpr double RAY_EPSILON = 1e-4;

        explicit Scene(const Vec3& backgroundColor = Vec3(0.0, 0.0, 0.0));

        Vec3 trace(const Ray& ray) const;
        bool findClosestHit(const Ray& ray, double minDistance,
                            double maxDistance, HitRecord& hit) const;
        bool isOccluded(const Ray& ray, double maxDistance) const;
        void addShape(std::unique_ptr<Shape> shape);
        void addLight(std::unique_ptr<LightSource> source);

    private:
        Vec3 backgroundColor;
        std::vector<std::unique_ptr<Shape>> shapes;
        std::vector<std::unique_ptr<LightSource>> lightSources;
};


#endif //RAYTRACER_SCENE_H
