//
// Created by chenx on 2020-05-25.
//

#ifndef RAYTRACER_SCENE_H
#define RAYTRACER_SCENE_H


#include "Light/LightSource.h"
#include "Environment.h"
#include "../Shapes/Shape.h"
#include "../Math/Ray.h"
#include "../Math/Sampler.h"
#include "../Materials/Bsdf.h"
#include "../Acceleration/Bvh.h"
#include <memory>

struct PathResult {
    Vec3 radiance;
    Vec3 albedo;
    Vec3 normal;
    Vec3 position;

    PathResult()
        : radiance(), albedo(), normal(), position() {}
    PathResult(const Vec3& r, const Vec3& a, const Vec3& n, const Vec3& p)
        : radiance(r), albedo(a), normal(n), position(p) {}
};

struct PathTraceSettings {
    unsigned int maxBounces;
    unsigned int russianRouletteStart;

    PathTraceSettings(unsigned int maxBounces = 8,
                      unsigned int russianRouletteStart = 4)
        : maxBounces(maxBounces),
          russianRouletteStart(russianRouletteStart) {
    }
};

class Scene {

    public:
        static constexpr double RAY_EPSILON = 1e-4;

        explicit Scene(const Vec3& backgroundColor = Vec3(0.0, 0.0, 0.0));

        PathResult trace(const Ray& ray) const;
        PathResult trace(const Ray& ray, Sampler& sampler,
                   const PathTraceSettings& settings) const;
        bool findClosestHit(const Ray& ray, double minDistance,
                            double maxDistance, HitRecord& hit) const;
        bool isOccluded(const Ray& ray, double maxDistance) const;
        void addShape(std::unique_ptr<Shape> shape);
        void addLight(std::unique_ptr<LightSource> source);
        void finalize() const;
        void setEnvironment(const Environment& environment);
        void setEnvironmentIntensity(double intensity) {
            environment.setIntensity(intensity);
        }
        bool loadEnvironmentMap(
                const std::string& path, double intensity) {
            return environment.loadPpm(path, intensity);
        }
        void setAccelerationEnabled(bool enabled) {
            accelerationEnabled = enabled;
        }
        std::size_t bvhNodeCount() const {
            finalize();
            return bvh.nodeCount();
        }
        std::size_t bvhBuildCount() const { return accelerationBuildCount; }
        std::size_t boundedShapeCount() const {
            return boundedShapes.size();
        }
        std::size_t shapeCount() const { return shapes.size(); }
        std::size_t lightCount() const { return lightSources.size(); }

        static double powerHeuristic(double firstPdf, double secondPdf);

    private:
        Vec3 directLighting(
            const HitRecord& hit, const Material& material,
            const Vec3& outgoing,
            Sampler& sampler) const;
        Vec3 pointLighting(
            const HitRecord& hit, const Material& material,
            const Vec3& outgoing) const;
        double emissiveLightPdf(
            const Vec3& origin, const HitRecord& lightHit) const;
        double environmentLightPdf(const Vec3& direction) const;
        std::size_t sampledLightCount() const;
        Environment environment;
        std::vector<std::unique_ptr<Shape>> shapes;
        std::vector<const Shape*> boundedShapes;
        std::vector<const Shape*> unboundedShapes;
        std::vector<std::unique_ptr<LightSource>> lightSources;
        std::vector<const Shape*> emissiveShapes;
        mutable Bvh bvh;
        mutable bool accelerationDirty;
        mutable std::size_t accelerationBuildCount;
        bool accelerationEnabled;
};


#endif //RAYTRACER_SCENE_H
