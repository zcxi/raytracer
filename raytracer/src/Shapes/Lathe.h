#ifndef RAYTRACER_LATHE_H
#define RAYTRACER_LATHE_H

#include "Triangle.h"
#include "../Acceleration/Bvh.h"
#include <cstddef>
#include <vector>

class Lathe : public Shape {
public:
    struct ProfilePoint {
        double r;
        double y;
        ProfilePoint(double radius, double height) : r(radius), y(height) {}
    };

    Lathe(const std::vector<ProfilePoint>& profile, unsigned int segments,
          const Material& material);

    bool intersect(const Ray& ray, double minDistance,
                   double maxDistance, HitRecord& hit) const override;
    bool boundingBox(Aabb& bounds) const override;
    double surfaceArea() const override { return totalArea; }
    bool sampleSurface(
        Sampler& sampler, SurfaceSample& sample) const override;
    std::size_t triangleCount() const { return triangles.size(); }
    std::size_t bvhNodeCount() const { return bvh.nodeCount(); }

private:
    std::vector<Triangle> triangles;
    std::vector<const Shape*> triangleShapes;
    std::vector<double> triangleCdf;
    Bvh bvh;
    Aabb bounds;
    double totalArea;
};

#endif
