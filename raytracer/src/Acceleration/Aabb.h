#ifndef RAYTRACER_AABB_H
#define RAYTRACER_AABB_H

#include "../Math/Ray.h"

class Aabb {
public:
    Aabb();
    Aabb(const Vec3& minimum, const Vec3& maximum);

    bool intersect(const Ray& ray, double minDistance,
                   double maxDistance) const;
    Vec3 centroid() const { return (minimum + maximum) * 0.5; }
    const Vec3& min() const { return minimum; }
    const Vec3& max() const { return maximum; }

    static Aabb surrounding(const Aabb& first, const Aabb& second);

private:
    Vec3 minimum;
    Vec3 maximum;
};

#endif

