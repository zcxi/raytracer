#ifndef RAYTRACER_ROUNDED_BOX_H
#define RAYTRACER_ROUNDED_BOX_H

#include "Shape.h"

class RoundedBox : public Shape {
public:
    RoundedBox(const Vec3& center, const Vec3& dimensions,
               double radius, const Material& material);

    bool intersect(const Ray& ray, double minDistance,
                   double maxDistance, HitRecord& hit) const override;
    bool boundingBox(Aabb& bounds) const override;

private:
    double signedDistance(const Vec3& point) const;
    Vec3 normalAt(const Vec3& point) const;

    Vec3 center;
    Vec3 halfDimensions;
    double radius;
};

#endif
