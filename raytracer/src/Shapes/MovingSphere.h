#ifndef RAYTRACER_MOVING_SPHERE_H
#define RAYTRACER_MOVING_SPHERE_H

#include "Shape.h"

class MovingSphere : public Shape {
public:
    MovingSphere(const Vec3& startCenter, const Vec3& endCenter,
                 double startTime, double endTime, double radius,
                 const Material& material);
    bool intersect(const Ray& ray, double minDistance,
                   double maxDistance, HitRecord& hit) const override;
    bool boundingBox(Aabb& bounds) const override;
    Vec3 centerAt(double time) const;
private:
    Vec3 startCenter;
    Vec3 endCenter;
    double startTime;
    double endTime;
    double radius;
};

#endif

