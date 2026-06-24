#ifndef RAYTRACER_TRANSFORM_H
#define RAYTRACER_TRANSFORM_H

#include "Shape.h"
#include <memory>

class Transform : public Shape {
public:
    Transform(std::unique_ptr<Shape> child,
              const Vec3& translation = Vec3(),
              const Vec3& rotationRadians = Vec3(),
              const Vec3& scale = Vec3(1.0, 1.0, 1.0));

    bool intersect(const Ray& ray, double minDistance,
                   double maxDistance, HitRecord& hit) const override;
    bool boundingBox(Aabb& bounds) const override;

private:
    Vec3 toLocalPoint(const Vec3& point) const;
    Vec3 toLocalDirection(const Vec3& direction) const;
    Vec3 toWorldPoint(const Vec3& point) const;
    Vec3 toWorldNormal(const Vec3& normal) const;
    Vec3 rotateForward(const Vec3& vector) const;
    Vec3 rotateInverse(const Vec3& vector) const;

    std::unique_ptr<Shape> child;
    Vec3 translation;
    Vec3 rotation;
    Vec3 scale;
};

#endif

