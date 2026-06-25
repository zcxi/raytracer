#ifndef RAYTRACER_AABB_H
#define RAYTRACER_AABB_H

#include "../Math/Ray.h"
#include <array>

struct TraceStats;

class Aabb {
public:
    struct RayData {
        std::array<double, 3> origin;
        std::array<double, 3> inverseDirection;
        std::array<bool, 3> negative;
        std::array<bool, 3> parallel;

        explicit RayData(const Ray& ray);
    };

    Aabb();
    Aabb(const Vec3& minimum, const Vec3& maximum);

    bool intersect(const Ray& ray, double minDistance,
                   double maxDistance, double* nearDistance = nullptr,
                   TraceStats* stats = nullptr) const;
    bool intersect(const RayData& ray, double minDistance,
                   double maxDistance, double* nearDistance = nullptr,
                   TraceStats* stats = nullptr) const;
    Vec3 centroid() const { return (minimum + maximum) * 0.5; }
    double surfaceArea() const;
    const Vec3& min() const { return minimum; }
    const Vec3& max() const { return maximum; }

    static Aabb surrounding(const Aabb& first, const Aabb& second);

private:
    Vec3 minimum;
    Vec3 maximum;
};

#endif

