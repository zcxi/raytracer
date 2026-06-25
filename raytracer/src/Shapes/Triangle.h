#ifndef RAYTRACER_TRIANGLE_H
#define RAYTRACER_TRIANGLE_H

#include "Shape.h"

class Sampler;
struct SurfaceSample;

struct Vertex {
    Vec3 position;
    Vec3 normal;
    double u;
    double v;

    Vertex(const Vec3& position = Vec3(),
           const Vec3& normal = Vec3(),
           double u = 0.0, double v = 0.0)
        : position(position), normal(normal), u(u), v(v) {}
};

class Triangle : public Shape {
public:
    Triangle(const Vertex& first, const Vertex& second,
             const Vertex& third, const Material& material);

    bool intersect(const Ray& ray, double minDistance,
                   double maxDistance, HitRecord& hit) const override;
    bool boundingBox(Aabb& bounds) const override;
    bool sampleSurface(
        Sampler& sampler, SurfaceSample& sample) const override;
    double computeArea() const;

private:
    Vertex first;
    Vertex second;
    Vertex third;
    Vec3 faceNormal;
};

#endif
